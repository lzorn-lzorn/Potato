#pragma once
// ============================================================================
// EventBus.hpp — 类型安全的事件总线系统
// ============================================================================
//
// 本文件实现了一个高性能、类型安全、线程安全的事件总线 (Event Bus) 系统,
// 支持发布-订阅 (Publish-Subscribe) 模式.
//
// ■ 核心组件:
//   Signal<Tag, Args...>    — 事件类型标识, 编译期绑定数据类型
//   Connection              — RAII 订阅连接句柄, 销毁时自动断开
//   ScopedConnectionGroup   — 管理多个 Connection 的 RAII 容器
//   EventBus                — 中央事件路由器, 管理所有信号通道
//   Publisher               — 事件发布者, 绑定到 EventBus
//   Acceptor                — 事件订阅者, 绑定到 EventBus
//
// ■ 设计要点:
//   1. 类型安全: 使用模板而非 std::any / void* / 虚函数等类型擦除技术,
//      所有公开 API 均在编译期保证类型匹配
//   2. 线程安全: EventBus 的所有操作可从任意线程安全调用;
//      内部采用 Copy-on-Write (写时复制) 策略, Emit 期间不持有锁,
//      因此回调中可以安全地 Subscribe/Unsubscribe (无递归死锁)
//   3. 高性能: Emit (热路径) 仅需一次 mutex lock/unlock 获取快照,
//      后续回调调用完全无锁; 异步路径使用 lock-free MPMC 队列
//   4. C++20: 使用 concepts、constexpr、requires、coroutines 等现代特性
//   5. 协程: 支持 co_await 等待事件, 用同步风格编写异步逻辑
//
// ■ 快速示例:
//   // 1. 定义信号
//   struct OnDamage : Core::Bus::Signal<OnDamage, int, std::string> {};
//
//   // 2. 创建总线 (也可使用全局默认总线 EventBus::GetDefault())
//   Core::Bus::EventBus bus;
//
//   // 3. 订阅
//   Core::Bus::Acceptor acceptor(bus);
//   acceptor.Subscribe<OnDamage>([](int dmg, const std::string& src) {
//       std::cout << src << " dealt " << dmg << " damage!" << std::endl;
//   });
//
//   // 4. 发布
//   Core::Bus::Publisher publisher(bus);
//   publisher.Emit<OnDamage>(50, std::string("Fireball"));
//
//   // 5. 协程等待 (返回 tuple<int, string>)
//   Core::Bus::EventTask WaitDamage(Core::Bus::EventBus& bus) {
//       auto [dmg, src] = co_await bus.Await<OnDamage>();
//   }
//
// ============================================================================

#include <atomic>
#include <cassert>
#include <concepts>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "MPMCQueue.h"
#include "Coroutine.h"

namespace Core::Bus
{

// ============================================================================
//  Section 1: TypeTag — 无 RTTI 的编译期类型标识
// ============================================================================
//
// 利用模板函数的实例化唯一性: 每个 TypeTag::Of<T>() 生成一个独立的函数,
// 其函数指针地址在整个进程中唯一, 可作为类型的运行时标识.
//
// 优势:
//   - 不依赖 RTTI (-fno-rtti / /GR- 下也能编译)
//   - 零运行时开销 (仅取函数指针地址, 无字符串比较)
//   - 可用于 unordered_map 的 key
//
using TypeId = void(*)();

template<typename T>
struct TypeTag
{
    static void Id() {} // 每个 T 实例化一个独立函数
    static constexpr TypeId Value = &Id;
};

/// TypeId 的哈希器, 用于 unordered_map
struct TypeIdHash
{
    size_t operator()(TypeId id) const noexcept
    {
        return std::hash<void(*)()>{}(id);
    }
};


// ============================================================================
//  Section 2: Signal — 事件类型标识
// ============================================================================
//
// Signal 使用 CRTP (Curiously Recurring Template Pattern) 模式,
// 让每个派生结构体拥有唯一的编译期类型标识.
// 模板参数 Args... 定义了事件携带的数据类型.
//
// 示例:
//   // 携带 int 和 string 数据的伤害事件
//   struct OnDamage : Signal<OnDamage, int, std::string> {};
//
//   // 不携带数据的死亡事件
//   struct OnDeath : Signal<OnDeath> {};
//
//   // 携带闭包的异步任务事件
//   struct OnTask : Signal<OnTask, std::function<void()>> {};
//
//   // 携带自定义对象的事件
//   struct DamageInfo { int amount; std::string source; };
//   struct OnDamageV2 : Signal<OnDamageV2, DamageInfo> {};
//
template<typename Tag, typename... Args>
struct Signal
{
    /// 事件携带的数据类型元组
    using ArgTypes = std::tuple<Args...>;

