#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <tuple>
#include <stdexcept>
#include <new>
#include <utility>
#include <algorithm>

namespace Core
{
// @brief 高性能 SoA（Structure of Arrays）动态数组。
// @tparam Types 存储的列类型列表。
// @note 内存布局：每列独立分配，确保类型对齐；支持非平凡析构类型。
template <typename... Types>
class alignas(std::max_align_t) SoaDynamicArray
{
public:
	using value_types = std::tuple<Types...>;
	static constexpr size_t NumTypes = sizeof...(Types);

	// ---------- 构造 / 析构 ----------
	SoaDynamicArray() = default;

	~SoaDynamicArray()
	{
		DestroyAllElements();
		FreeAllColumns();
	}

	// 拷贝构造（深拷贝）
	SoaDynamicArray(const SoaDynamicArray& other)
		: capacity(other.size), size(other.size)
	{
		if (size == 0) return;

		// 分配内存
		for (size_t i = 0; i < NumTypes; ++i)
		{
			data_arrays[i] = static_cast<std::byte*>(
				::operator new(size * GetElementSize(i), GetElementAlign(i)));
		}

		// 拷贝构造每个元素
		try
		{
			[&]<size_t... Is>(std::index_sequence<Is...>)
			{
				(CopyColumn<Is>(other), ...);
			}(std::index_sequence_for<Types...>{});
		}
		catch (...)
		{
			// 构造失败，清理已分配的资源
			DestroyAllElements();
			FreeAllColumns();
			throw;
		}
	}

	SoaDynamicArray& operator=(const SoaDynamicArray& other)
	{
		if (this != &other)
		{
			// 使用 copy-and-swap 惯用法提供强异常保证
			SoaDynamicArray temp(other);
			Swap(temp);
		}
		return *this;
	}

	// 移动构造 / 赋值
	SoaDynamicArray(SoaDynamicArray&& other) noexcept
		: data_arrays(std::exchange(other.data_arrays, {}))
		, capacity(std::exchange(other.capacity, 0))
		, size(std::exchange(other.size, 0))
	{
	}

	SoaDynamicArray& operator=(SoaDynamicArray&& other) noexcept
	{
		if (this != &other)
		{
			SoaDynamicArray temp(std::move(other));
			Swap(temp);
		}
		return *this;
	}

	// ---------- 元素访问 ----------
	// @brief 返回第 index 个逻辑元素的只读视图（tuple<const Types&...>）。
	auto operator[](size_t index) const noexcept
	{
		// 注意：不进行边界检查以匹配性能要求，调用者需保证 index < size
		return GetTupleAtConst(index, std::index_sequence_for<Types...>{});
	}

	// @brief 返回第 index 个逻辑元素的可读写视图（tuple<Types&...>）。
	auto operator[](size_t index) noexcept
	{
		return GetTupleAt(index, std::index_sequence_for<Types...>{});
	}

	// @brief 带边界检查的元素访问。
	auto At(size_t index) const
	{
		if (index >= size)
			throw std::out_of_range("SoaDynamicArray::At");
		return (*this)[index];
	}

	auto At(size_t index)
	{
		if (index >= size)
			throw std::out_of_range("SoaDynamicArray::At");
		return (*this)[index];
	}

	// ---------- 容量操作 ----------
	size_t Size() const noexcept { return size; }
	size_t Capacity() const noexcept { return capacity; }
	bool IsEmpty() const noexcept { return size == 0; }

	// @brief 预留至少 new_capacity 的存储空间。
	void Reserve(size_t new_capacity)
	{
		if (new_capacity <= capacity) return;
		GrowTo(new_capacity);
	}

	// @brief 请求移除未使用的容量（缩容至 size_）。
	void ShrinkToFit()
	{
		if (capacity == size) return;
		if (size == 0)
		{
			FreeAllColumns();
			capacity = 0;
			return;
		}
		GrowTo(size); // 重新分配为刚好 size 大小
	}

