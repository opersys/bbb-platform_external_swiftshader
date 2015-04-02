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

// Context.h: Defines the Context class, managing all GL state and performing
// rendering operations. It is the GLES2 specific implementation of EGLContext.

#ifndef LIBGLESV2_CONTEXT_H_
#define LIBGLESV2_CONTEXT_H_

#include "libEGL/Context.hpp"
#include "ResourceManager.h"
#include "common/NameSpace.hpp"
#include "common/Object.hpp"
#include "Image.hpp"
#include "Renderer/Sampler.hpp"

#define GL_APICALL
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl3.h>
#define EGLAPI
#include <EGL/egl.h>

#include <map>
#include <string>

namespace egl
{
class Display;
class Surface;
class Config;
}

namespace es2
{
struct TranslatedAttribute;
struct TranslatedIndexData;

class Device;
class Buffer;
class Shader;
class Program;
class Texture;
class Texture2D;
class Texture3D;
class TextureCubeMap;
class TextureExternal;
class Framebuffer;
class Renderbuffer;
class RenderbufferStorage;
class Colorbuffer;
class Depthbuffer;
class StreamingIndexBuffer;
class Stencilbuffer;
class DepthStencilbuffer;
class VertexDataManager;
class IndexDataManager;
class Fence;
class Query;

enum
{
    MAX_VERTEX_ATTRIBS = 16,
	MAX_UNIFORM_VECTORS = 256,   // Device limit
    MAX_VERTEX_UNIFORM_VECTORS = VERTEX_UNIFORM_VECTORS - 3,   // Reserve space for gl_DepthRange
    MAX_VARYING_VECTORS = 10,
    MAX_TEXTURE_IMAGE_UNITS = TEXTURE_IMAGE_UNITS,
    MAX_VERTEX_TEXTURE_IMAGE_UNITS = VERTEX_TEXTURE_IMAGE_UNITS,
    MAX_COMBINED_TEXTURE_IMAGE_UNITS = MAX_TEXTURE_IMAGE_UNITS + MAX_VERTEX_TEXTURE_IMAGE_UNITS,
    MAX_FRAGMENT_UNIFORM_VECTORS = FRAGMENT_UNIFORM_VECTORS - 3,    // Reserve space for gl_DepthRange
    MAX_DRAW_BUFFERS = 1,
};

const GLenum compressedTextureFormats[] =
{
	GL_ETC1_RGB8_OES,
#if (S3TC_SUPPORT)
	GL_COMPRESSED_RGB_S3TC_DXT1_EXT,
	GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,
	GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE,
	GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE,
#endif
#if (GL_ES_VERSION_3_0)
	GL_COMPRESSED_R11_EAC,
	GL_COMPRESSED_SIGNED_R11_EAC,
	GL_COMPRESSED_RG11_EAC,
	GL_COMPRESSED_SIGNED_RG11_EAC,
	GL_COMPRESSED_RGB8_ETC2,
	GL_COMPRESSED_SRGB8_ETC2,
	GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2,
	GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2,
	GL_COMPRESSED_RGBA8_ETC2_EAC,
	GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC,
#endif
};

const GLint NUM_COMPRESSED_TEXTURE_FORMATS = sizeof(compressedTextureFormats) / sizeof(compressedTextureFormats[0]);

const float ALIASED_LINE_WIDTH_RANGE_MIN = 1.0f;
const float ALIASED_LINE_WIDTH_RANGE_MAX = 1.0f;
const float ALIASED_POINT_SIZE_RANGE_MIN = 0.125f;
const float ALIASED_POINT_SIZE_RANGE_MAX = 8192.0f;
const float MAX_TEXTURE_MAX_ANISOTROPY = 16.0f;

enum QueryType
{
    QUERY_ANY_SAMPLES_PASSED,
    QUERY_ANY_SAMPLES_PASSED_CONSERVATIVE,

    QUERY_TYPE_COUNT
};

struct Color
{
    float red;
    float green;
    float blue;
    float alpha;
};

// Helper structure describing a single vertex attribute
class VertexAttribute
{
  public:
    VertexAttribute() : mType(GL_FLOAT), mSize(0), mNormalized(false), mStride(0), mPointer(NULL), mArrayEnabled(false)
    {
        mCurrentValue[0] = 0.0f;
        mCurrentValue[1] = 0.0f;
        mCurrentValue[2] = 0.0f;
        mCurrentValue[3] = 1.0f;
    }

    int typeSize() const
    {
        switch (mType)
        {
        case GL_BYTE:           return mSize * sizeof(GLbyte);
        case GL_UNSIGNED_BYTE:  return mSize * sizeof(GLubyte);
        case GL_SHORT:          return mSize * sizeof(GLshort);
        case GL_UNSIGNED_SHORT: return mSize * sizeof(GLushort);
        case GL_FIXED:          return mSize * sizeof(GLfixed);
        case GL_FLOAT:          return mSize * sizeof(GLfloat);
        default: UNREACHABLE(); return mSize * sizeof(GLfloat);
        }
    }

