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

// Query.h: Defines the gl::Query class

#ifndef LIBGL_QUERY_H_
#define LIBGL_QUERY_H_

#include "common/Object.hpp"
#include "Renderer/Renderer.hpp"

#define _GDI32_
#include <windows.h>
#include <GL/GL.h>
#define GL_GLEXT_PROTOTYPES
#include <GL/glext.h>

namespace gl
{

class Query : public NamedObject
{
  public:
    Query(GLuint name, GLenum type);
    virtual ~Query();

    void begin();
    void end();
    GLuint getResult();
    GLboolean isResultAvailable();

    GLenum getType() const;

  private:
    GLboolean testQuery();

    sw::Query* mQuery;
    GLenum mType;
    GLboolean mStatus;
    GLint mResult;
};

}

#endif   // LIBGL_QUERY_H_
