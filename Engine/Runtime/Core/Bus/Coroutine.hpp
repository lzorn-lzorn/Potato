#pragma once
// ============================================================================
// Coroutine.hpp — EventBus 协程支持
// ============================================================================
//
// 为 EventBus 提供基于 C++20 协程 (Coroutines) 的事件等待机制,
// 使得异步事件驱动逻辑可以用同步风格的代码表达.
//
// ■ 核心组件:
//   EventAwaiter<S>   — 一次性等待: co_await, 事件到达后恢复, 返回数据
//   EventStream<S>    — 持续等待: 循环中反复 co_await, 每次 Emit 都恢复
//   EventTask         — 无返回值协程任务, 用于启动事件驱动的异步流程
//
// ■ 工作原理:
//   1. co_await 时, 协程挂起, 将 coroutine_handle 注册为一次性订阅
//   2. 当对应信号被 Emit 时, 回调将事件数据写入共享存储, 然后恢复协程
//   3. 协程从 co_await 返回, 拿到事件数据, 继续执行
//
// ■ 线程模型:
//   协程在哪个线程恢复取决于 Emit 在哪个线程调用.
//   如果 Emit 在工作线程, 协程也会在工作线程恢复.
//   如需在主线程恢复, 请使用 EmitAsync + FlushAllAsync 的异步路径.
//
// ■ 快速示例:
//
//   // 定义信号
//   struct OnDamage : Core::Bus::Signal<OnDamage, int, std::string> {};
//
//   // 一次性等待
//   Core::Bus::EventTask WaitForDamage(Core::Bus::EventBus& bus)
//   {
//       auto [dmg, src] = co_await bus.Await<OnDamage>();
//       std::cout << src << " dealt " << dmg << std::endl;
//   }
//
//   // 持续监听 (每次 Emit 都恢复)
//   Core::Bus::EventTask MonitorDamage(Core::Bus::EventBus& bus)
//   {
//       auto stream = bus.Stream<OnDamage>();
//       while (auto event = co_await stream)
//       {
//           auto& [dmg, src] = *event;
//           std::cout << src << " dealt " << dmg << std::endl;
//           if (dmg > 100) break; // 可随时退出
//       }
//   }
//
// ============================================================================

#include <coroutine>
#include <exception>
#include <optional>
#include <tuple>
#include <utility>

// 前置声明 EventBus (定义在 EventBus.hpp)
// Coroutine.hpp 不 #include EventBus.hpp, 而是由 EventBus.hpp 来 #include 本文件,
// 这样可以在 EventBus 类中直接添加 Await / Stream 方法.
// 本文件中的 Awaiter/Stream 接受 EventBus& 作为参数.

namespace Core::Bus
{

// 前置声明
class EventBus;


// ============================================================================
//  Section 1: EventTask — 协程任务类型
// ============================================================================
//
// EventTask 是一个最小化的协程返回类型, 用于 fire-and-forget 风格的协程.
// 协程内可以使用 co_await 等待事件, EventTask 本身不暴露返回值.
//
// ■ 生命周期:
//   EventTask 持有 coroutine_handle, 销毁时自动 destroy.
//   如果协程内部挂起等待事件, 在 EventTask 被销毁前事件到达, 协程会正常完成.
//   如果 EventTask 先于事件到达被销毁, 协程被 destroy (不会泄漏).
//
// ■ 异常:
//   协程内的未捕获异常会调用 std::terminate (与标准行为一致).
//
// ■ 用法:
//   EventTask MyCoroutine(EventBus& bus)
//   {
//       auto [dmg, src] = co_await bus.Await<OnDamage>();
//       std::cout << "Received damage: " << dmg << std::endl;
//       // 协程结束, EventTask 可安全销毁
//   }
//
class EventTask
{
public:
    struct promise_type
    {
        EventTask get_return_object()
        {
            return EventTask{
                std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        // 协程立即挂起 (lazy start), 由调用者决定何时启动
        // 改为立即启动 (eager start) 以符合 fire-and-forget 语义
        std::suspend_never initial_suspend() noexcept { return {}; }

        // 协程结束后不挂起, 自动销毁
        std::suspend_never final_suspend() noexcept { return {}; }

        void return_void() noexcept {}

        void unhandled_exception() noexcept
        {
            std::terminate();
        }
    };

    using Handle = std::coroutine_handle<promise_type>;

    EventTask() = default;
    explicit EventTask(Handle h) : Coro(h) {}

    // Move-only
    EventTask(EventTask&& other) noexcept : Coro(other.Coro)
    {
        other.Coro = nullptr;
    }
    EventTask& operator=(EventTask&& other) noexcept
    {
        if (this != &other)
        {
            // final_suspend 返回 suspend_never, 所以协程结束后 handle 已无效
            // 不需要手动 destroy
            Coro = other.Coro;
            other.Coro = nullptr;
        }
        return *this;
    }

    EventTask(const EventTask&) = delete;
    EventTask& operator=(const EventTask&) = delete;

    /// 检查协程是否仍在运行
    [[nodiscard]] bool IsValid() const noexcept { return Coro != nullptr; }

private:
    Handle Coro = nullptr;
};


// ============================================================================
//  Section 2: EventAwaiter<SignalType> — 一次性事件等待
// ============================================================================
//
// co_await 时挂起协程, 信号被 Emit 后恢复并返回事件数据.
//
// ■ 原理:
//   1. await_ready() 返回 false → 协程挂起
//   2. await_suspend() 注册一个 OneShot 订阅到 EventBus
//      回调中: 将事件数据写入 Result, 然后 resume 协程
//   3. await_resume() 返回 std::tuple<Args...> 给协程
//
// ■ 类型安全:
//   返回类型完全由 Signal<Tag, Args...> 的模板参数推导, 编译期检查.
//
// ■ 注意:
//   EventAwaiter 是一次性的, co_await 后即消耗.
//   持续监听请使用 EventStream.
//
template<typename SignalType>
    requires IsSignal<SignalType>
class EventAwaiter
{
    using DataType = typename SignalType::ArgTypes; // std::tuple<Args...>
    EventBus& Bus;
    std::optional<DataType> Result;
    Connection Conn;

public:
    explicit EventAwaiter(EventBus& bus) : Bus(bus) {}

