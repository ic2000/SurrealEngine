#define SDL2_WINDOW

#include "Window.h"
#include "Precomp.h"
#ifdef SDL2_WINDOW
#include "SDL2/SDL2Window.h"
#elif defined(WIN32)
#include "Win32/Win32Window.h"
#else
#include "X11/X11Window.h"
#endif

#ifdef SDL2_WINDOW

std::unique_ptr<DisplayWindow> DisplayWindow::Create(Engine *engine)
{
	return std::make_unique<SDL2Window>(engine);
}

#elif defined(WIN32)

std::unique_ptr<DisplayWindow> DisplayWindow::Create(Engine *engine)
{
	return std::make_unique<Win32Window>(engine);
}

#else

std::unique_ptr<DisplayWindow> DisplayWindow::Create(Engine *engine)
{
	return std::make_unique<X11Window>(engine);
}

#endif
