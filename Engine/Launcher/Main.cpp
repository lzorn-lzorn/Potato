/*
  Copyright (C) 1997-2026 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely.
*/
#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>
#include <SDL3_ttf/SDL_ttf.h>
#include "Core/Core.h"

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

// // ============================================================
// //  测试宏
// // ============================================================
// #define TEST(name) \
//     SDL_Log("\n===== TEST: %s =====", name);

// #define CHECK(expr) \
//     do { \
//         if (!(expr)) { \
//             SDL_Log("  ❌ FAILED: %s (line %d)", #expr, __LINE__); \
//             failCount++; \
//         } else { \
//             SDL_Log("  ✅ PASSED: %s", #expr); \
//             passCount++; \
//         } \
//     } while (0)

// static SDL_Window *window = NULL;
// static SDL_Renderer *renderer = NULL;
// // static std::unique_ptr<PluginManager> g_plugin_manager;

// static void PluginHostLog(const char* message)
// {
//     SDL_Log("[PluginHost] %s", message != nullptr ? message : "(null)");
// }
// // int TestDelegate();
// struct SDLApplication {
//     uint64_t frame_counter { 0 };
//     double last_frame_seconds { 0.0 };
//     double this_frame_seconds { 0.0 };
//     double delta_time { 0.0 };
//     int32_t current_fps { 0 };
// };

// /* This function runs once at startup. */
// SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
// {
//     alignas(32) float a[8] = {1,2,3,4,5,6,7,8};
//     alignas(32) float b[8] = {8,7,6,5,4,3,2,1};
//     alignas(32) float c[8];
//     // vec_add_avx2(a, b, c, 8);
//     for (int i = 0; i < 8; i++)
//         SDL_Log("%f ", c[i]);
//     SDL_Log("\n");
//     // boost::signals2::signal<void ()> signal;
//     SDL_Log("size of std::function<std::function<int()>> is = %llu", sizeof(std::function<int()>));
//     SDL_Log("size of std::function<std::function<int(int, int)>> is = %llu", sizeof(std::function<int(int, int)>));
//     SDLApplication* app = new SDLApplication();

//     // PluginHostContext host_context{};
//     // host_context.Log = &PluginHostLog;

//     // g_plugin_manager = std::make_unique<PluginManager>(host_context);
//     // const std::filesystem::path plugin_directory = std::filesystem::current_path() / "Plugins";
//     // if (!g_plugin_manager->LoadFromDirectory(plugin_directory)) {
//     //     SDL_Log("Plugin loading completed with failures. Directory: %s", plugin_directory.string().c_str());
//     // }
//     // SDL_Log("Loaded plugins: %llu", static_cast<unsigned long long>(g_plugin_manager->GetLoadedCount()));

//     /* Create the window */
//     if (!SDL_CreateWindowAndRenderer("Hello World", 800, 600, SDL_WINDOW_FULLSCREEN, &window, &renderer)) {
//         SDL_Log("Couldn't create window and renderer: %s", SDL_GetError());
//         return SDL_APP_FAILURE;
//     }

//     *appstate = app;
//     SDL_Time current_time;
//     if (SDL_GetCurrentTime(&current_time)) {
//         app->this_frame_seconds = (double)current_time / 1.0e9;
//         app->last_frame_seconds = (double)current_time / 1.0e9;
//     }

//     return SDL_APP_CONTINUE;
// }

// /* This function runs when a new event (mouse input, keypresses, etc) occurs. */
// SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
// {
//     if (event->type == SDL_EVENT_KEY_DOWN ||
//         event->type == SDL_EVENT_QUIT) {
//         return SDL_APP_SUCCESS;  /* end the program, reporting success to the OS. */
//     }
//     return SDL_APP_CONTINUE;
// }


// /* This function runs once per frame, and is the heart of the program. */
// SDL_AppResult SDL_AppIterate(void *appstate)
// {
//     SDLApplication* app = (SDLApplication*)appstate;
//     ++app->frame_counter;
//     SDL_Log("Current Frame Number is %llu", app->frame_counter);

//     SDL_GetTicks();
//     SDL_Time current_time;
//     if (SDL_GetCurrentTime(&current_time)) {
//         app->last_frame_seconds = app->this_frame_seconds;
//         app->this_frame_seconds = (double)current_time / 1.0e9;
//         app->delta_time = app->this_frame_seconds - app->last_frame_seconds;
//         app->current_fps = (int32_t)(1.0 / app->delta_time);
//         SDL_Log("Current Delta Time is %f", app->delta_time);
//         SDL_Log("Current FPS is %d", app->current_fps);
//     }

//     // if (g_plugin_manager) {
//     //     g_plugin_manager->TickAll(app->delta_time);
//     // }

//     const char *message = "Hello World!";
//     int w = 0, h = 0;
//     float x, y;
//     const float scale = 4.0f;

