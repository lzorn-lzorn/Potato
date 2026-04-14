#pragma once
// ============================================================================
// A.hpp — 事件总线系统 使用示例与教程
// ============================================================================
//
// 本文件演示 Core::Bus 事件总线的各种用法.
// 可直接编译运行, 也可作为 API 参考.
//
// 目录:
//   示例 1: 基本发布-订阅
//   示例 2: 自监听 (同一个类既发布又订阅)
//   示例 3: 一次性订阅 (OneShot)
//   示例 4: 过滤订阅
//   示例 5: 异步发布与主线程刷新
//   示例 6: 多信号/多订阅者
//   示例 7: 携带对象/闭包的事件
//   示例 8: 全局默认总线
//   示例 9: 连接的手动管理
//

#include "Bus.h"

#include <iostream>
#include <string>
#include <functional>

// ============================================================================
// 信号定义 — 每种事件定义一个 Signal 派生结构体
// ============================================================================
//
// Signal<Tag, Args...> 中:
//   Tag    = 当前结构体自身 (CRTP), 保证唯一性
//   Args   = 事件携带的数据类型列表
//
// 命名约定: On + 事件名 (如 OnDamage, OnHeal, OnGameStart)
//

/// 伤害事件: 携带 伤害值(int) 和 伤害来源(string)
struct OnDamage : Core::Bus::Signal<OnDamage, int, std::string> {};

/// 治疗事件: 携带 治疗量(int)
struct OnHeal : Core::Bus::Signal<OnHeal, int> {};

/// 游戏开始事件: 不携带数据
struct OnGameStart : Core::Bus::Signal<OnGameStart> {};

/// 任务事件: 携带一个闭包 (可用于延迟执行任意操作)
struct OnTask : Core::Bus::Signal<OnTask, std::function<void()>> {};

/// 自定义结构体事件: 携带复合对象
struct DamageReport
{
    int         Amount;
    std::string Source;
    bool        IsCritical;
};
struct OnDamageReport : Core::Bus::Signal<OnDamageReport, DamageReport> {};

// ============================================================================
// 示例 1: 基本发布-订阅
// ============================================================================
//
// 最基本的用法: 一个类发布事件, 另一个类订阅.
//
// class A (发布者) --- OnDamage --→ class B (订阅者)
//
namespace Example1
{
    class A
    {
        Core::Bus::Publisher publisher;
    public:
        explicit A(Core::Bus::EventBus& bus) : publisher(bus) {}

        void Attack(int damage)
        {
            std::cout << "[A] Attacking for " << damage << " damage" << std::endl;
            // 发布伤害事件, 携带 damage 和来源
            publisher.Emit<OnDamage>(damage, std::string("A's sword"));
        }
    };

    class B
    {
        Core::Bus::Acceptor acceptor;
    public:
        explicit B(Core::Bus::EventBus& bus) : acceptor(bus)
        {
            // 订阅伤害事件, 回调签名必须匹配 Signal 定义: (int, const string&)
            acceptor.Subscribe<OnDamage>(
                [this](int dmg, const std::string& src)
            {
                OnDamageReceived(dmg, src);
            });
        }

    private:
        void OnDamageReceived(int dmg, const std::string& src)
        {
            std::cout << "[B] Received " << dmg
                      << " damage from " << src << std::endl;
        }
    };

    inline void Run()
    {
        Core::Bus::EventBus bus;
        A a(bus);
        B b(bus);

        a.Attack(25);
        // 输出:
        //   [A] Attacking for 25 damage
        //   [B] Received 25 damage from A's sword
    }
}


// ============================================================================
// 示例 2: 自监听 (同一个类既发布又订阅)
// ============================================================================
//
// 一个类可以同时拥有 Publisher 和 Acceptor, 监听自己发布的事件.
// 这对于组件化设计非常有用: 组件的行为逻辑和驱动逻辑解耦.
//
namespace Example2
{
    class SelfListening
    {
        Core::Bus::Publisher publisher;
        Core::Bus::Acceptor  acceptor;
        int Health = 100;

    public:
        explicit SelfListening(Core::Bus::EventBus& bus)
            : publisher(bus), acceptor(bus)
        {
            // 监听自己发布的伤害事件
            acceptor.Subscribe<OnDamage>(
                [this](int dmg, const std::string& src)
            {
                Health -= dmg;
                std::cout << "[Self] Took " << dmg << " from " << src
                          << ", HP=" << Health << std::endl;
            });
        }

        void HurtSelf(int amount)
        {
            publisher.Emit<OnDamage>(amount, std::string("self-inflicted"));
        }

        [[nodiscard]] int GetHealth() const { return Health; }
    };

    inline void Run()
    {
        Core::Bus::EventBus bus;
        SelfListening obj(bus);

        obj.HurtSelf(20);
        obj.HurtSelf(15);
        // 输出:
        //   [Self] Took 20 from self-inflicted, HP=80
        //   [Self] Took 15 from self-inflicted, HP=65
    }
}


