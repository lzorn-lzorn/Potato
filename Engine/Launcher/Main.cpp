/*
  Copyright (C) 1997-2026 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely.
*/
#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_time.h>

#include "ThreadPool/ThreadPool.h"
#include "Core/ModuleManager/ModuleAPI.h"
#include "Core/ModuleManager/ModuleManager.h"

#include <chrono>
#include <functional>
#include <print>
#include <cassert>
#include <iostream>
#include <cmath>
#include <string>
#include <cmath>
#include <numeric>
#include <immintrin.h>
#include <filesystem>
#include <memory>

// void vec_add_avx2(float *a, float *b, float *c, int n) {
//     // 假设 n 是 8 的倍数，且内存对齐到 32 字节
//     for (int i = 0; i < n; i += 8) {
//         __m256 va = _mm256_load_ps(a + i);
//         __m256 vb = _mm256_load_ps(b + i);
//         __m256 vc = _mm256_add_ps(va, vb);
//         _mm256_store_ps(c + i, vc);
//     }
// }



// ============================================================================
// 示例 1: UE 风格 - 继承 IQueueWork 创建任务
// ============================================================================
class FMyComputeWork : public IQueueWork
{
public:
    FMyComputeWork(int id, int iterations)
        : m_Id(id), m_Iterations(iterations) {}

    void DoThreadWork() override
    {
        double result = 0.0;
        for (int i = 0; i < m_Iterations; ++i)
            result += std::sin(static_cast<double>(i));

        SDL_Log("[UE风格] Task %d 完成, 结果: %f\n", m_Id, result);
    }

    void Abandon() override
    {
        SDL_Log("[UE风格] Task %d 被放弃\n", m_Id);
    }

    ETaskPriority GetPriority() const override
    {
        return ETaskPriority::High;
    }

private:
    int m_Id;
    int m_Iterations;
};

// ============================================================================
// 示例 2: 现代 C++ 风格 - Lambda + Future
// ============================================================================
void Example_LambdaWithFuture()
{
    SDL_Log("\n=== 示例 2: Lambda + Future ===\n");

    ThreadPool pool(4, "ComputePool");

    // 提交多个计算任务, 获取 future
    std::vector<std::future<double>> futures;
    for (int i = 0; i < 8; ++i)
    {
        futures.push_back(
            pool.Submit(ETaskPriority::Normal, [i]() -> double {
                double sum = 0.0;
                for (int j = 0; j < 100000; ++j)
                    sum += std::cos(static_cast<double>(i * j));
                return sum;
            })
        );
    }

    // 收集结果
    double total = 0.0;
    for (size_t i = 0; i < futures.size(); ++i)
    {
        double val = futures[i].get(); // 阻塞等待结果
        SDL_Log("  任务 %zu 结果: %f\n", i, val);
        total += val;
    }
    SDL_Log("  总计: %f\n", total);
}

// ============================================================================
// 示例 3: 优先级任务
// ============================================================================
void Example_PriorityTasks()
{
    SDL_Log("\n=== 示例 3: 优先级任务 ===\n");

    // 只用 1 个线程, 以便观察优先级顺序
    ThreadPool pool(1, "PriorityPool");

    // 先暂停一下让线程进入等待, 然后一次性塞入不同优先级的任务
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    pool.Submit(ETaskPriority::Low,     []() { SDL_Log("  [Low]     执行\n"); });
    pool.Submit(ETaskPriority::Highest, []() { SDL_Log("  [Highest] 执行\n"); });
    pool.Submit(ETaskPriority::Normal,  []() { SDL_Log("  [Normal]  执行\n"); });
    pool.Submit(ETaskPriority::High,    []() { SDL_Log("  [High]    执行\n"); });
    pool.Submit(ETaskPriority::Lowest,  []() { SDL_Log("  [Lowest]  执行\n"); });

    pool.WaitForIdle();
    SDL_Log("  已完成任务数: %llu\n", pool.GetCompletedTasks()) ;
}

