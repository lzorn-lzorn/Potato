
#include <SDL3/SDL.h>
#include "Core/Core.h"

#include <string>

namespace SoliCore {

/**
 * @brief NativeWindow 是平台相关的窗口接口, 由 SWindow 持有并管理
 */
class NativeWindow
{
public:
	NativeWindow() = default;
	virtual ~NativeWindow() = default;
	virtual void Resize(int width, int height) = 0;
	virtual Core::Math::Vector2D<uint32_t> GetSize() const = 0;
};

class SDLWindow : public NativeWindow
{
public: 	
	SDLWindow(const char* title, int width, int height) 
		: title(title), width(width), height(height) {
		window = SDL_CreateWindow(title, width, height, 0);
	}

	~SDLWindow() {
		if (window) {
			SDL_DestroyWindow(window);
		}
	}
	void Resize(int width, int height) override {
		this->width  = width;
		this->height = height;
		SDL_SetWindowSize(window, width, height);
	}
	Core::Math::Vector2D<uint32_t> GetSize() const override {
		return Core::Math::Vector2D<uint32_t>{width, height};
	}
private:
	SDL_Window* window;
	std::string title;
	uint32_t width, height;
};


}