    /// 回调函数签名: void(const Arg1&, const Arg2&, ...)
    using CallbackType = std::function<void(const Args&...)>;

    /// 获取此信号类型的唯一标识 (无 RTTI, 基于函数指针地址)
    static constexpr TypeId GetTypeId() { return TypeTag<Tag>::Value; }
};


// ============================================================================
//  Section 3: IsSignal Concept — 约束类型必须为 Signal 派生类
// ============================================================================

template<typename T>
concept IsSignal = requires
{
    typename T::ArgTypes;
    typename T::CallbackType;
    { T::GetTypeId() } -> std::same_as<TypeId>;
};


// ============================================================================
//  Section 3: Connection — RAII 订阅连接句柄
// ============================================================================
//
// 每次 Subscribe 返回一个 Connection 对象.
// 当 Connection 被销毁或调用 Disconnect() 时, 对应的订阅会自动断开.
// Connection 是 move-only 的, 不可复制.
//
// 生命周期注意:
//   Connection 内部引用了 EventBus 中的 Channel.
//   请确保 EventBus 的生命周期 >= 所有 Connection 的生命周期.
//   使用 Acceptor 时无需担心此问题 — Acceptor 统一管理 Connection 生命周期.
//
class Connection
{
    std::function<void()> DisconnectFn;
    bool Connected = false;

public:
    Connection() = default;

    explicit Connection(std::function<void()> disconnectFn)
        : DisconnectFn(std::move(disconnectFn))
        , Connected(true)
    {}

    ~Connection() { Disconnect(); }

    // Move-only
    Connection(const Connection&)            = delete;
    Connection& operator=(const Connection&) = delete;

    Connection(Connection&& other) noexcept
        : DisconnectFn(std::move(other.DisconnectFn))
        , Connected(other.Connected)
    {
        other.Connected = false;
    }

    Connection& operator=(Connection&& other) noexcept
    {
        if (this != &other)
        {
            Disconnect();
            DisconnectFn = std::move(other.DisconnectFn);
            Connected    = other.Connected;
            other.Connected = false;
        }
        return *this;
    }

    /// 手动断开此订阅
    void Disconnect()
    {
        if (Connected && DisconnectFn)
        {
            DisconnectFn();
            Connected = false;
        }
    }

    /// 查询连接状态
    [[nodiscard]] bool IsConnected() const noexcept { return Connected; }
};


// ============================================================================
//  Section 4: ScopedConnectionGroup — 连接组 (RAII 批量管理)
// ============================================================================
//
// 将多个 Connection 收集到一个组中, 统一管理生命周期.
// 组被销毁时, 所有连接自动断开.
//
// 示例:
//   ScopedConnectionGroup group;
//   group.Add(bus.Subscribe<OnDamage>(...));
//   group.Add(bus.Subscribe<OnHeal>(...));
//   group.DisconnectAll(); // 或等待 group 析构
//
class ScopedConnectionGroup
{
    std::vector<Connection> Connections;

public:
    /// 添加一个连接到组中
    void Add(Connection conn)
    {
        Connections.push_back(std::move(conn));
    }

    /// 断开并移除所有连接
    void DisconnectAll()
    {
        Connections.clear();
    }

    /// 获取组中的连接数量
    [[nodiscard]] size_t Count() const noexcept { return Connections.size(); }
};


// ============================================================================
//  Section 5: IChannel — 通道基类 (内部使用)
// ============================================================================
//
// 仅用于 EventBus 内部存储异构 Channel 对象.
// 所有公开 API 均通过模板直接操作具体的 Channel<Args...>,
// 不涉及 IChannel 接口 — 用户代码永远不会接触到此类.
//
struct IChannel
{
    virtual ~IChannel() = default;