//     /* Center the message and scale it up */
//     SDL_GetRenderOutputSize(renderer, &w, &h);
//     SDL_SetRenderScale(renderer, scale, scale);
//     x = ((w / scale) - SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * SDL_strlen(message)) / 2;
//     y = ((h / scale) - SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE) / 2;

//     /* Draw the message */
//     SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
//     SDL_RenderClear(renderer);
//     SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
//     SDL_RenderDebugText(renderer, x, y, message);
//     SDL_RenderPresent(renderer);

//     return SDL_APP_CONTINUE;
// }

// /* This function runs once at shutdown. */
// void SDL_AppQuit(void *appstate, SDL_AppResult result)
// {
//     // if (g_plugin_manager) {
//     //     g_plugin_manager->UnloadAll();
//     //     g_plugin_manager.reset();
//     // }
// }



// ---------- 简单 UI 控件基类 ----------
class UIWidget {
public:
    SDL_FRect rect;      // 逻辑坐标矩形 (x, y, w, h)
    bool isHovered = false;
    bool isPressed = false;

    UIWidget(float x, float y, float w, float h) : rect{x, y, w, h} {}
    virtual ~UIWidget() {}

    // 处理事件 (返回 true 表示事件被消费)
    virtual bool handleEvent(const SDL_Event& event, float mouseX, float mouseY) {
        bool inside = (mouseX >= rect.x && mouseX <= rect.x + rect.w &&
                       mouseY >= rect.y && mouseY <= rect.y + rect.h);
        switch (event.type) {
            case SDL_EVENT_MOUSE_MOTION:
                isHovered = inside;
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                if (inside && event.button.button == SDL_BUTTON_LEFT) {
                    isPressed = true;
                    return true;
                }
                break;
            case SDL_EVENT_MOUSE_BUTTON_UP:
                if (isPressed && event.button.button == SDL_BUTTON_LEFT) {
                    isPressed = false;
                    if (inside) {
                        onClick();   // 触发点击回调
                    }
                    return true;
                }
                break;
        }
        return false;
    }

    virtual void onClick() {}  // 子类重写
    virtual void render(SDL_Renderer* renderer) {
        // 根据状态改变颜色
        SDL_Color color;
        if (isPressed)      color = {100, 100, 100, 255};
        else if (isHovered) color = {200, 200, 200, 255};
        else                color = {150, 150, 150, 255};
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        SDL_RenderFillRect(renderer, &rect);
        // 边框
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderRect(renderer, &rect);
    }
};

// 具体按钮控件（无文本渲染，仅矩形）
class Button : public UIWidget {
public:
    Button(float x, float y, float w, float h, const char* text)
        : UIWidget(x, y, w, h), label(text) {}

    void onClick() override {
        printf("Button clicked! (%s)\n", label);
    }

private:
    const char* label;
};

// ---------- UI 管理器 ----------
class UIManager {
public:
    std::vector<UIWidget*> widgets;

    void addWidget(UIWidget* w) { widgets.push_back(w); }

    void handleEvents(const SDL_Event& event) {
        float mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);  // SDL3 返回 float 坐标
        for (auto* w : widgets) {
            if (w->handleEvent(event, mouseX, mouseY))
                break; // 事件被消费，停止传递
        }
    }

    void render(SDL_Renderer* renderer) {
        for (auto* w : widgets) {
            w->render(renderer);
        }
    }

    ~UIManager() {
        for (auto* w : widgets) delete w;
    }
};

// ---------- 主函数 ----------
int main(int argc, char* argv[]) {
    Core::Math::Vector2D<float> v1{3.0f, 4.0f};
    Core::Math::Vector2D<float> v2{3.0f, 4.0f};
    SDL_Log("v1 == v2: %s", (v1 == v2) ? "true" : "false");
    // 1. 初始化 SDL3
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init Error: %s", SDL_GetError());
        return -1;
    }

    // 2. 创建窗口和渲染器
    SDL_Window* window = SDL_CreateWindow("SDL3 UI Framework Demo", 800, 600, 0);
    if (!window) {
        SDL_Log("CreateWindow Error: %s", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
        SDL_Log("CreateRenderer Error: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    // 3. 创建 UI 控件
    UIManager ui;
    Button* btn = new Button(300, 250, 200, 50, "Click Me!");
    ui.addWidget(btn);

    // 4. 主循环
    bool running = true;
    SDL_Event event;
    Uint64 lastTime = SDL_GetTicks();
    const Uint32 frameDelay = 16; // 约 60 FPS

    while (running) {
        // 处理所有待处理事件
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
            // 让 UI 管理器处理事件
            ui.handleEvents(event);
        }

        // 渲染
        SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255); // 背景灰色
        SDL_RenderClear(renderer);
        ui.render(renderer);
        SDL_RenderPresent(renderer);

        // 帧率控制 (简单延迟)
        Uint64 now = SDL_GetTicks();
        Uint64 elapsed = now - lastTime;
        if (elapsed < frameDelay) {
            SDL_Delay((Uint32)(frameDelay - elapsed));
        }
        lastTime = SDL_GetTicks();
    }

    // 5. 清理
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}