// ============================================================================
// 示例 4: 全局线程池 (类似 UE GThreadPool)
// ============================================================================
void Example_GlobalPool()
{
    SDL_Log("\n=== 示例 4: 全局线程池 GThreadPool ===\n");

    auto future = GThreadPool().Submit([]() -> std::string {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return "来自全局线程池的结果!";
    });

    SDL_Log("  %s\n", future.get().c_str());
    SDL_Log("  全局池线程数: %llu\n", GThreadPool().GetThreadCount());
}

// ============================================================================
// 示例 5: 并行 Map-Reduce
// ============================================================================
void Example_ParallelMapReduce()
{
    SDL_Log("\n=== 示例 5: 并行 Map-Reduce ===\n");

    constexpr size_t DATA_SIZE = 1000000;
    constexpr size_t NUM_CHUNKS = 8;
    constexpr size_t CHUNK_SIZE = DATA_SIZE / NUM_CHUNKS;

    // 准备数据
    std::vector<double> data(DATA_SIZE);
    std::iota(data.begin(), data.end(), 1.0);

    ThreadPool pool(4, "MapReducePool");
    std::vector<std::future<double>> chunkResults;

    // Map: 分块并行求和
    for (size_t i = 0; i < NUM_CHUNKS; ++i)
    {
        size_t start = i * CHUNK_SIZE;
        size_t end   = (i == NUM_CHUNKS - 1) ? DATA_SIZE : start + CHUNK_SIZE;

        chunkResults.push_back(
            pool.Submit(ETaskPriority::High,
                [&data, start, end]() -> double {
                    double sum = 0.0;
                    for (size_t j = start; j < end; ++j)
                        sum += data[j];
                    return sum;
                })
        );
    }

    // Reduce: 汇总结果
    double totalSum = 0.0;
    for (auto& f : chunkResults)
        totalSum += f.get();

    double expected = (DATA_SIZE * (DATA_SIZE + 1.0)) / 2.0;
    SDL_Log("  并行求和结果: %f\n", totalSum);
    SDL_Log("  预期结果:     %f\n", expected);
    SDL_Log("  验证: %s\n", (std::abs(totalSum - expected) < 1e-6 ? "通过 ✓" : "失败 ✗"));
}

// ============================================================================
// 示例 6: 异常处理
// ============================================================================
void Example_ExceptionHandling()
{
    SDL_Log("\n=== 示例 6: 异常处理 ===\n");

    ThreadPool pool(2, "ExceptionPool");

    auto future = pool.Submit([]() -> int {
        throw std::runtime_error("故意抛出的异常!");
        return 42;
    });

    try
    {
        int result = future.get();
        SDL_Log("  结果: %d\n", result);
    }
    catch (const std::exception& e)
    {
        SDL_Log("  捕获异常: %s ✓\n", e.what());
    }
}

// ============================================================================
// 示例 7: WaitForIdle + 统计
// ============================================================================
void Example_WaitAndStats()
{
    SDL_Log("\n=== 示例 7: WaitForIdle + 统计 ===\n");

    ThreadPool pool(4, "StatsPool");

    for (int i = 0; i < 20; ++i)
    {
        pool.Submit([i]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        });
    }

    SDL_Log("  排队中任务数: %llu\n", pool.GetPendingTaskCount());
    pool.WaitForIdle();
    SDL_Log("  所有任务完成!\n");
    SDL_Log("  已完成任务数: %llu\n", pool.GetCompletedTasks());
}

int Test()
{
    SDL_Log("========================================\n");
    SDL_Log(" 线程池演示 (参考 UE FQueuedThreadPool)\n");
    SDL_Log("========================================\n");

    // 示例 1: UE 风格
    {
        SDL_Log("\n=== 示例 1: UE 风格 IQueuedWork ===\n");
        ThreadPool pool(4, "UE_StylePool");

        for (int i = 0; i < 5; ++i)
            pool.AddQueuedWork(new FMyComputeWork(i, 100000));

        pool.WaitForIdle();
    }

    // 其他示例
    Example_LambdaWithFuture();
    Example_PriorityTasks();
    Example_GlobalPool();
    Example_ParallelMapReduce();
    Example_ExceptionHandling();
    Example_WaitAndStats();

    SDL_Log("\n========================================\n");
    SDL_Log(" 所有示例完成!\n");
    SDL_Log("========================================\n");

    return 0;
}