    /// FlushAsyncEvents — 刷新异步队列中的所有待处理事件
    /// 返回: 本次刷新处理的事件数量
    virtual size_t FlushAsyncEvents() = 0;
};


// ============================================================================
//  Section 6: Channel<Args...> — 类型安全的事件通道
// ============================================================================
//
// 每种 Signal<Tag, Args...> 对应一个 Channel<Args...>,
// 管理该信号的所有订阅回调.
//
// ■ 线程安全策略 — Copy-on-Write (写时复制):
//
//   订阅列表 (SlotList) 通过 shared_ptr 持有, 保证以下特性:
//
//   Emit (读操作, 热路径):
//     1. 获取 mutex, 拷贝 shared_ptr (引用计数 +1), 释放 mutex
//     2. 迭代快照, 调用回调 — 此时不持有任何锁
//     → 多个线程可以并发 Emit
//     → 回调内可以安全调用 Subscribe / Unsubscribe (无递归死锁风险)
//
//   Subscribe / Unsubscribe (写操作, 冷路径):
//     1. 获取 mutex
//     2. 复制当前 SlotList 为新副本
//     3. 在副本上增删 Slot
//     4. 用新副本替换 shared_ptr
//     5. 释放 mutex
//     → 正在迭代旧快照的 Emit 不受影响 (它持有旧 shared_ptr)
//
// ■ 一次性订阅 (OneShot):
//   通过 atomic<bool> 保证即使多线程并发 Emit, 一次性回调也只执行一次.
//
template<typename... Args>
class Channel final : public IChannel
{
public:
    using CallbackType = std::function<void(const Args&...)>;
    using SlotId       = uint64_t;

    /// Slot — 订阅槽, 存储一个回调及其元信息
    struct Slot
    {
        SlotId       Id;
        CallbackType Callback;
        bool         OneShot;

        // 一次性订阅的 "已触发" 标记.
        // 使用 shared_ptr 以便在 CoW 快照间共享状态.
        // 仅 OneShot == true 时非空.
        std::shared_ptr<std::atomic<bool>> Fired;
    };

private:
    using SlotList    = std::vector<Slot>;
    using SlotListPtr = std::shared_ptr<const SlotList>;

    // Copy-on-Write 核心: mutex 只保护 SlotsPtr 的读写, 不保护回调执行
    mutable std::mutex Mutex;
    SlotListPtr        SlotsPtr = std::make_shared<const SlotList>();

    std::atomic<SlotId>   NextId{1};
    std::atomic<uint64_t> EmitCount{0};

    // 异步事件队列 (lock-free MPMC)
    MPMCQueue<std::tuple<Args...>, 4096> AsyncQueue;

    /// 获取当前订阅列表的快照 (线程安全)
    SlotListPtr GetSnapshot() const
    {
        std::lock_guard lock(Mutex);
        return SlotsPtr;
    }

public:
    // ----------------------------------------------------------------
    // Subscribe — 添加订阅
    //
    // 参数:
    //   callback — 回调函数, 签名为 void(const Args&...)
    //   oneShot  — 是否为一次性订阅 (触发一次后自动移除)
    //
    // 返回: 订阅 ID, 用于 Unsubscribe
    // ----------------------------------------------------------------
    SlotId Subscribe(CallbackType callback, bool oneShot = false)
    {
        SlotId id = NextId.fetch_add(1, std::memory_order_relaxed);

        std::lock_guard lock(Mutex);
        // CoW: 复制现有列表, 追加新 Slot, 替换 shared_ptr
        auto newSlots = std::make_shared<SlotList>(*SlotsPtr);
        newSlots->push_back(Slot{
            .Id       = id,
            .Callback = std::move(callback),
            .OneShot  = oneShot,
            .Fired    = oneShot
                ? std::make_shared<std::atomic<bool>>(false)
                : nullptr,
        });
        SlotsPtr = std::move(newSlots);

        return id;
    }

    // ----------------------------------------------------------------
    // Unsubscribe — 移除订阅 (按 ID)
    // ----------------------------------------------------------------
    void Unsubscribe(SlotId id)
    {
        std::lock_guard lock(Mutex);
        auto newSlots = std::make_shared<SlotList>(*SlotsPtr);
        std::erase_if(*newSlots, [id](const Slot& s) { return s.Id == id; });
        SlotsPtr = std::move(newSlots);
    }