	// ---------- 修改操作 ----------
	// @brief 清空所有元素（容量不变）。
	void Clear()
	{
		DestroyAllElements();
		size = 0;
	}

	// @brief 改变数组大小。
	// @param new_size 新的大小。
	// @note 若 new_size > size_，则默认构造新元素；若 new_size < size_，则析构多余元素。
	void Resize(size_t new_size)
	{
		if (new_size == size) return;

		if (new_size < size)
		{
			// 析构尾部多余元素
			DestroyRange(new_size, size);
			size = new_size;
		}
		else // new_size > size
		{
			if (new_size > capacity)
			{
				// 扩容策略：至少翻倍或达到要求
				size_t new_cap = std::max(capacity * 2, new_size);
				GrowTo(new_cap);
			}
			// 在尾部默认构造新元素
			try
			{
				[&]<size_t... Is>(std::index_sequence<Is...>)
				{
					(DefaultConstructRange<Is>(size, new_size), ...);
				}(std::index_sequence_for<Types...>{});
			}
			catch (...)
			{
				// 如果某列构造失败，已构造的部分会被 DestroyRange 清理
				DestroyRange(size, new_size);
				throw;
			}
			size = new_size;
		}
	}

	void PushBack(Types... args) {
		PushBack(std::make_tuple(std::forward<Types>(args)...));
	}

	void PushBack(std::tuple<Types...> args)
	{
		static_assert(sizeof...(Types) == NumTypes, "Number of arguments must match number of types");

		if (size >= capacity)
		{
			size_t new_cap = capacity == 0 ? 256 : capacity * 2;
			GrowTo(new_cap);
		}

		// RAII 守卫：若构造失败，回退已构造的对象
		struct ConstructGuard
		{
			SoaDynamicArray* self;
			size_t constructed_count = 0;
			~ConstructGuard() {
				if (constructed_count < NumTypes) {
					// 析构已构造的当前元素（索引为 self->size）
					self->DestroySingleAt(self->size);
				}
			}
		} guard{this, 0};

		[&]<size_t... Is>(std::index_sequence<Is...>)
		{
			((ConstructAt<Is>(size, std::get<Is>(args)), ++guard.constructed_count), ...);
		}(std::index_sequence_for<Types...>{});

		++size;
	}

	// @brief 移除指定位置的元素。
	void RemoveAt(size_t index)
	{
		if (index >= size)
			throw std::out_of_range("SoaDynamicArray::RemoveAt");

		// 析构被删除元素
		DestroySingleAt(index);

		// 将后续元素向前移动一位
		if (index < size - 1)
		{
			[&]<size_t... Is>(std::index_sequence<Is...>)
			{
				(MoveElementsLeft<Is>(index), ...);
			}(std::index_sequence_for<Types...>{});
		}
		--size;
	}

	// @brief 将另一个数组的所有元素追加到末尾。
	void Append(const SoaDynamicArray& other)
	{
		if (other.size == 0) return;

		size_t new_size = size + other.size;
		if (new_size > capacity)
		{
			size_t new_cap = std::max(capacity * 2, new_size);
			GrowTo(new_cap);
		}

		// 逐列拷贝构造新元素
		try
		{
			[&]<size_t... Is>(std::index_sequence<Is...>)
			{
				(CopyAppendColumn<Is>(other), ...);
			}(std::index_sequence_for<Types...>{});
		}
		catch (...)
		{
			// 失败时析构已构造的追加元素（范围 [size, new_constructed) ）
			DestroyRange(size, size + other.size);
			throw;
		}
		size = new_size;
	}

	// @brief 交换两个数组的内容。
	void Swap(SoaDynamicArray& other) noexcept
	{
		std::swap(data_arrays, other.data_arrays);
		std::swap(capacity, other.capacity);
		std::swap(size, other.size);
	}

private:
	// ---------- 内部辅助函数 ----------
	std::array<std::byte*, NumTypes> data_arrays{};
	size_t capacity = 0;
	size_t size = 0;

