#pragma once


#define DLLEXPORT 	     /* 动态库导出 */
#define DLLIMPORT 	     /* 动态库导入 */
#define APICALL
#define FORCEINLINE      /* 强制内联 */
#define FORCENOINLINE    /* 关闭内联 */



#if defined(_WIN32) || defined(__CYGWIN__)
	#include "Windows.hpp"
#elif defined(__linux__)
	#include "Linux.hpp"
#elif defined(__APPLE__)
	#include "Apple.hpp"
#else

#endif