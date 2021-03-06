#include <es_system.h>
#include "opengl_common/Presenter.h"

namespace
{
	HWND										Window;
	HDC											DeviceContext;
	HGLRC										RenderContext;
	
	LRESULT CALLBACK							WindowProc								(HWND aWindow, UINT aMessage, WPARAM aWParam, LPARAM aLParam)
	{
		if((aMessage == WM_SYSCOMMAND) && (aWParam == SC_SCREENSAVE))
		{
			return 0;
		}

		if(aMessage == WM_ACTIVATE)
		{
			ShowCursor(LOWORD(aWParam) == 0);
		}

		if(aMessage == WM_CLOSE)
		{
			PostQuitMessage(0);
			return 0;
		}

		return DefWindowProc(aWindow, aMessage, aWParam, aLParam);
	}
}

void											ESVideoPlatform::Initialize				(uint32_t& aWidth, uint32_t& aHeight)
{
	//TODO: Error checking!

	WNDCLASS windowClass = {CS_OWNDC, WindowProc, 0, 0, GetModuleHandle(0), 0, 0, 0, 0, "ESWindow"};
	RegisterClass(&windowClass);

	//TODO: Make name configurable
	Window = CreateWindow("ESWindow", "mednafen", WS_POPUP | WS_MAXIMIZE | WS_VISIBLE, 0, 0, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, GetModuleHandle(0), 0);
	DeviceContext = GetDC(Window);

	static const uint32_t formatFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_DEPTH_DONTCARE;
	static const PIXELFORMATDESCRIPTOR format = {sizeof(PIXELFORMATDESCRIPTOR), 1, formatFlags, PFD_TYPE_RGBA, 24};
	uint32_t formatID = ChoosePixelFormat(DeviceContext, &format);
	SetPixelFormat(DeviceContext, formatID, &format);
	RenderContext = wglCreateContext(DeviceContext);
	wglMakeCurrent(DeviceContext,RenderContext);

	//Get window size
	RECT windowSize;
	GetWindowRect(Window, &windowSize);
	aWidth = windowSize.right - windowSize.left;
	aHeight = windowSize.bottom - windowSize.top;

	glewInit();

	LibESGL::Presenter::Initialize();
}

void											ESVideoPlatform::Shutdown				()
{
	LibESGL::Presenter::Shutdown();

	wglMakeCurrent(DeviceContext, 0);
	wglDeleteContext(RenderContext);
	ReleaseDC(Window, DeviceContext);
}

void											ESVideoPlatform::Flip					()
{
	SwapBuffers(DeviceContext);
}

//PRESENTER
bool											ESVideoPlatform::SupportsShaders		()
{
	return true;
}

void											ESVideoPlatform::AttachBorder			(Texture* aTexture)
{
	LibESGL::Presenter::AttachBorder(aTexture);
}

void											ESVideoPlatform::SetFilter				(const std::string& aName, uint32_t aPrescale)
{
	LibESGL::Presenter::SetFilter(aName, aPrescale);
}

void											ESVideoPlatform::Present				(GLuint aID, uint32_t aWidth, uint32_t aHeight, const Area& aViewPort, const Area& aOutput, bool aFlip)
{
	LibESGL::Presenter::Present(aID, aWidth, aHeight, aViewPort, aOutput, aFlip);
}


//OTHER
bool											ESVideoPlatform::SupportsVSyncSelect	()
{
	return false;
}

bool											ESVideoPlatform::SupportsModeSwitch		()
{
	return false;
}

void											ESVideoPlatform::SetVSync				(bool aOn) {assert(false);}
void											ESVideoPlatform::SetMode				(uint32_t aIndex) {assert(false);}
const ESVideoPlatform::ModeList&				ESVideoPlatform::GetModes				() {assert(false);}

