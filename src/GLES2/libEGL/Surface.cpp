// SwiftShader Software Renderer
//
// Copyright(c) 2005-2013 TransGaming Inc.
//
// All rights reserved. No part of this software may be copied, distributed, transmitted,
// transcribed, stored in a retrieval system, translated into any human or computer
// language by any means, or disclosed to third parties without the explicit written
// agreement of TransGaming Inc. Without such an agreement, no rights or licenses, express
// or implied, including but not limited to any patent rights, are granted to you.
//

// Surface.cpp: Implements the egl::Surface class, representing a drawing surface
// such as the client area of a window, including any back buffers.
// Implements EGLSurface and related functionality. [EGL 1.4] section 2.2 page 3.

#include "Surface.h"

#include "main.h"
#include "Display.h"
#include "libGLESv2/Texture.h"
#include "libGLESv2/Device.hpp"
#include "common/debug.h"
#include "Main/FrameBuffer.hpp"

#if defined(_WIN32)
#include <tchar.h>
#endif

namespace egl
{

Surface::Surface(Display *display, const Config *config, EGLNativeWindowType window) 
    : mDisplay(display), mConfig(config), mWindow(window)
{
    frameBuffer = 0;
	backBuffer = 0;

    mDepthStencil = NULL;
    mTexture = NULL;
    mTextureFormat = EGL_NO_TEXTURE;
    mTextureTarget = EGL_NO_TEXTURE;

    mPixelAspectRatio = (EGLint)(1.0 * EGL_DISPLAY_SCALING);   // FIXME: Determine actual pixel aspect ratio
    mRenderBuffer = EGL_BACK_BUFFER;
    mSwapBehavior = EGL_BUFFER_PRESERVED;
    mSwapInterval = -1;
    setSwapInterval(1);

	#if defined(_WIN32)
		subclassWindow();
    #endif
}

Surface::Surface(Display *display, const Config *config, EGLint width, EGLint height, EGLenum textureFormat, EGLenum textureType)
    : mDisplay(display), mWindow(NULL), mConfig(config), mWidth(width), mHeight(height)
{
	frameBuffer = 0;
	backBuffer = 0;

    mDepthStencil = NULL;
    mWindowSubclassed = false;
    mTexture = NULL;
    mTextureFormat = textureFormat;
    mTextureTarget = textureType;

    mPixelAspectRatio = (EGLint)(1.0 * EGL_DISPLAY_SCALING);   // FIXME: Determine actual pixel aspect ratio
    mRenderBuffer = EGL_BACK_BUFFER;
    mSwapBehavior = EGL_BUFFER_PRESERVED;
    mSwapInterval = -1;
    setSwapInterval(1);
}

Surface::~Surface()
{
	#if defined(_WIN32)
		unsubclassWindow();
    #endif
    
    release();
}

bool Surface::initialize()
{
    ASSERT(!frameBuffer && !backBuffer && !mDepthStencil);

    return reset();
}

void Surface::release()
{	
    if(mDepthStencil)
    {
        mDepthStencil->release();
        mDepthStencil = NULL;
    }

    if(mTexture)
    {
        mTexture->releaseTexImage();
        mTexture = NULL;
    }

	if(backBuffer)
	{
		backBuffer->release();
		backBuffer = 0;
	}

	delete frameBuffer;
	frameBuffer = 0;
}

bool Surface::reset()
{
    if(!mWindow)
    {
        return reset(mWidth, mHeight);
    }

	// FIXME: Wrap into an abstract Window class
	#if defined(_WIN32)
		RECT windowRect;
		GetClientRect(mWindow, &windowRect);

		return reset(windowRect.right - windowRect.left, windowRect.bottom - windowRect.top);
	#else
		XWindowAttributes windowAttributes;
		XGetWindowAttributes(mDisplay->getNativeDisplay(), mWindow, &windowAttributes);
		
		return reset(windowAttributes.width, windowAttributes.height);
	#endif
}

bool Surface::reset(int backBufferWidth, int backBufferHeight)
{
    gl::Device *device = mDisplay->getDevice();

    if(device == NULL)
    {
        return false;
    }

    release();

    if(mWindow)
    {
		frameBuffer = gl::createFrameBuffer(mWindow, backBufferWidth, backBufferHeight);

		if(!frameBuffer)
		{
			ERR("Could not create frame buffer");
			release();
			return error(EGL_BAD_ALLOC, false);
		}

		#if defined(_WIN32)
			InvalidateRect(mWindow, NULL, FALSE);
		#endif
    }

	backBuffer = gl::createBackBuffer(backBufferWidth, backBufferHeight, mConfig);
	
    if(!backBuffer)
    {
        ERR("Could not create back buffer");
        release();
        return error(EGL_BAD_ALLOC, false);
    }

    if(mConfig->mDepthStencilFormat != sw::FORMAT_NULL)
    {
        mDepthStencil = device->createDepthStencilSurface(backBufferWidth, backBufferHeight, mConfig->mDepthStencilFormat, 1, false);

		if(!mDepthStencil)
		{
			ERR("Could not create depth/stencil buffer for surface");
			release();
			return error(EGL_BAD_ALLOC, false);
		}
    }

    mWidth = backBufferWidth;
    mHeight = backBufferHeight;

    return true;
}

EGLNativeWindowType Surface::getWindowHandle()
{
    return mWindow;
}

void Surface::swap()
{
    #if PERF_PROFILE
		profiler.nextFrame();
	#endif

	if(backBuffer)
    {
		bool HDR = backBuffer->getInternalFormat() == sw::FORMAT_A16B16G16R16;
		void *source = backBuffer->lockInternal(0, 0, 0, sw::LOCK_READONLY, sw::PUBLIC);
		frameBuffer->flip(source, HDR);
		backBuffer->unlockInternal();
	
		#if defined(_WIN32)
			checkForResize();
		#endif
	}
}

EGLint Surface::getWidth() const
{
    return mWidth;
}

EGLint Surface::getHeight() const
{
    return mHeight;
}

gl::Image *Surface::getRenderTarget()
{
    if(backBuffer)
    {
        backBuffer->addRef();
    }

    return backBuffer;
}

gl::Image *Surface::getDepthStencil()
{
    if(mDepthStencil)
    {
        mDepthStencil->addRef();
    }

    return mDepthStencil;
}

void Surface::setSwapInterval(EGLint interval)
{
    if(mSwapInterval == interval)
    {
        return;
    }
    
    mSwapInterval = interval;
    mSwapInterval = std::max(mSwapInterval, mDisplay->getMinSwapInterval());
    mSwapInterval = std::min(mSwapInterval, mDisplay->getMaxSwapInterval());
}

EGLenum Surface::getTextureFormat() const
{
    return mTextureFormat;
}

EGLenum Surface::getTextureTarget() const
{
    return mTextureTarget;
}

void Surface::setBoundTexture(gl::Texture2D *texture)
{
    mTexture = texture;
}

gl::Texture2D *Surface::getBoundTexture() const
{
    return mTexture;
}

sw::Format Surface::getInternalFormat() const
{
    return mConfig->mRenderTargetFormat;
}

#if defined(WIN32)
#define kSurfaceProperty _TEXT("Egl::SurfaceOwner")
#define kParentWndProc _TEXT("Egl::SurfaceParentWndProc")

static LRESULT CALLBACK SurfaceWindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	if(message == WM_SIZE)
	{
		Surface *surface = reinterpret_cast<Surface*>(GetProp(hwnd, kSurfaceProperty));
		
		if(surface)
		{
			surface->checkForResize();
		}
	}