    // ----------------------------------------------------------------
    // Emit — 同步发布事件
    //
    // 在调用线程上立即执行所有订阅者的回调.
    // Emit 期间不持有锁, 因此:
    //   - 多个线程可并发 Emit
    //   - 回调中可安全调用 Subscribe / Unsubscribe
    //
    // 对于 OneShot 订阅: 使用 CAS 保证即使并发 Emit 也只执行一次
    // ----------------------------------------------------------------
    void Emit(const Args&... args)
    {
        EmitCount.fetch_add(1, std::memory_order_relaxed);

        // 步骤 1: 获取快照 (短暂持锁, 仅拷贝 shared_ptr)
        SlotListPtr snapshot = GetSnapshot();

        // 步骤 2: 遍历快照调用回调 (无锁)
        std::vector<SlotId> oneShotsToRemove;

        for (const auto& slot : *snapshot)
        {
            if (slot.OneShot)
            {
                // CAS: 仅当 Fired 从 false → true 成功时才执行 (精确一次)
                bool expected = false;
                if (!slot.Fired->compare_exchange_strong(expected, true))
                    continue;
            }
            slot.Callback(args...);
            if (slot.OneShot)
                oneShotsToRemove.push_back(slot.Id);
        }

        // 步骤 3: 清理已触发的 OneShot 订阅
        if (!oneShotsToRemove.empty())
        {
            std::lock_guard lock(Mutex);
            auto newSlots = std::make_shared<SlotList>(*SlotsPtr);
            for (auto id : oneShotsToRemove)
                std::erase_if(*newSlots, [id](const Slot& s) { return s.Id == id; });
            SlotsPtr = std::move(newSlots);
        }
    }

    // ----------------------------------------------------------------
    // EnqueueAsync — 将事件放入异步队列 (不立即执行回调)
    //
    // 事件会被存入 lock-free MPMC 队列, 等待 FlushAsyncEvents() 时分发.
    // 适用场景: 生产者线程产生事件, 消费者线程 (如主线程) 统一处理.
    //
    // 返回: true = 入队成功, false = 队列已满
    // ----------------------------------------------------------------
    bool EnqueueAsync(Args... args)
    {
        return AsyncQueue.TryPush(std::make_tuple(std::move(args)...));
    }

    // ----------------------------------------------------------------
    // FlushAsyncEvents — 刷新异步队列, 分发所有待处理事件
    //
    // 将队列中的事件逐一取出并调用 Emit 分发.
    // 通常在主线程的帧循环中调用一次.
    //
    // 返回: 本次处理的事件数量
    // ----------------------------------------------------------------
    size_t FlushAsyncEvents() override
    {
        size_t count = 0;
        while (auto event = AsyncQueue.TryPop())
        {
            std::apply([this](const Args&... args) { Emit(args...); }, *event);
            ++count;
        }
        return count;
    }

    // ----------------------------------------------------------------
    // 查询接口
    // ----------------------------------------------------------------

    /// 当前订阅者数量
    [[nodiscard]] size_t SubscriberCount() const
    {
        return GetSnapshot()->size();
    }

    /// 累计 Emit 次数 (含同步和异步刷新)
    [[nodiscard]] uint64_t GetEmitCount() const noexcept
    {
        return EmitCount.load(std::memory_order_relaxed);
    }

    /// 异步队列中待处理的事件数量 (近似值)
    [[nodiscard]] size_t PendingAsyncCount() const noexcept
    {
        return AsyncQueue.ApproxSize();
    }
};


// ============================================================================
//  Section 7: 类型辅助 — 从 Signal 推导 Channel 类型
// ============================================================================

namespace Detail
{
    template<typename Tuple>
    struct TupleToChannel;

    template<typename... Args>
    struct TupleToChannel<std::tuple<Args...>>
    {
        using Type = Channel<Args...>;
    };
}

/// 从 Signal 类型推导对应的 Channel 类型
/// 例: ChannelFor<OnDamage> → Channel<int, std::string>
template<IsSignal S>
using ChannelFor = typename Detail::TupleToChannel<typename S::ArgTypes>::Type;


// ============================================================================
//  Section 8: EventBus — 中央事件总线
// ============================================================================
//
// EventBus 是事件系统的核心路由器, 管理所有信号通道.
//
// ■ 使用方式:
//   1. 全局默认总线:  EventBus::GetDefault()
//      → 整个程序共享一个总线, 适用于全局事件 (如游戏状态变更)
//
//   2. 作用域总线:    EventBus myBus;
//      → 独立的总线实例, 适用于子系统内部事件 (如 UI 子系统)
//
// ■ 线程安全:
//   EventBus 的所有公开方法均线程安全 (通过内部 Channel 的 CoW 机制保证).
//
class EventBus
{
    mutable std::mutex ChannelMutex;
    std::unordered_map<TypeId, std::unique_ptr<IChannel>, TypeIdHash> Channels;

