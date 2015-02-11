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

// Framebuffer.cpp: Implements the Framebuffer class. Implements GL framebuffer
// objects and related functionality. [OpenGL ES 2.0.24] section 4.4 page 105.

#include "Framebuffer.h"

#include "main.h"
#include "Renderbuffer.h"
#include "Texture.h"
#include "utilities.h"

namespace gl
{

Framebuffer::Framebuffer()
{
	mColorbufferType = GL_NONE;
	mDepthbufferType = GL_NONE;
	mStencilbufferType = GL_NONE;
}

Framebuffer::~Framebuffer()
{
	mColorbufferPointer.set(NULL);
	mDepthbufferPointer.set(NULL);
	mStencilbufferPointer.set(NULL);
}

Renderbuffer *Framebuffer::lookupRenderbuffer(GLenum type, GLuint handle) const
{
	Context *context = getContext();
	Renderbuffer *buffer = NULL;

	if(type == GL_NONE)
	{
		buffer = NULL;
	}
	else if(type == GL_RENDERBUFFER)
	{
		buffer = context->getRenderbuffer(handle);
	}
	else if(IsTextureTarget(type))
	{
		buffer = context->getTexture(handle)->getRenderbuffer(type);
	}
	else
	{
		UNREACHABLE();
	}

	return buffer;
}

void Framebuffer::setColorbuffer(GLenum type, GLuint colorbuffer)
{
	mColorbufferType = (colorbuffer != 0) ? type : GL_NONE;
	mColorbufferPointer.set(lookupRenderbuffer(type, colorbuffer));
}

void Framebuffer::setDepthbuffer(GLenum type, GLuint depthbuffer)
{
	mDepthbufferType = (depthbuffer != 0) ? type : GL_NONE;
	mDepthbufferPointer.set(lookupRenderbuffer(type, depthbuffer));
}

void Framebuffer::setStencilbuffer(GLenum type, GLuint stencilbuffer)
{
	mStencilbufferType = (stencilbuffer != 0) ? type : GL_NONE;
	mStencilbufferPointer.set(lookupRenderbuffer(type, stencilbuffer));
}

void Framebuffer::detachTexture(GLuint texture)
{
	if(mColorbufferPointer.name() == texture && IsTextureTarget(mColorbufferType))
	{
		mColorbufferType = GL_NONE;
		mColorbufferPointer.set(NULL);
	}

	if(mDepthbufferPointer.name() == texture && IsTextureTarget(mDepthbufferType))
	{
		mDepthbufferType = GL_NONE;
		mDepthbufferPointer.set(NULL);
	}

	if(mStencilbufferPointer.name() == texture && IsTextureTarget(mStencilbufferType))
	{
		mStencilbufferType = GL_NONE;
		mStencilbufferPointer.set(NULL);
	}
}

void Framebuffer::detachRenderbuffer(GLuint renderbuffer)
{
	if(mColorbufferPointer.name() == renderbuffer && mColorbufferType == GL_RENDERBUFFER)
	{
		mColorbufferType = GL_NONE;
		mColorbufferPointer.set(NULL);
	}

	if(mDepthbufferPointer.name() == renderbuffer && mDepthbufferType == GL_RENDERBUFFER)
	{
		mDepthbufferType = GL_NONE;
		mDepthbufferPointer.set(NULL);
	}

	if(mStencilbufferPointer.name() == renderbuffer && mStencilbufferType == GL_RENDERBUFFER)
	{
		mStencilbufferType = GL_NONE;
		mStencilbufferPointer.set(NULL);
	}
}

// Increments refcount on surface.
// caller must Release() the returned surface
egl::Image *Framebuffer::getRenderTarget()
{
	Renderbuffer *colorbuffer = mColorbufferPointer.get();

	if(colorbuffer)
	{
		return colorbuffer->getRenderTarget();
	}

	return NULL;
}

// Increments refcount on surface.
// caller must Release() the returned surface
egl::Image *Framebuffer::getDepthStencil()
{
	Renderbuffer *depthstencilbuffer = mDepthbufferPointer.get();
	
	if(!depthstencilbuffer)
	{
		depthstencilbuffer = mStencilbufferPointer.get();
	}

	if(depthstencilbuffer)
	{
		return depthstencilbuffer->getRenderTarget();
	}

	return NULL;
}

Renderbuffer *Framebuffer::getColorbuffer()
{
	return mColorbufferPointer.get();
}

Renderbuffer *Framebuffer::getDepthbuffer()
{
	return mDepthbufferPointer.get();
}

Renderbuffer *Framebuffer::getStencilbuffer()
{
	return mStencilbufferPointer.get();
}

GLenum Framebuffer::getColorbufferType()
{
	return mColorbufferType;
}

GLenum Framebuffer::getDepthbufferType()
{
	return mDepthbufferType;
}

GLenum Framebuffer::getStencilbufferType()
{
	return mStencilbufferType;
}

GLuint Framebuffer::getColorbufferName()
{
	return mColorbufferPointer.name();
}

GLuint Framebuffer::getDepthbufferName()
{
	return mDepthbufferPointer.name();
}

GLuint Framebuffer::getStencilbufferName()
{
	return mStencilbufferPointer.name();
}

bool Framebuffer::hasStencil()
{
	if(mStencilbufferType != GL_NONE)
	{
		Renderbuffer *stencilbufferObject = getStencilbuffer();

		if(stencilbufferObject)
		{
			return stencilbufferObject->getStencilSize() > 0;
		}
	}

	return false;
}

GLenum Framebuffer::completeness()
{
	int width;
	int height;
	int samples;

	return completeness(width, height, samples);
}

GLenum Framebuffer::completeness(int &width, int &height, int &samples)
{
	width = -1;
	height = -1;
	samples = -1;

	if(mColorbufferType != GL_NONE)
	{
		Renderbuffer *colorbuffer = getColorbuffer();

		if(!colorbuffer)
		{
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
		}

		if(colorbuffer->getWidth() == 0 || colorbuffer->getHeight() == 0)
		{
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
		}

		if(mColorbufferType == GL_RENDERBUFFER)
		{
			if(!gl::IsColorRenderable(colorbuffer->getFormat()))
			{
				return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
			}
		}
		else if(IsTextureTarget(mColorbufferType))
		{
			GLenum format = colorbuffer->getFormat();

			if(IsCompressed(format) ||
			   format == GL_ALPHA ||
			   format == GL_LUMINANCE ||
			   format == GL_LUMINANCE_ALPHA)
			{
				return GL_FRAMEBUFFER_UNSUPPORTED;
			}

			if(gl::IsDepthTexture(format) || gl::IsStencilTexture(format))
			{
				return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
			}
		}
		else
		{
			UNREACHABLE();
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
		}

		width = colorbuffer->getWidth();
		height = colorbuffer->getHeight();
		samples = colorbuffer->getSamples();
	}

	Renderbuffer *depthbuffer = NULL;
	Renderbuffer *stencilbuffer = NULL;

	if(mDepthbufferType != GL_NONE)
	{
		depthbuffer = getDepthbuffer();

		if(!depthbuffer)
		{
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
		}

		if(depthbuffer->getWidth() == 0 || depthbuffer->getHeight() == 0)
		{
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
		}

		if(mDepthbufferType == GL_RENDERBUFFER)
		{
			if(!gl::IsDepthRenderable(depthbuffer->getFormat()))
			{
				return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
			}
		}
		else if(IsTextureTarget(mDepthbufferType))
		{
			if(!gl::IsDepthTexture(depthbuffer->getFormat()))
			{
				return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
			}
		}
		else
		{
			UNREACHABLE();
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
		}

		if(width == -1 || height == -1)
		{
			width = depthbuffer->getWidth();
			height = depthbuffer->getHeight();
			samples = depthbuffer->getSamples();
		}
		else if(width != depthbuffer->getWidth() || height != depthbuffer->getHeight())
		{
			return GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS;
		}
		else if(samples != depthbuffer->getSamples())
		{
			return GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_ANGLE;
		}
	}

	if(mStencilbufferType != GL_NONE)
	{
		stencilbuffer = getStencilbuffer();

		if(!stencilbuffer)
		{
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
		}

		if(stencilbuffer->getWidth() == 0 || stencilbuffer->getHeight() == 0)
		{
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
		}

		if(mStencilbufferType == GL_RENDERBUFFER)
		{
			if(!gl::IsStencilRenderable(stencilbuffer->getFormat()))
			{
				return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
			}
		}
		else if(IsTextureTarget(mStencilbufferType))
		{
			GLenum internalformat = stencilbuffer->getFormat();

			if(!gl::IsStencilTexture(internalformat))
			{
				return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
			}
		}
		else
		{
			UNREACHABLE();
			return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
		}

		if(width == -1 || height == -1)
		{
			width = stencilbuffer->getWidth();
			height = stencilbuffer->getHeight();
			samples = stencilbuffer->getSamples();
		}
		else if(width != stencilbuffer->getWidth() || height != stencilbuffer->getHeight())
		{
			return GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS;
		}
		else if(samples != stencilbuffer->getSamples())
		{
			return GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_ANGLE;
		}
	}

	// If we have both a depth and stencil buffer, they must refer to the same object
	// since we only support packed_depth_stencil and not separate depth and stencil
	if(depthbuffer && stencilbuffer && (depthbuffer != stencilbuffer))
	{
		return GL_FRAMEBUFFER_UNSUPPORTED;
	}

	// We need to have at least one attachment to be complete
	if(width == -1 || height == -1)
	{
		return GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT;
	}

	return GL_FRAMEBUFFER_COMPLETE;
}

DefaultFramebuffer::DefaultFramebuffer(Colorbuffer *colorbuffer, DepthStencilbuffer *depthStencil)
{
	mColorbufferPointer.set(new Renderbuffer(0, colorbuffer));

	Renderbuffer *depthStencilRenderbuffer = new Renderbuffer(0, depthStencil);
	mDepthbufferPointer.set(depthStencilRenderbuffer);
	mStencilbufferPointer.set(depthStencilRenderbuffer);

	mColorbufferType = GL_RENDERBUFFER;
	mDepthbufferType = (depthStencilRenderbuffer->getDepthSize() != 0) ? GL_RENDERBUFFER : GL_NONE;
	mStencilbufferType = (depthStencilRenderbuffer->getStencilSize() != 0) ? GL_RENDERBUFFER : GL_NONE;
}

GLenum DefaultFramebuffer::completeness()
{
	// The default framebuffer should always be complete
	ASSERT(Framebuffer::completeness() == GL_FRAMEBUFFER_COMPLETE);

	return GL_FRAMEBUFFER_COMPLETE;
}

}