	// 获取第 I 列的元素大小
	static constexpr size_t GetElementSize(size_t i)
	{
		constexpr std::array<size_t, NumTypes> sizes = {sizeof(Types)...};
		return sizes[i];
	}

	template <size_t I>
	static constexpr size_t GetElementSize() { return sizeof(std::tuple_element_t<I, value_types>); }

	// 获取第 I 列的对齐要求
	static constexpr std::align_val_t GetElementAlign(size_t i)
	{
		constexpr std::array<std::align_val_t, NumTypes> aligns = {std::align_val_t(alignof(Types))...};
		return aligns[i];
	}

	template <size_t I>
	static constexpr std::align_val_t GetElementAlign()
	{
		return std::align_val_t(alignof(std::tuple_element_t<I, value_types>));
	}

	// 获取第 I 列第 idx 个元素的引用
	template <size_t I>
	auto& GetRef(size_t index)
	{
		using Type = std::tuple_element_t<I, value_types>;
		return (Type*)(data_arrays[I])[index];
	}

	template <size_t I>
	const auto& GetConstRef(size_t index) const
	{
		using Type = std::tuple_element_t<I, value_types>;
		return (const Type*)(data_arrays[I])[index];
	}

	// 构建 tuple 返回
	template <size_t... Is>
	auto GetTupleAt(size_t index, std::index_sequence<Is...>)
	{
		return std::tuple<Types&...>(GetRef<Is>(index)...);
	}

	template <size_t... Is>
	auto GetTupleAtConst(size_t index, std::index_sequence<Is...>) const
	{
		return std::tuple<const Types&...>(GetConstRef<Is>(index)...);
	}

	// 在指定位置构造元素
	template <size_t I, typename Arg>
	void ConstructAt(size_t pos, Arg&& arg)
	{
		using Type = std::tuple_element_t<I, value_types>;
		Type* dest = (Type*)(data_arrays[I]) + pos;
		std::construct_at(dest, std::forward<Arg>(arg));
	}

	// 析构单个元素
	void DestroySingleAt(size_t index)
	{
		[&]<size_t... Is>(std::index_sequence<Is...>)
		{
			((std::destroy_at(reinterpret_cast<std::tuple_element_t<Is, value_types>*>(data_arrays[Is]) + index)), ...);
		}(std::index_sequence_for<Types...>{});
	}

	// 析构范围 [begin, end) 内的元素
	void DestroyRange(size_t begin, size_t end)
	{
		if (begin >= end) return;
		[&]<size_t... Is>(std::index_sequence<Is...>)
		{
			(DestroyColumnByRange<Is>(begin, end), ...);
		}(std::index_sequence_for<Types...>{});
	}

	template <size_t I>
	void DestroyColumnByRange(size_t begin, size_t end)
	{
		using Type = std::tuple_element_t<I, value_types>;
		Type* col = (Type*)(data_arrays[I]);
		for (size_t i = begin; i < end; ++i)
		{
			std::destroy_at(col + i);
		}
			
	}

	// 析构所有元素
	void DestroyAllElements()
	{
		DestroyRange(0, size);
	}

	// 释放所有列内存
	void FreeAllColumns()
	{
		for (size_t i = 0; i < NumTypes; ++i)
		{
			if (data_arrays[i])
			{
				::operator delete(data_arrays[i], GetElementAlign(i));
				data_arrays[i] = nullptr;
			}
		}
	}