    /// 获取或创建指定 Signal 的 Channel
    template<IsSignal SignalType>
    ChannelFor<SignalType>& GetOrCreateChannel()
    {
        using ChannelType = ChannelFor<SignalType>;
        auto typeId = SignalType::GetTypeId();

        std::lock_guard lock(ChannelMutex);
        auto& ptr = Channels[typeId];
        if (!ptr)
            ptr = std::make_unique<ChannelType>();
        return static_cast<ChannelType&>(*ptr);
    }

public:
    EventBus() = default;

    // 不可复制, 不可移动 (Channel 中的闭包可能引用此 bus)
    EventBus(const EventBus&)            = delete;
    EventBus& operator=(const EventBus&) = delete;

    /// 获取全局默认 EventBus 单例
    static EventBus& GetDefault()
    {
        static EventBus instance;
        return instance;
    }

    // ----------------------------------------------------------------
    // Subscribe — 订阅信号
    //
    // 模板参数: SignalType — 要监听的信号类型
    // 参数:
    //   callback — 回调函数, 签名 void(const Args&...)
    //   oneShot  — 是否为一次性订阅
    //
    // 返回: Connection (RAII 句柄, 销毁时自动取消订阅)
    //
    // 示例:
    //   auto conn = bus.Subscribe<OnDamage>(
    //       [](int dmg, const std::string& src) { ... });
    // ----------------------------------------------------------------
    template<IsSignal SignalType>
    [[nodiscard]] Connection Subscribe(
        typename SignalType::CallbackType callback,
        bool oneShot = false)
    {
        auto& channel = GetOrCreateChannel<SignalType>();
        auto slotId   = channel.Subscribe(std::move(callback), oneShot);

        return Connection([&channel, slotId]() {
            channel.Unsubscribe(slotId);
        });
    }

    // ----------------------------------------------------------------
    // SubscribeFiltered — 带过滤条件的订阅
    //
    // 仅当 filter 返回 true 时才执行 callback.
    //
    // 示例:
    //   bus.SubscribeFiltered<OnDamage>(
    //       [](int dmg, const std::string&) { return dmg > 10; },
    //       [](int dmg, const std::string& src) {
    //           std::cout << "Heavy hit from " << src << std::endl;
    //       });
    // ----------------------------------------------------------------
    template<IsSignal SignalType, typename FilterFn>
    [[nodiscard]] Connection SubscribeFiltered(
        FilterFn&& filter,
        typename SignalType::CallbackType callback)
    {
        // 将 filter 和 callback 封装为一个新的回调
        auto wrapped = [filt = std::forward<FilterFn>(filter),
                        cb   = std::move(callback)]
                       (const auto&... args)
        {
            if (filt(args...))
                cb(args...);
        };

        return Subscribe<SignalType>(
            typename SignalType::CallbackType{std::move(wrapped)});
    }

    // ----------------------------------------------------------------
    // Emit — 同步发布事件
    //
    // 在调用线程上立即触发所有订阅者的回调.
    //
    // 模板参数: SignalType — 要发布的信号类型
    // 参数:     args...   — 事件携带的数据, 类型必须与 Signal 定义匹配
    //
    // 示例:
    //   bus.Emit<OnDamage>(50, std::string("Fireball"));
    // ----------------------------------------------------------------
    template<IsSignal SignalType, typename... EmitArgs>
    void Emit(EmitArgs&&... args)
    {
        auto& channel = GetOrCreateChannel<SignalType>();
        channel.Emit(std::forward<EmitArgs>(args)...);
    }

    // ----------------------------------------------------------------
    // EmitAsync — 异步发布事件 (入队, 不立即执行)
    //
    // 事件被放入 lock-free 队列, 等待 FlushAsync / FlushAllAsync 时分发.
    // 适用场景: 工作线程产生事件, 主线程统一处理.
    //
    // 返回: true = 入队成功, false = 队列已满
    // ----------------------------------------------------------------
    template<IsSignal SignalType, typename... EmitArgs>
    bool EmitAsync(EmitArgs&&... args)
    {
        auto& channel = GetOrCreateChannel<SignalType>();
        return channel.EnqueueAsync(std::forward<EmitArgs>(args)...);
    }

    // ----------------------------------------------------------------
    // FlushAsync — 刷新指定信号的异步队列
    // ----------------------------------------------------------------
    template<IsSignal SignalType>
    size_t FlushAsync()
    {
        auto& channel = GetOrCreateChannel<SignalType>();
        return channel.FlushAsyncEvents();
    }