// ============================================================================
// 示例 3: 一次性订阅 (OneShot)
// ============================================================================
//
// SubscribeOnce 注册的回调只执行一次, 之后自动断开.
// 即使多个线程并发 Emit, CAS 也保证回调精确地只执行一次.
//
namespace Example3
{
    inline void Run()
    {
        Core::Bus::EventBus bus;
        Core::Bus::Publisher publisher(bus);
        Core::Bus::Acceptor acceptor(bus);

        // 一次性订阅: 只接收第一次 OnGameStart
        acceptor.SubscribeOnce<OnGameStart>([]()
        {
            std::cout << "[OneShot] Game started! (only once)" << std::endl;
        });

        publisher.Emit<OnGameStart>(); // 触发 → 输出
        publisher.Emit<OnGameStart>(); // 不触发 → 已自动移除
        // 输出:
        //   [OneShot] Game started! (only once)
    }
}


// ============================================================================
// 示例 4: 带过滤条件的订阅
// ============================================================================
//
// SubscribeFiltered 接受一个谓词函数, 只有谓词返回 true 时才执行回调.
// 谓词签名与 Signal 的 Args 一致: bool(const Args&...)
//
namespace Example4
{
    inline void Run()
    {
        Core::Bus::EventBus bus;
        Core::Bus::Publisher publisher(bus);
        Core::Bus::Acceptor acceptor(bus);

        // 只监听伤害 > 30 的事件
        acceptor.SubscribeFiltered<OnDamage>(
            // Filter: 谓词
            [](int dmg, const std::string& /*src*/) { return dmg > 30; },
            // Callback: 实际处理
            [](int dmg, const std::string& src)
            {
                std::cout << "[Filtered] Heavy hit! " << dmg
                          << " from " << src << std::endl;
            }
        );

        publisher.Emit<OnDamage>(10, std::string("weak")); // 不触发 (10 <= 30)
        publisher.Emit<OnDamage>(50, std::string("boss"));  // 触发
        // 输出:
        //   [Filtered] Heavy hit! 50 from boss
    }
}


// ============================================================================
// 示例 5: 异步发布与主线程刷新
// ============================================================================
//
// EmitAsync 将事件放入 lock-free 队列而不立即执行回调.
// 在主线程帧循环中调用 FlushAllAsync() 统一分发.
//
// 典型场景: 工作线程产生物理碰撞/AI决策事件 → 主线程安全处理
//
namespace Example5
{
    inline void Run()
    {
        Core::Bus::EventBus bus;
        Core::Bus::Publisher publisher(bus);
        Core::Bus::Acceptor acceptor(bus);

        acceptor.Subscribe<OnDamage>(
            [](int dmg, const std::string& src)
        {
            std::cout << "[Async] Processed: " << dmg
                      << " from " << src << std::endl;
        });

        // 模拟工作线程: 异步入队 (不立即触发回调)
        publisher.EmitAsync<OnDamage>(30, std::string("physics"));
        publisher.EmitAsync<OnDamage>(45, std::string("AI"));

        std::cout << "[Main] Before flush — 回调尚未执行" << std::endl;

        // 主线程刷新: 此时才执行回调
        size_t flushed = bus.FlushAllAsync();
        std::cout << "[Main] Flushed " << flushed << " events" << std::endl;

        // 输出:
        //   [Main] Before flush — 回调尚未执行
        //   [Async] Processed: 30 from physics
        //   [Async] Processed: 45 from AI
        //   [Main] Flushed 2 events
    }
}


// ============================================================================
// 示例 6: 多信号 / 多订阅者 / 链式调用
// ============================================================================
//
// 一个 Acceptor 可以订阅多种信号, 支持链式调用.
// 多个 Acceptor 可以监听同一信号.
//
namespace Example6
{
    inline void Run()
    {
        Core::Bus::EventBus bus;
        Core::Bus::Publisher publisher(bus);
        Core::Bus::Acceptor acceptor(bus);

        // 链式调用: 一次订阅多种信号
        acceptor
            .Subscribe<OnDamage>(
                [](int dmg, const std::string& src) {
                    std::cout << "[Multi] Damage: " << dmg
                              << " from " << src << std::endl;
                })
            .Subscribe<OnHeal>(
                [](int amount) {
                    std::cout << "[Multi] Healed: " << amount << std::endl;
                })
            .SubscribeOnce<OnGameStart>(
                []() {
                    std::cout << "[Multi] Game started!" << std::endl;
                });

        // 第二个订阅者也监听 OnDamage
        Core::Bus::Acceptor acceptor2(bus);
        acceptor2.Subscribe<OnDamage>(
            [](int dmg, const std::string& /*src*/) {
                std::cout << "[Logger] Damage logged: " << dmg << std::endl;
            });

        publisher.Emit<OnGameStart>();
        publisher.Emit<OnDamage>(20, std::string("Arrow"));
        publisher.Emit<OnHeal>(35);

        // 输出:
        //   [Multi] Game started!
        //   [Multi] Damage: 20 from Arrow
        //   [Logger] Damage logged: 20
        //   [Multi] Healed: 35
    }
}


