#ifndef RLGAMECANVAS_CORE_EXPORTSPECS
#define RLGAMECANVAS_CORE_EXPORTSPECS





#ifdef RLGAMECANVAS_DYNAMIC
#ifdef RLGAMECANVAS_STATIC
#error Both RLGAMECANVAS_DYNAMIC and RLGAMECANVAS_STATIC were defined. Please only define one or the other.
#endif

#ifdef __cplusplus // C++
#define RLGAMECANVAS_API extern "C"
#else // C
#define RLGAMECANVAS_API extern
#ifndef RLGAMECANVAS_DYNAMIC
#define RLGAMECANVAS_DYNAMIC // C can only use the dynamic version of this library
#endif
#endif

#if defined RLGAMECANVAS_COMPILE
#define RLGAMECANVAS_LIB __declspec(dllexport) __stdcall
#else
#define RLGAMECANVAS_LIB __declspec(dllimport)
#endif

#elif defined(RLGAMECANVAS_STATIC)
#define RLGAMECANVAS_API
#define RLGAMECANVAS_LIB

#else
#error Please define either RLGAMECANVAS_DYNAMIC or RLGAMECANVAS_STATIC in projects including rlGameCanvas.
#endif





#endif // RLGAMECANVAS_CORE_EXPORTSPECS