	// 扩容至 new_capacity
	void GrowTo(size_t new_capacity)
	{
		if (new_capacity <= capacity) return;

		std::array<std::byte*, NumTypes> new_arrays{};
		// 分配新内存
		for (size_t i = 0; i < NumTypes; ++i)
		{
			new_arrays[i] = static_cast<std::byte*>(
				::operator new(new_capacity * GetElementSize(i), GetElementAlign(i)));
		}

		// RAII 守卫：若转移过程抛出异常，释放新内存
		struct NewMemoryGuard
		{
			std::array<std::byte*, NumTypes>& arrays;
			size_t count = NumTypes;
			~NewMemoryGuard()
			{
				for (size_t i = 0; i < count; ++i)
				{
					::operator delete(arrays[i], GetElementAlign(i));
				}
			}
		} guard{new_arrays, NumTypes};

		if (size > 0)
		{
			// 转移现有元素到新内存
			[&]<size_t... Is>(std::index_sequence<Is...>)
			{
				(TransferColumn<Is>(new_arrays[Is], new_capacity), ...);
			}(std::index_sequence_for<Types...>{});
		}

		// 释放旧内存
		for (size_t i = 0; i < NumTypes; ++i)
		{
			if (data_arrays[i])
				::operator delete(data_arrays[i], GetElementAlign(i));
		}

		data_arrays = new_arrays;
		capacity = new_capacity;
		// 守卫失效，新内存已移交
		guard.count = 0;
	}

	// 转移第 I 列数据到新内存
	template <size_t I>
	void TransferColumn(std::byte* new_memory, size_t /*new_capacity*/)
	{
		using Type = std::tuple_element_t<I, value_types>;
		Type* old_col = (Type*)(data_arrays[I]);
		Type* new_col = (Type*)(new_memory);

		if constexpr (std::is_trivially_copyable_v<Type>)
		{
			// 优化：平凡可复制类型直接 memcpy
			std::memcpy(new_col, old_col, size * sizeof(Type));
		}
		else
		{
			// 优先使用移动构造（若 noexcept），否则拷贝构造
			if constexpr (std::is_nothrow_move_constructible_v<Type>)
			{
				for (size_t j = 0; j < size; ++j)
				{
					std::construct_at(new_col + j, std::move(old_col[j]));
				}
			}
			else
			{
				for (size_t j = 0; j < size; ++j)
				{
					std::construct_at(new_col + j, old_col[j]);
				}					
			}
			// 析构旧对象
			for (size_t j = 0; j < size; ++j)
			{
				std::destroy_at(old_col + j);
			}
				
		}
	}

	// 拷贝列（用于拷贝构造）
	template <size_t I>
	void CopyColumn(const SoaDynamicArray& other)
	{
		using Type = std::tuple_element_t<I, value_types>;
		const Type* src = (const Type*)(other.data_arrays[I]);
		Type* dst = (Type*)(data_arrays[I]);
		for (size_t j = 0; j < size; ++j)
		{
			std::construct_at(dst + j, src[j]);
		}
			
	}

	// 追加拷贝列（用于 Append）
	template <size_t I>
	void CopyAppendColumn(const SoaDynamicArray& other)
	{
		using Type = std::tuple_element_t<I, value_types>;
		const Type* src = (const Type*)(other.data_arrays[I]);
		Type* dst = (Type*)(data_arrays[I]) + size;
		for (size_t j = 0; j < other.size; ++j)
		{
			std::construct_at(dst + j, src[j]);
		}
	}

	// 默认构造范围 [begin, end) 内的元素
	template <size_t I>
	void DefaultConstructRange(size_t begin, size_t end)
	{
		using Type = std::tuple_element_t<I, value_types>;
		Type* col = (Type*)(data_arrays[I]);
		for (size_t i = begin; i < end; ++i)
		{
			std::construct_at(col + i);
		}
	}

