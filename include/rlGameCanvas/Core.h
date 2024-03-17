#ifndef RLGAMECANVAS_CORE_C
#define RLGAMECANVAS_CORE_C





#include "ExportSpecs.h"
#include "Definitions.h"
#include "Types.h"

/// <summary>
/// Create an <c>rlGameCanvas</c> object.
/// </summary>
/// <param name="pStartupConfig">Pointer to the startup configuration to be used.</param>
/// <returns>
/// If the function succeeded, the return value is the handle of the newly created
/// <c>rlGameCanvas</c> object.<para/>
/// If the function failed, the return value is zero.
/// </returns>
RLGAMECANVAS_API rlGameCanvas RLGAMECANVAS_LIB rlGameCanvas_Create(
	const rlGameCanvas_StartupConfig *pStartupConfig
);

/// <summary>
/// Destroy an <c>rlGameCanvas</c> object.
/// </summary>
/// <param name="canvas">The handle of the <c>rlGameCanvas</c> object to be destroyed.</param>
RLGAMECANVAS_API void RLGAMECANVAS_LIB rlGameCanvas_Destroy(
	rlGameCanvas canvas
);

/// <summary>
/// Run a <c>rlGameCanvas</c> object.
/// </summary>
/// <param name="canvas">The canvas to run.</param>
/// <returns>Did the canvas successfully run?</returns>
RLGAMECANVAS_API rlGameCanvas_Bool RLGAMECANVAS_LIB rlGameCanvas_Run(
	rlGameCanvas canvas
);

/// <summary>
/// Quit a running <c>rlGameCanvas</c> object.<para />
/// Calling this function doesn't immediately stop execution of the graphics thread. Instead, a
/// request is made that will be handled after the current frame.
/// </summary>
/// <param name="canvas">The canvas to quit execution on.</param>
RLGAMECANVAS_API void RLGAMECANVAS_LIB rlGameCanvas_Quit(
	rlGameCanvas canvas
);





#endif // RLGAMECANVAS_CORE_C