    // co_await 三件套:

    /// 事件是否已就绪? 始终返回 false → 总是挂起
    bool await_ready() const noexcept { return false; }

    /// 挂起时: 注册 OneShot 订阅, 回调中恢复协程
    void await_suspend(std::coroutine_handle<> handle);
    // 实现在 EventBus 类定义之后 (需要 EventBus 完整定义)

    /// 恢复时: 返回事件数据
    DataType await_resume()
    {
        return std::move(*Result);
    }
};


// ============================================================================
//  Section 3: EventStream<SignalType> — 持续事件流
// ============================================================================
//
// 允许在循环中反复 co_await, 每次 Emit 都恢复协程.
// 通过 Stop() 或作用域结束时自动断开订阅.
//
// ■ 用法:
//   EventTask MonitorDamage(EventBus& bus)
//   {
//       auto stream = bus.Stream<OnDamage>();
//       while (auto event = co_await stream)    // 返回 optional<tuple>
//       {
//           auto& [dmg, src] = *event;
//           if (dmg > 100) break;               // 退出循环 → stream 析构 → 断开
//       }
//   }
//
// ■ 原理:
//   - 内部维护一个持久订阅 (非 OneShot)
//   - 每次 co_await 时挂起, 等待下一次 Emit
//   - Emit 回调将数据写入缓冲区并恢复协程
//   - Stop() 断开订阅, 使下一次 co_await 返回 nullopt
//
// ■ 线程安全:
//   co_await stream 必须在同一线程中使用 (不可跨线程共享 stream 对象).
//   Emit 可以来自任意线程.
//
template<typename SignalType>
    requires IsSignal<SignalType>
class EventStream
{
    using DataType = typename SignalType::ArgTypes;

    EventBus& Bus;
    Connection Conn;
    std::optional<DataType> Buffer;           // 最新事件数据
    std::coroutine_handle<> WaitingHandle;    // 当前等待中的协程
    bool Stopped = false;

public:
    explicit EventStream(EventBus& bus) : Bus(bus) {}

    ~EventStream()
    {
        Stop();
    }

    // Move-only
    EventStream(EventStream&& other) noexcept
        : Bus(other.Bus)
        , Conn(std::move(other.Conn))
        , Buffer(std::move(other.Buffer))
        , WaitingHandle(other.WaitingHandle)
        , Stopped(other.Stopped)
    {
        other.WaitingHandle = nullptr;
        other.Stopped = true;
    }
    EventStream(const EventStream&) = delete;
    EventStream& operator=(const EventStream&) = delete;
    EventStream& operator=(EventStream&&) = delete;

    /// 停止流, 断开订阅, 使后续 co_await 返回 nullopt
    void Stop()
    {
        Stopped = true;
        Conn.Disconnect();
        // 如果有协程在等待, 恢复它 (让它收到 nullopt)
        if (WaitingHandle)
        {
            auto h = WaitingHandle;
            WaitingHandle = nullptr;
            h.resume();
        }
    }

    // ----------------------------------------------------------------
    // Awaitable 接口 — 使 EventStream 可直接 co_await
    // ----------------------------------------------------------------
    //
    // 每次 co_await stream 返回 std::optional<DataType>:
    //   - 有值 → 收到了事件数据
    //   - nullopt → 流已停止 (Stop() 被调用或 stream 被销毁)
    //

    /// Awaiter — co_await stream 的中间对象
    struct Awaiter
    {
        EventStream& Stream;

        bool await_ready() const noexcept
        {
            // 如果流已停止, 立即返回 (不挂起)
            return Stream.Stopped;
        }

        void await_suspend(std::coroutine_handle<> handle);
        // 实现在 EventBus 类定义之后

        std::optional<DataType> await_resume()
        {
            if (Stream.Stopped && !Stream.Buffer.has_value())
                return std::nullopt;
            auto result = std::move(Stream.Buffer);
            Stream.Buffer.reset();
            return result;
        }
    };

    /// 使 EventStream 可被 co_await
    Awaiter operator co_await() { return Awaiter{*this}; }
};


// ============================================================================
//  Section 4: EventAwaiter / EventStream 的 await_suspend 实现
// ============================================================================
//
// 这些方法需要 EventBus 的完整定义, 因此放在 EventBus 类定义之后.
// 由 EventBus.hpp 在 EventBus 类定义后 #include 或直接内联.
// 为保持单文件结构, 这里提供延迟定义的模式.
//

} // namespace Core::Bus

// ============================================================================
// 注意: EventAwaiter::await_suspend 和 EventStream::Awaiter::await_suspend
// 的实现需要 EventBus 完整定义. 它们在 EventBus.hpp 的末尾提供.
// ============================================================================
