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
// libRAD.cpp: Implements the exported OpenGL ES 2.0 functions.

#include "main.h"
#include "mathutil.h"
#include "utilities.h"
#include "Buffer.h"
#include "Context.h"
#include "Fence.h"
#include "Framebuffer.h"
#include "Program.h"
#include "Renderbuffer.h"
#include "Shader.h"
#include "Texture.h"
#include "Query.h"
#include "common/debug.h"
#include "Common/Version.h"
#include "Main/Register.hpp"

#define GL_APICALL
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <RAD/rad.h>

#include <exception>
#include <limits>

static bool validImageSize(GLint level, GLsizei width, GLsizei height)
{
    if(level < 0 || level >= rad::IMPLEMENTATION_MAX_TEXTURE_LEVELS || width < 0 || height < 0)
    {
        return false;
    }

    return true;
}

static bool validateSubImageParams(bool compressed, GLsizei width, GLsizei height, GLint xoffset, GLint yoffset, GLenum target, GLint level, GLenum format, rad::Texture *texture)
{
    if(!texture)
    {
        return error(GL_INVALID_OPERATION, false);
    }

    if(compressed != texture->isCompressed(target, level))
    {
        return error(GL_INVALID_OPERATION, false);
    }

    if(format != GL_NONE && format != texture->getFormat(target, level))
    {
        return error(GL_INVALID_OPERATION, false);
    }

    if(compressed)
    {
        if((width % 4 != 0 && width != texture->getWidth(target, 0)) || 
           (height % 4 != 0 && height != texture->getHeight(target, 0)))
        {
            return error(GL_INVALID_OPERATION, false);
        }
    }

    if(xoffset + width > texture->getWidth(target, level) ||
       yoffset + height > texture->getHeight(target, level))
    {
        return error(GL_INVALID_VALUE, false);
    }

    return true;
}

// Check for combinations of format and type that are valid for ReadPixels
static bool validReadFormatType(GLenum format, GLenum type)
{
    switch(format)
    {
    case GL_RGBA:
        switch (type)
        {
        case GL_UNSIGNED_BYTE:
            break;
        default:
            return false;
        }
        break;
    case GL_BGRA_EXT:
        switch (type)
        {
        case GL_UNSIGNED_BYTE:
        case GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT:
        case GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT:
            break;
        default:
            return false;
        }
        break;
    case rad::IMPLEMENTATION_COLOR_READ_FORMAT:
        switch (type)
        {
        case rad::IMPLEMENTATION_COLOR_READ_TYPE:
            break;
        default:
            return false;
        }
        break;
    default:
        return false;
    }

    return true;
}

