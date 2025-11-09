#pragma once
#ifdef DLLEXPORT
	#undef DLLEXPORT
	#define DLLEXPORT __declspec(dllexport)
#endif

#ifdef DLLIMPORT
	#undef DLLIMPORT
	#define DLLIMPORT __declspec(dllimport)
#endif

#if defined(_M_X64) || defined(__x86_64__)
	#ifdef APICALL
		#undef APICALL
		#define APICALL
	#else
		#define APICALL
	#endif
#else
	#ifdef USE_STDCALL
		#define APICALL __stdcall
	#else
		#define APICALL __cdecl
	#endif
#endif

#ifdef FORCEINLINE
	#undef FORCEINLINE
	#define FORCEINLINE __forceinline
#endif

#ifdef FORCENOINLINE
	#undef FORCENOINLINE
	#define FORCENOINLINE __declspec(noinline)
#endif