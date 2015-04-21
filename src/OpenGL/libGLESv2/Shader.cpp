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

// Shader.cpp: Implements the Shader class and its  derived classes
// VertexShader and FragmentShader. Implements GL shader objects and related
// functionality. [OpenGL ES 2.0.24] section 2.10 page 24 and section 3.8 page 84.

#include "Shader.h"

#include "main.h"
#include "utilities.h"

#include <string>

namespace es2
{
bool Shader::compilerInitialized = false;

Shader::Shader(ResourceManager *manager, GLuint handle) : mHandle(handle), mResourceManager(manager)
{
    mSource = NULL;

	clear();

    mRefCount = 0;
    mDeleteStatus = false;
}

Shader::~Shader()
{
    delete[] mSource;
}

GLuint Shader::getName() const
{
    return mHandle;
}

void Shader::setSource(GLsizei count, const char *const *string, const GLint *length)
{
    delete[] mSource;
    int totalLength = 0;

    for(int i = 0; i < count; i++)
    {
        if(length && length[i] >= 0)
        {
            totalLength += length[i];
        }
        else
        {
            totalLength += (int)strlen(string[i]);
        }
    }

    mSource = new char[totalLength + 1];
    char *code = mSource;

    for(int i = 0; i < count; i++)
    {
        int stringLength;

        if(length && length[i] >= 0)
        {
            stringLength = length[i];
        }
        else
        {
            stringLength = (int)strlen(string[i]);
        }

        strncpy(code, string[i], stringLength);
        code += stringLength;
    }

    mSource[totalLength] = '\0';
}

int Shader::getInfoLogLength() const
{
    if(infoLog.empty())
    {
        return 0;
    }
    else
    {
       return infoLog.size() + 1;
    }
}

void Shader::getInfoLog(GLsizei bufSize, GLsizei *length, char *infoLogOut)
{
    int index = 0;

	if(bufSize > 0)
	{
		if(!infoLog.empty())
		{
			index = std::min(bufSize - 1, (GLsizei)infoLog.size());
			memcpy(infoLogOut, infoLog.c_str(), index);
		}

        infoLogOut[index] = '\0';
    }

    if(length)
    {
        *length = index;
    }
}

int Shader::getSourceLength() const
{
    if(!mSource)
    {
        return 0;
    }
    else
    {
       return strlen(mSource) + 1;
    }
}

void Shader::getSource(GLsizei bufSize, GLsizei *length, char *source)
{
    int index = 0;

	if(bufSize > 0)
	{
		if(mSource)
		{
			index = std::min(bufSize - 1, (int)strlen(mSource));
			memcpy(source, mSource, index);
		}

        source[index] = '\0';
    }

    if(length)
    {
        *length = index;
    }
}

TranslatorASM *Shader::createCompiler(GLenum shaderType)
{
	if(!compilerInitialized)
	{
		InitCompilerGlobals();
		compilerInitialized = true;
	}

	TranslatorASM *assembler = new TranslatorASM(this, shaderType);

	ShBuiltInResources resources;   
	resources.MaxVertexAttribs = MAX_VERTEX_ATTRIBS;
	resources.MaxVertexUniformVectors = MAX_VERTEX_UNIFORM_VECTORS;
	resources.MaxVaryingVectors = MAX_VARYING_VECTORS;
	resources.MaxVertexTextureImageUnits = MAX_VERTEX_TEXTURE_IMAGE_UNITS;
	resources.MaxCombinedTextureImageUnits = MAX_COMBINED_TEXTURE_IMAGE_UNITS;
	resources.MaxTextureImageUnits = MAX_TEXTURE_IMAGE_UNITS;
	resources.MaxFragmentUniformVectors = MAX_FRAGMENT_UNIFORM_VECTORS;
	resources.MaxDrawBuffers = MAX_DRAW_BUFFERS;
	resources.MaxVertexOutputVectors = MAX_VERTEX_OUTPUT_VECTORS;
	resources.MaxFragmentInputVectors = MAX_FRAGMENT_INPUT_VECTORS;
	resources.MinProgramTexelOffset = MIN_PROGRAM_TEXEL_OFFSET;
	resources.MaxProgramTexelOffset = MAX_PROGRAM_TEXEL_OFFSET;
	resources.OES_standard_derivatives = 1;
	resources.OES_fragment_precision_high = 1;
	resources.OES_EGL_image_external = 1;
	resources.MaxCallStackDepth = 16;
	assembler->Init(resources);

	return assembler;
}

void Shader::clear()
{
	infoLog.clear();

	varyings.clear();
	activeUniforms.clear();
	activeAttributes.clear();
}

void Shader::compile()
{
	clear();

	createShader();
	TranslatorASM *compiler = createCompiler(getType());

	// Ensure we don't pass a NULL source to the compiler
    char *source = "\0";
	if(mSource)
    {
        source = mSource;
    }

	bool success = compiler->compile(&source, 1, SH_OBJECT_CODE);

	if(false)
	{
		static int serial = 1;
		char buffer[256];
		sprintf(buffer, "shader-input-%d-%d.txt", getName(), serial);
		FILE *file = fopen(buffer, "wt");
		fprintf(file, "%s", mSource);
		fclose(file);
		getShader()->print("shader-output-%d-%d.txt", getName(), serial);
		serial++;
	}

	int shaderVersion = compiler->getShaderVersion();
	int clientVersion = es2::getContext()->getClientVersion();

	if(shaderVersion >= 300 && clientVersion < 3)
	{
		infoLog = "GLSL ES 3.00 is not supported by OpenGL ES 2.0 contexts";
		success = false;
	}

	if(!success)
	{
		deleteShader();

		infoLog += compiler->getInfoSink().info.c_str();
        TRACE("\n%s", infoLog.c_str());
	}

	delete compiler;
}

bool Shader::isCompiled()
{
    return getShader() != 0;
}

void Shader::addRef()
{
    mRefCount++;
}

void Shader::release()
{
    mRefCount--;

    if(mRefCount == 0 && mDeleteStatus)
    {
        mResourceManager->deleteShader(mHandle);
    }
}

unsigned int Shader::getRefCount() const
{
    return mRefCount;
}

bool Shader::isFlaggedForDeletion() const
{
    return mDeleteStatus;
}

void Shader::flagForDeletion()
{
    mDeleteStatus = true;
}

void Shader::releaseCompiler()
{
    FreeCompilerGlobals();
	compilerInitialized = false;
}

GLenum Shader::parseType(const std::string &type)
{
    if(type == "float")
    {
        return GL_FLOAT;
    }
    else if(type == "float2")
    {
        return GL_FLOAT_VEC2;
    }
    else if(type == "float3")
    {
        return GL_FLOAT_VEC3;
    }
    else if(type == "float4")
    {
        return GL_FLOAT_VEC4;
    }
    else if(type == "float2x2")
    {
        return GL_FLOAT_MAT2;
    }
    else if(type == "float3x3")
    {
        return GL_FLOAT_MAT3;
    }
    else if(type == "float4x4")
    {
        return GL_FLOAT_MAT4;
    }
    else UNREACHABLE();

    return GL_NONE;
}

// true if varying x has a higher priority in packing than y
bool Shader::compareVarying(const glsl::Varying &x, const glsl::Varying &y)
{
    if(x.type == y.type)
    {
        return x.size() > y.size();
    }

    switch (x.type)
    {
      case GL_FLOAT_MAT4: return true;
      case GL_FLOAT_MAT2:
        switch(y.type)
        {
          case GL_FLOAT_MAT4: return false;
          case GL_FLOAT_MAT2: return true;
          case GL_FLOAT_VEC4: return true;
          case GL_FLOAT_MAT3: return true;
          case GL_FLOAT_VEC3: return true;
          case GL_FLOAT_VEC2: return true;
          case GL_FLOAT:      return true;
          default: UNREACHABLE();
        }
        break;
      case GL_FLOAT_VEC4:
        switch(y.type)
        {
          case GL_FLOAT_MAT4: return false;
          case GL_FLOAT_MAT2: return false;
          case GL_FLOAT_VEC4: return true;
          case GL_FLOAT_MAT3: return true;
          case GL_FLOAT_VEC3: return true;
          case GL_FLOAT_VEC2: return true;
          case GL_FLOAT:      return true;
          default: UNREACHABLE();
        }
        break;
      case GL_FLOAT_MAT3:
        switch(y.type)
        {
          case GL_FLOAT_MAT4: return false;
          case GL_FLOAT_MAT2: return false;
          case GL_FLOAT_VEC4: return false;
          case GL_FLOAT_MAT3: return true;
          case GL_FLOAT_VEC3: return true;
          case GL_FLOAT_VEC2: return true;
          case GL_FLOAT:      return true;
          default: UNREACHABLE();
        }
        break;
      case GL_FLOAT_VEC3:
        switch(y.type)
        {
          case GL_FLOAT_MAT4: return false;
          case GL_FLOAT_MAT2: return false;
          case GL_FLOAT_VEC4: return false;
          case GL_FLOAT_MAT3: return false;
          case GL_FLOAT_VEC3: return true;
          case GL_FLOAT_VEC2: return true;
          case GL_FLOAT:      return true;
          default: UNREACHABLE();
        }
        break;
      case GL_FLOAT_VEC2:
        switch(y.type)
        {
          case GL_FLOAT_MAT4: return false;
          case GL_FLOAT_MAT2: return false;
          case GL_FLOAT_VEC4: return false;
          case GL_FLOAT_MAT3: return false;
          case GL_FLOAT_VEC3: return false;
          case GL_FLOAT_VEC2: return true;
          case GL_FLOAT:      return true;
          default: UNREACHABLE();
        }
        break;
      case GL_FLOAT: return false;
      default: UNREACHABLE();
    }

    return false;
}

VertexShader::VertexShader(ResourceManager *manager, GLuint handle) : Shader(manager, handle)
{
	vertexShader = 0;
}

VertexShader::~VertexShader()
{
	delete vertexShader;
}

GLenum VertexShader::getType()
{
    return GL_VERTEX_SHADER;
}

int VertexShader::getSemanticIndex(const std::string &attributeName)
{
    if(!attributeName.empty())
    {
		for(glsl::ActiveAttributes::iterator attribute = activeAttributes.begin(); attribute != activeAttributes.end(); attribute++)
        {
            if(attribute->name == attributeName)
            {
				return attribute->registerIndex;
            }
        }
    }

    return -1;
}

sw::Shader *VertexShader::getShader() const
{
	return vertexShader;
}

sw::VertexShader *VertexShader::getVertexShader() const
{
	return vertexShader;
}

void VertexShader::createShader()
{
	delete vertexShader;
	vertexShader = new sw::VertexShader();
}

void VertexShader::deleteShader()
{
	delete vertexShader;
	vertexShader = nullptr;
}

FragmentShader::FragmentShader(ResourceManager *manager, GLuint handle) : Shader(manager, handle)
{
	pixelShader = 0;
}

FragmentShader::~FragmentShader()
{
	delete pixelShader;
}

GLenum FragmentShader::getType()
{
    return GL_FRAGMENT_SHADER;
}

sw::Shader *FragmentShader::getShader() const
{
	return pixelShader;
}

sw::PixelShader *FragmentShader::getPixelShader() const
{
	return pixelShader;
}

void FragmentShader::createShader()
{
	delete pixelShader;
	pixelShader = new sw::PixelShader();
}

void FragmentShader::deleteShader()
{
	delete pixelShader;
	pixelShader = nullptr;
}

}
