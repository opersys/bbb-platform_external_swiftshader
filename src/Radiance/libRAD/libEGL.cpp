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

// libEGL.cpp: Implements the exported EGL functions.

#include "main.h"
#include "Display.h"
#include "Surface.h"
#include "TextureEGL.hpp"
#include "Context.hpp"
#include "Image.hpp"
#include "common/debug.h"
#include "Common/Version.h"

static bool validateDisplay(egl::Display *display)
{
    if(display == EGL_NO_DISPLAY)
    {
        return egl::error(EGL_BAD_DISPLAY, false);
    }

    if(!display->isInitialized())
    {
        return egl::error(EGL_NOT_INITIALIZED, false);
    }

    return true;
}

static bool validateConfig(egl::Display *display, EGLConfig config)
{
    if(!validateDisplay(display))
    {
        return false;
    }

    if(!display->isValidConfig(config))
    {
        return egl::error(EGL_BAD_CONFIG, false);
    }

    return true;
}

static bool validateContext(egl::Display *display, egl::Context *context)
{
    if(!validateDisplay(display))
    {
        return false;
    }

    if(!display->isValidContext(context))
    {
        return egl::error(EGL_BAD_CONTEXT, false);
    }

    return true;
}

static bool validateSurface(egl::Display *display, egl::Surface *surface)
{
    if(!validateDisplay(display))
    {
        return false;
    }

    if(!display->isValidSurface(surface))
    {
        return egl::error(EGL_BAD_SURFACE, false);
    }

    return true;
}

