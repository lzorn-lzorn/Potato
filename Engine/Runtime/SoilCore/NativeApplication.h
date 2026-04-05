
namespace SoliCore {


/**
 * @brief NativeApplication 是平台相关的应用程序接口
 */
class NativeApplication
{
public:
	static NativeApplication& Instance() {
		static NativeApplication instance;
		return instance;
	}
private:
	NativeApplication() = default;
};
}