    // ----------------------------------------------------------------
    // FlushAllAsync — 刷新所有信号的异步队列
    //
    // 通常在主线程的帧循环中每帧调用一次:
    //   void GameLoop() {
    //       bus.FlushAllAsync();
    //       // ... 后续逻辑 ...
    //   }
    // ----------------------------------------------------------------
    size_t FlushAllAsync()
    {
        size_t total = 0;
        std::lock_guard lock(ChannelMutex);
        for (auto& [typeId, channel] : Channels)
            total += channel->FlushAsyncEvents();
        return total;
    }

    // ----------------------------------------------------------------
    // 查询接口
    // ----------------------------------------------------------------

    /// 获取指定信号的订阅者数量
    template<IsSignal SignalType>
    [[nodiscard]] size_t SubscriberCount()
    {
        auto& channel = GetOrCreateChannel<SignalType>();
        return channel.SubscriberCount();
    }

    /// 检查指定信号是否有订阅者
    template<IsSignal SignalType>
    [[nodiscard]] bool HasSubscribers()
    {
        return SubscriberCount<SignalType>() > 0;
    }

    // ----------------------------------------------------------------
    // Await — 协程: 一次性等待某个信号 (co_await)
    //
    // 挂起当前协程, 直到信号被 Emit, 返回事件数据.
    //
    // 返回: EventAwaiter<SignalType> (可被 co_await)
    //       co_await 的结果为 std::tuple<Args...>
    //
    // 示例:
    //   EventTask MyCoroutine(EventBus& bus) {
    //       auto [dmg, src] = co_await bus.Await<OnDamage>();
    //       std::cout << src << " dealt " << dmg << std::endl;
    //   }
    //
    // 注意:
    //   一次性等待 — co_await 后订阅自动断开.
    //   持续监听请使用 Stream<SignalType>().
    // ----------------------------------------------------------------
    template<IsSignal SignalType>
    [[nodiscard]] EventAwaiter<SignalType> Await()
    {
        return EventAwaiter<SignalType>(*this);
    }

    // ----------------------------------------------------------------
    // Stream — 协程: 持续监听信号流 (循环 co_await)
    //
    // 返回一个 EventStream 对象, 可在循环中反复 co_await.
    // 每次 Emit 后恢复协程, 返回最新的事件数据.
    //
    // 返回: EventStream<SignalType>
    //       co_await stream 的结果为 std::optional<std::tuple<Args...>>
    //         - 有值: 收到事件
    //         - nullopt: 流已停止 (Stop() 或析构)
    //
    // 示例:
    //   EventTask MonitorDamage(EventBus& bus) {
    //       auto stream = bus.Stream<OnDamage>();
    //       while (auto event = co_await stream) {
    //           auto& [dmg, src] = *event;
    //           std::cout << src << " → " << dmg << std::endl;
    //           if (dmg > 100) break; // 退出循环, stream 析构自动断开
    //       }
    //   }
    // ----------------------------------------------------------------
    template<IsSignal SignalType>
    [[nodiscard]] EventStream<SignalType> Stream()
    {
        return EventStream<SignalType>(*this);
    }
};


// ============================================================================
//  Section 9: Publisher — 事件发布者
// ============================================================================
//
// Publisher 是 EventBus 的发布端封装, 嵌入到需要发布事件的类中.
//
// ■ 线程安全:
//   Publisher 本身 (Bind/赋值) 不是线程安全的 — 应在单一线程中初始化.
//   Emit / EmitAsync 通过 EventBus 保证线程安全, 可从任意线程调用.
//
// ■ 用法:
//   class Player {
//       Core::Bus::Publisher publisher;
//   public:
//       Player(Core::Bus::EventBus& bus) : publisher(bus) {}
//       void TakeDamage(int dmg) {
//           publisher.Emit<OnDamage>(dmg, std::string("Monster"));
//       }
//   };
//
class Publisher
{
    EventBus* Bus = nullptr;

public:
    Publisher() = default;
    explicit Publisher(EventBus& bus) : Bus(&bus) {}

    /// 绑定到 EventBus (延迟初始化)
    void Bind(EventBus& bus) { Bus = &bus; }

    /// 同步发布事件
    template<IsSignal SignalType, typename... EmitArgs>
    void Emit(EmitArgs&&... args)
    {
        assert(Bus != nullptr && "Publisher::Emit — Publisher 未绑定到 EventBus");
        Bus->Emit<SignalType>(std::forward<EmitArgs>(args)...);
    }

