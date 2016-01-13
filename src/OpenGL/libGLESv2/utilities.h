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

// utilities.h: Conversion functions and other utility routines.

#ifndef LIBGLESV2_UTILITIES_H
#define LIBGLESV2_UTILITIES_H

#include "Device.hpp"
#include "common/Image.hpp"
#include "Texture.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <string>

namespace es2
{
	struct Color;

	unsigned int UniformComponentCount(GLenum type);
	GLenum UniformComponentType(GLenum type);
	size_t UniformTypeSize(GLenum type);
	bool IsSamplerUniform(GLenum type);
	int VariableRowCount(GLenum type);
	int VariableColumnCount(GLenum type);
	int VariableRegisterCount(GLenum type);
	int VariableRegisterSize(GLenum type);

	int AllocateFirstFreeBits(unsigned int *bits, unsigned int allocationSize, unsigned int bitsSize);

	GLint floatToInt(GLfloat value);

	bool IsCompressed(GLenum format, egl::GLint clientVersion);
	GLenum ValidateCompressedFormat(GLenum format, egl::GLint clientVersion, bool expectCompressedFormats);
	bool ValidReadPixelsFormatType(GLenum internalFormat, GLenum internalType, GLenum format, GLenum type, egl::GLint clientVersion);
	bool IsDepthTexture(GLenum format);
	bool IsStencilTexture(GLenum format);
	bool IsCubemapTextureTarget(GLenum target);
	int CubeFaceIndex(GLenum cubeTarget);
	bool IsTextureTarget(GLenum target);
	bool CheckTextureFormatType(GLenum format, GLenum type, egl::GLint clientVersion);

	bool IsColorRenderable(GLenum internalformat, egl::GLint clientVersion);
	bool IsDepthRenderable(GLenum internalformat);
	bool IsStencilRenderable(GLenum internalformat);

	// Parse the base uniform name and array index.  Returns the base name of the uniform. outSubscript is
	// set to GL_INVALID_INDEX if the provided name is not an array or the array index is invalid.
	std::string ParseUniformName(const std::string &name, size_t *outSubscript);
}

namespace es2sw
{
	sw::DepthCompareMode ConvertDepthComparison(GLenum comparison);
	sw::StencilCompareMode ConvertStencilComparison(GLenum comparison);
	sw::Color<float> ConvertColor(es2::Color color);
	sw::BlendFactor ConvertBlendFunc(GLenum blend);
	sw::BlendOperation ConvertBlendOp(GLenum blendOp);
	sw::LogicalOperation ConvertLogicalOperation(GLenum logicalOperation);
	sw::StencilOperation ConvertStencilOp(GLenum stencilOp);
	sw::AddressingMode ConvertTextureWrap(GLenum wrap);
	sw::SwizzleType ConvertSwizzleType(GLenum swizzleType);
	sw::CullMode ConvertCullMode(GLenum cullFace, GLenum frontFace);
	unsigned int ConvertColorMask(bool red, bool green, bool blue, bool alpha);
	sw::FilterType ConvertMagFilter(GLenum magFilter);
	void ConvertMinFilter(GLenum texFilter, sw::FilterType *minFilter, sw::MipmapType *mipFilter, float maxAnisotropy);
	bool ConvertPrimitiveType(GLenum primitiveType, GLsizei elementCount,  es2::PrimitiveType &swPrimitiveType, int &primitiveCount);
	sw::Format ConvertRenderbufferFormat(GLenum format);
}

namespace sw2es
{
	GLuint GetAlphaSize(sw::Format colorFormat);
	GLuint GetRedSize(sw::Format colorFormat);
	GLuint GetGreenSize(sw::Format colorFormat);
	GLuint GetBlueSize(sw::Format colorFormat);
	GLuint GetDepthSize(sw::Format depthFormat);
	GLuint GetStencilSize(sw::Format stencilFormat);
	GLenum GetComponentType(sw::Format format, GLenum attachment);

	GLenum ConvertBackBufferFormat(sw::Format format);
	GLenum ConvertDepthStencilFormat(sw::Format format);
}

#endif  // LIBGLESV2_UTILITIES_H