	// 将索引 index 之后的元素向左移动一位
	template <size_t I>
	void MoveElementsLeft(size_t index)
	{
		using Type = std::tuple_element_t<I, value_types>;
		Type* col = (Type*)(data_arrays[I]);
		if constexpr (std::is_nothrow_move_assignable_v<Type>)
		{
			for (size_t i = index; i < size - 1; ++i)
			{
				col[i] = std::move(col[i + 1]);
			}
		}
		else
		{
			for (size_t i = index; i < size - 1; ++i)
			{
				col[i] = col[i + 1];
			}
		}
	}
};

// 全局 swap 特化
template <typename... Types>
void swap(SoaDynamicArray<Types...>& a, SoaDynamicArray<Types...>& b) noexcept
{
	a.Swap(b);
}


// namespace Test
// {
	
// struct Person
// {
//     std::string Name;
//     int Age;

//     // 用于验证默认构造
//     Person() : Name("Unknown"), Age(0) {}
//     Person(std::string n, int a) : Name(std::move(n)), Age(a) {}

//     bool operator==(const Person& other) const
//     {
//         return Name == other.Name && Age == other.Age;
//     }
// };

// // 辅助打印
// void print_person_array(const SoaDynamicArray<std::string, int>& arr)
// {
//     std::cout << "Array size = " << arr.Size() << "\n";
//     for (size_t i = 0; i < arr.Size(); ++i)
//     {
//         auto [name, age] = arr[i];
//         std::cout << "  [" << i << "] " << name << ", " << age << '\n';
//     }
//     std::cout << std::endl;
// }

	
// int TestSoAArray()
// {
//     // ---------- 基本操作 ----------
//     SoaDynamicArray<std::string, int> peoples;
//     peoples.PushBack({"Alice", 30});
//     peoples.PushBack({"Bob", 25});
//     peoples.PushBack({"Charlie", 35});

//     std::cout << "After PushBack:\n";
//     print_person_array(peoples);

//     // ---------- 访问与修改 ----------
//     auto [name, age] = peoples[1];
//     std::cout << "Element 1: " << name << ", " << age << '\n';
//     std::get<1>(peoples[1]) = 26; // 修改 Bob 的年龄
//     std::cout << "After modification: " << std::get<0>(peoples[1]) << ", " << std::get<1>(peoples[1]) << "\n\n";

//     // ---------- 删除元素 ----------
//     peoples.RemoveAt(0);
//     std::cout << "After RemoveAt(0):\n";
//     print_person_array(peoples);

//     // ---------- 调整大小 ----------
//     peoples.Resize(5); // 新增两个默认构造的元素
//     std::cout << "After Resize(5):\n";
//     print_person_array(peoples);
//     // 检查默认值
//     assert(std::get<0>(peoples[3]).empty());
//     assert(std::get<1>(peoples[3]) == 0);
//     peoples.Resize(2);
//     std::cout << "After Resize(2):\n";
//     print_person_array(peoples);

//     // ---------- 拷贝与追加 ----------
//     SoaDynamicArray<std::string, int> others;
//     others.PushBack({"Dave", 40});
//     others.PushBack({"Eve", 28});

//     peoples.Append(others);
//     std::cout << "After Append(others):\n";
//     print_person_array(peoples);

//     SoaDynamicArray<std::string, int> copy = peoples;
//     std::cout << "Copy constructed array:\n";
//     print_person_array(copy);

//     // ---------- 容量操作 ----------
//     std::cout << "Capacity before shrink: " << peoples.Capacity() << '\n';
//     peoples.ShrinkToFit();
//     std::cout << "Capacity after shrink: " << peoples.Capacity() << "\n\n";

//     // ---------- 移动语义 ----------
//     SoaDynamicArray<std::string, int> moved = std::move(peoples);
//     std::cout << "Moved-from array size: " << peoples.Size() << '\n';
//     std::cout << "Moved-to array:\n";
//     print_person_array(moved);

//     // ---------- 自定义结构体 ----------
//     SoaDynamicArray<Person, double> custom;
//     custom.PushBack({Person{"John", 45}, 3.14});
//     custom.PushBack({Person{"Jane", 33}, 2.71});
//     std::cout << "Custom struct array:\n";
//     for (size_t i = 0; i < custom.Size(); ++i)
//     {
//         auto [person, value] = custom[i];
//         std::cout << "  " << person.Name << " (" << person.Age << ") - " << value << '\n';
//     }

//     return 0;
// }

// }
} // namespace Core