    GLsizei stride() const
    {
        return mStride ? mStride : typeSize();
    }

    // From glVertexAttribPointer
    GLenum mType;
    GLint mSize;
    bool mNormalized;
    GLsizei mStride;   // 0 means natural stride

    union
    {
        const void *mPointer;
        intptr_t mOffset;
    };

    gl::BindingPointer<Buffer> mBoundBuffer;   // Captured when glVertexAttribPointer is called.

    bool mArrayEnabled;   // From glEnable/DisableVertexAttribArray
    float mCurrentValue[4];   // From glVertexAttrib
};

typedef VertexAttribute VertexAttributeArray[MAX_VERTEX_ATTRIBS];

// Helper structure to store all raw state
struct State
{
    Color colorClearValue;
    GLclampf depthClearValue;
    int stencilClearValue;

    bool cullFace;
    GLenum cullMode;
    GLenum frontFace;
    bool depthTest;
    GLenum depthFunc;
    bool blend;
    GLenum sourceBlendRGB;
    GLenum destBlendRGB;
    GLenum sourceBlendAlpha;
    GLenum destBlendAlpha;
    GLenum blendEquationRGB;
    GLenum blendEquationAlpha;
    Color blendColor;
    bool stencilTest;
    GLenum stencilFunc;
    GLint stencilRef;
    GLuint stencilMask;
    GLenum stencilFail;
    GLenum stencilPassDepthFail;
    GLenum stencilPassDepthPass;
    GLuint stencilWritemask;
    GLenum stencilBackFunc;
    GLint stencilBackRef;
    GLuint stencilBackMask;
    GLenum stencilBackFail;
    GLenum stencilBackPassDepthFail;
    GLenum stencilBackPassDepthPass;
    GLuint stencilBackWritemask;
    bool polygonOffsetFill;
    GLfloat polygonOffsetFactor;
    GLfloat polygonOffsetUnits;
    bool sampleAlphaToCoverage;
    bool sampleCoverage;
    GLclampf sampleCoverageValue;
    bool sampleCoverageInvert;
    bool scissorTest;
    bool dither;
    bool primitiveRestartFixedIndex;
    bool rasterizerDiscard;

    GLfloat lineWidth;

    GLenum generateMipmapHint;
    GLenum fragmentShaderDerivativeHint;

    GLint viewportX;
    GLint viewportY;
    GLsizei viewportWidth;
    GLsizei viewportHeight;
    float zNear;
    float zFar;

    GLint scissorX;
    GLint scissorY;
    GLsizei scissorWidth;
    GLsizei scissorHeight;

    bool colorMaskRed;
    bool colorMaskGreen;
    bool colorMaskBlue;
    bool colorMaskAlpha;
    bool depthMask;

    unsigned int activeSampler;   // Active texture unit selector - GL_TEXTURE0
    gl::BindingPointer<Buffer> arrayBuffer;
    gl::BindingPointer<Buffer> elementArrayBuffer;
    GLuint readFramebuffer;
    GLuint drawFramebuffer;
    gl::BindingPointer<Renderbuffer> renderbuffer;
    GLuint currentProgram;

    VertexAttribute vertexAttribute[MAX_VERTEX_ATTRIBS];
    gl::BindingPointer<Texture> samplerTexture[TEXTURE_TYPE_COUNT][MAX_COMBINED_TEXTURE_IMAGE_UNITS];
	gl::BindingPointer<Query> activeQuery[QUERY_TYPE_COUNT];

    GLint unpackAlignment;
    GLint packAlignment;
};

class Context : public egl::Context
{
public:
    Context(const egl::Config *config, const Context *shareContext, EGLint clientVersion);

	virtual void makeCurrent(egl::Surface *surface);
	virtual void destroy();
	virtual EGLint getClientVersion();

    void markAllStateDirty();

    // State manipulation
    void setClearColor(float red, float green, float blue, float alpha);
    void setClearDepth(float depth);
    void setClearStencil(int stencil);

    void setCullFace(bool enabled);
    bool isCullFaceEnabled() const;
    void setCullMode(GLenum mode);
    void setFrontFace(GLenum front);

    void setDepthTest(bool enabled);
    bool isDepthTestEnabled() const;
    void setDepthFunc(GLenum depthFunc);
    void setDepthRange(float zNear, float zFar);
    