	WNDPROC prevWndFunc = reinterpret_cast<WNDPROC>(GetProp(hwnd, kParentWndProc));

	return CallWindowProc(prevWndFunc, hwnd, message, wparam, lparam);
}

void Surface::subclassWindow()
{
	if(!mWindow)
	{
		return;
	}

	DWORD processId;
	DWORD threadId = GetWindowThreadProcessId(mWindow, &processId);
	if (processId != GetCurrentProcessId() || threadId != GetCurrentThreadId())
	{
		return;
	}

	SetLastError(0);
	LONG oldWndProc = SetWindowLong(mWindow, GWL_WNDPROC, reinterpret_cast<LONG>(SurfaceWindowProc));

	if(oldWndProc == 0 && GetLastError() != ERROR_SUCCESS)
	{
		mWindowSubclassed = false;
		return;
	}

	SetProp(mWindow, kSurfaceProperty, reinterpret_cast<HANDLE>(this));
	SetProp(mWindow, kParentWndProc, reinterpret_cast<HANDLE>(oldWndProc));
	mWindowSubclassed = true;
}

void Surface::unsubclassWindow()
{
	if(!mWindowSubclassed)
	{
		return;
	}

	// un-subclass
	LONG parentWndFunc = reinterpret_cast<LONG>(GetProp(mWindow, kParentWndProc));

	// Check the windowproc is still SurfaceWindowProc.
	// If this assert fails, then it is likely the application has subclassed the
	// hwnd as well and did not unsubclass before destroying its EGL context. The
	// application should be modified to either subclass before initializing the
	// EGL context, or to unsubclass before destroying the EGL context.
	if(parentWndFunc)
	{
		LONG prevWndFunc = SetWindowLong(mWindow, GWL_WNDPROC, parentWndFunc);
		ASSERT(prevWndFunc == reinterpret_cast<LONG>(SurfaceWindowProc));
	}

	RemoveProp(mWindow, kSurfaceProperty);
	RemoveProp(mWindow, kParentWndProc);
	mWindowSubclassed = false;
}

bool Surface::checkForResize()
{
    RECT client;
    if(!GetClientRect(mWindow, &client))
    {
        ASSERT(false);
        return false;
    }

    // Grow the buffer now, if the window has grown. We need to grow now to avoid losing information.
    int clientWidth = client.right - client.left;
    int clientHeight = client.bottom - client.top;
    bool sizeDirty = clientWidth != getWidth() || clientHeight != getHeight();

    if(sizeDirty)
    {
        reset(clientWidth, clientHeight);

        if(static_cast<egl::Surface*>(getCurrentDrawSurface()) == this)
        {
            gl::makeCurrent(gl::getCurrentContext(), static_cast<egl::Display*>(getCurrentDisplay()), this);
        }

        return true;
    }

    return false;
}
#endif
}