extern "C"
{
EGLint EGLAPIENTRY eglGetError(void)
{
    TRACE("()");

    EGLint error = egl::getCurrentError();

    if(error != EGL_SUCCESS)
    {
        egl::setCurrentError(EGL_SUCCESS);
    }

    return error;
}

EGLDisplay EGLAPIENTRY eglGetDisplay(EGLNativeDisplayType display_id)
{
    TRACE("(EGLNativeDisplayType display_id = %p)", display_id);

    try
    {
        return egl::Display::getDisplay(display_id);
    }
    catch(std::bad_alloc&)
    {
        return egl::error(EGL_BAD_ALLOC, EGL_NO_DISPLAY);
    }

    return EGL_NO_DISPLAY;
}

EGLBoolean EGLAPIENTRY eglInitialize(EGLDisplay dpy, EGLint *major, EGLint *minor)
{
    TRACE("(EGLDisplay dpy = %p, EGLint *major = %p, EGLint *minor = %p)",
          dpy, major, minor);

    try
    {
        if(dpy == EGL_NO_DISPLAY)
        {
            return egl::error(EGL_BAD_DISPLAY, EGL_FALSE);
        }

        egl::Display *display = static_cast<egl::Display*>(dpy);

        if(!display->initialize())
        {
            return egl::error(EGL_NOT_INITIALIZED, EGL_FALSE);
        }

        if(major) *major = 1;
        if(minor) *minor = 4;

        return egl::success(EGL_TRUE);
    }
    catch(std::bad_alloc&)
    {
        return egl::error(EGL_BAD_ALLOC, EGL_FALSE);
    }

    return EGL_FALSE;
}

EGLBoolean EGLAPIENTRY eglTerminate(EGLDisplay dpy)
{
    TRACE("(EGLDisplay dpy = %p)", dpy);

    try
    {
        if(dpy == EGL_NO_DISPLAY)
        {
            return egl::error(EGL_BAD_DISPLAY, EGL_FALSE);
        }

        egl::Display *display = static_cast<egl::Display*>(dpy);

        display->terminate();

        return egl::success(EGL_TRUE);
    }
    catch(std::bad_alloc&)
    {
        return egl::error(EGL_BAD_ALLOC, EGL_FALSE);
    }

    return EGL_FALSE;
}

const char *EGLAPIENTRY eglQueryString(EGLDisplay dpy, EGLint name)
{
    TRACE("(EGLDisplay dpy = %p, EGLint name = %d)", dpy, name);

    try
    {
        egl::Display *display = static_cast<egl::Display*>(dpy);

        if(!validateDisplay(display))
        {
            return NULL;
        }

        switch(name)
        {
        case EGL_CLIENT_APIS:
            return egl::success("OpenGL_ES");
        case EGL_EXTENSIONS:
            return egl::success("EGL_KHR_gl_texture_2D_image "
                           "EGL_KHR_gl_texture_cubemap_image "
                           "EGL_KHR_gl_renderbuffer_image "
                           "EGL_KHR_image_base");
        case EGL_VENDOR:
            return egl::success("TransGaming Inc.");
        case EGL_VERSION:
            return egl::success("1.4 SwiftShader " VERSION_STRING);
        }

        return egl::error(EGL_BAD_PARAMETER, (const char*)NULL);
    }
    catch(std::bad_alloc&)
    {
        return egl::error(EGL_BAD_ALLOC, (const char*)NULL);
    }

    return NULL;
}

EGLBoolean EGLAPIENTRY eglGetConfigs(EGLDisplay dpy, EGLConfig *configs, EGLint config_size, EGLint *num_config)
{
    TRACE("(EGLDisplay dpy = %p, EGLConfig *configs = %p, "
          "EGLint config_size = %d, EGLint *num_config = %p)",
          dpy, configs, config_size, num_config);

    try
    {
        egl::Display *display = static_cast<egl::Display*>(dpy);

        if(!validateDisplay(display))
        {
            return EGL_FALSE;
        }

        if(!num_config)
        {
            return egl::error(EGL_BAD_PARAMETER, EGL_FALSE);
        }

        const EGLint attribList[] = {EGL_NONE};

        if(!display->getConfigs(configs, attribList, config_size, num_config))
        {
            return egl::error(EGL_BAD_ATTRIBUTE, EGL_FALSE);
        }

        return egl::success(EGL_TRUE);
    }
    catch(std::bad_alloc&)
    {
        return egl::error(EGL_BAD_ALLOC, EGL_FALSE);
    }

    return EGL_FALSE;
}

EGLBoolean EGLAPIENTRY eglChooseConfig(EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config)
{
    TRACE("(EGLDisplay dpy = %p, const EGLint *attrib_list = %p, "
          "EGLConfig *configs = %p, EGLint config_size = %d, EGLint *num_config = %p)",
          dpy, attrib_list, configs, config_size, num_config);

    try
    {
        egl::Display *display = static_cast<egl::Display*>(dpy);

        if(!validateDisplay(display))
        {
            return EGL_FALSE;
        }

        if(!num_config)
        {
            return egl::error(EGL_BAD_PARAMETER, EGL_FALSE);
        }

        const EGLint attribList[] = {EGL_NONE};

        if(!attrib_list)
        {
            attrib_list = attribList;
        }

        display->getConfigs(configs, attrib_list, config_size, num_config);

        return egl::success(EGL_TRUE);
    }
    catch(std::bad_alloc&)
    {
        return egl::error(EGL_BAD_ALLOC, EGL_FALSE);
    }

    return EGL_FALSE;
}

EGLBoolean EGLAPIENTRY eglGetConfigAttrib(EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint *value)
{
    TRACE("(EGLDisplay dpy = %p, EGLConfig config = %p, EGLint attribute = %d, EGLint *value = %p)",
          dpy, config, attribute, value);

    try
    {
        egl::Display *display = static_cast<egl::Display*>(dpy);

        if(!validateConfig(display, config))
        {
            return EGL_FALSE;
        }

        if(!display->getConfigAttrib(config, attribute, value))
        {
            return egl::error(EGL_BAD_ATTRIBUTE, EGL_FALSE);
        }

        return egl::success(EGL_TRUE);
    }
    catch(std::bad_alloc&)
    {
        return egl::error(EGL_BAD_ALLOC, EGL_FALSE);
    }

    return EGL_FALSE;
}

EGLSurface EGLAPIENTRY eglCreateWindowSurface(EGLDisplay dpy, EGLConfig config, EGLNativeWindowType window, const EGLint *attrib_list)
{
    TRACE("(EGLDisplay dpy = %p, EGLConfig config = %p, EGLNativeWindowType win = %p, "
          "const EGLint *attrib_list = %p)", dpy, config, window, attrib_list);

    try
    {
        egl::Display *display = static_cast<egl::Display*>(dpy);

        if(!validateConfig(display, config))
        {
            return EGL_NO_SURFACE;
        }

		if(!display->isValidWindow(window))
		{
			return egl::error(EGL_BAD_NATIVE_WINDOW, EGL_NO_SURFACE);
		}

        return display->createWindowSurface(window, config, attrib_list);
    }
    catch(std::bad_alloc&)
    {
        return egl::error(EGL_BAD_ALLOC, EGL_NO_SURFACE);
    }

    return EGL_NO_SURFACE;
}

EGLSurface EGLAPIENTRY eglCreatePbufferSurface(EGLDisplay dpy, EGLConfig config, const EGLint *attrib_list)
{
    TRACE("(EGLDisplay dpy = %p, EGLConfig config = %p, const EGLint *attrib_list = %p)",
          dpy, config, attrib_list);

    try
    {
        egl::Display *display = static_cast<egl::Display*>(dpy);

        if(!validateConfig(display, config))
        {
            return EGL_NO_SURFACE;
        }

        return display->createOffscreenSurface(config, attrib_list);
    }
    catch(std::bad_alloc&)
    {
        return egl::error(EGL_BAD_ALLOC, EGL_NO_SURFACE);
    }

    return EGL_NO_SURFACE;
}

EGLSurface EGLAPIENTRY eglCreatePixmapSurface(EGLDisplay dpy, EGLConfig config, EGLNativePixmapType pixmap, const EGLint *attrib_list)
{
    TRACE("(EGLDisplay dpy = %p, EGLConfig config = %p, EGLNativePixmapType pixmap = %p, "
          "const EGLint *attrib_list = %p)", dpy, config, pixmap, attrib_list);

    try
    {
        egl::Display *display = static_cast<egl::Display*>(dpy);

        if(!validateConfig(display, config))
        {
            return EGL_NO_SURFACE;
        }

        UNIMPLEMENTED();   // FIXME

        return egl::success(EGL_NO_SURFACE);
    }
    catch(std::bad_alloc&)
    {
        return egl::error(EGL_BAD_ALLOC, EGL_NO_SURFACE);
    }

    return EGL_NO_SURFACE;
}

EGLBoolean EGLAPIENTRY eglDestroySurface(EGLDisplay dpy, EGLSurface surface)
{
    TRACE("(EGLDisplay dpy = %p, EGLSurface surface = %p)", dpy, surface);

    try
    {
        egl::Display *display = static_cast<egl::Display*>(dpy);
        egl::Surface *eglSurface = static_cast<egl::Surface*>(surface);

        if(!validateSurface(display, eglSurface))
        {
            return EGL_FALSE;
        }

        if(surface == EGL_NO_SURFACE)
        {
            return egl::error(EGL_BAD_SURFACE, EGL_FALSE);
        }

        display->destroySurface((egl::Surface*)surface);

        return egl::success(EGL_TRUE);
    }
    catch(std::bad_alloc&)
    {
        return egl::error(EGL_BAD_ALLOC, EGL_FALSE);
    }

    return EGL_FALSE;
}

EGLBoolean EGLAPIENTRY eglQuerySurface(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint *value)
{
    TRACE("(EGLDisplay dpy = %p, EGLSurface surface = %p, EGLint attribute = %d, EGLint *value = %p)",
          dpy, surface, attribute, value);

    try
    {
        egl::Display *display = static_cast<egl::Display*>(dpy);
        egl::Surface *eglSurface = (egl::Surface*)surface;

        if(!validateSurface(display, eglSurface))
        {
            return EGL_FALSE;
        }

        if(surface == EGL_NO_SURFACE)
        {
            return egl::error(EGL_BAD_SURFACE, EGL_FALSE);
        }

        switch(attribute)
        {
          case EGL_VG_ALPHA_FORMAT:
            UNIMPLEMENTED();   // FIXME
            break;
          case EGL_VG_COLORSPACE:
            UNIMPLEMENTED();   // FIXME
            break;
          case EGL_CONFIG_ID:
            *value = eglSurface->getConfigID();
            break;
          case EGL_HEIGHT:
            *value = eglSurface->getHeight();
            break;
          case EGL_HORIZONTAL_RESOLUTION:
            UNIMPLEMENTED();   // FIXME
            break;
          case EGL_LARGEST_PBUFFER:
            UNIMPLEMENTED();   // FIXME
            break;
          case EGL_MIPMAP_TEXTURE:
            UNIMPLEMENTED();   // FIXME
            break;
          case EGL_MIPMAP_LEVEL:
            UNIMPLEMENTED();   // FIXME
            break;
          case EGL_MULTISAMPLE_RESOLVE:
            UNIMPLEMENTED();   // FIXME
            break;
          case EGL_PIXEL_ASPECT_RATIO:
            *value = eglSurface->getPixelAspectRatio();
            break;
          case EGL_RENDER_BUFFER:
            *value = eglSurface->getRenderBuffer();
            break;
          case EGL_SWAP_BEHAVIOR:
            *value = eglSurface->getSwapBehavior();
            break;
          case EGL_TEXTURE_FORMAT:
            *value = eglSurface->getTextureFormat();
            break;
          case EGL_TEXTURE_TARGET:
            *value = eglSurface->getTextureTarget();
            break;
          case EGL_VERTICAL_RESOLUTION:
            UNIMPLEMENTED();   // FIXME
            break;
          case EGL_WIDTH:
            *value = eglSurface->getWidth();
            break;
          default:
            return egl::error(EGL_BAD_ATTRIBUTE, EGL_FALSE);
        }

        return egl::success(EGL_TRUE);
    }
    catch(std::bad_alloc&)
    {
        return egl::error(EGL_BAD_ALLOC, EGL_FALSE);
    }

    return EGL_FALSE;
}

EGLBoolean EGLAPIENTRY eglBindAPI(EGLenum api)
{
    TRACE("(EGLenum api = 0x%X)", api);

    try
    {
        switch (api)
        {
          case EGL_OPENGL_API:
          case EGL_OPENVG_API:
            return egl::error(EGL_BAD_PARAMETER, EGL_FALSE);   // Not supported by this implementation
          case EGL_OPENGL_ES_API:
            break;
          default:
            return egl::error(EGL_BAD_PARAMETER, EGL_FALSE);
        }

        egl::setCurrentAPI(api);

        return egl::success(EGL_TRUE);
    }
    catch(std::bad_alloc&)
    {
        return egl::error(EGL_BAD_ALLOC, EGL_FALSE);
    }

    return EGL_FALSE;
}

EGLenum EGLAPIENTRY eglQueryAPI(void)
{
    TRACE("()");

    try
    {
        EGLenum API = egl::getCurrentAPI();

        return egl::success(API);
    }
    catch(std::bad_alloc&)
    {
        return egl::error(EGL_BAD_ALLOC, EGL_FALSE);
    }

    return EGL_FALSE;
}

EGLBoolean EGLAPIENTRY eglWaitClient(void)
{
    TRACE("()");

    try
    {
        UNIMPLEMENTED();   // FIXME

        return egl::success(0);
    }
    catch(std::bad_alloc&)
    {
        return egl::error(EGL_BAD_ALLOC, EGL_FALSE);
    }

    return EGL_FALSE;
}

EGLBoolean EGLAPIENTRY eglReleaseThread(void)
{
    TRACE("()");

    try
    {
        eglMakeCurrent(EGL_NO_DISPLAY, EGL_NO_CONTEXT, EGL_NO_SURFACE, EGL_NO_SURFACE);

        return egl::success(EGL_TRUE);
    }
    catch(std::bad_alloc&)
    {
        return egl::error(EGL_BAD_ALLOC, EGL_FALSE);
    }

    return EGL_FALSE;
}

EGLSurface EGLAPIENTRY eglCreatePbufferFromClientBuffer(EGLDisplay dpy, EGLenum buftype, EGLClientBuffer buffer, EGLConfig config, const EGLint *attrib_list)
{
    TRACE("(EGLDisplay dpy = %p, EGLenum buftype = 0x%X, EGLClientBuffer buffer = %p, "
          "EGLConfig config = %p, const EGLint *attrib_list = %p)",
          dpy, buftype, buffer, config, attrib_list);

	UNIMPLEMENTED();

    return egl::error(EGL_BAD_PARAMETER, EGL_NO_SURFACE);
}

EGLBoolean EGLAPIENTRY eglSurfaceAttrib(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint value)
{
    TRACE("(EGLDisplay dpy = %p, EGLSurface surface = %p, EGLint attribute = %d, EGLint value = %d)",
          dpy, surface, attribute, value);

    try
    {
        egl::Display *display = static_cast<egl::Display*>(dpy);
        egl::Surface *eglSurface = static_cast<egl::Surface*>(surface);

        if(!validateSurface(display, eglSurface))
        {
            return EGL_FALSE;
        }

        UNIMPLEMENTED();   // FIXME

        return egl::success(EGL_TRUE);
    }
    catch(std::bad_alloc&)
    {
        return egl::error(EGL_BAD_ALLOC, EGL_FALSE);
    }

    return EGL_FALSE;
}

EGLBoolean EGLAPIENTRY eglSwapInterval(EGLDisplay dpy, EGLint interval)
{
    TRACE("(EGLDisplay dpy = %p, EGLint interval = %d)", dpy, interval);

    try
    {
        egl::Display *display = static_cast<egl::Display*>(dpy);

        if(!validateDisplay(display))
        {
            return EGL_FALSE;
        }

        egl::Surface *draw_surface = static_cast<egl::Surface*>(egl::getCurrentDrawSurface());

        if(draw_surface == NULL)
        {
            return egl::error(EGL_BAD_SURFACE, EGL_FALSE);
        }

        draw_surface->setSwapInterval(interval);

        return egl::success(EGL_TRUE);
    }
    catch(std::bad_alloc&)
    {
        return egl::error(EGL_BAD_ALLOC, EGL_FALSE);
    }

    return EGL_FALSE;
}

EGLContext EGLAPIENTRY eglCreateContext(EGLDisplay dpy, EGLConfig config, EGLContext share_context, const EGLint *attrib_list)
{
    TRACE("(EGLDisplay dpy = %p, EGLConfig config = %p, EGLContext share_context = %p, "
          "const EGLint *attrib_list = %p)", dpy, config, share_context, attrib_list);

    try
    {
        EGLint clientVersion = 1;
        if(attrib_list)
        {
            for(const EGLint* attribute = attrib_list; attribute[0] != EGL_NONE; attribute += 2)
            {
                if(attribute[0] == EGL_CONTEXT_CLIENT_VERSION)
                {
                    clientVersion = attribute[1];
                }
                else
                {
                    return egl::error(EGL_BAD_ATTRIBUTE, EGL_NO_CONTEXT);
                }
            }
        }

        egl::Display *display = static_cast<egl::Display*>(dpy);

        if(!validateConfig(display, config))
        {
            return EGL_NO_CONTEXT;
        }

        return display->createContext(config, static_cast<egl::Context*>(share_context), clientVersion);
    }
    catch(std::bad_alloc&)
    {
        return egl::error(EGL_BAD_ALLOC, EGL_NO_CONTEXT);
    }

    return EGL_NO_CONTEXT;
}

EGLBoolean EGLAPIENTRY eglDestroyContext(EGLDisplay dpy, EGLContext ctx)
{
    TRACE("(EGLDisplay dpy = %p, EGLContext ctx = %p)", dpy, ctx);

    try
    {
        egl::Display *display = static_cast<egl::Display*>(dpy);
        egl::Context *context = static_cast<egl::Context*>(ctx);

        if(!validateContext(display, context))
        {
            return EGL_FALSE;
        }

        if(ctx == EGL_NO_CONTEXT)
        {
            return egl::error(EGL_BAD_CONTEXT, EGL_FALSE);
        }

        display->destroyContext(context);

        return egl::success(EGL_TRUE);
    }
    catch(std::bad_alloc&)
    {
        return egl::error(EGL_BAD_ALLOC, EGL_FALSE);
    }

    return EGL_FALSE;
}

EGLBoolean EGLAPIENTRY eglMakeCurrent(EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx)
{
    TRACE("(EGLDisplay dpy = %p, EGLSurface draw = %p, EGLSurface read = %p, EGLContext ctx = %p)",
          dpy, draw, read, ctx);

    try
    {
        egl::Display *display = static_cast<egl::Display*>(dpy);
        egl::Context *context = static_cast<egl::Context*>(ctx);

		if(ctx != EGL_NO_CONTEXT || draw != EGL_NO_SURFACE || read != EGL_NO_SURFACE)
		{
			if(!validateDisplay(display))
			{
				return EGL_FALSE;
			}
		}

		if(ctx == EGL_NO_CONTEXT && (draw != EGL_NO_SURFACE || read != EGL_NO_SURFACE))
		{
			return egl::error(EGL_BAD_MATCH, EGL_FALSE);
		}

        if(ctx != EGL_NO_CONTEXT && !validateContext(display, context))
        {
            return EGL_FALSE;
        }

        if((draw != EGL_NO_SURFACE && !validateSurface(display, static_cast<egl::Surface*>(draw))) ||
           (read != EGL_NO_SURFACE && !validateSurface(display, static_cast<egl::Surface*>(read))))
        {
            return EGL_FALSE;
        }

		if((draw != EGL_NO_SURFACE) ^ (read != EGL_NO_SURFACE))
		{
			return egl::error(EGL_BAD_MATCH, EGL_FALSE);
		}

        if(draw != read)
        {
            UNIMPLEMENTED();   // FIXME
        }

        egl::setCurrentDisplay(dpy);
        egl::setCurrentDrawSurface(draw);
        egl::setCurrentReadSurface(read);
		egl::setCurrentContext(ctx);

		if(context)
		{
			context->makeCurrent(static_cast<egl::Surface*>(draw));
		}

        return egl::success(EGL_TRUE);
    }
    catch(std::bad_alloc&)
    {
        return egl::error(EGL_BAD_ALLOC, EGL_FALSE);
    }

    return EGL_FALSE;
}

EGLContext EGLAPIENTRY eglGetCurrentContext(void)
{
    TRACE("()");

    try
    {
		EGLContext context = egl::getCurrentContext();

		return egl::success(context);
    }
    catch(std::bad_alloc&)
    {
        return egl::error(EGL_BAD_ALLOC, EGL_NO_CONTEXT);
    }

    return EGL_NO_CONTEXT;
}

EGLSurface EGLAPIENTRY eglGetCurrentSurface(EGLint readdraw)
{
    TRACE("(EGLint readdraw = %d)", readdraw);

    try
    {
        if(readdraw == EGL_READ)
        {
            EGLSurface read = egl::getCurrentReadSurface();
            return egl::success(read);
        }
        else if(readdraw == EGL_DRAW)
        {
            EGLSurface draw = egl::getCurrentDrawSurface();
            return egl::success(draw);
        }
        else
        {
            return egl::error(EGL_BAD_PARAMETER, EGL_NO_SURFACE);
        }
    }
    catch(std::bad_alloc&)
    {
        return egl::error(EGL_BAD_ALLOC, EGL_NO_SURFACE);
    }

    return EGL_NO_SURFACE;
}

EGLDisplay EGLAPIENTRY eglGetCurrentDisplay(void)
{
    TRACE("()");

    try
    {
        EGLDisplay dpy = egl::getCurrentDisplay();

        return egl::success(dpy);
    }
    catch(std::bad_alloc&)
    {
        return egl::error(EGL_BAD_ALLOC, EGL_NO_DISPLAY);
    }

    return EGL_NO_DISPLAY;
}

EGLBoolean EGLAPIENTRY eglQueryContext(EGLDisplay dpy, EGLContext ctx, EGLint attribute, EGLint *value)
{
    TRACE("(EGLDisplay dpy = %p, EGLContext ctx = %p, EGLint attribute = %d, EGLint *value = %p)",
          dpy, ctx, attribute, value);

    try
    {
        egl::Display *display = static_cast<egl::Display*>(dpy);
        egl::Context *context = static_cast<egl::Context*>(ctx);

        if(!validateContext(display, context))
        {
            return EGL_FALSE;
        }

        UNIMPLEMENTED();   // FIXME

        return egl::success(0);
    }
    catch(std::bad_alloc&)
    {
        return egl::error(EGL_BAD_ALLOC, EGL_FALSE);
    }

    return EGL_FALSE;
}

EGLBoolean EGLAPIENTRY eglWaitGL(void)
{
    TRACE("()");

    try
    {
        UNIMPLEMENTED();   // FIXME

        return egl::success(0);
    }
    catch(std::bad_alloc&)
    {
        return egl::error(EGL_BAD_ALLOC, EGL_FALSE);
    }

    return EGL_FALSE;
}

EGLBoolean EGLAPIENTRY eglWaitNative(EGLint engine)
{
    TRACE("(EGLint engine = %d)", engine);

    try
    {
        UNIMPLEMENTED();   // FIXME

        return egl::success(0);
    }
    catch(std::bad_alloc&)
    {
        return egl::error(EGL_BAD_ALLOC, EGL_FALSE);
    }

    return EGL_FALSE;
}

EGLBoolean EGLAPIENTRY eglSwapBuffers(EGLDisplay dpy, EGLSurface surface)
{
    TRACE("(EGLDisplay dpy = %p, EGLSurface surface = %p)", dpy, surface);

    try
    {
        egl::Display *display = static_cast<egl::Display*>(dpy);
        egl::Surface *eglSurface = (egl::Surface*)surface;

        if(!validateSurface(display, eglSurface))
        {
            return EGL_FALSE;
        }

        if(surface == EGL_NO_SURFACE)
        {
            return egl::error(EGL_BAD_SURFACE, EGL_FALSE);
        }

        eglSurface->swap();

        return egl::success(EGL_TRUE);
    }
    catch(std::bad_alloc&)
    {
        return egl::error(EGL_BAD_ALLOC, EGL_FALSE);
    }

    return EGL_FALSE;
}

EGLBoolean EGLAPIENTRY eglCopyBuffers(EGLDisplay dpy, EGLSurface surface, EGLNativePixmapType target)
{
    TRACE("(EGLDisplay dpy = %p, EGLSurface surface = %p, EGLNativePixmapType target = %p)", dpy, surface, target);

    try
    {
        egl::Display *display = static_cast<egl::Display*>(dpy);
        egl::Surface *eglSurface = static_cast<egl::Surface*>(surface);

        if(!validateSurface(display, eglSurface))
        {
            return EGL_FALSE;
        }

        UNIMPLEMENTED();   // FIXME

        return egl::success(0);
    }
    catch(std::bad_alloc&)
    {
        return egl::error(EGL_BAD_ALLOC, EGL_FALSE);
    }

    return EGL_FALSE;
}

__eglMustCastToProperFunctionPointerType EGLAPIENTRY eglGetProcAddress(const char *procname)
{
    TRACE("(const char *procname = \"%s\")", procname);

	RADPROC RADAPIENTRY radGetProcAddress(const RADchar *procname);
	return (__eglMustCastToProperFunctionPointerType)radGetProcAddress;
}
}