    /// 异步发布事件 (入队)
    template<IsSignal SignalType, typename... EmitArgs>
    bool EmitAsync(EmitArgs&&... args)
    {
        assert(Bus != nullptr && "Publisher::EmitAsync — Publisher 未绑定到 EventBus");
        return Bus->EmitAsync<SignalType>(std::forward<EmitArgs>(args)...);
    }

    /// 检查是否已绑定
    [[nodiscard]] bool IsBound() const noexcept { return Bus != nullptr; }
};


// ============================================================================
//  Section 10: Acceptor — 事件订阅者
// ============================================================================
//
// Acceptor 是 EventBus 的订阅端封装, 嵌入到需要监听事件的类中.
// 当 Acceptor 被销毁时, 其管理的所有订阅自动断开.
//
// ■ 线程安全:
//   Acceptor 本身 (Subscribe/Bind) 不是线程安全的 — 应在单一线程中操作.
//   EventBus 上的回调执行通过 EventBus 保证线程安全.
//
// ■ 用法:
//   class UI {
//       Core::Bus::Acceptor acceptor;
//   public:
//       UI(Core::Bus::EventBus& bus) : acceptor(bus) {
//           acceptor.Subscribe<OnDamage>([this](int dmg, const std::string& src) {
//               ShowDamageNumber(dmg);
//           });
//       }
//   };
//
// ■ 支持链式调用:
//   acceptor.Subscribe<OnDamage>([](int, const std::string&) { ... })
//           .Subscribe<OnHeal>([](int amount) { ... })
//           .SubscribeOnce<OnDeath>([]() { ... });
//
class Acceptor
{
    EventBus*                  Bus = nullptr;
    std::vector<Connection>    Connections;

public:
    Acceptor() = default;
    explicit Acceptor(EventBus& bus) : Bus(&bus) {}

    // 析构时自动断开所有订阅 (由 vector<Connection> 的析构保证)
    ~Acceptor() = default;

    // Move-only
    Acceptor(Acceptor&&)            = default;
    Acceptor& operator=(Acceptor&&) = default;
    Acceptor(const Acceptor&)            = delete;
    Acceptor& operator=(const Acceptor&) = delete;

    /// 绑定到 EventBus (延迟初始化)
    void Bind(EventBus& bus) { Bus = &bus; }

    // ----------------------------------------------------------------
    // Subscribe — 订阅信号
    //
    // 返回 *this 以支持链式调用.
    // 订阅生命周期由 Acceptor 管理, 无需手动断开.
    //
    // 示例:
    //   acceptor.Subscribe<OnDamage>([](int dmg, const std::string& src) {
    //       std::cout << src << " → " << dmg << std::endl;
    //   });
    // ----------------------------------------------------------------
    template<IsSignal SignalType>
    Acceptor& Subscribe(typename SignalType::CallbackType callback)
    {
        assert(Bus != nullptr && "Acceptor::Subscribe — Acceptor 未绑定到 EventBus");
        Connections.push_back(Bus->Subscribe<SignalType>(std::move(callback)));
        return *this;
    }

    // ----------------------------------------------------------------
    // SubscribeOnce — 一次性订阅 (触发一次后自动移除)
    //
    // 即使多个线程并发触发, 回调也保证只执行一次 (内部使用 CAS).
    //
    // 示例:
    //   acceptor.SubscribeOnce<OnGameStart>([]() {
    //       std::cout << "Game started!" << std::endl;
    //   });
    // ----------------------------------------------------------------
    template<IsSignal SignalType>
    Acceptor& SubscribeOnce(typename SignalType::CallbackType callback)
    {
        assert(Bus != nullptr && "Acceptor::SubscribeOnce — Acceptor 未绑定到 EventBus");
        Connections.push_back(
            Bus->Subscribe<SignalType>(std::move(callback), /*oneShot=*/true));
        return *this;
    }

    // ----------------------------------------------------------------
    // SubscribeFiltered — 带过滤条件的订阅
    //
    // 仅当 filter(args...) 返回 true 时执行 callback.
    //
    // 示例:
    //   // 只监听伤害 > 50 的事件
    //   acceptor.SubscribeFiltered<OnDamage>(
    //       [](int dmg, const std::string&) { return dmg > 50; },
    //       [](int dmg, const std::string& src) {
    //           std::cout << "Critical hit from " << src << "!" << std::endl;
    //       });
    // ----------------------------------------------------------------
    template<IsSignal SignalType, typename FilterFn>
    Acceptor& SubscribeFiltered(
        FilterFn&& filter,
        typename SignalType::CallbackType callback)
    {
        assert(Bus != nullptr && "Acceptor::SubscribeFiltered — Acceptor 未绑定到 EventBus");
        Connections.push_back(
            Bus->SubscribeFiltered<SignalType>(
                std::forward<FilterFn>(filter), std::move(callback)));
        return *this;
    }

