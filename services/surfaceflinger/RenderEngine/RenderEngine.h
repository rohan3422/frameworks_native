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


#ifndef SF_RENDERENGINE_H_
#define SF_RENDERENGINE_H_

#include <stdint.h>
#include <sys/types.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

// ---------------------------------------------------------------------------
namespace android {
// ---------------------------------------------------------------------------

class String8;
class Rect;
class Region;
class Mesh;

class RenderEngine {
    enum GlesVersion {
        GLES_VERSION_1_0    = 0x10000,
        GLES_VERSION_1_1    = 0x10001,
        GLES_VERSION_2_0    = 0x20000,
        GLES_VERSION_3_0    = 0x30000,
    };
    static GlesVersion parseGlesVersion(const char* str);

    EGLContext mEGLContext;
    void setEGLContext(EGLContext ctxt);

protected:
    RenderEngine();
    virtual ~RenderEngine() = 0;

public:
    static RenderEngine* create(EGLDisplay display, EGLConfig config);

    // helpers
    void clearWithColor(float red, float green, float blue, float alpha);
    void fillRegionWithColor(const Region& region, uint32_t height,
            float red, float green, float blue, float alpha);

    // common to all GL versions
    void setScissor(uint32_t left, uint32_t bottom, uint32_t right, uint32_t top);
    void disableScissor();
    void genTextures(size_t count, uint32_t* names);
    void deleteTextures(size_t count, uint32_t const* names);

    class BindImageAsFramebuffer {
        RenderEngine& mEngine;
        unsigned int mTexName, mFbName;
        unsigned int mStatus;
    public:
        BindImageAsFramebuffer(RenderEngine& engine, EGLImageKHR image);
        ~BindImageAsFramebuffer();
        int getStatus() const;
    };

    // set-up
    virtual void checkErrors() const;
    virtual void dump(String8& result) = 0;
    virtual void setViewportAndProjection(size_t vpw, size_t vph, size_t w, size_t h, bool yswap) = 0;
    virtual void setupLayerBlending(bool premultipliedAlpha, bool opaque, int alpha) = 0;
    virtual void setupDimLayerBlending(int alpha) = 0;
    virtual void setupLayerTexturing(size_t textureName, bool useFiltering, const float* textureMatrix) = 0;
    virtual void setupLayerBlackedOut() = 0;

    virtual void disableTexturing() = 0;
    virtual void disableBlending() = 0;

    // drawing
    virtual void fillWithColor(const Mesh& mesh, float r, float g, float b, float a) = 0;
    virtual void drawMesh(const Mesh& mesh) = 0;

    // queries
    virtual size_t getMaxTextureSize() const = 0;
    virtual size_t getMaxViewportDims() const = 0;



    EGLContext getEGLContext() const;
};

// ---------------------------------------------------------------------------
}; // namespace android
// ---------------------------------------------------------------------------

#endif /* SF_RENDERENGINE_H_ */