    void setBlend(bool enabled);
    bool isBlendEnabled() const;
    void setBlendFactors(GLenum sourceRGB, GLenum destRGB, GLenum sourceAlpha, GLenum destAlpha);
    void setBlendColor(float red, float green, float blue, float alpha);
    void setBlendEquation(GLenum rgbEquation, GLenum alphaEquation);

    void setStencilTest(bool enabled);
    bool isStencilTestEnabled() const;
    void setStencilParams(GLenum stencilFunc, GLint stencilRef, GLuint stencilMask);
    void setStencilBackParams(GLenum stencilBackFunc, GLint stencilBackRef, GLuint stencilBackMask);
    void setStencilWritemask(GLuint stencilWritemask);
    void setStencilBackWritemask(GLuint stencilBackWritemask);
    void setStencilOperations(GLenum stencilFail, GLenum stencilPassDepthFail, GLenum stencilPassDepthPass);
    void setStencilBackOperations(GLenum stencilBackFail, GLenum stencilBackPassDepthFail, GLenum stencilBackPassDepthPass);

    void setPolygonOffsetFill(bool enabled);
    bool isPolygonOffsetFillEnabled() const;
    void setPolygonOffsetParams(GLfloat factor, GLfloat units);

    void setSampleAlphaToCoverage(bool enabled);
    bool isSampleAlphaToCoverageEnabled() const;
    void setSampleCoverage(bool enabled);
    bool isSampleCoverageEnabled() const;
    void setSampleCoverageParams(GLclampf value, bool invert);

    void setDither(bool enabled);
    bool isDitherEnabled() const;

    void setPrimitiveRestartFixedIndex(bool enabled);
    bool isPrimitiveRestartFixedIndexEnabled() const;

    void setRasterizerDiscard(bool enabled);
    bool isRasterizerDiscardEnabled() const;

    void setLineWidth(GLfloat width);

    void setGenerateMipmapHint(GLenum hint);
    void setFragmentShaderDerivativeHint(GLenum hint);

    void setViewportParams(GLint x, GLint y, GLsizei width, GLsizei height);

	void setScissorTest(bool enabled);
    bool isScissorTestEnabled() const;
    void setScissorParams(GLint x, GLint y, GLsizei width, GLsizei height);

    void setColorMask(bool red, bool green, bool blue, bool alpha);
    void setDepthMask(bool mask);

    void setActiveSampler(unsigned int active);

    GLuint getReadFramebufferName() const;
    GLuint getDrawFramebufferName() const;
    GLuint getRenderbufferName() const;

	GLuint getActiveQuery(GLenum target) const;

    GLuint getArrayBufferName() const;

    void setEnableVertexAttribArray(unsigned int attribNum, bool enabled);
    const VertexAttribute &getVertexAttribState(unsigned int attribNum);
    void setVertexAttribState(unsigned int attribNum, Buffer *boundBuffer, GLint size, GLenum type,
                              bool normalized, GLsizei stride, const void *pointer);
    const void *getVertexAttribPointer(unsigned int attribNum) const;

    const VertexAttributeArray &getVertexAttributes();

    void setUnpackAlignment(GLint alignment);
    GLint getUnpackAlignment() const;

    void setPackAlignment(GLint alignment);
    GLint getPackAlignment() const;

    // These create  and destroy methods are merely pass-throughs to 
    // ResourceManager, which owns these object types
    GLuint createBuffer();
    GLuint createShader(GLenum type);
    GLuint createProgram();
    GLuint createTexture();
    GLuint createRenderbuffer();

    void deleteBuffer(GLuint buffer);
    void deleteShader(GLuint shader);
    void deleteProgram(GLuint program);
    void deleteTexture(GLuint texture);
    void deleteRenderbuffer(GLuint renderbuffer);

    // Framebuffers are owned by the Context, so these methods do not pass through
    GLuint createFramebuffer();
    void deleteFramebuffer(GLuint framebuffer);

    // Fences are owned by the Context
    GLuint createFence();
    void deleteFence(GLuint fence);

	// Queries are owned by the Context
    GLuint createQuery();
    void deleteQuery(GLuint query);

    void bindArrayBuffer(GLuint buffer);
    void bindElementArrayBuffer(GLuint buffer);
    void bindTexture2D(GLuint texture);
    void bindTextureCubeMap(GLuint texture);
    void bindTextureExternal(GLuint texture);
	void bindTexture3D(GLuint texture);
    void bindReadFramebuffer(GLuint framebuffer);
    void bindDrawFramebuffer(GLuint framebuffer);
    void bindRenderbuffer(GLuint renderbuffer);
    void useProgram(GLuint program);

	void beginQuery(GLenum target, GLuint query);
    void endQuery(GLenum target);

    void setFramebufferZero(Framebuffer *framebuffer);

    void setRenderbufferStorage(RenderbufferStorage *renderbuffer);

