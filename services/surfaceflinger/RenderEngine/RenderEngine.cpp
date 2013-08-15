/*
 * Copyright 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cutils/log.h>
#include <ui/Rect.h>
#include <ui/Region.h>

#include "RenderEngine.h"
#include "GLES10RenderEngine.h"
#include "GLES11RenderEngine.h"
#include "GLES20RenderEngine.h"
#include "GLExtensions.h"
#include "Mesh.h"

// ---------------------------------------------------------------------------
namespace android {
// ---------------------------------------------------------------------------

RenderEngine* RenderEngine::create(EGLDisplay display, EGLConfig config) {
    // Also create our EGLContext
    EGLint contextAttributes[] = {
            EGL_CONTEXT_CLIENT_VERSION, 2,      // MUST be first
#ifdef EGL_IMG_context_priority
#ifdef HAS_CONTEXT_PRIORITY
#warning "using EGL_IMG_context_priority"
            EGL_CONTEXT_PRIORITY_LEVEL_IMG, EGL_CONTEXT_PRIORITY_HIGH_IMG,
#endif
#endif
            EGL_NONE, EGL_NONE
    };

    EGLContext ctxt = EGL_NO_CONTEXT; // eglCreateContext(display, config, NULL, contextAttributes);
    if (ctxt == EGL_NO_CONTEXT) {
        // maybe ES 2.x is not supported
        ALOGW("can't create an ES 2.x context, trying 1.x");
        ctxt = eglCreateContext(display, config, NULL, contextAttributes + 2);
    }

    // if can't create a GL context, we can only abort.
    LOG_ALWAYS_FATAL_IF(ctxt==EGL_NO_CONTEXT, "EGLContext creation failed");


    // now figure out what version of GL did we actually get
    // NOTE: a dummy surface is not needed if KHR_create_context is supported

    EGLint attribs[] = { EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE, EGL_NONE };
    EGLSurface dummy = eglCreatePbufferSurface(display, config, attribs);
    LOG_ALWAYS_FATAL_IF(dummy==EGL_NO_SURFACE, "can't create dummy pbuffer");
    EGLBoolean success = eglMakeCurrent(display, dummy, dummy, ctxt);
    LOG_ALWAYS_FATAL_IF(!success, "can't make dummy pbuffer current");

    GLExtensions& extensions(GLExtensions::getInstance());
    extensions.initWithGLStrings(
            glGetString(GL_VENDOR),
            glGetString(GL_RENDERER),
            glGetString(GL_VERSION),
            glGetString(GL_EXTENSIONS));

    GlesVersion version = parseGlesVersion( extensions.getVersion() );

    // initialize the renderer while GL is current

    RenderEngine* engine = NULL;
    switch (version) {
    case GLES_VERSION_1_0:
        engine = new GLES10RenderEngine();
        break;
    case GLES_VERSION_1_1:
        engine = new GLES11RenderEngine();
        break;
    case GLES_VERSION_2_0:
    case GLES_VERSION_3_0:
        engine = new GLES20RenderEngine();
        break;
    }
    engine->setEGLContext(ctxt);

    ALOGI("OpenGL ES informations:");
    ALOGI("vendor    : %s", extensions.getVendor());
    ALOGI("renderer  : %s", extensions.getRenderer());
    ALOGI("version   : %s", extensions.getVersion());
    ALOGI("extensions: %s", extensions.getExtension());
    ALOGI("GL_MAX_TEXTURE_SIZE = %d", engine->getMaxTextureSize());
    ALOGI("GL_MAX_VIEWPORT_DIMS = %d", engine->getMaxViewportDims());

    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroySurface(display, dummy);

    return engine;
}

RenderEngine::RenderEngine() : mEGLContext(EGL_NO_CONTEXT) {
}

RenderEngine::~RenderEngine() {
}

void RenderEngine::setEGLContext(EGLContext ctxt) {
    mEGLContext = ctxt;
}

EGLContext RenderEngine::getEGLContext() const {
    return mEGLContext;
}

void RenderEngine::checkErrors() const {
    do {
        // there could be more than one error flag
        GLenum error = glGetError();
        if (error == GL_NO_ERROR)
            break;
        ALOGE("GL error 0x%04x", int(error));
    } while (true);
}

RenderEngine::GlesVersion RenderEngine::parseGlesVersion(const char* str) {
    int major, minor;
    if (sscanf(str, "OpenGL ES-CM %d.%d", &major, &minor) != 2) {
        if (sscanf(str, "OpenGL ES %d.%d", &major, &minor) != 2) {
            ALOGW("Unable to parse GL_VERSION string: \"%s\"", str);
            return GLES_VERSION_1_0;
        }
    }

    if (major == 1 && minor == 0) return GLES_VERSION_1_0;
    if (major == 1 && minor >= 1) return GLES_VERSION_1_1;
    if (major == 2 && minor >= 0) return GLES_VERSION_2_0;
    if (major == 3 && minor >= 0) return GLES_VERSION_3_0;

    ALOGW("Unrecognized OpenGL ES version: %d.%d", major, minor);
    return GLES_VERSION_1_0;
}

void RenderEngine::fillRegionWithColor(const Region& region, uint32_t height,
        float red, float green, float blue, float alpha) {
    size_t c;
    Rect const* r = region.getArray(&c);
    Mesh mesh(Mesh::TRIANGLES, c*6, 2);
    Mesh::VertexArray position(mesh.getPositionArray());
    for (size_t i=0 ; i<c ; i++, r++) {
        position[i*6 + 0].x = r->left;
        position[i*6 + 0].y = height - r->top;
        position[i*6 + 1].x = r->left;
        position[i*6 + 1].y = height - r->bottom;
        position[i*6 + 2].x = r->right;
        position[i*6 + 2].y = height - r->bottom;
        position[i*6 + 3].x = r->left;
        position[i*6 + 3].y = height - r->top;
        position[i*6 + 4].x = r->right;
        position[i*6 + 4].y = height - r->bottom;
        position[i*6 + 5].x = r->right;
        position[i*6 + 5].y = height - r->top;
    }
    fillWithColor(mesh, red, green, blue, alpha);
}

void RenderEngine::clearWithColor(float red, float green, float blue, float alpha) {
    glClearColor(red, green, blue, alpha);
    glClear(GL_COLOR_BUFFER_BIT);
}

void RenderEngine::setScissor(
        uint32_t left, uint32_t bottom, uint32_t right, uint32_t top) {
    glScissor(left, bottom, right, top);
    glEnable(GL_SCISSOR_TEST);
}

void RenderEngine::disableScissor() {
    glDisable(GL_SCISSOR_TEST);
}

void RenderEngine::genTextures(size_t count, uint32_t* names) {
    glGenTextures(count, names);
}

void RenderEngine::deleteTextures(size_t count, uint32_t const* names) {
    glDeleteTextures(count, names);
}

// ---------------------------------------------------------------------------

RenderEngine::BindImageAsFramebuffer::BindImageAsFramebuffer(
        RenderEngine& engine, EGLImageKHR image) : mEngine(engine)
{
    GLuint tname, name;
    // turn our EGLImage into a texture
    glGenTextures(1, &tname);
    glBindTexture(GL_TEXTURE_2D, tname);
    glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, (GLeglImageOES)image);
    // create a Framebuffer Object to render into
    glGenFramebuffersOES(1, &name);
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, name);
    glFramebufferTexture2DOES(GL_FRAMEBUFFER_OES,
            GL_COLOR_ATTACHMENT0_OES, GL_TEXTURE_2D, tname, 0);
    mStatus = glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES);
    ALOGE_IF(mStatus != GL_FRAMEBUFFER_COMPLETE_OES,
            "glCheckFramebufferStatusOES error %d", mStatus);
    mTexName = tname;
    mFbName = name;
}

RenderEngine::BindImageAsFramebuffer::~BindImageAsFramebuffer() {
    // back to main framebuffer
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, 0);
    glDeleteFramebuffersOES(1, &mFbName);
    glDeleteTextures(1, &mTexName);

}

status_t RenderEngine::BindImageAsFramebuffer::getStatus() const {
    return mStatus == GL_FRAMEBUFFER_COMPLETE_OES ? NO_ERROR : BAD_VALUE;
}

// ---------------------------------------------------------------------------
}; // namespace android
// ---------------------------------------------------------------------------