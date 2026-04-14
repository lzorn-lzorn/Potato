#pragma once
// ============================================================================
// Bus.h — 事件总线系统 聚合头文件
// ============================================================================
//
// 包含此头文件即可使用完整的事件总线系统:
//   #include "Core/Bus/Bus.h"
//
// 组件:
//   Signal<Tag, Args...>    — 事件类型定义
//   Connection              — RAII 订阅句柄
//   ScopedConnectionGroup   — 连接组 (批量管理)
//   EventBus                — 中央事件路由器
//   Publisher               — 事件发布者
//   Acceptor                — 事件订阅者 (自动管理生命周期)
//   MPMCQueue<T, N>         — 无锁多生产者多消费者队列 (底层组件)
//   EventTask               — 协程任务类型 (fire-and-forget)
//   EventAwaiter<S>         — 一次性事件等待 (co_await)
//   EventStream<S>          — 持续事件流 (循环 co_await)
//

#include "MPMCQueue.hpp"
#include "EventBus.hpp"
