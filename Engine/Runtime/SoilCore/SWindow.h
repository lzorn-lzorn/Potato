
#include "NativeWindow.h"

#include <utility>
#include <memory>
#include <string>
namespace SoliCore {



class SWindow : public std::enable_shared_from_this<SWindow>
{
public:
	static std::shared_ptr<SWindow> Create(const char* title, int width, int height) {
		return std::shared_ptr<SWindow>(new SWindow(title, width, height));
	}


	SWindow() = default;
	SWindow(const char* title, int width, int height) 
	{
		
		this->native_window = std::make_unique<NativeWindow>();
	}
	~SWindow() = default;
private:
	std::unique_ptr<NativeWindow> native_window;
};
}