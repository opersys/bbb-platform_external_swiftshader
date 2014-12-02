// SwiftShader Software Renderer
//
// Copyright(c) 2005-2012 TransGaming Inc.
//
// All rights reserved. No part of this software may be copied, distributed, transmitted,
// transcribed, stored in a retrieval system, translated into any human or computer
// language by any means, or disclosed to third parties without the explicit written
// agreement of TransGaming Inc. Without such an agreement, no rights or licenses, express
// or implied, including but not limited to any patent rights, are granted to you.
//

// main.h: Management of thread-local data.

#ifndef LIBEGL_MAIN_H_
#define LIBEGL_MAIN_H_

#define EGLAPI
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <RAD/rad.h>

namespace egl
{
	struct Current
	{
		EGLint error;
		EGLenum API;
		EGLDisplay display;
		EGLContext context;
		EGLSurface drawSurface;
		EGLSurface readSurface;
	};

	void setCurrentError(EGLint error);
	EGLint getCurrentError();

	void setCurrentAPI(EGLenum API);
	EGLenum getCurrentAPI();

	void setCurrentDisplay(EGLDisplay dpy);
	EGLDisplay getCurrentDisplay();

	void setCurrentContext(EGLContext ctx);
	EGLContext getCurrentContext();

	void setCurrentDrawSurface(EGLSurface surface);
	EGLSurface getCurrentDrawSurface();

	void setCurrentReadSurface(EGLSurface surface);
	EGLSurface getCurrentReadSurface();
}

void error(EGLint errorCode);

template<class T>
const T &error(EGLint errorCode, const T &returnValue)
{
    error(errorCode);

    return returnValue;
}

template<class T>
const T &success(const T &returnValue)
{
    egl::setCurrentError(EGL_SUCCESS);

    return returnValue;
}

namespace egl
{
	class Config;
	class Surface;
	class Display;
	class Context;
	class Image;
}

namespace sw
{
	class FrameBuffer;
	enum Format : unsigned char;
}

// libRAD dependencies
namespace es2
{
	extern egl::Context *(*createContext)(const egl::Config *config, const egl::Context *shareContext);

	extern egl::Image *(*createBackBuffer)(int width, int height, const egl::Config *config);
	extern egl::Image *(*createDepthStencil)(unsigned int width, unsigned int height, sw::Format format, int multiSampleDepth, bool discard);
	extern sw::FrameBuffer *(*createFrameBuffer)(EGLNativeDisplayType display, EGLNativeWindowType window, int width, int height);
}

namespace rad
{
	extern __eglMustCastToProperFunctionPointerType (RADAPIENTRY *getProcAddress)(const char *procname);
}

extern void *libRAD;    // Handle to the libRAD module

#endif  // LIBEGL_MAIN_H_