// #include "Delegate.h"

// ============================================================
//  测试宏
// ============================================================
#define TEST(name) \
    SDL_Log("\n===== TEST: %s =====", name);

#define CHECK(expr) \
    do { \
        if (!(expr)) { \
            SDL_Log("  ❌ FAILED: %s (line %d)", #expr, __LINE__); \
            failCount++; \
        } else { \
            SDL_Log("  ✅ PASSED: %s", #expr); \
            passCount++; \
        } \
    } while (0)

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
// static std::unique_ptr<PluginManager> g_plugin_manager;

static void PluginHostLog(const char* message)
{
    SDL_Log("[PluginHost] %s", message != nullptr ? message : "(null)");
}
// int TestDelegate();
struct SDLApplication {
    uint64_t frame_counter { 0 };
    double last_frame_seconds { 0.0 };
    double this_frame_seconds { 0.0 };
    double delta_time { 0.0 };
    int32_t current_fps { 0 };
};

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    alignas(32) float a[8] = {1,2,3,4,5,6,7,8};
    alignas(32) float b[8] = {8,7,6,5,4,3,2,1};
    alignas(32) float c[8];
    // vec_add_avx2(a, b, c, 8);
    for (int i = 0; i < 8; i++)
        SDL_Log("%f ", c[i]);
    SDL_Log("\n");

    Test();
    // boost::signals2::signal<void ()> signal;
    SDL_Log("size of std::function<std::function<int()>> is = %llu", sizeof(std::function<int()>));
    SDL_Log("size of std::function<std::function<int(int, int)>> is = %llu", sizeof(std::function<int(int, int)>));
    SDLApplication* app = new SDLApplication();

    // PluginHostContext host_context{};
    // host_context.Log = &PluginHostLog;

    // g_plugin_manager = std::make_unique<PluginManager>(host_context);
    // const std::filesystem::path plugin_directory = std::filesystem::current_path() / "Plugins";
    // if (!g_plugin_manager->LoadFromDirectory(plugin_directory)) {
    //     SDL_Log("Plugin loading completed with failures. Directory: %s", plugin_directory.string().c_str());
    // }
    // SDL_Log("Loaded plugins: %llu", static_cast<unsigned long long>(g_plugin_manager->GetLoadedCount()));

    /* Create the window */
    if (!SDL_CreateWindowAndRenderer("Hello World", 800, 600, SDL_WINDOW_FULLSCREEN, &window, &renderer)) {
        SDL_Log("Couldn't create window and renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    *appstate = app;
    SDL_Time current_time;
    if (SDL_GetCurrentTime(&current_time)) {
        app->this_frame_seconds = (double)current_time / 1.0e9;
        app->last_frame_seconds = (double)current_time / 1.0e9;
    }

    return SDL_APP_CONTINUE;
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    if (event->type == SDL_EVENT_KEY_DOWN ||
        event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;  /* end the program, reporting success to the OS. */
    }
    return SDL_APP_CONTINUE;
}


/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate)
{
    SDLApplication* app = (SDLApplication*)appstate;
    ++app->frame_counter;
    SDL_Log("Current Frame Number is %llu", app->frame_counter);

    SDL_GetTicks();
    SDL_Time current_time;
    if (SDL_GetCurrentTime(&current_time)) {
        app->last_frame_seconds = app->this_frame_seconds;
        app->this_frame_seconds = (double)current_time / 1.0e9;
        app->delta_time = app->this_frame_seconds - app->last_frame_seconds;
        app->current_fps = (int32_t)(1.0 / app->delta_time);
        SDL_Log("Current Delta Time is %f", app->delta_time);
        SDL_Log("Current FPS is %d", app->current_fps);
    }

    // if (g_plugin_manager) {
    //     g_plugin_manager->TickAll(app->delta_time);
    // }

    const char *message = "Hello World!";
    int w = 0, h = 0;
    float x, y;
    const float scale = 4.0f;

    /* Center the message and scale it up */
    SDL_GetRenderOutputSize(renderer, &w, &h);
    SDL_SetRenderScale(renderer, scale, scale);
    x = ((w / scale) - SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * SDL_strlen(message)) / 2;
    y = ((h / scale) - SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE) / 2;

    /* Draw the message */
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDebugText(renderer, x, y, message);
    SDL_RenderPresent(renderer);

    return SDL_APP_CONTINUE;
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    // if (g_plugin_manager) {
    //     g_plugin_manager->UnloadAll();
    //     g_plugin_manager.reset();
    // }
}


// ============================================================
//  辅助: 自由函数
// ============================================================
int Add(int a, int b) { return a + b; }

void PrintMessage(const std::string& msg) {
    SDL_Log("[自由函数] %s\n", msg.c_str());
}

float ComputeDistance(float x, float y) {
    return std::sqrt(x * x + y * y);
}

// ============================================================
//  辅助: 测试用的类
// ============================================================
class Calculator {
public:
    int Multiply(int a, int b) const { return a * b; }
    void Log(const std::string& msg) { SDL_Log("[Calculator] %s\n", msg.c_str()); }
};

class Button {
public:
    void OnClick() { SDL_Log("[Button] 被点击了!\n"); clickCount++; }
    int GetClickCount() const { return clickCount; }
private:
    int clickCount = 0;
};

// ============================================================
//  辅助: 仿函数
// ============================================================
struct Adder {
    int offset;
    int operator()(int x) const { return x + offset; }
};

// ============================================================
//  测试辅助
// ============================================================
static int g_callCount = 0;

void FreeFunc_Increment(int val) { g_callCount += val; }
void FreeFunc_Double(int val)    { g_callCount += val * 2; }

class Observer {
public:
    void OnEvent(int val) { sum += val; callCount++; }
    int sum = 0;
    int callCount = 0;
};

class AudioSystem {
public:
    void OnDamage(int dmg) { lastDmg = dmg; called = true; }
    int lastDmg = 0;
    bool called = false;
};

class UISystem {
public:
    void OnDamage(int dmg) { totalDmg += dmg; count++; }
    int totalDmg = 0;
    int count = 0;
};

int Compute_A(int x) { return x + 1; }
int Compute_B(int x) { return x * 2; }
int Compute_C(int x) { return x * x; }

bool IsPositive(int x)    { return x > 0; }
bool IsLessThan100(int x) { return x < 100; }
bool IsEven(int x)        { return x % 2 == 0; }
// -------------------- 辅助函数 --------------------
int GlobalFunc(int a, int b) { return a + b; }
void GlobalVoidFunc(int a) { SDL_Log("GlobalVoidFunc: %d\n", a); }

struct TestClass {
    int Multiply(int a, int b) { return a * b; }
    void Print(int a) { SDL_Log("TestClass::Print: %d\n", a); }
    static int StaticSub(int a, int b) { return a - b; }
};

// -------------------- 测试用例 --------------------
// int TestDelegate() {
//     SDL_Log << "\n=== TestDelegate ===" << std::endl;
//
//     // 1. 绑定普通函数
//     Delegate<int(int, int)> d1("d1");
//     auto h1 = d1.Bind(GlobalFunc);
//     assert(d1.IsBound());
//     assert(d1.Invoke(3, 5) == 8);
//     assert(d1(10, 2) == 12);
//
//     // 2. 绑定 lambda
//     Delegate<int(int, int)> d2("d2");
//     auto h2 = d2.Bind([](int x, int y) { return x * y; });
//     assert(d2(4, 5) == 20);
//
//     // 3. 绑定成员函数
//     TestClass obj;
//     Delegate<int(int, int)> d3("d3");
//     auto h3 = d3.Bind(&obj, &TestClass::Multiply);
//     assert(d3(6, 7) == 42);
//
//     // 4. 绑定静态成员函数
//     Delegate<int(int, int)> d4("d4");
//     auto h4 = d4.Bind(TestClass::StaticSub);
//     assert(d4(20, 8) == 12);
//
//     // 5. 绑定 std::function
//     std::function<int(int, int)> func = GlobalFunc;
//     Delegate<int(int, int)> d5("d5");
//     auto h5 = d5.Bind(func);
//     assert(d5(1, 2) == 3);
//
//     // 6. void 返回类型
//     Delegate<void(int)> d6("d6");
//     auto h6 = d6.Bind(GlobalVoidFunc);
//     d6.Invoke(100);  // 输出 GlobalVoidFunc: 100
//     d6(200);         // 输出 GlobalVoidFunc: 200
//
//     // 7. 解绑
//     d1.Unbind();
//     assert(!d1.IsBound());
//     // d1(1,2); // 会触发 assert (如果启用)
//
//     // 8. 重置
//     d2.Reset();
//     assert(!d2.IsBound());
//     assert(d2.GetName().empty());
//
//     // 9. 拷贝构造和移动
//     Delegate<int(int, int)> d7 = std::move(d3);
//     assert(d7(2, 3) == 6);
//     assert(!d3.IsBound());
//
//     // 10. 操作符 bool
//     Delegate<int(int, int)> d8;
//     assert(!static_cast<bool>(d8));
//     d8.Bind(GlobalFunc);
//     assert(static_cast<bool>(d8));
//
//     return 0;
// }
//
// void TestMultiDelegate() {
//     SDL_Log << "\n=== TestMultiDelegate ===" << std::endl;
//
//     MultiDelegate<int(int, int)> md1("md1");
//     TestClass obj;
//
//     // 绑定多个委托
//     auto h1 = md1.Bind(GlobalFunc);                     // 普通函数
//     auto h2 = md1.Bind(&obj, &TestClass::Multiply);     // 成员函数
//     auto h3 = md1.Bind([](int a, int b) { return a - b; }); // lambda
//     auto h4 = md1.Bind(TestClass::StaticSub);           // 静态函数
//
//     assert(md1.GetCount() == 4);
//     assert(md1.IsBound());
//
//     // 调用，后处理器会返回最后一个值（假设修复后处理器）
//     // 假设 DefaultDelegatePostProcessor 返回最后一个值
//     int* last = md1.Invoke(10, 5);
//     if (last) {
//         SDL_Log << "Last result: " << *last << std::endl; // 预期 10-5=5
//     }
//
//     // 使用 operator()
//     auto result = md1(10, 5);
//     if (result) {
//         SDL_Log << "operator() result: " << *result << std::endl;
//     }
//
//     // 解绑
//     assert(md1.Unbind(h1));
//     assert(md1.GetCount() == 3);
//     assert(!md1.Unbind(DelegateHandle())); // 无效 handle
//
//     // 包含检查
//     assert(md1.Contains(h2));
//     assert(!md1.Contains(h1));
//
//     // 清空
//     md1.UnbindAll();
//     assert(md1.GetCount() == 0);
//     assert(!md1.IsBound());
//
//     // void 返回类型
//     MultiDelegate<void(int)> md2("md2");
//     md2.Bind(GlobalVoidFunc);
//     md2.Bind([](int x) { SDL_Log << "lambda: " << x << std::endl; });
//     md2.Invoke(42); // 输出两行
//     md2(42);        // 同样输出两行
// }

// void TestDelegateHandle() {
//     SDL_Log << "\n=== TestDelegateHandle ===" << std::endl;
//
//     DelegateHandle h1(DelegateHandle::NewDelegateHandleTag{});
//     DelegateHandle h2(DelegateHandle::NewDelegateHandleTag{});
//     assert(h1.IsValid() && h2.IsValid());
//     assert(h1 != h2);
//
//     h1.Reset();
//     assert(!h1.IsValid());
//     assert(h1.GetUID() == InvalidDelegateUID);
// }
//
// void TestPostProcessor() {
//     SDL_Log << "\n=== TestPostProcessor ===" << std::endl;
//
//     // 使用 NegativeDelegatePostProcessor
//     MultiDelegate<int(int), NegativeDelegatePostProcessor<int>> md;
//     md.Bind([](int x) { return x; });
//     md.Bind([](int x) { return x * 2; });
//
//     int* result = md.Invoke(5);
//     // 假设后处理器累加并取负，这里只是示例，实际需要自定义后处理器
//     if (result) {
//         SDL_Log << "Negative result: " << *result << std::endl;
//     }
// }
//
// void Test() {
//     TestDelegate();
//     TestMultiDelegate();
//     TestDelegateHandle();
//     TestPostProcessor();
// }