    // ----------------------------------------------------------------
    // 管理接口
    // ----------------------------------------------------------------

    /// 断开所有订阅
    void DisconnectAll()
    {
        Connections.clear();
    }

    /// 当前活跃的订阅数量
    [[nodiscard]] size_t ConnectionCount() const noexcept
    {
        return Connections.size();
    }

    /// 检查是否已绑定
    [[nodiscard]] bool IsBound() const noexcept { return Bus != nullptr; }

    // ----------------------------------------------------------------
    // Await — 协程: 一次性等待信号
    //
    // 与 EventBus::Await 相同, 但通过 Acceptor 绑定的 Bus 调用.
    //
    // 示例:
    //   EventTask HandleEvents(Acceptor& acceptor) {
    //       auto [dmg, src] = co_await acceptor.Await<OnDamage>();
    //   }
    // ----------------------------------------------------------------
    template<IsSignal SignalType>
    [[nodiscard]] EventAwaiter<SignalType> Await()
    {
        assert(Bus != nullptr && "Acceptor::Await — Acceptor 未绑定到 EventBus");
        return Bus->Await<SignalType>();
    }

    // ----------------------------------------------------------------
    // Stream — 协程: 持续监听信号流
    //
    // 示例:
    //   EventTask Monitor(Acceptor& acceptor) {
    //       auto stream = acceptor.Stream<OnDamage>();
    //       while (auto ev = co_await stream) { ... }
    //   }
    // ----------------------------------------------------------------
    template<IsSignal SignalType>
    [[nodiscard]] EventStream<SignalType> Stream()
    {
        assert(Bus != nullptr && "Acceptor::Stream — Acceptor 未绑定到 EventBus");
        return Bus->Stream<SignalType>();
    }
};


// ============================================================================
//  延迟实现: EventAwaiter::await_suspend
// ============================================================================
//
// 此方法需要 EventBus 的完整定义, 因此放在 EventBus 类之后.
//
// 流程:
//   1. 向 EventBus 注册一个 OneShot 订阅
//   2. 回调中: 将事件参数打包为 tuple, 存入 Result
//   3. 恢复协程 (handle.resume())
//   4. Connection 保存在 Awaiter 中, 保证生命周期
//
template<typename SignalType>
    requires IsSignal<SignalType>
void EventAwaiter<SignalType>::await_suspend(std::coroutine_handle<> handle)
{
    // 注册 OneShot 订阅: 事件到达时写入 Result 并恢复协程
    Conn = Bus.Subscribe<SignalType>(
        [this, handle](const auto&... args)
        {
            Result.emplace(args...);
            handle.resume();
        },
        /*oneShot=*/true
    );
}


// ============================================================================
//  延迟实现: EventStream::Awaiter::await_suspend
// ============================================================================
//
// 流程:
//   1. 保存协程 handle 到 Stream.WaitingHandle
//   2. 如果是首次 co_await, 注册持久订阅 (非 OneShot)
//      回调中: 将数据写入 Buffer, 恢复协程
//   3. 后续 co_await 复用同一订阅, 仅更新 WaitingHandle
//
template<typename SignalType>
    requires IsSignal<SignalType>
void EventStream<SignalType>::Awaiter::await_suspend(
    std::coroutine_handle<> handle)
{
    Stream.WaitingHandle = handle;

    // 首次 co_await 时注册订阅
    if (!Stream.Conn.IsConnected() && !Stream.Stopped)
    {
        Stream.Conn = Stream.Bus.template Subscribe<SignalType>(
            [&stream = Stream](const auto&... args)
            {
                stream.Buffer.emplace(args...);
                if (stream.WaitingHandle)
                {
                    auto h = stream.WaitingHandle;
                    stream.WaitingHandle = nullptr;
                    h.resume();
                }
                // 如果没有协程在等待, 数据留在 Buffer 中
                // 下次 co_await 时 await_ready 仍返回 false,
                // 但 await_resume 会立即取走 Buffer 中的数据.
                // 注意: 如果 Emit 频率 > co_await 频率, 中间的事件会被覆盖.
                // 这是 Stream 的设计意图: 始终获取最新事件.
            }
        );
    }
}

} // namespace Core::Bus