extern "C"
{

void GL_APIENTRY glActiveTexture(GLenum texture)
{
    TRACE("(GLenum texture = 0x%X)", texture);

    try
    {
        rad::Context *context = rad::getContext();

        if(context)
        {
            if(texture < GL_TEXTURE0 || texture > GL_TEXTURE0 + rad::MAX_COMBINED_TEXTURE_IMAGE_UNITS - 1)
            {
                return error(GL_INVALID_ENUM);
            }

            context->setActiveSampler(texture - GL_TEXTURE0);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glAttachShader(GLuint program, GLuint shader)
{
    TRACE("(GLuint program = %d, GLuint shader = %d)", program, shader);

    try
    {
        rad::Context *context = rad::getContext();

        if(context)
        {
            rad::Program *programObject = context->getProgram(program);
            rad::Shader *shaderObject = context->getShader(shader);

            if(!programObject)
            {
                if(context->getShader(program))
                {
                    return error(GL_INVALID_OPERATION);
                }
                else
                {
                    return error(GL_INVALID_VALUE);
                }
            }

            if(!shaderObject)
            {
                if(context->getProgram(shader))
                {
                    return error(GL_INVALID_OPERATION);
                }
                else
                {
                    return error(GL_INVALID_VALUE);
                }
            }

            if(!programObject->attachShader(shaderObject))
            {
                return error(GL_INVALID_OPERATION);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glBeginQueryEXT(GLenum target, GLuint id)
{
    TRACE("(GLenum target = 0x%X, GLuint %d)", target, id);

    try
    {
        switch(target)
        {
        case GL_ANY_SAMPLES_PASSED_EXT: 
        case GL_ANY_SAMPLES_PASSED_CONSERVATIVE_EXT:
            break;
        default: 
            return error(GL_INVALID_ENUM);
        }

        if(id == 0)
        {
            return error(GL_INVALID_OPERATION);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            context->beginQuery(target, id);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glBindAttribLocation(GLuint program, GLuint index, const GLchar* name)
{
    TRACE("(GLuint program = %d, GLuint index = %d, const GLchar* name = 0x%0.8p)", program, index, name);

    try
    {
        if(index >= rad::MAX_VERTEX_ATTRIBS)
        {
            return error(GL_INVALID_VALUE);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            rad::Program *programObject = context->getProgram(program);

            if(!programObject)
            {
                if(context->getShader(program))
                {
                    return error(GL_INVALID_OPERATION);
                }
                else
                {
                    return error(GL_INVALID_VALUE);
                }
            }

            if(strncmp(name, "gl_", 3) == 0)
            {
                return error(GL_INVALID_OPERATION);
            }

            programObject->bindAttributeLocation(index, name);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glBindBuffer(GLenum target, GLuint buffer)
{
    TRACE("(GLenum target = 0x%X, GLuint buffer = %d)", target, buffer);

    try
    {
        rad::Context *context = rad::getContext();

        if(context)
        {
            switch(target)
            {
              case GL_ARRAY_BUFFER:
                context->bindArrayBuffer(buffer);
                return;
              case GL_ELEMENT_ARRAY_BUFFER:
                context->bindElementArrayBuffer(buffer);
                return;
              default:
                return error(GL_INVALID_ENUM);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glBindFramebuffer(GLenum target, GLuint framebuffer)
{
    TRACE("(GLenum target = 0x%X, GLuint framebuffer = %d)", target, framebuffer);

    try
    {
        if(target != GL_FRAMEBUFFER && target != GL_DRAW_FRAMEBUFFER_ANGLE && target != GL_READ_FRAMEBUFFER_ANGLE)
        {
            return error(GL_INVALID_ENUM);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            if(target == GL_READ_FRAMEBUFFER_ANGLE || target == GL_FRAMEBUFFER)
            {
                context->bindReadFramebuffer(framebuffer);
            }
            
            if(target == GL_DRAW_FRAMEBUFFER_ANGLE || target == GL_FRAMEBUFFER)
            {
                context->bindDrawFramebuffer(framebuffer);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glBindRenderbuffer(GLenum target, GLuint renderbuffer)
{
    TRACE("(GLenum target = 0x%X, GLuint renderbuffer = %d)", target, renderbuffer);

    try
    {
        if(target != GL_RENDERBUFFER)
        {
            return error(GL_INVALID_ENUM);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
			if (renderbuffer != 0 && !context->getRenderbuffer(renderbuffer))
			{
				// [OpenGL ES 2.0.25] Section 4.4.3 page 112
				// [OpenGL ES 3.0.2] Section 4.4.2 page 201
				// 'renderbuffer' must be either zero or the name of an existing renderbuffer object of
				// type 'renderbuffertarget', otherwise an INVALID_OPERATION error is generated.
				return error(GL_INVALID_OPERATION);
			}

            context->bindRenderbuffer(renderbuffer);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glBindTexture(GLenum target, GLuint texture)
{
    TRACE("(GLenum target = 0x%X, GLuint texture = %d)", target, texture);

    try
    {
        rad::Context *context = rad::getContext();

        if(context)
        {
            rad::Texture *textureObject = context->getTexture(texture);

            if(textureObject && textureObject->getTarget() != target && texture != 0)
            {
                return error(GL_INVALID_OPERATION);
            }

            switch(target)
            {
            case GL_TEXTURE_2D:
                context->bindTexture2D(texture);
                return;
            case GL_TEXTURE_CUBE_MAP:
                context->bindTextureCubeMap(texture);
                return;
            case GL_TEXTURE_EXTERNAL_OES:
                context->bindTextureExternal(texture);
                return;
            default:
                return error(GL_INVALID_ENUM);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glBlendColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
    TRACE("(GLclampf red = %f, GLclampf green = %f, GLclampf blue = %f, GLclampf alpha = %f)",
          red, green, blue, alpha);

    try
    {
        rad::Context* context = rad::getContext();

        if(context)
        {
            context->setBlendColor(rad::clamp01(red), rad::clamp01(green), rad::clamp01(blue), rad::clamp01(alpha));
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glBlendEquation(GLenum mode)
{
    glBlendEquationSeparate(mode, mode);
}

void GL_APIENTRY glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha)
{
    TRACE("(GLenum modeRGB = 0x%X, GLenum modeAlpha = 0x%X)", modeRGB, modeAlpha);

    try
    {
        switch(modeRGB)
        {
        case GL_FUNC_ADD:
        case GL_FUNC_SUBTRACT:
        case GL_FUNC_REVERSE_SUBTRACT:
        case GL_MIN_EXT:
        case GL_MAX_EXT:
            break;
        default:
            return error(GL_INVALID_ENUM);
        }

        switch(modeAlpha)
        {
        case GL_FUNC_ADD:
        case GL_FUNC_SUBTRACT:
        case GL_FUNC_REVERSE_SUBTRACT:
        case GL_MIN_EXT:
        case GL_MAX_EXT:
            break;
        default:
            return error(GL_INVALID_ENUM);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            context->setBlendEquation(modeRGB, modeAlpha);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glBlendFunc(GLenum sfactor, GLenum dfactor)
{
    glBlendFuncSeparate(sfactor, dfactor, sfactor, dfactor);
}

void GL_APIENTRY glBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha)
{
    TRACE("(GLenum srcRGB = 0x%X, GLenum dstRGB = 0x%X, GLenum srcAlpha = 0x%X, GLenum dstAlpha = 0x%X)",
          srcRGB, dstRGB, srcAlpha, dstAlpha);

    try
    {
        switch(srcRGB)
        {
          case GL_ZERO:
          case GL_ONE:
          case GL_SRC_COLOR:
          case GL_ONE_MINUS_SRC_COLOR:
          case GL_DST_COLOR:
          case GL_ONE_MINUS_DST_COLOR:
          case GL_SRC_ALPHA:
          case GL_ONE_MINUS_SRC_ALPHA:
          case GL_DST_ALPHA:
          case GL_ONE_MINUS_DST_ALPHA:
          case GL_CONSTANT_COLOR:
          case GL_ONE_MINUS_CONSTANT_COLOR:
          case GL_CONSTANT_ALPHA:
          case GL_ONE_MINUS_CONSTANT_ALPHA:
          case GL_SRC_ALPHA_SATURATE:
            break;
          default:
            return error(GL_INVALID_ENUM);
        }

        switch(dstRGB)
        {
          case GL_ZERO:
          case GL_ONE:
          case GL_SRC_COLOR:
          case GL_ONE_MINUS_SRC_COLOR:
          case GL_DST_COLOR:
          case GL_ONE_MINUS_DST_COLOR:
          case GL_SRC_ALPHA:
          case GL_ONE_MINUS_SRC_ALPHA:
          case GL_DST_ALPHA:
          case GL_ONE_MINUS_DST_ALPHA:
          case GL_CONSTANT_COLOR:
          case GL_ONE_MINUS_CONSTANT_COLOR:
          case GL_CONSTANT_ALPHA:
          case GL_ONE_MINUS_CONSTANT_ALPHA:
            break;
          default:
            return error(GL_INVALID_ENUM);
        }

        switch(srcAlpha)
        {
          case GL_ZERO:
          case GL_ONE:
          case GL_SRC_COLOR:
          case GL_ONE_MINUS_SRC_COLOR:
          case GL_DST_COLOR:
          case GL_ONE_MINUS_DST_COLOR:
          case GL_SRC_ALPHA:
          case GL_ONE_MINUS_SRC_ALPHA:
          case GL_DST_ALPHA:
          case GL_ONE_MINUS_DST_ALPHA:
          case GL_CONSTANT_COLOR:
          case GL_ONE_MINUS_CONSTANT_COLOR:
          case GL_CONSTANT_ALPHA:
          case GL_ONE_MINUS_CONSTANT_ALPHA:
          case GL_SRC_ALPHA_SATURATE:
            break;
          default:
            return error(GL_INVALID_ENUM);
        }

        switch(dstAlpha)
        {
          case GL_ZERO:
          case GL_ONE:
          case GL_SRC_COLOR:
          case GL_ONE_MINUS_SRC_COLOR:
          case GL_DST_COLOR:
          case GL_ONE_MINUS_DST_COLOR:
          case GL_SRC_ALPHA:
          case GL_ONE_MINUS_SRC_ALPHA:
          case GL_DST_ALPHA:
          case GL_ONE_MINUS_DST_ALPHA:
          case GL_CONSTANT_COLOR:
          case GL_ONE_MINUS_CONSTANT_COLOR:
          case GL_CONSTANT_ALPHA:
          case GL_ONE_MINUS_CONSTANT_ALPHA:
            break;
          default:
            return error(GL_INVALID_ENUM);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            context->setBlendFactors(srcRGB, dstRGB, srcAlpha, dstAlpha);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glBufferData(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage)
{
    TRACE("(GLenum target = 0x%X, GLsizeiptr size = %d, const GLvoid* data = 0x%0.8p, GLenum usage = %d)",
          target, size, data, usage);

    try
    {
        if(size < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        switch(usage)
        {
          case GL_STREAM_DRAW:
          case GL_STATIC_DRAW:
          case GL_DYNAMIC_DRAW:
            break;
          default:
            return error(GL_INVALID_ENUM);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            rad::Buffer *buffer;

            switch(target)
            {
              case GL_ARRAY_BUFFER:
                buffer = context->getArrayBuffer();
                break;
              case GL_ELEMENT_ARRAY_BUFFER:
                buffer = context->getElementArrayBuffer();
                break;
              default:
                return error(GL_INVALID_ENUM);
            }

            if(!buffer)
            {
                return error(GL_INVALID_OPERATION);
            }

            buffer->bufferData(data, size, usage);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid* data)
{
    TRACE("(GLenum target = 0x%X, GLintptr offset = %d, GLsizeiptr size = %d, const GLvoid* data = 0x%0.8p)",
          target, offset, size, data);

    try
    {
        if(size < 0 || offset < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        if(data == NULL)
        {
            return;
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            rad::Buffer *buffer;

            switch(target)
            {
              case GL_ARRAY_BUFFER:
                buffer = context->getArrayBuffer();
                break;
              case GL_ELEMENT_ARRAY_BUFFER:
                buffer = context->getElementArrayBuffer();
                break;
              default:
                return error(GL_INVALID_ENUM);
            }

            if(!buffer)
            {
                return error(GL_INVALID_OPERATION);
            }

            if((size_t)size + offset > buffer->size())
            {
                return error(GL_INVALID_VALUE);
            }

            buffer->bufferSubData(data, size, offset);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

GLenum GL_APIENTRY glCheckFramebufferStatus(GLenum target)
{
    TRACE("(GLenum target = 0x%X)", target);

    try
    {
        if(target != GL_FRAMEBUFFER && target != GL_DRAW_FRAMEBUFFER_ANGLE && target != GL_READ_FRAMEBUFFER_ANGLE)
        {
            return error(GL_INVALID_ENUM, 0);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            rad::Framebuffer *framebuffer = NULL;
            if(target == GL_READ_FRAMEBUFFER_ANGLE)
            {
                framebuffer = context->getReadFramebuffer();
            }
            else
            {
                framebuffer = context->getDrawFramebuffer();
            }

            return framebuffer->completeness();
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY, 0);
    }

    return 0;
}

void GL_APIENTRY glClear(GLbitfield mask)
{
    TRACE("(GLbitfield mask = %X)", mask);

    try
    {
		if((mask & ~(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT)) != 0)
		{
			return error(GL_INVALID_VALUE);
		}

        rad::Context *context = rad::getContext();

        if(context)
        {
            context->clear(mask);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
    TRACE("(GLclampf red = %f, GLclampf green = %f, GLclampf blue = %f, GLclampf alpha = %f)",
          red, green, blue, alpha);

    try
    {
        rad::Context *context = rad::getContext();

        if(context)
        {
            context->setClearColor(red, green, blue, alpha);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glClearDepthf(GLclampf depth)
{
    TRACE("(GLclampf depth = %f)", depth);

    try
    {
        rad::Context *context = rad::getContext();

        if(context)
        {
            context->setClearDepth(depth);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glClearStencil(GLint s)
{
    TRACE("(GLint s = %d)", s);

    try
    {
        rad::Context *context = rad::getContext();

        if(context)
        {
            context->setClearStencil(s);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
    TRACE("(GLboolean red = %d, GLboolean green = %d, GLboolean blue = %d, GLboolean alpha = %d)",
          red, green, blue, alpha);

    try
    {
        rad::Context *context = rad::getContext();

        if(context)
        {
            context->setColorMask(red == GL_TRUE, green == GL_TRUE, blue == GL_TRUE, alpha == GL_TRUE);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glCompileShader(GLuint shader)
{
    TRACE("(GLuint shader = %d)", shader);

    try
    {
        rad::Context *context = rad::getContext();

        if(context)
        {
            rad::Shader *shaderObject = context->getShader(shader);

            if(!shaderObject)
            {
                if(context->getProgram(shader))
                {
                    return error(GL_INVALID_OPERATION);
                }
                else
                {
                    return error(GL_INVALID_VALUE);
                }
            }

            shaderObject->compile();
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, 
                                        GLint border, GLsizei imageSize, const GLvoid* data)
{
    TRACE("(GLenum target = 0x%X, GLint level = %d, GLenum internalformat = 0x%X, GLsizei width = %d, " 
          "GLsizei height = %d, GLint border = %d, GLsizei imageSize = %d, const GLvoid* data = 0x%0.8p)",
          target, level, internalformat, width, height, border, imageSize, data);

    try
    {
        if(!validImageSize(level, width, height) || border != 0 || imageSize < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        switch(internalformat)
        {
        case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
		case GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE:
		case GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE:
			if(!S3TC_SUPPORT)
            {
                return error(GL_INVALID_ENUM);
            }
            break;
		case GL_DEPTH_COMPONENT:
		case GL_DEPTH_COMPONENT16:
		case GL_DEPTH_COMPONENT32_OES:
		case GL_DEPTH_STENCIL_OES:
		case GL_DEPTH24_STENCIL8_OES:
			return error(GL_INVALID_OPERATION);
        default:
            return error(GL_INVALID_ENUM);
        }

        if(border != 0)
        {
            return error(GL_INVALID_VALUE);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
			if(level > rad::IMPLEMENTATION_MAX_TEXTURE_LEVELS)
            {
                return error(GL_INVALID_VALUE);
            }

            switch(target)
            {
              case GL_TEXTURE_2D:
                if(width > (rad::IMPLEMENTATION_MAX_TEXTURE_SIZE >> level) ||
                    height > (rad::IMPLEMENTATION_MAX_TEXTURE_SIZE >> level))
                {
                    return error(GL_INVALID_VALUE);
                }
                break;
              case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
              case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
              case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
              case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
              case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
              case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
                if(width != height)
                {
                    return error(GL_INVALID_VALUE);
                }

                if(width > (rad::IMPLEMENTATION_MAX_CUBE_MAP_TEXTURE_SIZE >> level) ||
                   height > (rad::IMPLEMENTATION_MAX_CUBE_MAP_TEXTURE_SIZE >> level))
                {
                    return error(GL_INVALID_VALUE);
                }
                break;
              default:
                return error(GL_INVALID_ENUM);
            }

            if(imageSize != rad::ComputeCompressedSize(width, height, internalformat))
            {
                return error(GL_INVALID_VALUE);
            }

            if(target == GL_TEXTURE_2D)
            {
                rad::Texture2D *texture = context->getTexture2D();

                if(!texture)
                {
                    return error(GL_INVALID_OPERATION);
                }

                texture->setCompressedImage(level, internalformat, width, height, imageSize, data);
            }
            else
            {
                rad::TextureCubeMap *texture = context->getTextureCubeMap();

                if(!texture)
                {
                    return error(GL_INVALID_OPERATION);
                }

                switch(target)
                {
                  case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
                  case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
                  case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
                  case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
                  case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
                  case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
                    texture->setCompressedImage(target, level, internalformat, width, height, imageSize, data);
                    break;
                  default: UNREACHABLE();
                }
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glCompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                                         GLenum format, GLsizei imageSize, const GLvoid* data)
{
    TRACE("(GLenum target = 0x%X, GLint level = %d, GLint xoffset = %d, GLint yoffset = %d, "
          "GLsizei width = %d, GLsizei height = %d, GLenum format = 0x%X, "
          "GLsizei imageSize = %d, const GLvoid* data = 0x%0.8p)",
          target, level, xoffset, yoffset, width, height, format, imageSize, data);

    try
    {
        if(!rad::IsTextureTarget(target))
        {
            return error(GL_INVALID_ENUM);
        }

        if(xoffset < 0 || yoffset < 0 || !validImageSize(level, width, height) || imageSize < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        switch(format)
        {
        case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
		case GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE:
		case GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE:
			if(!S3TC_SUPPORT)
            {
                return error(GL_INVALID_ENUM);
            }
            break;
        default:
            return error(GL_INVALID_ENUM);
        }

        if(width == 0 || height == 0 || data == NULL)
        {
            return;
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            if(level > rad::IMPLEMENTATION_MAX_TEXTURE_LEVELS)
            {
                return error(GL_INVALID_VALUE);
            }

            if(imageSize != rad::ComputeCompressedSize(width, height, format))
            {
                return error(GL_INVALID_VALUE);
            }

            if(xoffset % 4 != 0 || yoffset % 4 != 0)
            {
				// We wait to check the offsets until this point, because the multiple-of-four restriction does not exist unless DXT1 textures are supported
                return error(GL_INVALID_OPERATION);
            }

            if(target == GL_TEXTURE_2D)
            {
                rad::Texture2D *texture = context->getTexture2D();

                if(validateSubImageParams(true, width, height, xoffset, yoffset, target, level, format, texture))
				{
					texture->subImageCompressed(level, xoffset, yoffset, width, height, format, imageSize, data);
				}
            }
            else if(rad::IsCubemapTextureTarget(target))
            {
                rad::TextureCubeMap *texture = context->getTextureCubeMap();

                if(validateSubImageParams(true, width, height, xoffset, yoffset, target, level, format, texture))
				{
					texture->subImageCompressed(target, level, xoffset, yoffset, width, height, format, imageSize, data);
				}
            }
            else
            {
                UNREACHABLE();
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
    TRACE("(GLenum target = 0x%X, GLint level = %d, GLenum internalformat = 0x%X, "
          "GLint x = %d, GLint y = %d, GLsizei width = %d, GLsizei height = %d, GLint border = %d)",
          target, level, internalformat, x, y, width, height, border);

    try
    {
        if(!validImageSize(level, width, height))
        {
            return error(GL_INVALID_VALUE);
        }

        if(border != 0)
        {
            return error(GL_INVALID_VALUE);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            switch(target)
            {
              case GL_TEXTURE_2D:
                if(width > (rad::IMPLEMENTATION_MAX_TEXTURE_SIZE >> level) ||
                   height > (rad::IMPLEMENTATION_MAX_TEXTURE_SIZE >> level))
                {
                    return error(GL_INVALID_VALUE);
                }
                break;
              case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
              case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
              case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
              case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
              case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
              case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
                if(width != height)
                {
                    return error(GL_INVALID_VALUE);
                }

                if(width > (rad::IMPLEMENTATION_MAX_CUBE_MAP_TEXTURE_SIZE >> level) ||
                   height > (rad::IMPLEMENTATION_MAX_CUBE_MAP_TEXTURE_SIZE >> level))
                {
                    return error(GL_INVALID_VALUE);
                }
                break;
              default:
                return error(GL_INVALID_ENUM);
            }

            rad::Framebuffer *framebuffer = context->getReadFramebuffer();

            if(framebuffer->completeness() != GL_FRAMEBUFFER_COMPLETE)
            {
                return error(GL_INVALID_FRAMEBUFFER_OPERATION);
            }

            if(context->getReadFramebufferHandle() != 0 && framebuffer->getColorbuffer()->getSamples() > 1)
            {
                return error(GL_INVALID_OPERATION);
            }

            rad::Renderbuffer *source = framebuffer->getColorbuffer();
            GLenum colorbufferFormat = source->getFormat();

            // [OpenGL ES 2.0.24] table 3.9
            switch(internalformat)
            {
            case GL_ALPHA:
                if(colorbufferFormat != GL_ALPHA &&
                   colorbufferFormat != GL_RGBA &&
                   colorbufferFormat != GL_RGBA4 &&
                   colorbufferFormat != GL_RGB5_A1 &&
                   colorbufferFormat != GL_RGBA8_OES)
                {
                    return error(GL_INVALID_OPERATION);
                }
                break;
            case GL_LUMINANCE:
            case GL_RGB:
                if(colorbufferFormat != GL_RGB &&
                   colorbufferFormat != GL_RGB565 &&
                   colorbufferFormat != GL_RGB8_OES &&
                   colorbufferFormat != GL_RGBA &&
                   colorbufferFormat != GL_RGBA4 &&
                   colorbufferFormat != GL_RGB5_A1 &&
                   colorbufferFormat != GL_RGBA8_OES)
                {
                    return error(GL_INVALID_OPERATION);
                }
                break;
            case GL_LUMINANCE_ALPHA:
            case GL_RGBA:
                if(colorbufferFormat != GL_RGBA &&
                   colorbufferFormat != GL_RGBA4 &&
                   colorbufferFormat != GL_RGB5_A1 &&
                   colorbufferFormat != GL_RGBA8_OES)
                 {
                     return error(GL_INVALID_OPERATION);
                 }
                 break;
            case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
            case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
			case GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE:
			case GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE:
                if(S3TC_SUPPORT)
                {
                    return error(GL_INVALID_OPERATION);
                }
                else
                {
                    return error(GL_INVALID_ENUM);
                }
                break;
            default:
                return error(GL_INVALID_ENUM);
            }

            if(target == GL_TEXTURE_2D)
            {
                rad::Texture2D *texture = context->getTexture2D();

                if(!texture)
                {
                    return error(GL_INVALID_OPERATION);
                }

                texture->copyImage(level, internalformat, x, y, width, height, framebuffer);
            }
            else if(rad::IsCubemapTextureTarget(target))
            {
                rad::TextureCubeMap *texture = context->getTextureCubeMap();

                if(!texture)
                {
                    return error(GL_INVALID_OPERATION);
                }

                texture->copyImage(target, level, internalformat, x, y, width, height, framebuffer);
            }
            else UNREACHABLE();
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
    TRACE("(GLenum target = 0x%X, GLint level = %d, GLint xoffset = %d, GLint yoffset = %d, "
          "GLint x = %d, GLint y = %d, GLsizei width = %d, GLsizei height = %d)",
          target, level, xoffset, yoffset, x, y, width, height);

    try
    {
        if(!rad::IsTextureTarget(target))
        {
            return error(GL_INVALID_ENUM);
        }

        if(level < 0 || xoffset < 0 || yoffset < 0 || width < 0 || height < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        if(std::numeric_limits<GLsizei>::max() - xoffset < width || std::numeric_limits<GLsizei>::max() - yoffset < height)
        {
            return error(GL_INVALID_VALUE);
        }

        if(width == 0 || height == 0)
        {
            return;
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            if(level > rad::IMPLEMENTATION_MAX_TEXTURE_LEVELS)
            {
                return error(GL_INVALID_VALUE);
            }

            rad::Framebuffer *framebuffer = context->getReadFramebuffer();

            if(framebuffer->completeness() != GL_FRAMEBUFFER_COMPLETE)
            {
                return error(GL_INVALID_FRAMEBUFFER_OPERATION);
            }

            if(context->getReadFramebufferHandle() != 0 && framebuffer->getColorbuffer()->getSamples() > 1)
            {
                return error(GL_INVALID_OPERATION);
            }

            rad::Renderbuffer *source = framebuffer->getColorbuffer();
            GLenum colorbufferFormat = source->getFormat();
            rad::Texture *texture = NULL;

            if(target == GL_TEXTURE_2D)
            {
                texture = context->getTexture2D();
            }
            else if(rad::IsCubemapTextureTarget(target))
            {
                texture = context->getTextureCubeMap();
            }
            else UNREACHABLE();

            if(!validateSubImageParams(false, width, height, xoffset, yoffset, target, level, GL_NONE, texture))
			{
				return;
			}

            GLenum textureFormat = texture->getFormat(target, level);

            // [OpenGL ES 2.0.24] table 3.9
            switch(textureFormat)
            {
            case GL_ALPHA:
                if(colorbufferFormat != GL_ALPHA &&
                   colorbufferFormat != GL_RGBA &&
                   colorbufferFormat != GL_RGBA4 &&
                   colorbufferFormat != GL_RGB5_A1 &&
                   colorbufferFormat != GL_RGBA8_OES)
                {
                    return error(GL_INVALID_OPERATION);
                }
                break;
            case GL_LUMINANCE:
            case GL_RGB:
                if(colorbufferFormat != GL_RGB &&
                   colorbufferFormat != GL_RGB565 &&
                   colorbufferFormat != GL_RGB8_OES &&
                   colorbufferFormat != GL_RGBA &&
                   colorbufferFormat != GL_RGBA4 &&
                   colorbufferFormat != GL_RGB5_A1 &&
                   colorbufferFormat != GL_RGBA8_OES)
                {
                    return error(GL_INVALID_OPERATION);
                }
                break;
            case GL_LUMINANCE_ALPHA:
            case GL_RGBA:
                if(colorbufferFormat != GL_RGBA &&
                   colorbufferFormat != GL_RGBA4 &&
                   colorbufferFormat != GL_RGB5_A1 &&
                   colorbufferFormat != GL_RGBA8_OES)
                {
                    return error(GL_INVALID_OPERATION);
                }
                break;
            case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
            case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
			case GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE:
			case GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE:
                return error(GL_INVALID_OPERATION);
			case GL_DEPTH_COMPONENT:
			case GL_DEPTH_STENCIL_OES:
				return error(GL_INVALID_OPERATION);
            default:
                return error(GL_INVALID_OPERATION);
            }

            texture->copySubImage(target, level, xoffset, yoffset, x, y, width, height, framebuffer);
        }
    }

    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

GLuint GL_APIENTRY glCreateProgram(void)
{
    TRACE("()");

    try
    {
        rad::Context *context = rad::getContext();

        if(context)
        {
            return context->createProgram();
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY, 0);
    }

    return 0;
}

GLuint GL_APIENTRY glCreateShader(GLenum type)
{
    TRACE("(GLenum type = 0x%X)", type);

    try
    {
        rad::Context *context = rad::getContext();

        if(context)
        {
            switch(type)
            {
              case GL_FRAGMENT_SHADER:
              case GL_VERTEX_SHADER:
                return context->createShader(type);
              default:
                return error(GL_INVALID_ENUM, 0);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY, 0);
    }

    return 0;
}

void GL_APIENTRY glCullFace(GLenum mode)
{
    TRACE("(GLenum mode = 0x%X)", mode);

    try
    {
        switch(mode)
        {
          case GL_FRONT:
          case GL_BACK:
          case GL_FRONT_AND_BACK:
            {
                rad::Context *context = rad::getContext();

                if(context)
                {
                    context->setCullMode(mode);
                }
            }
            break;
          default:
            return error(GL_INVALID_ENUM);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glDeleteBuffers(GLsizei n, const GLuint* buffers)
{
    TRACE("(GLsizei n = %d, const GLuint* buffers = 0x%0.8p)", n, buffers);

    try
    {
        if(n < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            for(int i = 0; i < n; i++)
            {
                context->deleteBuffer(buffers[i]);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glDeleteFencesNV(GLsizei n, const GLuint* fences)
{
    TRACE("(GLsizei n = %d, const GLuint* fences = 0x%0.8p)", n, fences);

    try
    {
        if(n < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            for(int i = 0; i < n; i++)
            {
                context->deleteFence(fences[i]);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glDeleteFramebuffers(GLsizei n, const GLuint* framebuffers)
{
    TRACE("(GLsizei n = %d, const GLuint* framebuffers = 0x%0.8p)", n, framebuffers);

    try
    {
        if(n < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            for(int i = 0; i < n; i++)
            {
                if(framebuffers[i] != 0)
                {
                    context->deleteFramebuffer(framebuffers[i]);
                }
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glDeleteProgram(GLuint program)
{
    TRACE("(GLuint program = %d)", program);

    try
    {
        if(program == 0)
        {
            return;
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            if(!context->getProgram(program))
            {
                if(context->getShader(program))
                {
                    return error(GL_INVALID_OPERATION);
                }
                else
                {
                    return error(GL_INVALID_VALUE);
                }
            }

            context->deleteProgram(program);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glDeleteQueriesEXT(GLsizei n, const GLuint *ids)
{
    TRACE("(GLsizei n = %d, const GLuint *ids = 0x%0.8p)", n, ids);

    try
    {
        if(n < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            for(int i = 0; i < n; i++)
            {
                context->deleteQuery(ids[i]);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glDeleteRenderbuffers(GLsizei n, const GLuint* renderbuffers)
{
    TRACE("(GLsizei n = %d, const GLuint* renderbuffers = 0x%0.8p)", n, renderbuffers);

    try
    {
        if(n < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            for(int i = 0; i < n; i++)
            {
                context->deleteRenderbuffer(renderbuffers[i]);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glDeleteShader(GLuint shader)
{
    TRACE("(GLuint shader = %d)", shader);

    try
    {
        if(shader == 0)
        {
            return;
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            if(!context->getShader(shader))
            {
                if(context->getProgram(shader))
                {
                    return error(GL_INVALID_OPERATION);
                }
                else
                {
                    return error(GL_INVALID_VALUE);
                }
            }

            context->deleteShader(shader);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glDeleteTextures(GLsizei n, const GLuint* textures)
{
    TRACE("(GLsizei n = %d, const GLuint* textures = 0x%0.8p)", n, textures);

    try
    {
        if(n < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            for(int i = 0; i < n; i++)
            {
                if(textures[i] != 0)
                {
                    context->deleteTexture(textures[i]);
                }
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glDepthFunc(GLenum func)
{
    TRACE("(GLenum func = 0x%X)", func);

    try
    {
        switch(func)
        {
          case GL_NEVER:
          case GL_ALWAYS:
          case GL_LESS:
          case GL_LEQUAL:
          case GL_EQUAL:
          case GL_GREATER:
          case GL_GEQUAL:
          case GL_NOTEQUAL:
            break;
          default:
            return error(GL_INVALID_ENUM);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            context->setDepthFunc(func);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glDepthMask(GLboolean flag)
{
    TRACE("(GLboolean flag = %d)", flag);

    try
    {
        rad::Context *context = rad::getContext();

        if(context)
        {
            context->setDepthMask(flag != GL_FALSE);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glDepthRangef(GLclampf zNear, GLclampf zFar)
{
    TRACE("(GLclampf zNear = %f, GLclampf zFar = %f)", zNear, zFar);

    try
    {
        rad::Context *context = rad::getContext();

        if(context)
        {
            context->setDepthRange(zNear, zFar);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glDetachShader(GLuint program, GLuint shader)
{
    TRACE("(GLuint program = %d, GLuint shader = %d)", program, shader);

    try
    {
        rad::Context *context = rad::getContext();

        if(context)
        {

            rad::Program *programObject = context->getProgram(program);
            rad::Shader *shaderObject = context->getShader(shader);
            
            if(!programObject)
            {
                rad::Shader *shaderByProgramHandle;
                shaderByProgramHandle = context->getShader(program);
                if(!shaderByProgramHandle)
                {
                    return error(GL_INVALID_VALUE);
                }
                else
                {
                    return error(GL_INVALID_OPERATION);
                }
            }

            if(!shaderObject)
            {
                rad::Program *programByShaderHandle = context->getProgram(shader);
                if(!programByShaderHandle)
                {
                    return error(GL_INVALID_VALUE);
                }
                else
                {
                    return error(GL_INVALID_OPERATION);
                }
            }

            if(!programObject->detachShader(shaderObject))
            {
                return error(GL_INVALID_OPERATION);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glDisable(GLenum cap)
{
    TRACE("(GLenum cap = 0x%X)", cap);

    try
    {
        rad::Context *context = rad::getContext();

        if(context)
        {
            switch(cap)
            {
              case GL_CULL_FACE:                context->setCullFace(false);              break;
              case GL_POLYGON_OFFSET_FILL:      context->setPolygonOffsetFill(false);     break;
              case GL_SAMPLE_ALPHA_TO_COVERAGE: context->setSampleAlphaToCoverage(false); break;
              case GL_SAMPLE_COVERAGE:          context->setSampleCoverage(false);        break;
              case GL_SCISSOR_TEST:             context->setScissorTest(false);           break;
              case GL_STENCIL_TEST:             context->setStencilTest(false);           break;
              case GL_DEPTH_TEST:               context->setDepthTest(false);             break;
              case GL_BLEND:                    context->setBlend(false);                 break;
              case GL_DITHER:                   context->setDither(false);                break;
              default:
                return error(GL_INVALID_ENUM);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glDisableVertexAttribArray(GLuint index)
{
    TRACE("(GLuint index = %d)", index);

    try
    {
        if(index >= rad::MAX_VERTEX_ATTRIBS)
        {
            return error(GL_INVALID_VALUE);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            context->setEnableVertexAttribArray(index, false);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glDrawArrays(GLenum mode, GLint first, GLsizei count)
{
    TRACE("(GLenum mode = 0x%X, GLint first = %d, GLsizei count = %d)", mode, first, count);

    try
    {
        if(count < 0 || first < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            context->drawArrays(mode, first, count);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices)
{
    TRACE("(GLenum mode = 0x%X, GLsizei count = %d, GLenum type = 0x%X, const GLvoid* indices = 0x%0.8p)",
          mode, count, type, indices);

    try
    {
        if(count < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            switch(type)
            {
              case GL_UNSIGNED_BYTE:
              case GL_UNSIGNED_SHORT:
              case GL_UNSIGNED_INT:
                break;
              default:
                return error(GL_INVALID_ENUM);
            }
        
            context->drawElements(mode, count, type, indices);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glEnable(GLenum cap)
{
    TRACE("(GLenum cap = 0x%X)", cap);

    try
    {
        rad::Context *context = rad::getContext();

        if(context)
        {
            switch(cap)
            {
              case GL_CULL_FACE:                context->setCullFace(true);              break;
              case GL_POLYGON_OFFSET_FILL:      context->setPolygonOffsetFill(true);     break;
              case GL_SAMPLE_ALPHA_TO_COVERAGE: context->setSampleAlphaToCoverage(true); break;
              case GL_SAMPLE_COVERAGE:          context->setSampleCoverage(true);        break;
              case GL_SCISSOR_TEST:             context->setScissorTest(true);           break;
              case GL_STENCIL_TEST:             context->setStencilTest(true);           break;
              case GL_DEPTH_TEST:               context->setDepthTest(true);             break;
              case GL_BLEND:                    context->setBlend(true);                 break;
              case GL_DITHER:                   context->setDither(true);                break;
              default:
                return error(GL_INVALID_ENUM);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glEnableVertexAttribArray(GLuint index)
{
    TRACE("(GLuint index = %d)", index);

    try
    {
        if(index >= rad::MAX_VERTEX_ATTRIBS)
        {
            return error(GL_INVALID_VALUE);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            context->setEnableVertexAttribArray(index, true);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glEndQueryEXT(GLenum target)
{
    TRACE("GLenum target = 0x%X)", target);

    try
    {
        switch(target)
        {
        case GL_ANY_SAMPLES_PASSED_EXT: 
        case GL_ANY_SAMPLES_PASSED_CONSERVATIVE_EXT:
            break;
        default: 
            return error(GL_INVALID_ENUM);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            context->endQuery(target);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glFinishFenceNV(GLuint fence)
{
    TRACE("(GLuint fence = %d)", fence);

    try
    {
        rad::Context *context = rad::getContext();

        if(context)
        {
            rad::Fence* fenceObject = context->getFence(fence);

            if(fenceObject == NULL)
            {
                return error(GL_INVALID_OPERATION);
            }

            fenceObject->finishFence();
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glFinish(void)
{
    TRACE("()");

    try
    {
        rad::Context *context = rad::getContext();

        if(context)
        {
            context->finish();
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glFlush(void)
{
    TRACE("()");

    try
    {
        rad::Context *context = rad::getContext();

        if(context)
        {
            context->flush();
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)
{
    TRACE("(GLenum target = 0x%X, GLenum attachment = 0x%X, GLenum renderbuffertarget = 0x%X, "
          "GLuint renderbuffer = %d)", target, attachment, renderbuffertarget, renderbuffer);

    try
    {
        if((target != GL_FRAMEBUFFER && target != GL_DRAW_FRAMEBUFFER_ANGLE && target != GL_READ_FRAMEBUFFER_ANGLE)
            || (renderbuffertarget != GL_RENDERBUFFER && renderbuffer != 0))
        {
            return error(GL_INVALID_ENUM);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            rad::Framebuffer *framebuffer = NULL;
            GLuint framebufferHandle = 0;
            if(target == GL_READ_FRAMEBUFFER_ANGLE)
            {
                framebuffer = context->getReadFramebuffer();
                framebufferHandle = context->getReadFramebufferHandle();
            }
            else
            {
                framebuffer = context->getDrawFramebuffer();
                framebufferHandle = context->getDrawFramebufferHandle();
            }

            if(!framebuffer || (framebufferHandle == 0 && renderbuffer != 0))
            {
                return error(GL_INVALID_OPERATION);
            }

            switch(attachment)
            {
              case GL_COLOR_ATTACHMENT0:
                framebuffer->setColorbuffer(GL_RENDERBUFFER, renderbuffer);
                break;
              case GL_DEPTH_ATTACHMENT:
                framebuffer->setDepthbuffer(GL_RENDERBUFFER, renderbuffer);
                break;
              case GL_STENCIL_ATTACHMENT:
                framebuffer->setStencilbuffer(GL_RENDERBUFFER, renderbuffer);
                break;
              default:
                return error(GL_INVALID_ENUM);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{
    TRACE("(GLenum target = 0x%X, GLenum attachment = 0x%X, GLenum textarget = 0x%X, "
          "GLuint texture = %d, GLint level = %d)", target, attachment, textarget, texture, level);

    try
    {
        if(target != GL_FRAMEBUFFER && target != GL_DRAW_FRAMEBUFFER_ANGLE && target != GL_READ_FRAMEBUFFER_ANGLE)
        {
            return error(GL_INVALID_ENUM);
        }

        switch(attachment)
        {
          case GL_COLOR_ATTACHMENT0:
          case GL_DEPTH_ATTACHMENT:
          case GL_STENCIL_ATTACHMENT:
            break;
          default:
            return error(GL_INVALID_ENUM);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            if(texture == 0)
            {
                textarget = GL_NONE;
            }
            else
            {
                rad::Texture *tex = context->getTexture(texture);

                if(tex == NULL)
                {
                    return error(GL_INVALID_OPERATION);
                }

                if(tex->isCompressed(textarget, level))
                {
                    return error(GL_INVALID_OPERATION);
                }

                switch(textarget)
                {
                  case GL_TEXTURE_2D:
                    if(tex->getTarget() != GL_TEXTURE_2D)
                    {
                        return error(GL_INVALID_OPERATION);
                    }
                    break;

                  case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
                  case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
                  case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
                  case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
                  case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
                  case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
                    if(tex->getTarget() != GL_TEXTURE_CUBE_MAP)
                    {
                        return error(GL_INVALID_OPERATION);
                    }
                    break;

                  default:
                    return error(GL_INVALID_ENUM);
                }

                if(level != 0)
                {
                    return error(GL_INVALID_VALUE);
                }
            }

            rad::Framebuffer *framebuffer = NULL;
            GLuint framebufferHandle = 0;
            if(target == GL_READ_FRAMEBUFFER_ANGLE)
            {
                framebuffer = context->getReadFramebuffer();
                framebufferHandle = context->getReadFramebufferHandle();
            }
            else
            {
                framebuffer = context->getDrawFramebuffer();
                framebufferHandle = context->getDrawFramebufferHandle();
            }

            if(framebufferHandle == 0 || !framebuffer)
            {
                return error(GL_INVALID_OPERATION);
            }

            switch(attachment)
            {
              case GL_COLOR_ATTACHMENT0:  framebuffer->setColorbuffer(textarget, texture);   break;
              case GL_DEPTH_ATTACHMENT:   framebuffer->setDepthbuffer(textarget, texture);   break;
              case GL_STENCIL_ATTACHMENT: framebuffer->setStencilbuffer(textarget, texture); break;
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glFrontFace(GLenum mode)
{
    TRACE("(GLenum mode = 0x%X)", mode);

    try
    {
        switch(mode)
        {
          case GL_CW:
          case GL_CCW:
            {
                rad::Context *context = rad::getContext();

                if(context)
                {
                    context->setFrontFace(mode);
                }
            }
            break;
          default:
            return error(GL_INVALID_ENUM);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glGenBuffers(GLsizei n, GLuint* buffers)
{
    TRACE("(GLsizei n = %d, GLuint* buffers = 0x%0.8p)", n, buffers);

    try
    {
        if(n < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            for(int i = 0; i < n; i++)
            {
                buffers[i] = context->createBuffer();
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glGenerateMipmap(GLenum target)
{
    TRACE("(GLenum target = 0x%X)", target);

    try
    {
        rad::Context *context = rad::getContext();

        if(context)
        {
            rad::Texture *texture;

            switch(target)
            {
              case GL_TEXTURE_2D:
                texture = context->getTexture2D();
                break;
              case GL_TEXTURE_CUBE_MAP:
                texture = context->getTextureCubeMap();
                break;
              default:
                return error(GL_INVALID_ENUM);
            }

            if(texture->isCompressed(target, 0) || texture->isDepth(target, 0))
            {
                return error(GL_INVALID_OPERATION);
            }

            texture->generateMipmaps();
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glGenFencesNV(GLsizei n, GLuint* fences)
{
    TRACE("(GLsizei n = %d, GLuint* fences = 0x%0.8p)", n, fences);

    try
    {
        if(n < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            for(int i = 0; i < n; i++)
            {
                fences[i] = context->createFence();
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glGenFramebuffers(GLsizei n, GLuint* framebuffers)
{
    TRACE("(GLsizei n = %d, GLuint* framebuffers = 0x%0.8p)", n, framebuffers);

    try
    {
        if(n < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            for(int i = 0; i < n; i++)
            {
                framebuffers[i] = context->createFramebuffer();
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glGenQueriesEXT(GLsizei n, GLuint* ids)
{
    TRACE("(GLsizei n = %d, GLuint* ids = 0x%0.8p)", n, ids);

    try
    {
        if(n < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            for(int i = 0; i < n; i++)
            {
                ids[i] = context->createQuery();
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glGenRenderbuffers(GLsizei n, GLuint* renderbuffers)
{
    TRACE("(GLsizei n = %d, GLuint* renderbuffers = 0x%0.8p)", n, renderbuffers);

    try
    {
        if(n < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            for(int i = 0; i < n; i++)
            {
                renderbuffers[i] = context->createRenderbuffer();
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glGenTextures(GLsizei n, GLuint* textures)
{
    TRACE("(GLsizei n = %d, GLuint* textures =  0x%0.8p)", n, textures);

    try
    {
        if(n < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            for(int i = 0; i < n; i++)
            {
                textures[i] = context->createTexture();
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glGetActiveAttrib(GLuint program, GLuint index, GLsizei bufsize, GLsizei *length, GLint *size, GLenum *type, GLchar *name)
{
    TRACE("(GLuint program = %d, GLuint index = %d, GLsizei bufsize = %d, GLsizei *length = 0x%0.8p, "
          "GLint *size = 0x%0.8p, GLenum *type = %0.8p, GLchar *name = %0.8p)",
          program, index, bufsize, length, size, type, name);

    try
    {
        if(bufsize < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            rad::Program *programObject = context->getProgram(program);

            if(!programObject)
            {
                if(context->getShader(program))
                {
                    return error(GL_INVALID_OPERATION);
                }
                else
                {
                    return error(GL_INVALID_VALUE);
                }
            }

            if(index >= (GLuint)programObject->getActiveAttributeCount())
            {
                return error(GL_INVALID_VALUE);
            }

            programObject->getActiveAttribute(index, bufsize, length, size, type, name);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glGetActiveUniform(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, GLchar* name)
{
    TRACE("(GLuint program = %d, GLuint index = %d, GLsizei bufsize = %d, "
          "GLsizei* length = 0x%0.8p, GLint* size = 0x%0.8p, GLenum* type = 0x%0.8p, GLchar* name = 0x%0.8p)",
          program, index, bufsize, length, size, type, name);

    try
    {
        if(bufsize < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            rad::Program *programObject = context->getProgram(program);

            if(!programObject)
            {
                if(context->getShader(program))
                {
                    return error(GL_INVALID_OPERATION);
                }
                else
                {
                    return error(GL_INVALID_VALUE);
                }
            }

            if(index >= (GLuint)programObject->getActiveUniformCount())
            {
                return error(GL_INVALID_VALUE);
            }

            programObject->getActiveUniform(index, bufsize, length, size, type, name);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glGetAttachedShaders(GLuint program, GLsizei maxcount, GLsizei* count, GLuint* shaders)
{
    TRACE("(GLuint program = %d, GLsizei maxcount = %d, GLsizei* count = 0x%0.8p, GLuint* shaders = 0x%0.8p)",
          program, maxcount, count, shaders);

    try
    {
        if(maxcount < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            rad::Program *programObject = context->getProgram(program);

            if(!programObject)
            {
                if(context->getShader(program))
                {
                    return error(GL_INVALID_OPERATION);
                }
                else
                {
                    return error(GL_INVALID_VALUE);
                }
            }

            return programObject->getAttachedShaders(maxcount, count, shaders);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

int GL_APIENTRY glGetAttribLocation(GLuint program, const GLchar* name)
{
    TRACE("(GLuint program = %d, const GLchar* name = %s)", program, name);

    try
    {
        rad::Context *context = rad::getContext();

        if(context)
        {

            rad::Program *programObject = context->getProgram(program);

            if(!programObject)
            {
                if(context->getShader(program))
                {
                    return error(GL_INVALID_OPERATION, -1);
                }
                else
                {
                    return error(GL_INVALID_VALUE, -1);
                }
            }

            if(!programObject->isLinked())
            {
                return error(GL_INVALID_OPERATION, -1);
            }

            return programObject->getAttributeLocation(name);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY, -1);
    }

    return -1;
}

void GL_APIENTRY glGetBooleanv(GLenum pname, GLboolean* params)
{
    TRACE("(GLenum pname = 0x%X, GLboolean* params = 0x%0.8p)",  pname, params);

    try
    {
        rad::Context *context = rad::getContext();

        if(context)
        {
            if(!(context->getBooleanv(pname, params)))
            {
                GLenum nativeType;
                unsigned int numParams = 0;
                if(!context->getQueryParameterInfo(pname, &nativeType, &numParams))
                    return error(GL_INVALID_ENUM);

                if(numParams == 0)
                    return; // it is known that the pname is valid, but there are no parameters to return

                if(nativeType == GL_FLOAT)
                {
                    GLfloat *floatParams = NULL;
                    floatParams = new GLfloat[numParams];

                    context->getFloatv(pname, floatParams);

                    for(unsigned int i = 0; i < numParams; ++i)
                    {
                        if(floatParams[i] == 0.0f)
                            params[i] = GL_FALSE;
                        else
                            params[i] = GL_TRUE;
                    }

                    delete [] floatParams;
                }
                else if(nativeType == GL_INT)
                {
                    GLint *intParams = NULL;
                    intParams = new GLint[numParams];

                    context->getIntegerv(pname, intParams);

                    for(unsigned int i = 0; i < numParams; ++i)
                    {
                        if(intParams[i] == 0)
                            params[i] = GL_FALSE;
                        else
                            params[i] = GL_TRUE;
                    }

                    delete [] intParams;
                }
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glGetBufferParameteriv(GLenum target, GLenum pname, GLint* params)
{
    TRACE("(GLenum target = 0x%X, GLenum pname = 0x%X, GLint* params = 0x%0.8p)", target, pname, params);

    try
    {
        rad::Context *context = rad::getContext();

        if(context)
        {
            rad::Buffer *buffer;

            switch(target)
            {
              case GL_ARRAY_BUFFER:
                buffer = context->getArrayBuffer();
                break;
              case GL_ELEMENT_ARRAY_BUFFER:
                buffer = context->getElementArrayBuffer();
                break;
              default: return error(GL_INVALID_ENUM);
            }

            if(!buffer)
            {
                // A null buffer means that "0" is bound to the requested buffer target
                return error(GL_INVALID_OPERATION);
            }

            switch(pname)
            {
              case GL_BUFFER_USAGE:
                *params = buffer->usage();
                break;
              case GL_BUFFER_SIZE:
                *params = buffer->size();
                break;
              default: return error(GL_INVALID_ENUM);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

GLenum GL_APIENTRY glGetError(void)
{
    TRACE("()");

    rad::Context *context = rad::getContext();

    if(context)
    {
        return context->getError();
    }

    return GL_NO_ERROR;
}

void GL_APIENTRY glGetFenceivNV(GLuint fence, GLenum pname, GLint *params)
{
    TRACE("(GLuint fence = %d, GLenum pname = 0x%X, GLint *params = 0x%0.8p)", fence, pname, params);

    try
    {
        rad::Context *context = rad::getContext();

        if(context)
        {
            rad::Fence *fenceObject = context->getFence(fence);

            if(fenceObject == NULL)
            {
                return error(GL_INVALID_OPERATION);
            }

            fenceObject->getFenceiv(pname, params);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glGetFloatv(GLenum pname, GLfloat* params)
{
    TRACE("(GLenum pname = 0x%X, GLfloat* params = 0x%0.8p)", pname, params);

    try
    {
        rad::Context *context = rad::getContext();

        if(context)
        {
            if(!(context->getFloatv(pname, params)))
            {
                GLenum nativeType;
                unsigned int numParams = 0;
                if(!context->getQueryParameterInfo(pname, &nativeType, &numParams))
                    return error(GL_INVALID_ENUM);

                if(numParams == 0)
                    return; // it is known that the pname is valid, but that there are no parameters to return.

                if(nativeType == GL_BOOL)
                {
                    GLboolean *boolParams = NULL;
                    boolParams = new GLboolean[numParams];

                    context->getBooleanv(pname, boolParams);

                    for(unsigned int i = 0; i < numParams; ++i)
                    {
                        if(boolParams[i] == GL_FALSE)
                            params[i] = 0.0f;
                        else
                            params[i] = 1.0f;
                    }

                    delete [] boolParams;
                }
                else if(nativeType == GL_INT)
                {
                    GLint *intParams = NULL;
                    intParams = new GLint[numParams];

                    context->getIntegerv(pname, intParams);

                    for(unsigned int i = 0; i < numParams; ++i)
                    {
                        params[i] = (GLfloat)intParams[i];
                    }

                    delete [] intParams;
                }
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glGetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment, GLenum pname, GLint* params)
{
    TRACE("(GLenum target = 0x%X, GLenum attachment = 0x%X, GLenum pname = 0x%X, GLint* params = 0x%0.8p)",
          target, attachment, pname, params);

    try
    {
        rad::Context *context = rad::getContext();

        if(context)
        {
            if(target != GL_FRAMEBUFFER && target != GL_DRAW_FRAMEBUFFER_ANGLE && target != GL_READ_FRAMEBUFFER_ANGLE)
            {
                return error(GL_INVALID_ENUM);
            }

            rad::Framebuffer *framebuffer = NULL;
            if(target == GL_READ_FRAMEBUFFER_ANGLE)
            {
                if(context->getReadFramebufferHandle() == 0)
                {
                    return error(GL_INVALID_OPERATION);
                }

                framebuffer = context->getReadFramebuffer();
            }
            else 
            {
                if(context->getDrawFramebufferHandle() == 0)
                {
                    return error(GL_INVALID_OPERATION);
                }

                framebuffer = context->getDrawFramebuffer();
            }

            GLenum attachmentType;
            GLuint attachmentHandle;
            switch(attachment)
            {
              case GL_COLOR_ATTACHMENT0:    
                attachmentType = framebuffer->getColorbufferType();
                attachmentHandle = framebuffer->getColorbufferHandle(); 
                break;
              case GL_DEPTH_ATTACHMENT:     
                attachmentType = framebuffer->getDepthbufferType();
                attachmentHandle = framebuffer->getDepthbufferHandle();
                break;
              case GL_STENCIL_ATTACHMENT:   
                attachmentType = framebuffer->getStencilbufferType();
                attachmentHandle = framebuffer->getStencilbufferHandle();
                break;
              default: return error(GL_INVALID_ENUM);
            }

            GLenum attachmentObjectType;   // Type category
            if(attachmentType == GL_NONE || attachmentType == GL_RENDERBUFFER)
            {
                attachmentObjectType = attachmentType;
            }
            else if(rad::IsTextureTarget(attachmentType))
            {
                attachmentObjectType = GL_TEXTURE;
            }
            else UNREACHABLE();

            switch(pname)
            {
              case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE:
                *params = attachmentObjectType;
                break;
              case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME:
                if(attachmentObjectType == GL_RENDERBUFFER || attachmentObjectType == GL_TEXTURE)
                {
                    *params = attachmentHandle;
                }
                else
                {
                    return error(GL_INVALID_ENUM);
                }
                break;
              case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL:
                if(attachmentObjectType == GL_TEXTURE)
                {
                    *params = 0; // FramebufferTexture2D will not allow level to be set to anything else in GL ES 2.0
                }
                else
                {
                    return error(GL_INVALID_ENUM);
                }
                break;
              case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE:
                if(attachmentObjectType == GL_TEXTURE)
                {
                    if(rad::IsCubemapTextureTarget(attachmentType))
                    {
                        *params = attachmentType;
                    }
                    else
                    {
                        *params = 0;
                    }
                }
                else
                {
                    return error(GL_INVALID_ENUM);
                }
                break;
              default:
                return error(GL_INVALID_ENUM);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

GLenum GL_APIENTRY glGetGraphicsResetStatusEXT(void)
{
    TRACE("()");

    return GL_NO_ERROR;
}

void GL_APIENTRY glGetIntegerv(GLenum pname, GLint* params)
{
    TRACE("(GLenum pname = 0x%X, GLint* params = 0x%0.8p)", pname, params);

    try
    {
        rad::Context *context = rad::getContext();

        if(context)
        {
            if(!(context->getIntegerv(pname, params)))
            {
                GLenum nativeType;
                unsigned int numParams = 0;
                if(!context->getQueryParameterInfo(pname, &nativeType, &numParams))
                    return error(GL_INVALID_ENUM);

                if(numParams == 0)
                    return; // it is known that pname is valid, but there are no parameters to return

                if(nativeType == GL_BOOL)
                {
                    GLboolean *boolParams = NULL;
                    boolParams = new GLboolean[numParams];

                    context->getBooleanv(pname, boolParams);

                    for(unsigned int i = 0; i < numParams; ++i)
                    {
                        if(boolParams[i] == GL_FALSE)
                            params[i] = 0;
                        else
                            params[i] = 1;
                    }

                    delete [] boolParams;
                }
                else if(nativeType == GL_FLOAT)
                {
                    GLfloat *floatParams = NULL;
                    floatParams = new GLfloat[numParams];

                    context->getFloatv(pname, floatParams);

                    for(unsigned int i = 0; i < numParams; ++i)
                    {
                        if(pname == GL_DEPTH_RANGE || pname == GL_COLOR_CLEAR_VALUE || pname == GL_DEPTH_CLEAR_VALUE || pname == GL_BLEND_COLOR)
                        {
                            params[i] = (GLint)(((GLfloat)(0xFFFFFFFF) * floatParams[i] - 1.0f) / 2.0f);
                        }
                        else
                            params[i] = (GLint)(floatParams[i] > 0.0f ? floor(floatParams[i] + 0.5) : ceil(floatParams[i] - 0.5));
                    }

                    delete [] floatParams;
                }
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glGetProgramiv(GLuint program, GLenum pname, GLint* params)
{
    TRACE("(GLuint program = %d, GLenum pname = %d, GLint* params = 0x%0.8p)", program, pname, params);

    try
    {
        rad::Context *context = rad::getContext();

        if(context)
        {
            rad::Program *programObject = context->getProgram(program);

            if(!programObject)
            {
                return error(GL_INVALID_VALUE);
            }

            switch(pname)
            {
              case GL_DELETE_STATUS:
                *params = programObject->isFlaggedForDeletion();
                return;
              case GL_LINK_STATUS:
                *params = programObject->isLinked();
                return;
              case GL_VALIDATE_STATUS:
                *params = programObject->isValidated();
                return;
              case GL_INFO_LOG_LENGTH:
                *params = programObject->getInfoLogLength();
                return;
              case GL_ATTACHED_SHADERS:
                *params = programObject->getAttachedShadersCount();
                return;
              case GL_ACTIVE_ATTRIBUTES:
                *params = programObject->getActiveAttributeCount();
                return;
              case GL_ACTIVE_ATTRIBUTE_MAX_LENGTH:
                *params = programObject->getActiveAttributeMaxLength();
                return;
              case GL_ACTIVE_UNIFORMS:
                *params = programObject->getActiveUniformCount();
                return;
              case GL_ACTIVE_UNIFORM_MAX_LENGTH:
                *params = programObject->getActiveUniformMaxLength();
                return;
              default:
                return error(GL_INVALID_ENUM);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glGetProgramInfoLog(GLuint program, GLsizei bufsize, GLsizei* length, GLchar* infolog)
{
    TRACE("(GLuint program = %d, GLsizei bufsize = %d, GLsizei* length = 0x%0.8p, GLchar* infolog = 0x%0.8p)",
          program, bufsize, length, infolog);

    try
    {
        if(bufsize < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            rad::Program *programObject = context->getProgram(program);

            if(!programObject)
            {
                return error(GL_INVALID_VALUE);
            }

            programObject->getInfoLog(bufsize, length, infolog);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glGetQueryivEXT(GLenum target, GLenum pname, GLint *params)
{
    TRACE("GLenum target = 0x%X, GLenum pname = 0x%X, GLint *params = 0x%0.8p)", target, pname, params);

    try
    {
        switch(pname)
        {
        case GL_CURRENT_QUERY_EXT:
            break;
        default:
            return error(GL_INVALID_ENUM);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            params[0] = context->getActiveQuery(target);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glGetQueryObjectuivEXT(GLuint id, GLenum pname, GLuint *params)
{
    TRACE("(GLuint id = %d, GLenum pname = 0x%X, GLuint *params = 0x%0.8p)", id, pname, params);

    try
    {
        switch(pname)
        {
        case GL_QUERY_RESULT_EXT:
        case GL_QUERY_RESULT_AVAILABLE_EXT:
            break;
        default:
            return error(GL_INVALID_ENUM);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            rad::Query *queryObject = context->getQuery(id, false, GL_NONE);

            if(!queryObject)
            {
                return error(GL_INVALID_OPERATION);
            }

            if(context->getActiveQuery(queryObject->getType()) == id)
            {
                return error(GL_INVALID_OPERATION);
            }

            switch(pname)
            {
            case GL_QUERY_RESULT_EXT:
                params[0] = queryObject->getResult();
                break;
            case GL_QUERY_RESULT_AVAILABLE_EXT:
                params[0] = queryObject->isResultAvailable();
                break;
            default:
                ASSERT(false);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glGetRenderbufferParameteriv(GLenum target, GLenum pname, GLint* params)
{
    TRACE("(GLenum target = 0x%X, GLenum pname = 0x%X, GLint* params = 0x%0.8p)", target, pname, params);

    try
    {
        rad::Context *context = rad::getContext();

        if(context)
        {
            if(target != GL_RENDERBUFFER)
            {
                return error(GL_INVALID_ENUM);
            }

            if(context->getRenderbufferHandle() == 0)
            {
                return error(GL_INVALID_OPERATION);
            }

            rad::Renderbuffer *renderbuffer = context->getRenderbuffer(context->getRenderbufferHandle());

            switch(pname)
            {
            case GL_RENDERBUFFER_WIDTH:           *params = renderbuffer->getWidth();       break;
            case GL_RENDERBUFFER_HEIGHT:          *params = renderbuffer->getHeight();      break;
            case GL_RENDERBUFFER_INTERNAL_FORMAT: *params = renderbuffer->getFormat();      break;
            case GL_RENDERBUFFER_RED_SIZE:        *params = renderbuffer->getRedSize();     break;
            case GL_RENDERBUFFER_GREEN_SIZE:      *params = renderbuffer->getGreenSize();   break;
            case GL_RENDERBUFFER_BLUE_SIZE:       *params = renderbuffer->getBlueSize();    break;
            case GL_RENDERBUFFER_ALPHA_SIZE:      *params = renderbuffer->getAlphaSize();   break;
            case GL_RENDERBUFFER_DEPTH_SIZE:      *params = renderbuffer->getDepthSize();   break;
            case GL_RENDERBUFFER_STENCIL_SIZE:    *params = renderbuffer->getStencilSize(); break;
            case GL_RENDERBUFFER_SAMPLES_ANGLE:   *params = renderbuffer->getSamples();     break;
            default:
                return error(GL_INVALID_ENUM);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glGetShaderiv(GLuint shader, GLenum pname, GLint* params)
{
    TRACE("(GLuint shader = %d, GLenum pname = %d, GLint* params = 0x%0.8p)", shader, pname, params);

    try
    {
        rad::Context *context = rad::getContext();

        if(context)
        {
            rad::Shader *shaderObject = context->getShader(shader);

            if(!shaderObject)
            {
                return error(GL_INVALID_VALUE);
            }

            switch(pname)
            {
              case GL_SHADER_TYPE:
                *params = shaderObject->getType();
                return;
              case GL_DELETE_STATUS:
                *params = shaderObject->isFlaggedForDeletion();
                return;
              case GL_COMPILE_STATUS:
                *params = shaderObject->isCompiled() ? GL_TRUE : GL_FALSE;
                return;
              case GL_INFO_LOG_LENGTH:
                *params = shaderObject->getInfoLogLength();
                return;
              case GL_SHADER_SOURCE_LENGTH:
                *params = shaderObject->getSourceLength();
                return;
              default:
                return error(GL_INVALID_ENUM);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glGetShaderInfoLog(GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* infolog)
{
    TRACE("(GLuint shader = %d, GLsizei bufsize = %d, GLsizei* length = 0x%0.8p, GLchar* infolog = 0x%0.8p)",
          shader, bufsize, length, infolog);

    try
    {
        if(bufsize < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            rad::Shader *shaderObject = context->getShader(shader);

            if(!shaderObject)
            {
                return error(GL_INVALID_VALUE);
            }

            shaderObject->getInfoLog(bufsize, length, infolog);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glGetShaderPrecisionFormat(GLenum shadertype, GLenum precisiontype, GLint* range, GLint* precision)
{
    TRACE("(GLenum shadertype = 0x%X, GLenum precisiontype = 0x%X, GLint* range = 0x%0.8p, GLint* precision = 0x%0.8p)",
          shadertype, precisiontype, range, precision);

    try
    {
        switch(shadertype)
        {
          case GL_VERTEX_SHADER:
          case GL_FRAGMENT_SHADER:
            break;
          default:
            return error(GL_INVALID_ENUM);
        }

        switch(precisiontype)
        {
          case GL_LOW_FLOAT:
          case GL_MEDIUM_FLOAT:
          case GL_HIGH_FLOAT:
            // IEEE 754 single-precision
            range[0] = 127;
            range[1] = 127;
            *precision = 23;
            break;
          case GL_LOW_INT:
          case GL_MEDIUM_INT:
          case GL_HIGH_INT:
            // Single-precision floating-point numbers can accurately represent integers up to +/-16777216
            range[0] = 24;
            range[1] = 24;
            *precision = 0;
            break;
          default:
            return error(GL_INVALID_ENUM);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glGetShaderSource(GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* source)
{
    TRACE("(GLuint shader = %d, GLsizei bufsize = %d, GLsizei* length = 0x%0.8p, GLchar* source = 0x%0.8p)",
          shader, bufsize, length, source);

    try
    {
        if(bufsize < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            rad::Shader *shaderObject = context->getShader(shader);

            if(!shaderObject)
            {
                return error(GL_INVALID_OPERATION);
            }

            shaderObject->getSource(bufsize, length, source);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

const GLubyte* GL_APIENTRY glGetString(GLenum name)
{
    TRACE("(GLenum name = 0x%X)", name);

    try
    {
        rad::Context *context = rad::getContext();

        switch(name)
        {
        case GL_VENDOR:
            return (GLubyte*)"TransGaming Inc.";
        case GL_RENDERER:
            return (GLubyte*)"SwiftShader";
        case GL_VERSION:
            return (GLubyte*)"OpenGL ES 2.0 SwiftShader "VERSION_STRING;
        case GL_SHADING_LANGUAGE_VERSION:
            return (GLubyte*)"OpenGL ES GLSL ES 1.00 SwiftShader "VERSION_STRING;
        case GL_EXTENSIONS:
            // Keep list sorted in following order:
	        // OES extensions
	        // EXT extensions
	        // Vendor extensions
            return (GLubyte*)
                "GL_OES_depth_texture "
                "GL_OES_depth_texture_cube_map "
                "GL_OES_EGL_image "
                "GL_OES_EGL_image_external "
                "GL_OES_element_index_uint "
                "GL_OES_packed_depth_stencil "
                "GL_OES_rgb8_rgba8 "
                "GL_OES_standard_derivatives "
                "GL_OES_texture_float "
                "GL_OES_texture_float_linear "
                "GL_OES_texture_half_float "
                "GL_OES_texture_half_float_linear "
                "GL_OES_texture_npot "
                "GL_EXT_blend_minmax "
                "GL_EXT_occlusion_query_boolean "
                "GL_EXT_read_format_bgra "
                   #if (S3TC_SUPPORT)
                "GL_EXT_texture_compression_dxt1 "
                "GL_ANGLE_texture_compression_dxt3 "
                "GL_ANGLE_texture_compression_dxt5 "
                   #endif
                "GL_EXT_texture_filter_anisotropic "
                "GL_EXT_texture_format_BGRA8888 "
                "GL_ANGLE_framebuffer_blit "
                "GL_ANGLE_framebuffer_multisample "
                "GL_NV_fence";
        default:
            return error(GL_INVALID_ENUM, (GLubyte*)NULL);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY, (GLubyte*)NULL);
    }

    return NULL;
}

void GL_APIENTRY glGetTexParameterfv(GLenum target, GLenum pname, GLfloat* params)
{
    TRACE("(GLenum target = 0x%X, GLenum pname = 0x%X, GLfloat* params = 0x%0.8p)", target, pname, params);

    try
    {
        rad::Context *context = rad::getContext();

        if(context)
        {
            rad::Texture *texture;

            switch(target)
            {
            case GL_TEXTURE_2D:
                texture = context->getTexture2D();
                break;
            case GL_TEXTURE_CUBE_MAP:
                texture = context->getTextureCubeMap();
                break;
            case GL_TEXTURE_EXTERNAL_OES:
                texture = context->getTextureExternal();
                break;
            default:
                return error(GL_INVALID_ENUM);
            }

            switch(pname)
            {
            case GL_TEXTURE_MAG_FILTER:
                *params = (GLfloat)texture->getMagFilter();
                break;
            case GL_TEXTURE_MIN_FILTER:
                *params = (GLfloat)texture->getMinFilter();
                break;
            case GL_TEXTURE_WRAP_S:
                *params = (GLfloat)texture->getWrapS();
                break;
            case GL_TEXTURE_WRAP_T:
                *params = (GLfloat)texture->getWrapT();
                break;
			case GL_TEXTURE_MAX_ANISOTROPY_EXT:
                *params = texture->getMaxAnisotropy();
                break;
            case GL_REQUIRED_TEXTURE_IMAGE_UNITS_OES:
                *params = (GLfloat)1;
                break;
            default:
                return error(GL_INVALID_ENUM);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glGetTexParameteriv(GLenum target, GLenum pname, GLint* params)
{
    TRACE("(GLenum target = 0x%X, GLenum pname = 0x%X, GLint* params = 0x%0.8p)", target, pname, params);

    try
    {
        rad::Context *context = rad::getContext();

        if(context)
        {
            rad::Texture *texture;

            switch(target)
            {
            case GL_TEXTURE_2D:
                texture = context->getTexture2D();
                break;
            case GL_TEXTURE_CUBE_MAP:
                texture = context->getTextureCubeMap();
                break;
            case GL_TEXTURE_EXTERNAL_OES:
                texture = context->getTextureExternal();
                break;
            default:
                return error(GL_INVALID_ENUM);
            }

            switch(pname)
            {
            case GL_TEXTURE_MAG_FILTER:
                *params = texture->getMagFilter();
                break;
            case GL_TEXTURE_MIN_FILTER:
                *params = texture->getMinFilter();
                break;
            case GL_TEXTURE_WRAP_S:
                *params = texture->getWrapS();
                break;
            case GL_TEXTURE_WRAP_T:
                *params = texture->getWrapT();
                break;
		    case GL_TEXTURE_MAX_ANISOTROPY_EXT:
                *params = (GLint)texture->getMaxAnisotropy();
                break;
            case GL_REQUIRED_TEXTURE_IMAGE_UNITS_OES:
                *params = 1;
                break;
            default:
                return error(GL_INVALID_ENUM);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glGetnUniformfvEXT(GLuint program, GLint location, GLsizei bufSize, GLfloat* params)
{
    TRACE("(GLuint program = %d, GLint location = %d, GLsizei bufSize = %d, GLfloat* params = 0x%0.8p)",
          program, location, bufSize, params);

    try
    {
        if(bufSize < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            if(program == 0)
            {
                return error(GL_INVALID_VALUE);
            }

            rad::Program *programObject = context->getProgram(program);

            if(!programObject || !programObject->isLinked())
            {
                return error(GL_INVALID_OPERATION);
            }

            if(!programObject->getUniformfv(location, &bufSize, params))
            {
                return error(GL_INVALID_OPERATION);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glGetUniformfv(GLuint program, GLint location, GLfloat* params)
{
    TRACE("(GLuint program = %d, GLint location = %d, GLfloat* params = 0x%0.8p)", program, location, params);

    try
    {
        rad::Context *context = rad::getContext();

        if(context)
        {
            if(program == 0)
            {
                return error(GL_INVALID_VALUE);
            }

            rad::Program *programObject = context->getProgram(program);

            if(!programObject || !programObject->isLinked())
            {
                return error(GL_INVALID_OPERATION);
            }

            if(!programObject->getUniformfv(location, NULL, params))
            {
                return error(GL_INVALID_OPERATION);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glGetnUniformivEXT(GLuint program, GLint location, GLsizei bufSize, GLint* params)
{
    TRACE("(GLuint program = %d, GLint location = %d, GLsizei bufSize = %d, GLint* params = 0x%0.8p)", 
          program, location, bufSize, params);

    try
    {
        if(bufSize < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            if(program == 0)
            {
                return error(GL_INVALID_VALUE);
            }

            rad::Program *programObject = context->getProgram(program);

            if(!programObject || !programObject->isLinked())
            {
                return error(GL_INVALID_OPERATION);
            }

            if(!programObject)
            {
                return error(GL_INVALID_OPERATION);
            }

            if(!programObject->getUniformiv(location, &bufSize, params))
            {
                return error(GL_INVALID_OPERATION);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glGetUniformiv(GLuint program, GLint location, GLint* params)
{
    TRACE("(GLuint program = %d, GLint location = %d, GLint* params = 0x%0.8p)", program, location, params);

    try
    {
        rad::Context *context = rad::getContext();

        if(context)
        {
            if(program == 0)
            {
                return error(GL_INVALID_VALUE);
            }

            rad::Program *programObject = context->getProgram(program);

            if(!programObject || !programObject->isLinked())
            {
                return error(GL_INVALID_OPERATION);
            }

            if(!programObject)
            {
                return error(GL_INVALID_OPERATION);
            }

            if(!programObject->getUniformiv(location, NULL, params))
            {
                return error(GL_INVALID_OPERATION);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

int GL_APIENTRY glGetUniformLocation(GLuint program, const GLchar* name)
{
    TRACE("(GLuint program = %d, const GLchar* name = 0x%0.8p)", program, name);

    try
    {
        rad::Context *context = rad::getContext();

        if(strstr(name, "gl_") == name)
        {
            return -1;
        }

        if(context)
        {
            rad::Program *programObject = context->getProgram(program);

            if(!programObject)
            {
                if(context->getShader(program))
                {
                    return error(GL_INVALID_OPERATION, -1);
                }
                else
                {
                    return error(GL_INVALID_VALUE, -1);
                }
            }

            if(!programObject->isLinked())
            {
                return error(GL_INVALID_OPERATION, -1);
            }

            return programObject->getUniformLocation(name);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY, -1);
    }

    return -1;
}

void GL_APIENTRY glGetVertexAttribfv(GLuint index, GLenum pname, GLfloat* params)
{
    TRACE("(GLuint index = %d, GLenum pname = 0x%X, GLfloat* params = 0x%0.8p)", index, pname, params);

    try
    {
        rad::Context *context = rad::getContext();

        if(context)
        {
            if(index >= rad::MAX_VERTEX_ATTRIBS)
            {
                return error(GL_INVALID_VALUE);
            }

            const rad::VertexAttribute &attribState = context->getVertexAttribState(index);

            switch(pname)
            {
              case GL_VERTEX_ATTRIB_ARRAY_ENABLED:
                *params = (GLfloat)(attribState.mArrayEnabled ? GL_TRUE : GL_FALSE);
                break;
              case GL_VERTEX_ATTRIB_ARRAY_SIZE:
                *params = (GLfloat)attribState.mSize;
                break;
              case GL_VERTEX_ATTRIB_ARRAY_STRIDE:
                *params = (GLfloat)attribState.mStride;
                break;
              case GL_VERTEX_ATTRIB_ARRAY_TYPE:
                *params = (GLfloat)attribState.mType;
                break;
              case GL_VERTEX_ATTRIB_ARRAY_NORMALIZED:
                *params = (GLfloat)(attribState.mNormalized ? GL_TRUE : GL_FALSE);
                break;
              case GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING:
                *params = (GLfloat)attribState.mBoundBuffer.id();
                break;
              case GL_CURRENT_VERTEX_ATTRIB:
                for(int i = 0; i < 4; ++i)
                {
                    params[i] = attribState.mCurrentValue[i];
                }
                break;
              default: return error(GL_INVALID_ENUM);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glGetVertexAttribiv(GLuint index, GLenum pname, GLint* params)
{
    TRACE("(GLuint index = %d, GLenum pname = 0x%X, GLint* params = 0x%0.8p)", index, pname, params);

    try
    {
        rad::Context *context = rad::getContext();

        if(context)
        {
            if(index >= rad::MAX_VERTEX_ATTRIBS)
            {
                return error(GL_INVALID_VALUE);
            }

            const rad::VertexAttribute &attribState = context->getVertexAttribState(index);

            switch(pname)
            {
              case GL_VERTEX_ATTRIB_ARRAY_ENABLED:
                *params = (attribState.mArrayEnabled ? GL_TRUE : GL_FALSE);
                break;
              case GL_VERTEX_ATTRIB_ARRAY_SIZE:
                *params = attribState.mSize;
                break;
              case GL_VERTEX_ATTRIB_ARRAY_STRIDE:
                *params = attribState.mStride;
                break;
              case GL_VERTEX_ATTRIB_ARRAY_TYPE:
                *params = attribState.mType;
                break;
              case GL_VERTEX_ATTRIB_ARRAY_NORMALIZED:
                *params = (attribState.mNormalized ? GL_TRUE : GL_FALSE);
                break;
              case GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING:
                *params = attribState.mBoundBuffer.id();
                break;
              case GL_CURRENT_VERTEX_ATTRIB:
                for(int i = 0; i < 4; ++i)
                {
                    float currentValue = attribState.mCurrentValue[i];
                    params[i] = (GLint)(currentValue > 0.0f ? floor(currentValue + 0.5f) : ceil(currentValue - 0.5f));
                }
                break;
              default: return error(GL_INVALID_ENUM);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glGetVertexAttribPointerv(GLuint index, GLenum pname, GLvoid** pointer)
{
    TRACE("(GLuint index = %d, GLenum pname = 0x%X, GLvoid** pointer = 0x%0.8p)", index, pname, pointer);

    try
    {
        rad::Context *context = rad::getContext();

        if(context)
        {
            if(index >= rad::MAX_VERTEX_ATTRIBS)
            {
                return error(GL_INVALID_VALUE);
            }

            if(pname != GL_VERTEX_ATTRIB_ARRAY_POINTER)
            {
                return error(GL_INVALID_ENUM);
            }

            *pointer = const_cast<GLvoid*>(context->getVertexAttribPointer(index));
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glHint(GLenum target, GLenum mode)
{
    TRACE("(GLenum target = 0x%X, GLenum mode = 0x%X)", target, mode);

    try
    {
        switch(mode)
        {
          case GL_FASTEST:
          case GL_NICEST:
          case GL_DONT_CARE:
            break;
          default:
            return error(GL_INVALID_ENUM); 
        }

        rad::Context *context = rad::getContext();
        switch(target)
        {
          case GL_GENERATE_MIPMAP_HINT:
            if(context) context->setGenerateMipmapHint(mode);
            break;
          case GL_FRAGMENT_SHADER_DERIVATIVE_HINT_OES:
            if(context) context->setFragmentShaderDerivativeHint(mode);
            break;
          default:
            return error(GL_INVALID_ENUM);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

GLboolean GL_APIENTRY glIsBuffer(GLuint buffer)
{
    TRACE("(GLuint buffer = %d)", buffer);

    try
    {
        rad::Context *context = rad::getContext();

        if(context && buffer)
        {
            rad::Buffer *bufferObject = context->getBuffer(buffer);

            if(bufferObject)
            {
                return GL_TRUE;
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY, GL_FALSE);
    }

    return GL_FALSE;
}

GLboolean GL_APIENTRY glIsEnabled(GLenum cap)
{
    TRACE("(GLenum cap = 0x%X)", cap);

    try
    {
        rad::Context *context = rad::getContext();

        if(context)
        {
            switch(cap)
            {
              case GL_CULL_FACE:                return context->isCullFaceEnabled();
              case GL_POLYGON_OFFSET_FILL:      return context->isPolygonOffsetFillEnabled();
              case GL_SAMPLE_ALPHA_TO_COVERAGE: return context->isSampleAlphaToCoverageEnabled();
              case GL_SAMPLE_COVERAGE:          return context->isSampleCoverageEnabled();
              case GL_SCISSOR_TEST:             return context->isScissorTestEnabled();
              case GL_STENCIL_TEST:             return context->isStencilTestEnabled();
              case GL_DEPTH_TEST:               return context->isDepthTestEnabled();
              case GL_BLEND:                    return context->isBlendEnabled();
              case GL_DITHER:                   return context->isDitherEnabled();
              default:
                return error(GL_INVALID_ENUM, false);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY, false);
    }

    return false;
}

GLboolean GL_APIENTRY glIsFenceNV(GLuint fence)
{
    TRACE("(GLuint fence = %d)", fence);

    try
    {
        rad::Context *context = rad::getContext();

        if(context)
        {
            rad::Fence *fenceObject = context->getFence(fence);

            if(fenceObject == NULL)
            {
                return GL_FALSE;
            }

            return fenceObject->isFence();
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY, GL_FALSE);
    }

    return GL_FALSE;
}

GLboolean GL_APIENTRY glIsFramebuffer(GLuint framebuffer)
{
    TRACE("(GLuint framebuffer = %d)", framebuffer);

    try
    {
        rad::Context *context = rad::getContext();

        if(context && framebuffer)
        {
            rad::Framebuffer *framebufferObject = context->getFramebuffer(framebuffer);

            if(framebufferObject)
            {
                return GL_TRUE;
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY, GL_FALSE);
    }

    return GL_FALSE;
}

GLboolean GL_APIENTRY glIsProgram(GLuint program)
{
    TRACE("(GLuint program = %d)", program);

    try
    {
        rad::Context *context = rad::getContext();

        if(context && program)
        {
            rad::Program *programObject = context->getProgram(program);

            if(programObject)
            {
                return GL_TRUE;
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY, GL_FALSE);
    }

    return GL_FALSE;
}

GLboolean GL_APIENTRY glIsQueryEXT(GLuint id)
{
    TRACE("(GLuint id = %d)", id);

    try
    {
        if(id == 0)
        {
            return GL_FALSE;
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            rad::Query *queryObject = context->getQuery(id, false, GL_NONE);

            if(queryObject)
            {
                return GL_TRUE;
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY, GL_FALSE);
    }

    return GL_FALSE;
}

GLboolean GL_APIENTRY glIsRenderbuffer(GLuint renderbuffer)
{
    TRACE("(GLuint renderbuffer = %d)", renderbuffer);

    try
    {
        rad::Context *context = rad::getContext();

        if(context && renderbuffer)
        {
            rad::Renderbuffer *renderbufferObject = context->getRenderbuffer(renderbuffer);

            if(renderbufferObject)
            {
                return GL_TRUE;
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY, GL_FALSE);
    }

    return GL_FALSE;
}

GLboolean GL_APIENTRY glIsShader(GLuint shader)
{
    TRACE("(GLuint shader = %d)", shader);

    try
    {
        rad::Context *context = rad::getContext();

        if(context && shader)
        {
            rad::Shader *shaderObject = context->getShader(shader);

            if(shaderObject)
            {
                return GL_TRUE;
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY, GL_FALSE);
    }

    return GL_FALSE;
}

GLboolean GL_APIENTRY glIsTexture(GLuint texture)
{
    TRACE("(GLuint texture = %d)", texture);

    try
    {
        rad::Context *context = rad::getContext();

        if(context && texture)
        {
            rad::Texture *textureObject = context->getTexture(texture);

            if(textureObject)
            {
                return GL_TRUE;
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY, GL_FALSE);
    }

    return GL_FALSE;
}

void GL_APIENTRY glLineWidth(GLfloat width)
{
    TRACE("(GLfloat width = %f)", width);

    try
    {
        if(width <= 0.0f)
        {
            return error(GL_INVALID_VALUE);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            context->setLineWidth(width);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glLinkProgram(GLuint program)
{
    TRACE("(GLuint program = %d)", program);

    try
    {
        rad::Context *context = rad::getContext();

        if(context)
        {
            rad::Program *programObject = context->getProgram(program);

            if(!programObject)
            {
                if(context->getShader(program))
                {
                    return error(GL_INVALID_OPERATION);
                }
                else
                {
                    return error(GL_INVALID_VALUE);
                }
            }

            programObject->link();
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glPixelStorei(GLenum pname, GLint param)
{
    TRACE("(GLenum pname = 0x%X, GLint param = %d)", pname, param);

    try
    {
        rad::Context *context = rad::getContext();

        if(context)
        {
            switch(pname)
            {
              case GL_UNPACK_ALIGNMENT:
                if(param != 1 && param != 2 && param != 4 && param != 8)
                {
                    return error(GL_INVALID_VALUE);
                }

                context->setUnpackAlignment(param);
                break;

              case GL_PACK_ALIGNMENT:
                if(param != 1 && param != 2 && param != 4 && param != 8)
                {
                    return error(GL_INVALID_VALUE);
                }

                context->setPackAlignment(param);
                break;

              default:
                return error(GL_INVALID_ENUM);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glPolygonOffset(GLfloat factor, GLfloat units)
{
    TRACE("(GLfloat factor = %f, GLfloat units = %f)", factor, units);

    try
    {
        rad::Context *context = rad::getContext();

        if(context)
        {
            context->setPolygonOffsetParams(factor, units);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glReadnPixelsEXT(GLint x, GLint y, GLsizei width, GLsizei height,
                                GLenum format, GLenum type, GLsizei bufSize,
                                GLvoid *data)
{
    TRACE("(GLint x = %d, GLint y = %d, GLsizei width = %d, GLsizei height = %d, "
          "GLenum format = 0x%X, GLenum type = 0x%X, GLsizei bufSize = 0x%d, GLvoid *data = 0x%0.8p)",
          x, y, width, height, format, type, bufSize, data);

    try
    {
        if(width < 0 || height < 0 || bufSize < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        if(!validReadFormatType(format, type))
        {
            return error(GL_INVALID_OPERATION);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            context->readPixels(x, y, width, height, format, type, &bufSize, data);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* pixels)
{
    TRACE("(GLint x = %d, GLint y = %d, GLsizei width = %d, GLsizei height = %d, "
          "GLenum format = 0x%X, GLenum type = 0x%X, GLvoid* pixels = 0x%0.8p)",
          x, y, width, height, format, type,  pixels);

    try
    {
        if(width < 0 || height < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        if(!validReadFormatType(format, type))
        {
            return error(GL_INVALID_OPERATION);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            context->readPixels(x, y, width, height, format, type, NULL, pixels);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glReleaseShaderCompiler(void)
{
    TRACE("()");

    try
    {
        rad::Shader::releaseCompiler();
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glRenderbufferStorageMultisampleANGLE(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height)
{
    TRACE("(GLenum target = 0x%X, GLsizei samples = %d, GLenum internalformat = 0x%X, GLsizei width = %d, GLsizei height = %d)",
          target, samples, internalformat, width, height);

    try
    {
        switch(target)
        {
          case GL_RENDERBUFFER:
            break;
          default:
            return error(GL_INVALID_ENUM);
        }

        if(!rad::IsColorRenderable(internalformat) && !rad::IsDepthRenderable(internalformat) && !rad::IsStencilRenderable(internalformat))
        {
            return error(GL_INVALID_ENUM);
        }

        if(width < 0 || height < 0 || samples < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            if(width > rad::IMPLEMENTATION_MAX_RENDERBUFFER_SIZE || 
               height > rad::IMPLEMENTATION_MAX_RENDERBUFFER_SIZE ||
               samples > rad::IMPLEMENTATION_MAX_SAMPLES)
            {
                return error(GL_INVALID_VALUE);
            }

            GLuint handle = context->getRenderbufferHandle();
            if(handle == 0)
            {
                return error(GL_INVALID_OPERATION);
            }

            switch(internalformat)
            {
              case GL_DEPTH_COMPONENT16:
                context->setRenderbufferStorage(new rad::Depthbuffer(width, height, samples));
                break;
              case GL_RGBA4:
              case GL_RGB5_A1:
              case GL_RGB565:
              case GL_RGB8_OES:
              case GL_RGBA8_OES:
                context->setRenderbufferStorage(new rad::Colorbuffer(width, height, internalformat, samples));
                break;
              case GL_STENCIL_INDEX8:
                context->setRenderbufferStorage(new rad::Stencilbuffer(width, height, samples));
                break;
              case GL_DEPTH24_STENCIL8_OES:
                context->setRenderbufferStorage(new rad::DepthStencilbuffer(width, height, samples));
                break;
              default:
                return error(GL_INVALID_ENUM);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
{
    glRenderbufferStorageMultisampleANGLE(target, 0, internalformat, width, height);
}

void GL_APIENTRY glSampleCoverage(GLclampf value, GLboolean invert)
{
    TRACE("(GLclampf value = %f, GLboolean invert = %d)", value, invert);

    try
    {
        rad::Context* context = rad::getContext();

        if(context)
        {
            context->setSampleCoverageParams(rad::clamp01(value), invert == GL_TRUE);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glSetFenceNV(GLuint fence, GLenum condition)
{
    TRACE("(GLuint fence = %d, GLenum condition = 0x%X)", fence, condition);

    try
    {
        if(condition != GL_ALL_COMPLETED_NV)
        {
            return error(GL_INVALID_ENUM);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            rad::Fence *fenceObject = context->getFence(fence);

            if(fenceObject == NULL)
            {
                return error(GL_INVALID_OPERATION);
            }

            fenceObject->setFence(condition);    
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glScissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
    TRACE("(GLint x = %d, GLint y = %d, GLsizei width = %d, GLsizei height = %d)", x, y, width, height);

    try
    {
        if(width < 0 || height < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        rad::Context* context = rad::getContext();

        if(context)
        {
            context->setScissorParams(x, y, width, height);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glShaderBinary(GLsizei n, const GLuint* shaders, GLenum binaryformat, const GLvoid* binary, GLsizei length)
{
    TRACE("(GLsizei n = %d, const GLuint* shaders = 0x%0.8p, GLenum binaryformat = 0x%X, "
          "const GLvoid* binary = 0x%0.8p, GLsizei length = %d)",
          n, shaders, binaryformat, binary, length);

    try
    {
        // No binary shader formats are supported.
        return error(GL_INVALID_ENUM);
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glShaderSource(GLuint shader, GLsizei count, const GLchar *const *string, const GLint *length)
{
    TRACE("(GLuint shader = %d, GLsizei count = %d, const GLchar** string = 0x%0.8p, const GLint* length = 0x%0.8p)",
          shader, count, string, length);

    try
    {
        if(count < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            rad::Shader *shaderObject = context->getShader(shader);

            if(!shaderObject)
            {
                if(context->getProgram(shader))
                {
                    return error(GL_INVALID_OPERATION);
                }
                else
                {
                    return error(GL_INVALID_VALUE);
                }
            }

            shaderObject->setSource(count, string, length);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glStencilFunc(GLenum func, GLint ref, GLuint mask)
{
    glStencilFuncSeparate(GL_FRONT_AND_BACK, func, ref, mask);
}

void GL_APIENTRY glStencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask)
{
    TRACE("(GLenum face = 0x%X, GLenum func = 0x%X, GLint ref = %d, GLuint mask = %d)", face, func, ref, mask);

    try
    {
        switch(face)
        {
          case GL_FRONT:
          case GL_BACK:
          case GL_FRONT_AND_BACK:
            break;
          default:
            return error(GL_INVALID_ENUM);
        }

        switch(func)
        {
          case GL_NEVER:
          case GL_ALWAYS:
          case GL_LESS:
          case GL_LEQUAL:
          case GL_EQUAL:
          case GL_GEQUAL:
          case GL_GREATER:
          case GL_NOTEQUAL:
            break;
          default:
            return error(GL_INVALID_ENUM);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            if(face == GL_FRONT || face == GL_FRONT_AND_BACK)
            {
                context->setStencilParams(func, ref, mask);
            }

            if(face == GL_BACK || face == GL_FRONT_AND_BACK)
            {
                context->setStencilBackParams(func, ref, mask);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glStencilMask(GLuint mask)
{
    glStencilMaskSeparate(GL_FRONT_AND_BACK, mask);
}

void GL_APIENTRY glStencilMaskSeparate(GLenum face, GLuint mask)
{
    TRACE("(GLenum face = 0x%X, GLuint mask = %d)", face, mask);

    try
    {
        switch(face)
        {
          case GL_FRONT:
          case GL_BACK:
          case GL_FRONT_AND_BACK:
            break;
          default:
            return error(GL_INVALID_ENUM);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            if(face == GL_FRONT || face == GL_FRONT_AND_BACK)
            {
                context->setStencilWritemask(mask);
            }

            if(face == GL_BACK || face == GL_FRONT_AND_BACK)
            {
                context->setStencilBackWritemask(mask);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glStencilOp(GLenum fail, GLenum zfail, GLenum zpass)
{
    glStencilOpSeparate(GL_FRONT_AND_BACK, fail, zfail, zpass);
}

void GL_APIENTRY glStencilOpSeparate(GLenum face, GLenum fail, GLenum zfail, GLenum zpass)
{
    TRACE("(GLenum face = 0x%X, GLenum fail = 0x%X, GLenum zfail = 0x%X, GLenum zpas = 0x%Xs)",
          face, fail, zfail, zpass);

    try
    {
        switch(face)
        {
          case GL_FRONT:
          case GL_BACK:
          case GL_FRONT_AND_BACK:
            break;
          default:
            return error(GL_INVALID_ENUM);
        }

        switch(fail)
        {
          case GL_ZERO:
          case GL_KEEP:
          case GL_REPLACE:
          case GL_INCR:
          case GL_DECR:
          case GL_INVERT:
          case GL_INCR_WRAP:
          case GL_DECR_WRAP:
            break;
          default:
            return error(GL_INVALID_ENUM);
        }

        switch(zfail)
        {
          case GL_ZERO:
          case GL_KEEP:
          case GL_REPLACE:
          case GL_INCR:
          case GL_DECR:
          case GL_INVERT:
          case GL_INCR_WRAP:
          case GL_DECR_WRAP:
            break;
          default:
            return error(GL_INVALID_ENUM);
        }

        switch(zpass)
        {
          case GL_ZERO:
          case GL_KEEP:
          case GL_REPLACE:
          case GL_INCR:
          case GL_DECR:
          case GL_INVERT:
          case GL_INCR_WRAP:
          case GL_DECR_WRAP:
            break;
          default:
            return error(GL_INVALID_ENUM);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            if(face == GL_FRONT || face == GL_FRONT_AND_BACK)
            {
                context->setStencilOperations(fail, zfail, zpass);
            }

            if(face == GL_BACK || face == GL_FRONT_AND_BACK)
            {
                context->setStencilBackOperations(fail, zfail, zpass);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

GLboolean GL_APIENTRY glTestFenceNV(GLuint fence)
{
    TRACE("(GLuint fence = %d)", fence);

    try
    {
        rad::Context *context = rad::getContext();

        if(context)
        {
            rad::Fence *fenceObject = context->getFence(fence);

            if(fenceObject == NULL)
            {
                return error(GL_INVALID_OPERATION, GL_TRUE);
            }

            return fenceObject->testFence();
        }
    }
    catch(std::bad_alloc&)
    {
        error(GL_OUT_OF_MEMORY);
    }
    
    return GL_TRUE;
}

void GL_APIENTRY glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height,
                              GLint border, GLenum format, GLenum type, const GLvoid* pixels)
{
    TRACE("(GLenum target = 0x%X, GLint level = %d, GLint internalformat = %d, GLsizei width = %d, GLsizei height = %d, "
          "GLint border = %d, GLenum format = 0x%X, GLenum type = 0x%X, const GLvoid* pixels =  0x%0.8p)",
          target, level, internalformat, width, height, border, format, type, pixels);

    try
    {
        if(!validImageSize(level, width, height))
        {
            return error(GL_INVALID_VALUE);
        }

        if(internalformat != format)
        {
            return error(GL_INVALID_OPERATION);
        }

        switch(format)
        {
        case GL_ALPHA:
        case GL_LUMINANCE:
        case GL_LUMINANCE_ALPHA:
            switch(type)
            {
            case GL_UNSIGNED_BYTE:
            case GL_FLOAT:
            case GL_HALF_FLOAT_OES:
                break;
            default:
                return error(GL_INVALID_ENUM);
            }
            break;
        case GL_RGB:
            switch(type)
            {
            case GL_UNSIGNED_BYTE:
            case GL_UNSIGNED_SHORT_5_6_5:
            case GL_FLOAT:
            case GL_HALF_FLOAT_OES:
                break;
            default:
                return error(GL_INVALID_ENUM);
            }
            break;
        case GL_RGBA:
            switch(type)
            {
            case GL_UNSIGNED_BYTE:
            case GL_UNSIGNED_SHORT_4_4_4_4:
            case GL_UNSIGNED_SHORT_5_5_5_1:
            case GL_FLOAT:
            case GL_HALF_FLOAT_OES:
                break;
            default:
                return error(GL_INVALID_ENUM);
            }
            break;
        case GL_BGRA_EXT:
            switch(type)
            {
            case GL_UNSIGNED_BYTE:
                break;
            default:
                return error(GL_INVALID_ENUM);
            }
            break;
        case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:  // error cases for compressed textures are handled below
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
		case GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE:
		case GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE:
            break;
		case GL_DEPTH_COMPONENT:
			switch(type)
			{
			case GL_UNSIGNED_SHORT:
			case GL_UNSIGNED_INT:
				break;
			default:
				return error(GL_INVALID_ENUM);
			}
			break;
		case GL_DEPTH_STENCIL_OES:
			switch(type)
			{
			case GL_UNSIGNED_INT_24_8_OES:
				break;
			default:
				return error(GL_INVALID_ENUM);
			}
			break;
        default:
            return error(GL_INVALID_VALUE);
        }

        if(border != 0)
        {
            return error(GL_INVALID_VALUE);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            switch(target)
            {
              case GL_TEXTURE_2D:
                if(width > (rad::IMPLEMENTATION_MAX_TEXTURE_SIZE >> level) ||
                   height > (rad::IMPLEMENTATION_MAX_TEXTURE_SIZE >> level))
                {
                    return error(GL_INVALID_VALUE);
                }
                break;
              case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
              case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
              case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
              case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
              case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
              case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
                if(width != height)
                {
                    return error(GL_INVALID_VALUE);
                }

                if(width > (rad::IMPLEMENTATION_MAX_CUBE_MAP_TEXTURE_SIZE >> level) ||
                   height > (rad::IMPLEMENTATION_MAX_CUBE_MAP_TEXTURE_SIZE >> level))
                {
                    return error(GL_INVALID_VALUE);
                }
                break;
              default:
                return error(GL_INVALID_ENUM);
            }

            if(format == GL_COMPRESSED_RGB_S3TC_DXT1_EXT ||
               format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT ||
			   format == GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE ||
			   format == GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE)
            {
                if(S3TC_SUPPORT)
                {
                    return error(GL_INVALID_OPERATION);
                }
                else
                {
                    return error(GL_INVALID_ENUM);
                }
            }
			
            if(target == GL_TEXTURE_2D)
            {
                rad::Texture2D *texture = context->getTexture2D();

                if(!texture)
                {
                    return error(GL_INVALID_OPERATION);
                }

                texture->setImage(level, width, height, format, type, context->getUnpackAlignment(), pixels);
            }
            else
            {
                rad::TextureCubeMap *texture = context->getTextureCubeMap();

                if(!texture)
                {
                    return error(GL_INVALID_OPERATION);
                }

				texture->setImage(target, level, width, height, format, type, context->getUnpackAlignment(), pixels);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glTexParameterf(GLenum target, GLenum pname, GLfloat param)
{
    TRACE("(GLenum target = 0x%X, GLenum pname = 0x%X, GLfloat param = %f)", target, pname, param);

    try
    {
        rad::Context *context = rad::getContext();

        if(context)
        {
            rad::Texture *texture;

            switch(target)
            {
            case GL_TEXTURE_2D:
                texture = context->getTexture2D();
                break;
            case GL_TEXTURE_CUBE_MAP:
                texture = context->getTextureCubeMap();
                break;
            case GL_TEXTURE_EXTERNAL_OES:
                texture = context->getTextureExternal();
                break;
            default:
                return error(GL_INVALID_ENUM);
            }

            switch(pname)
            {
            case GL_TEXTURE_WRAP_S:
                if(!texture->setWrapS((GLenum)param))
                {
                    return error(GL_INVALID_ENUM);
                }
                break;
            case GL_TEXTURE_WRAP_T:
                if(!texture->setWrapT((GLenum)param))
                {
                    return error(GL_INVALID_ENUM);
                }
                break;
            case GL_TEXTURE_MIN_FILTER:
                if(!texture->setMinFilter((GLenum)param))
                {
                    return error(GL_INVALID_ENUM);
                }
                break;
            case GL_TEXTURE_MAG_FILTER:
                if(!texture->setMagFilter((GLenum)param))
                {
                    return error(GL_INVALID_ENUM);
                }
                break;
            case GL_TEXTURE_MAX_ANISOTROPY_EXT:
                if(!texture->setMaxAnisotropy(param))
                {
                    return error(GL_INVALID_VALUE);
                }
                break;
            default:
                return error(GL_INVALID_ENUM);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glTexParameterfv(GLenum target, GLenum pname, const GLfloat* params)
{
    glTexParameterf(target, pname, *params);
}

void GL_APIENTRY glTexParameteri(GLenum target, GLenum pname, GLint param)
{
    TRACE("(GLenum target = 0x%X, GLenum pname = 0x%X, GLint param = %d)", target, pname, param);

    try
    {
        rad::Context *context = rad::getContext();

        if(context)
        {
            rad::Texture *texture;

            switch(target)
            {
            case GL_TEXTURE_2D:
                texture = context->getTexture2D();
                break;
            case GL_TEXTURE_CUBE_MAP:
                texture = context->getTextureCubeMap();
                break;
            case GL_TEXTURE_EXTERNAL_OES:
                  texture = context->getTextureExternal();
                  break;
            default:
                return error(GL_INVALID_ENUM);
            }

            switch(pname)
            {
            case GL_TEXTURE_WRAP_S:
                if(!texture->setWrapS((GLenum)param))
                {
                    return error(GL_INVALID_ENUM);
                }
                break;
            case GL_TEXTURE_WRAP_T:
                if(!texture->setWrapT((GLenum)param))
                {
                    return error(GL_INVALID_ENUM);
                }
                break;
            case GL_TEXTURE_MIN_FILTER:
                if(!texture->setMinFilter((GLenum)param))
                {
                    return error(GL_INVALID_ENUM);
                }
                break;
            case GL_TEXTURE_MAG_FILTER:
                if(!texture->setMagFilter((GLenum)param))
                {
                    return error(GL_INVALID_ENUM);
                }
                break;
			case GL_TEXTURE_MAX_ANISOTROPY_EXT:
                if(!texture->setMaxAnisotropy((GLfloat)param))
                {
                    return error(GL_INVALID_VALUE);
                }
                break;
            default:
                return error(GL_INVALID_ENUM);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glTexParameteriv(GLenum target, GLenum pname, const GLint* params)
{
    glTexParameteri(target, pname, *params);
}

void GL_APIENTRY glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                               GLenum format, GLenum type, const GLvoid* pixels)
{
    TRACE("(GLenum target = 0x%X, GLint level = %d, GLint xoffset = %d, GLint yoffset = %d, "
          "GLsizei width = %d, GLsizei height = %d, GLenum format = 0x%X, GLenum type = 0x%X, "
          "const GLvoid* pixels = 0x%0.8p)",
           target, level, xoffset, yoffset, width, height, format, type, pixels);

    try
    {
        if(!rad::IsTextureTarget(target))
        {
            return error(GL_INVALID_ENUM);
        }

        if(level < 0 || xoffset < 0 || yoffset < 0 || width < 0 || height < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        if(std::numeric_limits<GLsizei>::max() - xoffset < width || std::numeric_limits<GLsizei>::max() - yoffset < height)
        {
            return error(GL_INVALID_VALUE);
        }

        if(!rad::CheckTextureFormatType(format, type))
        {
            return error(GL_INVALID_ENUM);
        }

        if(width == 0 || height == 0 || pixels == NULL)
        {
            return;
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            if(level > rad::IMPLEMENTATION_MAX_TEXTURE_LEVELS)
            {
                return error(GL_INVALID_VALUE);
            }

            if(target == GL_TEXTURE_2D)
            {
                rad::Texture2D *texture = context->getTexture2D();

                if(validateSubImageParams(false, width, height, xoffset, yoffset, target, level, format, texture))
				{
					texture->subImage(level, xoffset, yoffset, width, height, format, type, context->getUnpackAlignment(), pixels);
				}
            }
            else if(rad::IsCubemapTextureTarget(target))
            {
                rad::TextureCubeMap *texture = context->getTextureCubeMap();
				
				if(validateSubImageParams(false, width, height, xoffset, yoffset, target, level, format, texture))
				{
					texture->subImage(target, level, xoffset, yoffset, width, height, format, type, context->getUnpackAlignment(), pixels);
				}
            }
            else
            {
                UNREACHABLE();
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glUniform1f(GLint location, GLfloat x)
{
    glUniform1fv(location, 1, &x);
}

void GL_APIENTRY glUniform1fv(GLint location, GLsizei count, const GLfloat* v)
{
    TRACE("(GLint location = %d, GLsizei count = %d, const GLfloat* v = 0x%0.8p)", location, count, v);

    try
    {
        if(count < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        if(location == -1)
        {
            return;
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            rad::Program *program = context->getCurrentProgram();

            if(!program)
            {
                return error(GL_INVALID_OPERATION);
            }

            if(!program->setUniform1fv(location, count, v))
            {
                return error(GL_INVALID_OPERATION);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glUniform1i(GLint location, GLint x)
{
    glUniform1iv(location, 1, &x);
}

void GL_APIENTRY glUniform1iv(GLint location, GLsizei count, const GLint* v)
{
    TRACE("(GLint location = %d, GLsizei count = %d, const GLint* v = 0x%0.8p)", location, count, v);

    try
    {
        if(count < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        if(location == -1)
        {
            return;
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            rad::Program *program = context->getCurrentProgram();

            if(!program)
            {
                return error(GL_INVALID_OPERATION);
            }

            if(!program->setUniform1iv(location, count, v))
            {
                return error(GL_INVALID_OPERATION);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glUniform2f(GLint location, GLfloat x, GLfloat y)
{
    GLfloat xy[2] = {x, y};

    glUniform2fv(location, 1, (GLfloat*)&xy);
}

void GL_APIENTRY glUniform2fv(GLint location, GLsizei count, const GLfloat* v)
{
    TRACE("(GLint location = %d, GLsizei count = %d, const GLfloat* v = 0x%0.8p)", location, count, v);

    try
    {
        if(count < 0)
        {
            return error(GL_INVALID_VALUE);
        }
        
        if(location == -1)
        {
            return;
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            rad::Program *program = context->getCurrentProgram();

            if(!program)
            {
                return error(GL_INVALID_OPERATION);
            }

            if(!program->setUniform2fv(location, count, v))
            {
                return error(GL_INVALID_OPERATION);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glUniform2i(GLint location, GLint x, GLint y)
{
    GLint xy[4] = {x, y};

    glUniform2iv(location, 1, (GLint*)&xy);
}

void GL_APIENTRY glUniform2iv(GLint location, GLsizei count, const GLint* v)
{
    TRACE("(GLint location = %d, GLsizei count = %d, const GLint* v = 0x%0.8p)", location, count, v);

    try
    {
        if(count < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        if(location == -1)
        {
            return;
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            rad::Program *program = context->getCurrentProgram();

            if(!program)
            {
                return error(GL_INVALID_OPERATION);
            }

            if(!program->setUniform2iv(location, count, v))
            {
                return error(GL_INVALID_OPERATION);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glUniform3f(GLint location, GLfloat x, GLfloat y, GLfloat z)
{
    GLfloat xyz[3] = {x, y, z};

    glUniform3fv(location, 1, (GLfloat*)&xyz);
}

void GL_APIENTRY glUniform3fv(GLint location, GLsizei count, const GLfloat* v)
{
    TRACE("(GLint location = %d, GLsizei count = %d, const GLfloat* v = 0x%0.8p)", location, count, v);

    try
    {
        if(count < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        if(location == -1)
        {
            return;
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            rad::Program *program = context->getCurrentProgram();

            if(!program)
            {
                return error(GL_INVALID_OPERATION);
            }

            if(!program->setUniform3fv(location, count, v))
            {
                return error(GL_INVALID_OPERATION);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glUniform3i(GLint location, GLint x, GLint y, GLint z)
{
    GLint xyz[3] = {x, y, z};

    glUniform3iv(location, 1, (GLint*)&xyz);
}

void GL_APIENTRY glUniform3iv(GLint location, GLsizei count, const GLint* v)
{
    TRACE("(GLint location = %d, GLsizei count = %d, const GLint* v = 0x%0.8p)", location, count, v);

    try
    {
        if(count < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        if(location == -1)
        {
            return;
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            rad::Program *program = context->getCurrentProgram();

            if(!program)
            {
                return error(GL_INVALID_OPERATION);
            }

            if(!program->setUniform3iv(location, count, v))
            {
                return error(GL_INVALID_OPERATION);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glUniform4f(GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    GLfloat xyzw[4] = {x, y, z, w};

    glUniform4fv(location, 1, (GLfloat*)&xyzw);
}

void GL_APIENTRY glUniform4fv(GLint location, GLsizei count, const GLfloat* v)
{
    TRACE("(GLint location = %d, GLsizei count = %d, const GLfloat* v = 0x%0.8p)", location, count, v);

    try
    {
        if(count < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        if(location == -1)
        {
            return;
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            rad::Program *program = context->getCurrentProgram();

            if(!program)
            {
                return error(GL_INVALID_OPERATION);
            }

            if(!program->setUniform4fv(location, count, v))
            {
                return error(GL_INVALID_OPERATION);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glUniform4i(GLint location, GLint x, GLint y, GLint z, GLint w)
{
    GLint xyzw[4] = {x, y, z, w};

    glUniform4iv(location, 1, (GLint*)&xyzw);
}

void GL_APIENTRY glUniform4iv(GLint location, GLsizei count, const GLint* v)
{
    TRACE("(GLint location = %d, GLsizei count = %d, const GLint* v = 0x%0.8p)", location, count, v);

    try
    {
        if(count < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        if(location == -1)
        {
            return;
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            rad::Program *program = context->getCurrentProgram();

            if(!program)
            {
                return error(GL_INVALID_OPERATION);
            }

            if(!program->setUniform4iv(location, count, v))
            {
                return error(GL_INVALID_OPERATION);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
    TRACE("(GLint location = %d, GLsizei count = %d, GLboolean transpose = %d, const GLfloat* value = 0x%0.8p)",
          location, count, transpose, value);

    try
    {
        if(count < 0 || transpose != GL_FALSE)
        {
            return error(GL_INVALID_VALUE);
        }

        if(location == -1)
        {
            return;
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            rad::Program *program = context->getCurrentProgram();

            if(!program)
            {
                return error(GL_INVALID_OPERATION);
            }

            if(!program->setUniformMatrix2fv(location, count, value))
            {
                return error(GL_INVALID_OPERATION);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
    TRACE("(GLint location = %d, GLsizei count = %d, GLboolean transpose = %d, const GLfloat* value = 0x%0.8p)",
          location, count, transpose, value);

    try
    {
        if(count < 0 || transpose != GL_FALSE)
        {
            return error(GL_INVALID_VALUE);
        }

        if(location == -1)
        {
            return;
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            rad::Program *program = context->getCurrentProgram();

            if(!program)
            {
                return error(GL_INVALID_OPERATION);
            }

            if(!program->setUniformMatrix3fv(location, count, value))
            {
                return error(GL_INVALID_OPERATION);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
    TRACE("(GLint location = %d, GLsizei count = %d, GLboolean transpose = %d, const GLfloat* value = 0x%0.8p)",
          location, count, transpose, value);

    try
    {
        if(count < 0 || transpose != GL_FALSE)
        {
            return error(GL_INVALID_VALUE);
        }

        if(location == -1)
        {
            return;
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            rad::Program *program = context->getCurrentProgram();

            if(!program)
            {
                return error(GL_INVALID_OPERATION);
            }

            if(!program->setUniformMatrix4fv(location, count, value))
            {
                return error(GL_INVALID_OPERATION);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glUseProgram(GLuint program)
{
    TRACE("(GLuint program = %d)", program);

    try
    {
        rad::Context *context = rad::getContext();

        if(context)
        {
            rad::Program *programObject = context->getProgram(program);

            if(!programObject && program != 0)
            {
                if(context->getShader(program))
                {
                    return error(GL_INVALID_OPERATION);
                }
                else
                {
                    return error(GL_INVALID_VALUE);
                }
            }

            if(program != 0 && !programObject->isLinked())
            {
                return error(GL_INVALID_OPERATION);
            }

            context->useProgram(program);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glValidateProgram(GLuint program)
{
    TRACE("(GLuint program = %d)", program);

    try
    {
        rad::Context *context = rad::getContext();

        if(context)
        {
            rad::Program *programObject = context->getProgram(program);

            if(!programObject)
            {
                if(context->getShader(program))
                {
                    return error(GL_INVALID_OPERATION);
                }
                else
                {
                    return error(GL_INVALID_VALUE);
                }
            }

            programObject->validate();
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glVertexAttrib1f(GLuint index, GLfloat x)
{
    TRACE("(GLuint index = %d, GLfloat x = %f)", index, x);

    try
    {
        if(index >= rad::MAX_VERTEX_ATTRIBS)
        {
            return error(GL_INVALID_VALUE);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            GLfloat vals[4] = { x, 0, 0, 1 };
            context->setVertexAttrib(index, vals);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glVertexAttrib1fv(GLuint index, const GLfloat* values)
{
    TRACE("(GLuint index = %d, const GLfloat* values = 0x%0.8p)", index, values);

    try
    {
        if(index >= rad::MAX_VERTEX_ATTRIBS)
        {
            return error(GL_INVALID_VALUE);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            GLfloat vals[4] = { values[0], 0, 0, 1 };
            context->setVertexAttrib(index, vals);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glVertexAttrib2f(GLuint index, GLfloat x, GLfloat y)
{
    TRACE("(GLuint index = %d, GLfloat x = %f, GLfloat y = %f)", index, x, y);

    try
    {
        if(index >= rad::MAX_VERTEX_ATTRIBS)
        {
            return error(GL_INVALID_VALUE);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            GLfloat vals[4] = { x, y, 0, 1 };
            context->setVertexAttrib(index, vals);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glVertexAttrib2fv(GLuint index, const GLfloat* values)
{
    TRACE("(GLuint index = %d, const GLfloat* values = 0x%0.8p)", index, values);

    try
    {
        if(index >= rad::MAX_VERTEX_ATTRIBS)
        {
            return error(GL_INVALID_VALUE);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            GLfloat vals[4] = { values[0], values[1], 0, 1 };
            context->setVertexAttrib(index, vals);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glVertexAttrib3f(GLuint index, GLfloat x, GLfloat y, GLfloat z)
{
    TRACE("(GLuint index = %d, GLfloat x = %f, GLfloat y = %f, GLfloat z = %f)", index, x, y, z);

    try
    {
        if(index >= rad::MAX_VERTEX_ATTRIBS)
        {
            return error(GL_INVALID_VALUE);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            GLfloat vals[4] = { x, y, z, 1 };
            context->setVertexAttrib(index, vals);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glVertexAttrib3fv(GLuint index, const GLfloat* values)
{
    TRACE("(GLuint index = %d, const GLfloat* values = 0x%0.8p)", index, values);

    try
    {
        if(index >= rad::MAX_VERTEX_ATTRIBS)
        {
            return error(GL_INVALID_VALUE);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            GLfloat vals[4] = { values[0], values[1], values[2], 1 };
            context->setVertexAttrib(index, vals);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glVertexAttrib4f(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    TRACE("(GLuint index = %d, GLfloat x = %f, GLfloat y = %f, GLfloat z = %f, GLfloat w = %f)", index, x, y, z, w);

    try
    {
        if(index >= rad::MAX_VERTEX_ATTRIBS)
        {
            return error(GL_INVALID_VALUE);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            GLfloat vals[4] = { x, y, z, w };
            context->setVertexAttrib(index, vals);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glVertexAttrib4fv(GLuint index, const GLfloat* values)
{
    TRACE("(GLuint index = %d, const GLfloat* values = 0x%0.8p)", index, values);

    try
    {
        if(index >= rad::MAX_VERTEX_ATTRIBS)
        {
            return error(GL_INVALID_VALUE);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            context->setVertexAttrib(index, values);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr)
{
    TRACE("(GLuint index = %d, GLint size = %d, GLenum type = 0x%X, "
          "GLboolean normalized = %d, GLsizei stride = %d, const GLvoid* ptr = 0x%0.8p)",
          index, size, type, normalized, stride, ptr);

    try
    {
        if(index >= rad::MAX_VERTEX_ATTRIBS)
        {
            return error(GL_INVALID_VALUE);
        }

        if(size < 1 || size > 4)
        {
            return error(GL_INVALID_VALUE);
        }

        switch(type)
        {
          case GL_BYTE:
          case GL_UNSIGNED_BYTE:
          case GL_SHORT:
          case GL_UNSIGNED_SHORT:
          case GL_FIXED:
          case GL_FLOAT:
            break;
          default:
            return error(GL_INVALID_ENUM);
        }

        if(stride < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            context->setVertexAttribState(index, context->getArrayBuffer(), size, type, (normalized == GL_TRUE), stride, ptr);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
    TRACE("(GLint x = %d, GLint y = %d, GLsizei width = %d, GLsizei height = %d)", x, y, width, height);

    try
    {
        if(width < 0 || height < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            context->setViewportParams(x, y, width, height);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glBlitFramebufferANGLE(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                                      GLbitfield mask, GLenum filter)
{
    TRACE("(GLint srcX0 = %d, GLint srcY0 = %d, GLint srcX1 = %d, GLint srcY1 = %d, "
          "GLint dstX0 = %d, GLint dstY0 = %d, GLint dstX1 = %d, GLint dstY1 = %d, "
          "GLbitfield mask = 0x%X, GLenum filter = 0x%X)",
          srcX0, srcY0, srcX1, srcX1, dstX0, dstY0, dstX1, dstY1, mask, filter);

    try
    {
        switch(filter)
        {
          case GL_NEAREST:
            break;
          default:
            return error(GL_INVALID_ENUM);
        }

        if((mask & ~(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)) != 0)
        {
            return error(GL_INVALID_VALUE);
        }

        if(srcX1 - srcX0 != dstX1 - dstX0 || srcY1 - srcY0 != dstY1 - dstY0)
        {
            ERR("Scaling and flipping in BlitFramebufferANGLE not supported by this implementation");
            return error(GL_INVALID_OPERATION);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            if(context->getReadFramebufferHandle() == context->getDrawFramebufferHandle())
            {
                ERR("Blits with the same source and destination framebuffer are not supported by this implementation.");
                return error(GL_INVALID_OPERATION);
            }

            context->blitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glTexImage3DOES(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth,
                               GLint border, GLenum format, GLenum type, const GLvoid* pixels)
{
    TRACE("(GLenum target = 0x%X, GLint level = %d, GLenum internalformat = 0x%X, "
          "GLsizei width = %d, GLsizei height = %d, GLsizei depth = %d, GLint border = %d, "
          "GLenum format = 0x%X, GLenum type = 0x%x, const GLvoid* pixels = 0x%0.8p)",
          target, level, internalformat, width, height, depth, border, format, type, pixels);

    try
    {
		UNIMPLEMENTED();   // FIXME
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void GL_APIENTRY glEGLImageTargetTexture2DOES(GLenum target, GLeglImageOES image)
{
    TRACE("(GLenum target = 0x%X, GLeglImageOES image = 0x%0.8p)", target, image);

    try
    {
        switch(target)
        {
        case GL_TEXTURE_EXTERNAL_OES:
            break;
        default:
            return error(GL_INVALID_ENUM);
        }

        if(!image)
        {
            return error(GL_INVALID_OPERATION);
        }

        rad::Context *context = rad::getContext();

        if(context)
        {
            rad::TextureExternal *texture = context->getTextureExternal();

            if(!texture)
            {
                return error(GL_INVALID_OPERATION);
            }

            rad::Image *glImage = static_cast<rad::Image*>(image);

            texture->setImage(glImage);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

RADdevice RADAPIENTRY radCreateDevice (void) {UNIMPLEMENTED(); return 0;}
void RADAPIENTRY radReferenceDevice (RADdevice device) {UNIMPLEMENTED();}
void RADAPIENTRY radReleaseDevice (RADdevice device) {UNIMPLEMENTED();}
RADuint RADAPIENTRY radGetTokenHeader (RADdevice device, RADtokenName name) {UNIMPLEMENTED(); return 0;}
RADqueue RADAPIENTRY radCreateQueue (RADdevice device, RADqueueType queuetype) {UNIMPLEMENTED(); return 0;}
void RADAPIENTRY radReferenceQueue (RADqueue queue) {UNIMPLEMENTED();}
void RADAPIENTRY radReleaseQueue (RADqueue queue) {UNIMPLEMENTED();}
void RADAPIENTRY radQueueTagBuffer (RADqueue queue, RADbuffer buffer) {UNIMPLEMENTED();}
void RADAPIENTRY radQueueTagTexture (RADqueue queue, RADtexture texture) {UNIMPLEMENTED();}
void RADAPIENTRY radQueueSubmitCommands (RADqueue queue, RADuint numCommands, const RADcommandHandle *handles) {UNIMPLEMENTED();}
void RADAPIENTRY radFlushQueue (RADqueue queue) {UNIMPLEMENTED();}
void RADAPIENTRY radFinishQueue (RADqueue queue) {UNIMPLEMENTED();}
void RADAPIENTRY radQueueViewport (RADqueue queue, RADint x, RADint y, RADint w, RADint h) {UNIMPLEMENTED();}
void RADAPIENTRY radQueueScissor (RADqueue queue, RADint x, RADint y, RADint w, RADint h) {UNIMPLEMENTED();}
void RADAPIENTRY radQueueCopyBufferToImage (RADqueue queue, RADbuffer buffer, RADintptr bufferOffset, RADtexture texture, RADint level, RADuint xoffset, RADuint yoffset, RADuint zoffset, RADsizei width, RADsizei height, RADsizei depth) {UNIMPLEMENTED();}
void RADAPIENTRY radQueueCopyImageToBuffer (RADqueue queue, RADbuffer buffer, RADintptr bufferOffset, RADtexture texture, RADint level, RADuint xoffset, RADuint yoffset, RADuint zoffset, RADsizei width, RADsizei height, RADsizei depth) {UNIMPLEMENTED();}
void RADAPIENTRY radQueueCopyBuffer (RADqueue queue, RADbuffer srcBuffer, RADintptr srcOffset, RADbuffer dstBuffer, RADintptr dstOffset, RADsizei size) {UNIMPLEMENTED();}
void RADAPIENTRY radQueueClearColor (RADqueue queue, RADuint index, const RADfloat *color) {UNIMPLEMENTED();}
void RADAPIENTRY radQueueClearDepth (RADqueue queue, RADfloat depth) {UNIMPLEMENTED();}
void RADAPIENTRY radQueueClearStencil (RADqueue queue, RADuint stencil) {UNIMPLEMENTED();}
void RADAPIENTRY radQueuePresent (RADqueue queue, RADtexture texture) {UNIMPLEMENTED();}
void RADAPIENTRY radQueueDrawArrays (RADqueue queue, RADprimitiveType mode, RADint first, RADsizei count) {UNIMPLEMENTED();}
void RADAPIENTRY radQueueDrawElements (RADqueue queue, RADprimitiveType mode, RADindexType type, RADsizei count, RADindexHandle indexHandle, RADuint offset) {UNIMPLEMENTED();}
void RADAPIENTRY radQueueBindPipeline (RADqueue queue, RADpipelineType pipelineType, RADpipelineHandle pipelineHandle) {UNIMPLEMENTED();}
void RADAPIENTRY radQueueBindGroup (RADqueue queue, RADbitfield stages, RADuint group, RADuint count, RADbindGroupHandle groupHandle, RADuint offset) {UNIMPLEMENTED();}
void RADAPIENTRY radQueueBeginPass (RADqueue queue, RADpass pass) {UNIMPLEMENTED();}
void RADAPIENTRY radQueueEndPass (RADqueue queue, RADpass pass) {UNIMPLEMENTED();}
void RADAPIENTRY radQueueSubmitDynamic (RADqueue queue, const void *dynamic, RADsizei length) {UNIMPLEMENTED();}
void RADAPIENTRY radQueueStencilValueMask (RADqueue queue, RADfaceBitfield faces, RADuint mask) {UNIMPLEMENTED();}
void RADAPIENTRY radQueueStencilMask (RADqueue queue, RADfaceBitfield faces, RADuint mask) {UNIMPLEMENTED();}
void RADAPIENTRY radQueueStencilRef (RADqueue queue, RADfaceBitfield faces, RADint ref) {UNIMPLEMENTED();}
void RADAPIENTRY radQueueBlendColor (RADqueue queue, const RADfloat *blendColor) {UNIMPLEMENTED();}
void RADAPIENTRY radQueuePointSize (RADqueue queue, RADfloat pointSize) {UNIMPLEMENTED();}
void RADAPIENTRY radQueueLineWidth (RADqueue queue, RADfloat lineWidth) {UNIMPLEMENTED();}
void RADAPIENTRY radQueuePolygonOffsetClamp (RADqueue queue, RADfloat factor, RADfloat units, RADfloat clamp) {UNIMPLEMENTED();}
void RADAPIENTRY radQueueSampleMask (RADqueue queue, RADuint mask) {UNIMPLEMENTED();}
RADprogram RADAPIENTRY radCreateProgram (RADdevice device) {UNIMPLEMENTED(); return 0;}
void RADAPIENTRY radReferenceProgram (RADprogram program) {UNIMPLEMENTED();}
void RADAPIENTRY radReleaseProgram (RADprogram program) {UNIMPLEMENTED();}
void RADAPIENTRY radProgramSource (RADprogram program, RADprogramFormat format, RADsizei length, const void *source) {UNIMPLEMENTED();}
RADbuffer RADAPIENTRY radCreateBuffer (RADdevice device) {UNIMPLEMENTED(); return 0;}
void RADAPIENTRY radReferenceBuffer (RADbuffer buffer) {UNIMPLEMENTED();}
void RADAPIENTRY radReleaseBuffer (RADbuffer buffer, RADtagMode tagMode) {UNIMPLEMENTED();}
void RADAPIENTRY radBufferAccess (RADbuffer buffer, RADbitfield access) {UNIMPLEMENTED();}
void RADAPIENTRY radBufferMapAccess (RADbuffer buffer, RADbitfield mapAccess) {UNIMPLEMENTED();}
void RADAPIENTRY radBufferStorage (RADbuffer buffer, RADsizei size) {UNIMPLEMENTED();}
void* RADAPIENTRY radMapBuffer (RADbuffer buffer) {UNIMPLEMENTED(); return 0;}
RADvertexHandle RADAPIENTRY radGetVertexHandle (RADbuffer buffer) {UNIMPLEMENTED(); return 0;}
RADindexHandle RADAPIENTRY radGetIndexHandle (RADbuffer buffer) {UNIMPLEMENTED(); return 0;}
RADuniformHandle RADAPIENTRY radGetUniformHandle (RADbuffer buffer) {UNIMPLEMENTED(); return 0;}
RADbindGroupHandle RADAPIENTRY radGetBindGroupHandle (RADbuffer buffer) {UNIMPLEMENTED(); return 0;}
RADtexture RADAPIENTRY radCreateTexture (RADdevice device) {UNIMPLEMENTED(); return 0;}
void RADAPIENTRY radReferenceTexture (RADtexture texture) {UNIMPLEMENTED();}
void RADAPIENTRY radReleaseTexture (RADtexture texture, RADtagMode tagMode) {UNIMPLEMENTED();}
void RADAPIENTRY radTextureAccess (RADtexture texture, RADbitfield access) {UNIMPLEMENTED();}
void RADAPIENTRY radTextureStorage (RADtexture texture, RADtextureTarget target, RADsizei levels, RADinternalFormat internalFormat, RADsizei width, RADsizei height, RADsizei depth, RADsizei samples) {UNIMPLEMENTED();}
RADtextureHandle RADAPIENTRY radGetTextureSamplerHandle (RADtexture texture, RADsampler sampler, RADtextureTarget target, RADinternalFormat internalFormat, RADuint minLevel, RADuint numLevels, RADuint minLayer, RADuint numLayers) {UNIMPLEMENTED(); return 0;}
RADrenderTargetHandle RADAPIENTRY radGetTextureRenderTargetHandle (RADtexture texture, RADtextureTarget target, RADinternalFormat internalFormat, RADuint level, RADuint minLayer, RADuint numLayers) {UNIMPLEMENTED(); return 0;}
RADsampler RADAPIENTRY radCreateSampler (RADdevice device) {UNIMPLEMENTED(); return 0;}
void RADAPIENTRY radReferenceSampler (RADsampler sampler) {UNIMPLEMENTED();}
void RADAPIENTRY radReleaseSampler (RADsampler sampler) {UNIMPLEMENTED();}
void RADAPIENTRY radSamplerDefault (RADsampler sampler) {UNIMPLEMENTED();}
void RADAPIENTRY radSamplerMinMagFilter (RADsampler sampler, RADminFilter min, RADmagFilter mag) {UNIMPLEMENTED();}
void RADAPIENTRY radSamplerWrapMode (RADsampler sampler, RADwrapMode s, RADwrapMode t, RADwrapMode r) {UNIMPLEMENTED();}
void RADAPIENTRY radSamplerLodClamp (RADsampler sampler, RADfloat min, RADfloat max) {UNIMPLEMENTED();}
void RADAPIENTRY radSamplerLodBias (RADsampler sampler, RADfloat bias) {UNIMPLEMENTED();}
void RADAPIENTRY radSamplerCompare (RADsampler sampler, RADcompareMode mode, RADcompareFunc func) {UNIMPLEMENTED();}
void RADAPIENTRY radSamplerBorderColorFloat (RADsampler sampler, const RADfloat *borderColor) {UNIMPLEMENTED();}
void RADAPIENTRY radSamplerBorderColorInt (RADsampler sampler, const RADuint *borderColor) {UNIMPLEMENTED();}
RADcolorState RADAPIENTRY radCreateColorState (RADdevice device) {UNIMPLEMENTED(); return 0;}
void RADAPIENTRY radReferenceColorState (RADcolorState color) {UNIMPLEMENTED();}
void RADAPIENTRY radReleaseColorState (RADcolorState color) {UNIMPLEMENTED();}
void RADAPIENTRY radColorDefault (RADcolorState color) {UNIMPLEMENTED();}
void RADAPIENTRY radColorBlendEnable (RADcolorState color, RADuint index, RADboolean enable) {UNIMPLEMENTED();}
void RADAPIENTRY radColorBlendFunc (RADcolorState color, RADuint index, RADblendFunc srcFunc, RADblendFunc dstFunc, RADblendFunc srcFuncAlpha, RADblendFunc dstFuncAlpha) {UNIMPLEMENTED();}
void RADAPIENTRY radColorBlendEquation (RADcolorState color, RADuint index, RADblendEquation modeRGB, RADblendEquation modeAlpha) {UNIMPLEMENTED();}
void RADAPIENTRY radColorMask (RADcolorState color, RADuint index, RADboolean r, RADboolean g, RADboolean b, RADboolean a) {UNIMPLEMENTED();}
void RADAPIENTRY radColorNumTargets (RADcolorState color, RADuint numTargets) {UNIMPLEMENTED();}
void RADAPIENTRY radColorLogicOpEnable (RADcolorState color, RADboolean enable) {UNIMPLEMENTED();}
void RADAPIENTRY radColorLogicOp (RADcolorState color, RADlogicOp logicOp) {UNIMPLEMENTED();}
void RADAPIENTRY radColorAlphaToCoverageEnable (RADcolorState color, RADboolean enable) {UNIMPLEMENTED();}
void RADAPIENTRY radColorBlendColor (RADcolorState color, const RADfloat *blendColor) {UNIMPLEMENTED();}
void RADAPIENTRY radColorDynamic (RADcolorState color, RADcolorDynamic dynamic, RADboolean enable) {UNIMPLEMENTED();}
RADrasterState RADAPIENTRY radCreateRasterState (RADdevice device) {UNIMPLEMENTED(); return 0;}
void RADAPIENTRY radReferenceRasterState (RADrasterState raster) {UNIMPLEMENTED();}
void RADAPIENTRY radReleaseRasterState (RADrasterState raster) {UNIMPLEMENTED();}
void RADAPIENTRY radRasterDefault (RADrasterState raster) {UNIMPLEMENTED();}
void RADAPIENTRY radRasterPointSize (RADrasterState raster, RADfloat pointSize) {UNIMPLEMENTED();}
void RADAPIENTRY radRasterLineWidth (RADrasterState raster, RADfloat lineWidth) {UNIMPLEMENTED();}
void RADAPIENTRY radRasterCullFace (RADrasterState raster, RADfaceBitfield face) {UNIMPLEMENTED();}
void RADAPIENTRY radRasterFrontFace (RADrasterState raster, RADfrontFace face) {UNIMPLEMENTED();}
void RADAPIENTRY radRasterPolygonMode (RADrasterState raster, RADpolygonMode polygonMode) {UNIMPLEMENTED();}
void RADAPIENTRY radRasterPolygonOffsetClamp (RADrasterState raster, RADfloat factor, RADfloat units, RADfloat clamp) {UNIMPLEMENTED();}
void RADAPIENTRY radRasterPolygonOffsetEnables (RADrasterState raster, RADpolygonOffsetEnables enables) {UNIMPLEMENTED();}
void RADAPIENTRY radRasterDiscardEnable (RADrasterState raster, RADboolean enable) {UNIMPLEMENTED();}
void RADAPIENTRY radRasterMultisampleEnable (RADrasterState raster, RADboolean enable) {UNIMPLEMENTED();}
void RADAPIENTRY radRasterSamples (RADrasterState raster, RADuint samples) {UNIMPLEMENTED();}
void RADAPIENTRY radRasterSampleMask (RADrasterState raster, RADuint mask) {UNIMPLEMENTED();}
void RADAPIENTRY radRasterDynamic (RADrasterState raster, RADrasterDynamic dynamic, RADboolean enable) {UNIMPLEMENTED();}
RADdepthStencilState RADAPIENTRY radCreateDepthStencilState (RADdevice device) {UNIMPLEMENTED(); return 0;}
void RADAPIENTRY radReferenceDepthStencilState (RADdepthStencilState depthStencil) {UNIMPLEMENTED();}
void RADAPIENTRY radReleaseDepthStencilState (RADdepthStencilState depthStencil) {UNIMPLEMENTED();}
void RADAPIENTRY radDepthStencilDefault (RADdepthStencilState depthStencil) {UNIMPLEMENTED();}
void RADAPIENTRY radDepthStencilDepthTestEnable (RADdepthStencilState depthStencil, RADboolean enable) {UNIMPLEMENTED();}
void RADAPIENTRY radDepthStencilDepthWriteEnable (RADdepthStencilState depthStencil, RADboolean enable) {UNIMPLEMENTED();}
void RADAPIENTRY radDepthStencilDepthFunc (RADdepthStencilState depthStencil, RADdepthFunc func) {UNIMPLEMENTED();}
void RADAPIENTRY radDepthStencilStencilTestEnable (RADdepthStencilState depthStencil, RADboolean enable) {UNIMPLEMENTED();}
void RADAPIENTRY radDepthStencilStencilFunc (RADdepthStencilState depthStencil, RADfaceBitfield faces, RADstencilFunc func, RADint ref, RADuint mask) {UNIMPLEMENTED();}
void RADAPIENTRY radDepthStencilStencilOp (RADdepthStencilState depthStencil, RADfaceBitfield faces, RADstencilOp fail, RADstencilOp depthFail, RADstencilOp depthPass) {UNIMPLEMENTED();}
void RADAPIENTRY radDepthStencilStencilMask (RADdepthStencilState depthStencil, RADfaceBitfield faces, RADuint mask) {UNIMPLEMENTED();}
void RADAPIENTRY radDepthStencilDynamic (RADdepthStencilState depthStencil, RADdepthStencilDynamic dynamic, RADboolean enable) {UNIMPLEMENTED();}
RADvertexState RADAPIENTRY radCreateVertexState (RADdevice device) {UNIMPLEMENTED(); return 0;}
void RADAPIENTRY radReferenceVertexState (RADvertexState vertex) {UNIMPLEMENTED();}
void RADAPIENTRY radReleaseVertexState (RADvertexState vertex) {UNIMPLEMENTED();}
void RADAPIENTRY radVertexDefault (RADvertexState vertex) {UNIMPLEMENTED();}
void RADAPIENTRY radVertexAttribFormat (RADvertexState vertex, RADint attribIndex, RADint numComponents, RADint bytesPerComponent, RADattribType type, RADuint relativeOffset) {UNIMPLEMENTED();}
void RADAPIENTRY radVertexAttribBinding (RADvertexState vertex, RADint attribIndex, RADint bindingIndex) {UNIMPLEMENTED();}
void RADAPIENTRY radVertexBindingGroup (RADvertexState vertex, RADint bindingIndex, RADint group, RADint index) {UNIMPLEMENTED();}
void RADAPIENTRY radVertexAttribEnable (RADvertexState vertex, RADint attribIndex, RADboolean enable) {UNIMPLEMENTED();}
void RADAPIENTRY radVertexBindingStride (RADvertexState vertex, RADint bindingIndex, RADuint stride) {UNIMPLEMENTED();}
RADrtFormatState RADAPIENTRY radCreateRtFormatState (RADdevice device) {UNIMPLEMENTED(); return 0;}
void RADAPIENTRY radReferenceRtFormatState (RADrtFormatState rtFormat) {UNIMPLEMENTED();}
void RADAPIENTRY radReleaseRtFormatState (RADrtFormatState rtFormat) {UNIMPLEMENTED();}
void RADAPIENTRY radRtFormatDefault (RADrtFormatState rtFormat) {UNIMPLEMENTED();}
void RADAPIENTRY radRtFormatColorFormat (RADrtFormatState rtFormat, RADuint index, RADinternalFormat format) {UNIMPLEMENTED();}
void RADAPIENTRY radRtFormatDepthFormat (RADrtFormatState rtFormat, RADinternalFormat format) {UNIMPLEMENTED();}
void RADAPIENTRY radRtFormatStencilFormat (RADrtFormatState rtFormat, RADinternalFormat format) {UNIMPLEMENTED();}
void RADAPIENTRY radRtFormatColorSamples (RADrtFormatState rtFormat, RADuint samples) {UNIMPLEMENTED();}
void RADAPIENTRY radRtFormatDepthStencilSamples (RADrtFormatState rtFormat, RADuint samples) {UNIMPLEMENTED();}
RADpipeline RADAPIENTRY radCreatePipeline (RADdevice device, RADpipelineType pipelineType) {UNIMPLEMENTED(); return 0;}
void RADAPIENTRY radReferencePipeline (RADpipeline pipeline) {UNIMPLEMENTED();}
void RADAPIENTRY radReleasePipeline (RADpipeline pipeline) {UNIMPLEMENTED();}
void RADAPIENTRY radPipelineProgramStages (RADpipeline pipeline, RADbitfield stages, RADprogram program) {UNIMPLEMENTED();}
void RADAPIENTRY radPipelineVertexState (RADpipeline pipeline, RADvertexState vertex) {UNIMPLEMENTED();}
void RADAPIENTRY radPipelineColorState (RADpipeline pipeline, RADcolorState color) {UNIMPLEMENTED();}
void RADAPIENTRY radPipelineRasterState (RADpipeline pipeline, RADrasterState raster) {UNIMPLEMENTED();}
void RADAPIENTRY radPipelineDepthStencilState (RADpipeline pipeline, RADdepthStencilState depthStencil) {UNIMPLEMENTED();}
void RADAPIENTRY radPipelineRtFormatState (RADpipeline pipeline, RADrtFormatState rtFormat) {UNIMPLEMENTED();}
void RADAPIENTRY radPipelinePrimitiveType (RADpipeline pipeline, RADprimitiveType mode) {UNIMPLEMENTED();}
void RADAPIENTRY radCompilePipeline (RADpipeline pipeline) {UNIMPLEMENTED();}
RADpipelineHandle RADAPIENTRY radGetPipelineHandle (RADpipeline pipeline) {UNIMPLEMENTED(); return 0;}
RADcommandBuffer RADAPIENTRY radCreateCommandBuffer (RADdevice device, RADqueueType queueType) {UNIMPLEMENTED(); return 0;}
void RADAPIENTRY radReferenceCommandBuffer (RADcommandBuffer cmdBuf) {UNIMPLEMENTED();}
void RADAPIENTRY radReleaseCommandBuffer (RADcommandBuffer cmdBuf) {UNIMPLEMENTED();}
void RADAPIENTRY radCmdBindPipeline (RADcommandBuffer cmdBuf, RADpipelineType pipelineType, RADpipelineHandle pipelineHandle) {UNIMPLEMENTED();}
void RADAPIENTRY radCmdBindGroup (RADcommandBuffer cmdBuf, RADbitfield stages, RADuint group, RADuint count, RADbindGroupHandle groupHandle, RADuint offset) {UNIMPLEMENTED();}
void RADAPIENTRY radCmdDrawArrays (RADcommandBuffer cmdBuf, RADprimitiveType mode, RADint first, RADsizei count) {UNIMPLEMENTED();}
void RADAPIENTRY radCmdDrawElements (RADcommandBuffer cmdBuf, RADprimitiveType mode, RADindexType type, RADsizei count, RADindexHandle indexHandle, RADuint offset) {UNIMPLEMENTED();}
RADboolean RADAPIENTRY radCompileCommandBuffer (RADcommandBuffer cmdBuf) {UNIMPLEMENTED(); return 0;}
RADcommandHandle RADAPIENTRY radGetCommandHandle (RADcommandBuffer cmdBuf) {UNIMPLEMENTED(); return 0;}
void RADAPIENTRY radCmdStencilValueMask (RADcommandBuffer cmdBuf, RADfaceBitfield faces, RADuint mask) {UNIMPLEMENTED();}
void RADAPIENTRY radCmdStencilMask (RADcommandBuffer cmdBuf, RADfaceBitfield faces, RADuint mask) {UNIMPLEMENTED();}
void RADAPIENTRY radCmdStencilRef (RADcommandBuffer cmdBuf, RADfaceBitfield faces, RADint ref) {UNIMPLEMENTED();}
void RADAPIENTRY radCmdBlendColor (RADcommandBuffer cmdBuf, const RADfloat *blendColor) {UNIMPLEMENTED();}
void RADAPIENTRY radCmdPointSize (RADcommandBuffer cmdBuf, RADfloat pointSize) {UNIMPLEMENTED();}
void RADAPIENTRY radCmdLineWidth (RADcommandBuffer cmdBuf, RADfloat lineWidth) {UNIMPLEMENTED();}
void RADAPIENTRY radCmdPolygonOffsetClamp (RADcommandBuffer cmdBuf, RADfloat factor, RADfloat units, RADfloat clamp) {UNIMPLEMENTED();}
void RADAPIENTRY radCmdSampleMask (RADcommandBuffer cmdBuf, RADuint mask) {UNIMPLEMENTED();}
RADpass RADAPIENTRY radCreatePass (RADdevice device) {UNIMPLEMENTED(); return 0;}
void RADAPIENTRY radReferencePass (RADpass pass) {UNIMPLEMENTED();}
void RADAPIENTRY radReleasePass (RADpass pass) {UNIMPLEMENTED();}
void RADAPIENTRY radPassDefault (RADpass pass) {UNIMPLEMENTED();}
void RADAPIENTRY radCompilePass (RADpass pass) {UNIMPLEMENTED();}
void RADAPIENTRY radPassRenderTargets (RADpass pass, RADuint numColors, const RADrenderTargetHandle *colors, RADrenderTargetHandle depth, RADrenderTargetHandle stencil) {UNIMPLEMENTED();}
void RADAPIENTRY radPassPreserveEnable (RADpass pass, RADrtAttachment attachment, RADboolean enable) {UNIMPLEMENTED();}
void RADAPIENTRY radPassDiscard (RADpass pass, RADuint numTextures, const RADtexture *textures, const RADoffset2D *offsets) {UNIMPLEMENTED();}
void RADAPIENTRY radPassResolve (RADpass pass, RADrtAttachment attachment, RADtexture texture) {UNIMPLEMENTED();}
void RADAPIENTRY radPassStore (RADpass pass, RADuint numTextures, const RADtexture *textures, const RADoffset2D *offsets) {UNIMPLEMENTED();}
void RADAPIENTRY radPassClip (RADpass pass, const RADrect2D *rect) {UNIMPLEMENTED();}
void RADAPIENTRY radPassDependencies (RADpass pass, RADuint numPasses, const RADpass *otherPasses, const RADbitfield *srcMask, const RADbitfield *dstMask, const RADbitfield *flushMask, const RADbitfield *invalidateMask) {UNIMPLEMENTED();}
void RADAPIENTRY radPassTilingBoundary (RADpass pass, RADboolean boundary) {UNIMPLEMENTED();}
void RADAPIENTRY radPassTileFilterWidth (RADpass pass, RADuint filterWidth, RADuint filterHeight) {UNIMPLEMENTED();}
void RADAPIENTRY radPassTileFootprint (RADpass pass, RADuint bytesPerPixel, RADuint maxFilterWidth, RADuint maxFilterHeight) {UNIMPLEMENTED();}
RADsync RADAPIENTRY radCreateSync (RADdevice device) {UNIMPLEMENTED(); return 0;}
void RADAPIENTRY radReferenceSync (RADsync sync) {UNIMPLEMENTED();}
void RADAPIENTRY radReleaseSync (RADsync sync) {UNIMPLEMENTED();}
void RADAPIENTRY radQueueFenceSync (RADqueue queue, RADsync sync, RADsyncCondition condition, RADbitfield flags) {UNIMPLEMENTED();}
RADwaitSyncResult RADAPIENTRY radWaitSync (RADsync sync, RADuint64 timeout) {UNIMPLEMENTED(); return RAD_WAIT_SYNC_FAILED;}
RADboolean RADAPIENTRY radQueueWaitSync (RADqueue queue, RADsync sync) {UNIMPLEMENTED(); return 0;}

RADPROC RADAPIENTRY radGetProcAddress(const RADchar *procname)
{
	struct Extension
    {
        const char *name;
        RADPROC address;
    };

    static const Extension glExtensions[] =
    {
        #define EXTENSION(name) {#name, (RADPROC)name}
		
		EXTENSION(radGetProcAddress),
		EXTENSION(radCreateDevice),
		EXTENSION(radReferenceDevice),
		EXTENSION(radReleaseDevice),
		EXTENSION(radGetTokenHeader),
		EXTENSION(radCreateQueue),
		EXTENSION(radReferenceQueue),
		EXTENSION(radReleaseQueue),
		EXTENSION(radQueueTagBuffer),
		EXTENSION(radQueueTagTexture),
		EXTENSION(radQueueSubmitCommands),
		EXTENSION(radFlushQueue),
		EXTENSION(radFinishQueue),
		EXTENSION(radQueueViewport),
		EXTENSION(radQueueScissor),
		EXTENSION(radQueueCopyBufferToImage),
		EXTENSION(radQueueCopyImageToBuffer),
		EXTENSION(radQueueCopyBuffer),
		EXTENSION(radQueueClearColor),
		EXTENSION(radQueueClearDepth),
		EXTENSION(radQueueClearStencil),
		EXTENSION(radQueuePresent),
		EXTENSION(radQueueDrawArrays),
		EXTENSION(radQueueDrawElements),
		EXTENSION(radQueueBindPipeline),
		EXTENSION(radQueueBindGroup),
		EXTENSION(radQueueBeginPass),
		EXTENSION(radQueueEndPass),
		EXTENSION(radQueueSubmitDynamic),
		EXTENSION(radQueueStencilValueMask),
		EXTENSION(radQueueStencilMask),
		EXTENSION(radQueueStencilRef),
		EXTENSION(radQueueBlendColor),
		EXTENSION(radQueuePointSize),
		EXTENSION(radQueueLineWidth),
		EXTENSION(radQueuePolygonOffsetClamp),
		EXTENSION(radQueueSampleMask),
		EXTENSION(radCreateProgram),
		EXTENSION(radReferenceProgram),
		EXTENSION(radReleaseProgram),
		EXTENSION(radProgramSource),
		EXTENSION(radCreateBuffer),
		EXTENSION(radReferenceBuffer),
		EXTENSION(radReleaseBuffer),
		EXTENSION(radBufferAccess),
		EXTENSION(radBufferMapAccess),
		EXTENSION(radBufferStorage),
		EXTENSION(radMapBuffer),
		EXTENSION(radGetVertexHandle),
		EXTENSION(radGetIndexHandle),
		EXTENSION(radGetUniformHandle),
		EXTENSION(radGetBindGroupHandle),
		EXTENSION(radCreateTexture),
		EXTENSION(radReferenceTexture),
		EXTENSION(radReleaseTexture),
		EXTENSION(radTextureAccess),
		EXTENSION(radTextureStorage),
		EXTENSION(radGetTextureSamplerHandle),
		EXTENSION(radGetTextureRenderTargetHandle),
		EXTENSION(radCreateSampler),
		EXTENSION(radReferenceSampler),
		EXTENSION(radReleaseSampler),
		EXTENSION(radSamplerDefault),
		EXTENSION(radSamplerMinMagFilter),
		EXTENSION(radSamplerWrapMode),
		EXTENSION(radSamplerLodClamp),
		EXTENSION(radSamplerLodBias),
		EXTENSION(radSamplerCompare),
		EXTENSION(radSamplerBorderColorFloat),
		EXTENSION(radSamplerBorderColorInt),
		EXTENSION(radCreateColorState),
		EXTENSION(radReferenceColorState),
		EXTENSION(radReleaseColorState),
		EXTENSION(radColorDefault),
		EXTENSION(radColorBlendEnable),
		EXTENSION(radColorBlendFunc),
		EXTENSION(radColorBlendEquation),
		EXTENSION(radColorMask),
		EXTENSION(radColorNumTargets),
		EXTENSION(radColorLogicOpEnable),
		EXTENSION(radColorLogicOp),
		EXTENSION(radColorAlphaToCoverageEnable),
		EXTENSION(radColorBlendColor),
		EXTENSION(radColorDynamic),
		EXTENSION(radCreateRasterState),
		EXTENSION(radReferenceRasterState),
		EXTENSION(radReleaseRasterState),
		EXTENSION(radRasterDefault),
		EXTENSION(radRasterPointSize),
		EXTENSION(radRasterLineWidth),
		EXTENSION(radRasterCullFace),
		EXTENSION(radRasterFrontFace),
		EXTENSION(radRasterPolygonMode),
		EXTENSION(radRasterPolygonOffsetClamp),
		EXTENSION(radRasterPolygonOffsetEnables),
		EXTENSION(radRasterDiscardEnable),
		EXTENSION(radRasterMultisampleEnable),
		EXTENSION(radRasterSamples),
		EXTENSION(radRasterSampleMask),
		EXTENSION(radRasterDynamic),
		EXTENSION(radCreateDepthStencilState),
		EXTENSION(radReferenceDepthStencilState),
		EXTENSION(radReleaseDepthStencilState),
		EXTENSION(radDepthStencilDefault),
		EXTENSION(radDepthStencilDepthTestEnable),
		EXTENSION(radDepthStencilDepthWriteEnable),
		EXTENSION(radDepthStencilDepthFunc),
		EXTENSION(radDepthStencilStencilTestEnable),
		EXTENSION(radDepthStencilStencilFunc),
		EXTENSION(radDepthStencilStencilOp),
		EXTENSION(radDepthStencilStencilMask),
		EXTENSION(radDepthStencilDynamic),
		EXTENSION(radCreateVertexState),
		EXTENSION(radReferenceVertexState),
		EXTENSION(radReleaseVertexState),
		EXTENSION(radVertexDefault),
		EXTENSION(radVertexAttribFormat),
		EXTENSION(radVertexAttribBinding),
		EXTENSION(radVertexBindingGroup),
		EXTENSION(radVertexAttribEnable),
		EXTENSION(radVertexBindingStride),
		EXTENSION(radCreateRtFormatState),
		EXTENSION(radReferenceRtFormatState),
		EXTENSION(radReleaseRtFormatState),
		EXTENSION(radRtFormatDefault),
		EXTENSION(radRtFormatColorFormat),
		EXTENSION(radRtFormatDepthFormat),
		EXTENSION(radRtFormatStencilFormat),
		EXTENSION(radRtFormatColorSamples),
		EXTENSION(radRtFormatDepthStencilSamples),
		EXTENSION(radCreatePipeline),
		EXTENSION(radReferencePipeline),
		EXTENSION(radReleasePipeline),
		EXTENSION(radPipelineProgramStages),
		EXTENSION(radPipelineVertexState),
		EXTENSION(radPipelineColorState),
		EXTENSION(radPipelineRasterState),
		EXTENSION(radPipelineDepthStencilState),
		EXTENSION(radPipelineRtFormatState),
		EXTENSION(radPipelinePrimitiveType),
		EXTENSION(radCompilePipeline),
		EXTENSION(radGetPipelineHandle),
		EXTENSION(radCreateCommandBuffer),
		EXTENSION(radReferenceCommandBuffer),
		EXTENSION(radReleaseCommandBuffer),
		EXTENSION(radCmdBindPipeline),
		EXTENSION(radCmdBindGroup),
		EXTENSION(radCmdDrawArrays),
		EXTENSION(radCmdDrawElements),
		EXTENSION(radCompileCommandBuffer),
		EXTENSION(radGetCommandHandle),
		EXTENSION(radCmdStencilValueMask),
		EXTENSION(radCmdStencilMask),
		EXTENSION(radCmdStencilRef),
		EXTENSION(radCmdBlendColor),
		EXTENSION(radCmdPointSize),
		EXTENSION(radCmdLineWidth),
		EXTENSION(radCmdPolygonOffsetClamp),
		EXTENSION(radCmdSampleMask),
		EXTENSION(radCreatePass),
		EXTENSION(radReferencePass),
		EXTENSION(radReleasePass),
		EXTENSION(radPassDefault),
		EXTENSION(radCompilePass),
		EXTENSION(radPassRenderTargets),
		EXTENSION(radPassPreserveEnable),
		EXTENSION(radPassDiscard),
		EXTENSION(radPassResolve),
		EXTENSION(radPassStore),
		EXTENSION(radPassClip),
		EXTENSION(radPassDependencies),
		EXTENSION(radPassTilingBoundary),
		EXTENSION(radPassTileFilterWidth),
		EXTENSION(radPassTileFootprint),
		EXTENSION(radCreateSync),
		EXTENSION(radReferenceSync),
		EXTENSION(radReleaseSync),
		EXTENSION(radQueueFenceSync),
		EXTENSION(radWaitSync),
		EXTENSION(radQueueWaitSync),

		#undef EXTENSION
    };

    for(int ext = 0; ext < sizeof(glExtensions) / sizeof(Extension); ext++)
    {
        if(strcmp(procname, glExtensions[ext].name) == 0)
        {
            return (RADPROC)glExtensions[ext].address;
        }
    }

    return NULL;
}

__eglMustCastToProperFunctionPointerType glGetProcAddress(const char *procname)
{
    struct Extension
    {
        const char *name;
        __eglMustCastToProperFunctionPointerType address;
    };

    static const Extension glExtensions[] =
    {
		#define EXTENSION(name) {#name, (__eglMustCastToProperFunctionPointerType)name}

        EXTENSION(glTexImage3DOES),
        EXTENSION(glBlitFramebufferANGLE),
        EXTENSION(glRenderbufferStorageMultisampleANGLE),
        EXTENSION(glDeleteFencesNV),
        EXTENSION(glGenFencesNV),
        EXTENSION(glIsFenceNV),
        EXTENSION(glTestFenceNV),
        EXTENSION(glGetFenceivNV),
        EXTENSION(glFinishFenceNV),
        EXTENSION(glSetFenceNV),
		EXTENSION(glGetGraphicsResetStatusEXT),
        EXTENSION(glReadnPixelsEXT),
        EXTENSION(glGetnUniformfvEXT),
        EXTENSION(glGetnUniformivEXT),
		EXTENSION(glGenQueriesEXT),
        EXTENSION(glDeleteQueriesEXT),
        EXTENSION(glIsQueryEXT),
        EXTENSION(glBeginQueryEXT),
        EXTENSION(glEndQueryEXT),
        EXTENSION(glGetQueryivEXT),
        EXTENSION(glGetQueryObjectuivEXT),
        EXTENSION(glEGLImageTargetTexture2DOES),

		EXTENSION(radGetProcAddress),

		#undef EXTENSION
    };

    for(int ext = 0; ext < sizeof(glExtensions) / sizeof(Extension); ext++)
    {
        if(strcmp(procname, glExtensions[ext].name) == 0)
        {
            return (__eglMustCastToProperFunctionPointerType)glExtensions[ext].address;
        }
    }

    return NULL;
}

void GL_APIENTRY Register(const char *licenseKey)
{
	RegisterLicenseKey(licenseKey);
}

}
