
#pragma once

#include <array>
#include <vector>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <tuple>
#include <span>
#include <cassert>
#include <stdexcept>

namespace Core
{


template<typename... Types>
class alignas(std::max_align_t) SoaArray
{
	static_assert((... && std::is_trivially_destructible_v<Types>),
                  "For simplicity, this implementation requires trivially destructible types. "
                  "Full support for non-trivially destructible types would need manual destructor calls.");
public: 
	using value_types = std::tuple<Types...>;

	static constexpr size_t NumTypes = sizeof...(Types);

	SoaArray() = default;

    // 禁止拷贝（简化实现），可根据需要添加
    SoaArray(const SoaArray&) = delete;
    SoaArray& operator=(const SoaArray&) = delete;

    // 移动构造 / 赋值
    SoaArray(SoaArray&&) = default;
    SoaArray& operator=(SoaArray&&) = default;

	~SoaArray() {
        // 如果类型不是 trivially destructible，需要在此调用每个元素的析构函数
        // 当前实现假定为 trivially destructible，不做额外处理
    }

	auto operator[](size_t index) const noexcept
	{
		return GetTupleAtConst(index, std::make_index_sequence<NumTypes>{});
	}

	auto operator[](size_t index) noexcept
	{
		return GetTupleAt(index, std::make_index_sequence<NumTypes>{});
	}

	void RemoveAt(size_t index)
	{

	}

	void Clear()
	{
		for (size_t i = 0; i < NumTypes; ++i)
		{
			using Type = std::tuple_element_t<i, value_types>;
			Type* column = (Type*)data_arrays[i];
			for (size_t j = 0; j < size; ++j)
			{
				std::destroy_at(column + j);
			}
		}
		size = 0;
	}

	template <typename... Args>
	void PushBack(std::tuple<Args...> args)
	{
		static_assert(sizeof...(Args) == NumTypes,
                      "Number of arguments must match number of types");
        if (size >= capacity)
		{
			Grow();
		}

		struct ConstructGuard
		{
			SoaArray *self;
			size_t constructed_counter = 0;
			~ConstructGuard()
			{
				if (constructed_counter < NumTypes)
				{
					for (size_t i = 0; i < constructed_counter; ++i)
					{
						using Type = std::tuple_element_t<i, value_types>;
						Type* dest = (Type*)self->data_arrays[i] + self->size;
						std::destroy_at(dest);
					}
				}
			}
		} guard {this, 0};

		[&]<size_t... Idx>(std::index_sequence<Idx...>)
		{
			((PushBackSigle<Idx>(std::get<Idx>(args)), ++guard.constructed_counter), ... );
		}(std::index_sequence_for<Types...>{});

		++size;
	}

private:
	template <size_t Idx>
	const auto& GetConstRef(size_t index) const 
	{
		using Type = std::tuple_element_t<Idx, value_types>;
		Type* column = (Type*)data_arrays[Idx];
		return column[index];
	}

	template <size_t Idx>
	auto& GetRef(size_t index) 
	{
		using Type = std::tuple_element_t<Idx, value_types>;
		Type* column = (Type*)data_arrays[Idx];
		return column[index];
	}

	template <size_t Idx>
	auto GetTupleAtConst(size_t index, std::index_sequence<Idx>) const
	{
		return std::tuple<const Type&...>(GetConstRef<Idx>(index)...);
	}

	template <size_t Idx>
	auto GetTupleAt(size_t index, std::index_sequence<Idx>)
	{
		return std::tuple<Type&...>(GetRef<Idx>(index)...);
	}


	template <size_t Idx, typename Arg>
	void PushBackSigle(Arg&& arg)
	{
		using Type = std::tuple_element_t<Idx, value_types>;
		Type* dest = (Type*)data_arrays[Idx] + size;
		std::construct_at(dest, std::forward<Arg>(arg));
	}

	void Grow()
	{
		size_t new_capacity = capacity == 0 ? 4 : capacity * 2;
		std::array<std::byte *, NumTypes> new_arrays{};
		for (size_t i = 0; i < NumTypes; ++i)
		{ 
			new_arrays[i] = static_cast<std::byte*>(::operator new(new_capacity * element_size<i>(), element_align<i>()));
		}

		struct NewMemoryGuard
		{
			std::array<std::byte*, NumTypes>& arrays;
			size_t count = 0;
			~NewMemoryGuard()
			{
				for (size_t i = 0; i < count; ++i)
				{
					::operator delete(arrays[i], element_align<i>());
				}
			}
		} new_memory_guard { new_arrays };

		[&]<size_t... Idx> (std::index_sequence<Idx...>)
		{
			(TransferColumn<Idx>(new_arrays[Idx], new_capacity), ...);
		}(std::index_sequence_for<Types...>{});

		for (size_t i = 0; i < NumTypes; ++i)
		{
			::operator delete(data_arrays[i], element_align<i>());
		}

		data_arrays = new_arrays;
		capacity = new_capacity;
		new_arrays.fill(nullptr);
	}

	template <size_t Idx>
	void TransferColumn(std::byte* new_memory, size_t new_capacity)
	{
		using Type = std::tuple_element_t<Idx, value_types>;
		Type* old_column = (Type*)data_arrays[Idx];
		Type* new_column = (Type*)new_memory;

		if constexpr (std::is_nothrow_move_constructible_v<Type>) 
		{
            for (size_t j = 0; j < size_; ++j) 
			{
                std::construct_at(new_column + j, std::move(old_column[j]));
            }
        } 
		else 
		{
            for (size_t j = 0; j < size_; ++j) 
			{
                std::construct_at(new_column + j, old_column[j]); // 拷贝构造，可能抛出异常
            }
        }

		for (size_t j = 0; j < size; ++j)
		{
			std::destroy_at(old_column + j);
		}
	}

	template <size_t I>
    static constexpr size_t element_size() 
	{
        return sizeof(std::tuple_element_t<I, value_types>);
    }

	template <size_t I>
    static constexpr std::align_val_t element_align() 
	{
        return std::align_val_t(alignof(std::tuple_element_t<I, value_types>));
    }
	std::array<std::byte*, NumTypes> data_arrays;
	size_t capacity { 0 };
	size_t size { 0 };
};
// Case1:
int id;
std::string name;

SoaArray<int, std::string> peoples;
peoples.PushBack({1, "Alice"});
peoples.PushBack({2, "Bob"});


struct Person
{
	std::string Name;
	int Age;
};


}