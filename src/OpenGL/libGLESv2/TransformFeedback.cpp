// SwiftShader Software Renderer
//
// Copyright(c) 2015 Google Inc.
//
// All rights reserved. No part of this software may be copied, distributed, transmitted,
// transcribed, stored in a retrieval system, translated into any human or computer
// language by any means, or disclosed to third parties without the explicit written
// agreement of Google Inc. Without such an agreement, no rights or licenses, express
// or implied, including but not limited to any patent rights, are granted to you.
//

// TransformFeedback.cpp: Implements the es2::TransformFeedback class

#include "TransformFeedback.h"

#include "Buffer.h"

namespace es2
{

TransformFeedback::TransformFeedback(GLuint name) : NamedObject(name), mActive(false), mPaused(false)
{
	mGenericBuffer = NULL;
}

TransformFeedback::~TransformFeedback()
{
	mGenericBuffer = NULL;
	for(int i = 0; i < MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS; ++i)
	{
		mBuffer[i] = NULL;
	}
}

Buffer* TransformFeedback::getGenericBuffer() const
{
	return mGenericBuffer;
}

Buffer* TransformFeedback::getBuffer(GLuint index) const
{
	return mBuffer[index];
}

bool TransformFeedback::isActive() const
{
	return mActive;
}

bool TransformFeedback::isPaused() const
{
	return mPaused;
}

GLenum TransformFeedback::primitiveMode() const
{
	return mPrimitiveMode;
}

void TransformFeedback::begin(GLenum primitiveMode)
{
	mActive = true; mPrimitiveMode = primitiveMode;
}

void TransformFeedback::end()
{
	mActive = false;
	mPaused = false;
}

void TransformFeedback::setPaused(bool paused)
{
	mPaused = paused;
}

void TransformFeedback::setGenericBuffer(Buffer* buffer)
{
	mGenericBuffer = buffer;
}

void TransformFeedback::setBuffer(GLuint index, Buffer* buffer)
{
	mBuffer[index] = buffer;
}

void TransformFeedback::setBuffer(GLuint index, Buffer* buffer, GLintptr offset, GLsizeiptr size)
{
	mBuffer[index] = buffer;
	if(buffer)
	{
		buffer->mapRange(offset, size, buffer->access());
	}
}

void TransformFeedback::detachBuffer(GLuint buffer)
{
	if(mGenericBuffer.name() == buffer)
	{
		mGenericBuffer = NULL;
	}

	for(int i = 0; i < MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS; ++i)
	{
		if(mBuffer[i].name() == buffer)
		{
			mBuffer[i] = NULL;
		}
	}
}

}
