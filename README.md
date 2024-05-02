# rlGameCanvas – A pixel-based game graphics framework
<img alt="rlGameCanvas Logo" src="res/logo.jpg" width="200px">

This library acts like a canvas for pixel-based (= bitmap-like graphics) video games.

At this point in time, only Windows is supported.

While the library interface is written to be backwards compatible to older C++ standards, C++20 is
required to compile the library.



## Basics
The core object of this engine are bitmap graphics - arrays of pixels that are modified at runtime.

### Background Color
The very background consists of a single color which can be modified at any point.
It's visible everywhere where all the visible layers are transparent.

### Layer
A layer is basically a single, (possibly) transparent bitmap.
It's visibility can be toggled on or off.
Layers can be bigger than the screen, in which case the screen position of a layer can also be
changed. The "screen position" is the ID of the topmost, leftmost pixel visible on the screen.
For example, the first pixel on the top left has the position (0,0). The pixel to the right of that
pixel has the position (1,0).
The screen position is limited by the size of the layer. The screen can't go beyond the borders of
the layer.

### Screen
The screen is the currently visible image - it's basically a "camera".

### Mode
A "mode" is a screen size as well as layer definitions.



## Window Behaviour
The bitmap is always automatically scaled up via nearest-neighbor scaling to fill up as much of the
available space as possible.

### Windowed
The provided window...
* is **not manually resizable** by the user
* is sized up to 80% of the work area width and/or height of the current monitor

### Maximized
When maximized, the client area is basically a smaller fullscreen mode.

### Fullscreen
In fullscreen mode, the bitmap is scaled up as much as it can be, possibly with black borders if
the monitor has a different aspect ratio than the bitmap.

The application can configure the bitmap to only be upscaled in exact multiples which would possibly
result in black border on each side around the bitmap (I call this "pixel perfect mode").
By default, this behaviour is disabled.

Fullscreen mode always uses the primary monitor.


## Options
When using a canvas, there are a multiple of options that can be tweaked either at initialization
or at runtime.

### Initialization options
The following options can **only** be set on startup.
* window caption
* window icon(s)
* "pixel perfect mode" setting (see [Fullscreen](#fullscreen))

### Runtime options
The following options can also be changed at runtime.
* restrict cursor to client area
* hide cursor
* fullscreen mode (can also be toggled by the user via [ALT]+[Return])
* the current ["mode"](#mode)