// ============================================================================
// 示例 7: 携带对象/闭包的事件
// ============================================================================
//
// Signal 的 Args 可以是任意类型: 基本类型、自定义结构体、std::function 等.
//
namespace Example7
{
    inline void Run()
    {
        Core::Bus::EventBus bus;
        Core::Bus::Publisher publisher(bus);
        Core::Bus::Acceptor acceptor(bus);

        // 订阅携带自定义对象的事件
        acceptor.Subscribe<OnDamageReport>(
            [](const DamageReport& report)
        {
            std::cout << "[Report] " << report.Source
                      << " dealt " << report.Amount
                      << (report.IsCritical ? " (CRIT!)" : "")
                      << std::endl;
        });

        // 订阅携带闭包的事件: 收到后直接执行闭包
        acceptor.Subscribe<OnTask>(
            [](const std::function<void()>& task)
        {
            std::cout << "[Task] Executing received task..." << std::endl;
            task();
        });

        // 发布对象事件
        publisher.Emit<OnDamageReport>(DamageReport{
            .Amount     = 99,
            .Source     = "Dragon",
            .IsCritical = true
        });

        // 发布闭包事件
        publisher.Emit<OnTask>(std::function<void()>([]() {
            std::cout << "[Task] Hello from closure!" << std::endl;
        }));

        // 输出:
        //   [Report] Dragon dealt 99 (CRIT!)
        //   [Task] Executing received task...
        //   [Task] Hello from closure!
    }
}


// ============================================================================
// 示例 8: 全局默认总线
// ============================================================================
//
// EventBus::GetDefault() 返回一个全局单例, 适用于跨模块通信.
// 无需手动传递 bus 引用.
//
namespace Example8
{
    class GlobalPublisher
    {
        Core::Bus::Publisher publisher{Core::Bus::EventBus::GetDefault()};
    public:
        void Notify()
        {
            publisher.Emit<OnGameStart>();
        }
    };

    class GlobalListener
    {
        Core::Bus::Acceptor acceptor{Core::Bus::EventBus::GetDefault()};
    public:
        GlobalListener()
        {
            acceptor.Subscribe<OnGameStart>([]() {
                std::cout << "[Global] Event received via default bus" << std::endl;
            });
        }
    };

    inline void Run()
    {
        GlobalListener listener;
        GlobalPublisher pub;
        pub.Notify();
        // 输出:
        //   [Global] Event received via default bus
    }
}


// ============================================================================
// 示例 9: 手动管理连接 (不使用 Acceptor)
// ============================================================================
//
// 如果需要精细控制单个连接, 可以直接使用 EventBus::Subscribe 返回的 Connection.
//
namespace Example9
{
    inline void Run()
    {
        Core::Bus::EventBus bus;
        Core::Bus::Publisher publisher(bus);

        // 直接订阅, 获得 Connection 句柄
        auto conn = bus.Subscribe<OnDamage>(
            [](int dmg, const std::string& src) {
                std::cout << "[Manual] " << dmg << " from " << src << std::endl;
            });

        publisher.Emit<OnDamage>(10, std::string("test"));
        // 输出: [Manual] 10 from test

        // 手动断开
        conn.Disconnect();

        publisher.Emit<OnDamage>(20, std::string("test2"));
        // 无输出 — 已断开

        // 也可以用 ScopedConnectionGroup 批量管理
        Core::Bus::ScopedConnectionGroup group;
        group.Add(bus.Subscribe<OnDamage>(
            [](int dmg, const std::string& src) {
                std::cout << "[Group] " << dmg << " from " << src << std::endl;
            }));
        group.Add(bus.Subscribe<OnHeal>(
            [](int amount) {
                std::cout << "[Group] Healed " << amount << std::endl;
            }));

        publisher.Emit<OnDamage>(5, std::string("grouped"));
        // 输出: [Group] 5 from grouped

        group.DisconnectAll(); // 断开组内所有连接
    }
}


// ============================================================================
// 运行所有示例
// ============================================================================
namespace BusExamples
{
    inline void RunAll()
    {
        std::cout << "===== Example 1: Basic Pub-Sub =====" << std::endl;
        Example1::Run();

        std::cout << "\n===== Example 2: Self-Listening =====" << std::endl;
        Example2::Run();

        std::cout << "\n===== Example 3: OneShot =====" << std::endl;
        Example3::Run();

        std::cout << "\n===== Example 4: Filtered Subscribe =====" << std::endl;
        Example4::Run();

        std::cout << "\n===== Example 5: Async Dispatch =====" << std::endl;
        Example5::Run();

        std::cout << "\n===== Example 6: Multi-Signal Chain =====" << std::endl;
        Example6::Run();

        std::cout << "\n===== Example 7: Object & Closure Events =====" << std::endl;
        Example7::Run();

        std::cout << "\n===== Example 8: Global Default Bus =====" << std::endl;
        Example8::Run();

        std::cout << "\n===== Example 9: Manual Connection Management =====" << std::endl;
        Example9::Run();
    }
}
