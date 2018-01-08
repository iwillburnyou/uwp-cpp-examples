﻿
#include "pch.h"
#include "AngleResources.h"


using namespace Windows::UI::Core;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Platform;

// Constructor for AngleResources.
ANGLE::AngleResources::AngleResources(HANDLE sharedHandle, float width, float height) :
    mEglDisplay(EGL_NO_DISPLAY),
    mEglContext(EGL_NO_CONTEXT),
    mEglSurface(EGL_NO_SURFACE),
    m_defaultSharedTexture(sharedHandle),
    m_defaultWidth(width),
    m_defaultHeight(height)
{
    InitializeEGL(m_defaultSharedTexture, m_defaultWidth, m_defaultHeight);
}

// Recreate all device resources and set them back to the current state.
// Locks the set of holographic camera resources until the function exits.
void ANGLE::AngleResources::HandleDeviceLost()
{
    CleanupEGL();
    InitializeEGL(m_defaultSharedTexture, m_defaultWidth, m_defaultHeight);
}

// Present the contents of the swap chain to the screen.
// Locks the set of holographic camera resources until the function exits.
void ANGLE::AngleResources::Present()
{
    glFlush();
}

void ANGLE::AngleResources::InitializeEGL(HANDLE sharedHandle, float width, float height)
{
    const EGLint configAttributes[] =
    {
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 8,
        EGL_STENCIL_SIZE, 8,
        EGL_NONE
    };

    const EGLint contextAttributes[] =
    {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    const EGLint surfaceAttributes[] =
    {
        // EGL_ANGLE_SURFACE_RENDER_TO_BACK_BUFFER is part of the same optimization as EGL_ANGLE_DISPLAY_ALLOW_RENDER_TO_BACK_BUFFER (see above).
        // If you have compilation issues with it then please update your Visual Studio templates.
        EGL_ANGLE_SURFACE_RENDER_TO_BACK_BUFFER, EGL_TRUE,
        EGL_NONE
    };

    const EGLint defaultDisplayAttributes[] =
    {
        // These are the default display attributes, used to request ANGLE's D3D11 renderer.
        // eglInitialize will only succeed with these attributes if the hardware supports D3D11 Feature Level 10_0+.
        EGL_PLATFORM_ANGLE_TYPE_ANGLE, EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,

        // EGL_ANGLE_DISPLAY_ALLOW_RENDER_TO_BACK_BUFFER is an optimization that can have large performance benefits on mobile devices.
        // Its syntax is subject to change, though. Please update your Visual Studio templates if you experience compilation issues with it.
        EGL_ANGLE_DISPLAY_ALLOW_RENDER_TO_BACK_BUFFER, EGL_TRUE,

        // EGL_PLATFORM_ANGLE_ENABLE_AUTOMATIC_TRIM_ANGLE is an option that enables ANGLE to automatically call 
        // the IDXGIDevice3::Trim method on behalf of the application when it gets suspended. 
        // Calling IDXGIDevice3::Trim when an application is suspended is a Windows Store application certification requirement.
        EGL_PLATFORM_ANGLE_ENABLE_AUTOMATIC_TRIM_ANGLE, EGL_TRUE,
        EGL_NONE,
    };

    const EGLint fl9_3DisplayAttributes[] =
    {
        // These can be used to request ANGLE's D3D11 renderer, with D3D11 Feature Level 9_3.
        // These attributes are used if the call to eglInitialize fails with the default display attributes.
        EGL_PLATFORM_ANGLE_TYPE_ANGLE, EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
        EGL_PLATFORM_ANGLE_MAX_VERSION_MAJOR_ANGLE, 9,
        EGL_PLATFORM_ANGLE_MAX_VERSION_MINOR_ANGLE, 3,
        EGL_ANGLE_DISPLAY_ALLOW_RENDER_TO_BACK_BUFFER, EGL_TRUE,
        EGL_PLATFORM_ANGLE_ENABLE_AUTOMATIC_TRIM_ANGLE, EGL_TRUE,
        EGL_NONE,
    };

    const EGLint warpDisplayAttributes[] =
    {
        // These attributes can be used to request D3D11 WARP.
        // They are used if eglInitialize fails with both the default display attributes and the 9_3 display attributes.
        EGL_PLATFORM_ANGLE_TYPE_ANGLE, EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
        EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE, EGL_PLATFORM_ANGLE_DEVICE_TYPE_WARP_ANGLE,
        EGL_ANGLE_DISPLAY_ALLOW_RENDER_TO_BACK_BUFFER, EGL_TRUE,
        EGL_PLATFORM_ANGLE_ENABLE_AUTOMATIC_TRIM_ANGLE, EGL_TRUE,
        EGL_NONE,
    };

    mEGLConfig = NULL;

    // eglGetPlatformDisplayEXT is an alternative to eglGetDisplay. It allows us to pass in display attributes, used to configure D3D11.
    PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplayEXT = reinterpret_cast<PFNEGLGETPLATFORMDISPLAYEXTPROC>(eglGetProcAddress("eglGetPlatformDisplayEXT"));
    if (!eglGetPlatformDisplayEXT)
    {
        throw Exception::CreateException(E_FAIL, L"Failed to get function eglGetPlatformDisplayEXT");
    }

    //
    // To initialize the display, we make three sets of calls to eglGetPlatformDisplayEXT and eglInitialize, with varying 
    // parameters passed to eglGetPlatformDisplayEXT:
    // 1) The first calls uses "defaultDisplayAttributes" as a parameter. This corresponds to D3D11 Feature Level 10_0+.
    // 2) If eglInitialize fails for step 1 (e.g. because 10_0+ isn't supported by the default GPU), then we try again 
    //    using "fl9_3DisplayAttributes". This corresponds to D3D11 Feature Level 9_3.
    // 3) If eglInitialize fails for step 2 (e.g. because 9_3+ isn't supported by the default GPU), then we try again 
    //    using "warpDisplayAttributes".  This corresponds to D3D11 Feature Level 11_0 on WARP, a D3D11 software rasterizer.
    //

    // This tries to initialize EGL to D3D11 Feature Level 10_0+. See above comment for details.
    mEglDisplay = eglGetPlatformDisplayEXT(EGL_PLATFORM_ANGLE_ANGLE, EGL_DEFAULT_DISPLAY, defaultDisplayAttributes);
    if (mEglDisplay == EGL_NO_DISPLAY)
    {
        throw Exception::CreateException(E_FAIL, L"Failed to get EGL display");
    }

    if (eglInitialize(mEglDisplay, NULL, NULL) == EGL_FALSE)
    {
        // This tries to initialize EGL to D3D11 Feature Level 9_3, if 10_0+ is unavailable (e.g. on some mobile devices).
        mEglDisplay = eglGetPlatformDisplayEXT(EGL_PLATFORM_ANGLE_ANGLE, EGL_DEFAULT_DISPLAY, fl9_3DisplayAttributes);
        if (mEglDisplay == EGL_NO_DISPLAY)
        {
            throw Exception::CreateException(E_FAIL, L"Failed to get EGL display");
        }

        if (eglInitialize(mEglDisplay, NULL, NULL) == EGL_FALSE)
        {
            // This initializes EGL to D3D11 Feature Level 11_0 on WARP, if 9_3+ is unavailable on the default GPU.
            mEglDisplay = eglGetPlatformDisplayEXT(EGL_PLATFORM_ANGLE_ANGLE, EGL_DEFAULT_DISPLAY, warpDisplayAttributes);
            if (mEglDisplay == EGL_NO_DISPLAY)
            {
                throw Exception::CreateException(E_FAIL, L"Failed to get EGL display");
            }

            if (eglInitialize(mEglDisplay, NULL, NULL) == EGL_FALSE)
            {
                // If all of the calls to eglInitialize returned EGL_FALSE then an error has occurred.
                throw Exception::CreateException(E_FAIL, L"Failed to initialize EGL");
            }
        }
    }

    EGLint numConfigs = 0;
    if ((eglChooseConfig(mEglDisplay, configAttributes, &mEGLConfig, 1, &numConfigs) == EGL_FALSE) || (numConfigs == 0))
    {
        throw Exception::CreateException(E_FAIL, L"Failed to choose first EGLConfig");
    }

 
    AddSharedTexture(sharedHandle, width, height);



    mEglContext = eglCreateContext(mEglDisplay, mEGLConfig, EGL_NO_CONTEXT, contextAttributes);
    if (mEglContext == EGL_NO_CONTEXT)
    {
        throw Exception::CreateException(E_FAIL, L"Failed to create EGL context");
    }

    MakeCurrect(sharedHandle);
}

void ANGLE::AngleResources::CleanupEGL()
{
    if (mEglDisplay != EGL_NO_DISPLAY && m_elgSurfaces.size() > 0)
    {
        for (auto it = m_elgSurfaces.begin(); it != m_elgSurfaces.end(); ++it)
        {
            eglDestroySurface(mEglDisplay, it->second);
        }
        m_elgSurfaces.clear();
    }

    if (mEglDisplay != EGL_NO_DISPLAY && mEglSurface != EGL_NO_SURFACE)
    {
        eglDestroySurface(mEglDisplay, mEglSurface);
        mEglSurface = EGL_NO_SURFACE;
    }

    if (mEglDisplay != EGL_NO_DISPLAY && mEglContext != EGL_NO_CONTEXT)
    {
        eglDestroyContext(mEglDisplay, mEglContext);
        mEglContext = EGL_NO_CONTEXT;
    }

    if (mEglDisplay != EGL_NO_DISPLAY)
    {
        eglTerminate(mEglDisplay);
        mEglDisplay = EGL_NO_DISPLAY;
    }
}

void ANGLE::AngleResources::AddSharedTexture(HANDLE sharedHandle, float width, float height)
{
    auto it = m_elgSurfaces.find(sharedHandle);
    if (it == m_elgSurfaces.end())
    {
        EGLint pBufferAttributes[] =
        {
            EGL_WIDTH, static_cast<int>(width),
            EGL_HEIGHT, static_cast<int>(height),
            EGL_TEXTURE_TARGET, EGL_TEXTURE_2D,
            EGL_TEXTURE_FORMAT, EGL_TEXTURE_RGBA,
            EGL_NONE
        };

        EGLSurface surface = eglCreatePbufferFromClientBuffer(mEglDisplay, EGL_D3D_TEXTURE_2D_SHARE_HANDLE_ANGLE, sharedHandle, mEGLConfig, pBufferAttributes);
        if (surface != EGL_NO_SURFACE)
        {
            m_elgSurfaces[sharedHandle] = surface;
        }
    }
}

void ANGLE::AngleResources::MakeCurrect(HANDLE sharedHandle)
{
    auto it = m_elgSurfaces.find(sharedHandle);
    if (it != m_elgSurfaces.end())
    {
        EGLSurface surface = it->second;
        auto result = eglMakeCurrent(mEglDisplay, surface, surface, mEglContext);
        assert(result == EGL_TRUE);
    }
}

void ANGLE::AngleResources::RemoveSharedTexture(HANDLE sharedHandle)
{
    auto it = m_elgSurfaces.find(sharedHandle);
    if (it != m_elgSurfaces.end())
    {
        eglDestroySurface(mEglDisplay, it->second);       
        m_elgSurfaces.erase(it);
    }
}

