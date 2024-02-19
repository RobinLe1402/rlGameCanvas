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
/// If the function succeeded, the return value is the handle of the newly created <c>rlGameCanvas</c>
/// object.<para/>
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





#endif // RLGAMECANVAS_CORE_C