    void setVertexAttrib(GLuint index, const GLfloat *values);

    Buffer *getBuffer(GLuint handle);
    Fence *getFence(GLuint handle);
    Shader *getShader(GLuint handle);
    Program *getProgram(GLuint handle);
    virtual Texture *getTexture(GLuint handle);
    Framebuffer *getFramebuffer(GLuint handle);
    virtual Renderbuffer *getRenderbuffer(GLuint handle);
	Query *getQuery(GLuint handle, bool create, GLenum type);

    Buffer *getArrayBuffer();
    Buffer *getElementArrayBuffer();
    Program *getCurrentProgram();
    Texture2D *getTexture2D();
	Texture3D *getTexture3D();
	TextureCubeMap *getTextureCubeMap();
    TextureExternal *getTextureExternal();
    Texture *getSamplerTexture(unsigned int sampler, TextureType type);
    Framebuffer *getReadFramebuffer();
    Framebuffer *getDrawFramebuffer();

    bool getFloatv(GLenum pname, GLfloat *params);
    bool getIntegerv(GLenum pname, GLint *params);
    bool getBooleanv(GLenum pname, GLboolean *params);
	bool getTransformFeedbackiv(GLuint xfb, GLenum pname, GLint *param);

    bool getQueryParameterInfo(GLenum pname, GLenum *type, unsigned int *numParams);

    void readPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLsizei *bufSize, void* pixels);
    void clear(GLbitfield mask);
    void drawArrays(GLenum mode, GLint first, GLsizei count);
    void drawElements(GLenum mode, GLsizei count, GLenum type, const void *indices);
    void finish();
    void flush();

    void recordInvalidEnum();
    void recordInvalidValue();
    void recordInvalidOperation();
    void recordOutOfMemory();
    void recordInvalidFramebufferOperation();

    GLenum getError();

    static int getSupportedMultiSampleDepth(sw::Format format, int requested);
    
    void blitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, 
                         GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                         GLbitfield mask);

	virtual void bindTexImage(egl::Surface *surface);
	virtual EGLenum validateSharedImage(EGLenum target, GLuint name, GLuint textureLevel);
	virtual egl::Image *createSharedImage(EGLenum target, GLuint name, GLuint textureLevel);

	Device *getDevice();

private:
	virtual ~Context();

    bool applyRenderTarget();
    void applyState(GLenum drawMode);
    GLenum applyVertexBuffer(GLint base, GLint first, GLsizei count);
    GLenum applyIndexBuffer(const void *indices, GLsizei count, GLenum mode, GLenum type, TranslatedIndexData *indexInfo);
    void applyShaders();
    void applyTextures();
    void applyTextures(sw::SamplerType type);
	void applyTexture(sw::SamplerType type, int sampler, Texture *texture);

    void detachBuffer(GLuint buffer);
    void detachTexture(GLuint texture);
    void detachFramebuffer(GLuint framebuffer);
    void detachRenderbuffer(GLuint renderbuffer);

    bool cullSkipsDraw(GLenum drawMode);
    bool isTriangleMode(GLenum drawMode);

	const EGLint clientVersion;
    const egl::Config *const mConfig;

    State mState;

	gl::BindingPointer<Texture2D> mTexture2DZero;
	gl::BindingPointer<Texture3D> mTexture3DZero;
	gl::BindingPointer<TextureCubeMap> mTextureCubeMapZero;
    gl::BindingPointer<TextureExternal> mTextureExternalZero;

    typedef std::map<GLint, Framebuffer*> FramebufferMap;
    FramebufferMap mFramebufferMap;
    gl::NameSpace mFramebufferNameSpace;

    typedef std::map<GLint, Fence*> FenceMap;
    FenceMap mFenceMap;
    gl::NameSpace mFenceNameSpace;

	typedef std::map<GLint, Query*> QueryMap;
    QueryMap mQueryMap;
    gl::NameSpace mQueryNameSpace;

    VertexDataManager *mVertexDataManager;
    IndexDataManager *mIndexDataManager;

    // Recorded errors
    bool mInvalidEnum;
    bool mInvalidValue;
    bool mInvalidOperation;
    bool mOutOfMemory;
    bool mInvalidFramebufferOperation;

    bool mHasBeenCurrent;

    unsigned int mAppliedProgramSerial;
    
    // state caching flags
    bool mDepthStateDirty;
    bool mMaskStateDirty;
    bool mPixelPackingStateDirty;
    bool mBlendStateDirty;
    bool mStencilStateDirty;
    bool mPolygonOffsetStateDirty;
    bool mSampleStateDirty;
    bool mFrontFaceDirty;
    bool mDitherStateDirty;

	Device *device;
    ResourceManager *mResourceManager;
};
}

#endif   // INCLUDE_CONTEXT_H_
