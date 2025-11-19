#pragma once

#include <cstdint>
#include <cstring>
#include <array>

namespace dx8bgfx {

// Basic types
using DWORD = uint32_t;
using WORD = uint16_t;
using BYTE = uint8_t;
using BOOL = int32_t;
using UINT = uint32_t;
using INT = int32_t;
using FLOAT = float;
using HRESULT = int32_t;

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#ifndef S_OK
#define S_OK 0
#define E_FAIL -1
#define D3D_OK S_OK
#define D3DERR_INVALIDCALL -2
#endif

// Color type (ARGB format)
struct D3DCOLORVALUE {
    float r, g, b, a;
};

using D3DCOLOR = DWORD;

inline D3DCOLORVALUE ColorFromD3DCOLOR(D3DCOLOR color) {
    return {
        ((color >> 16) & 0xFF) / 255.0f,  // R
        ((color >> 8) & 0xFF) / 255.0f,   // G
        (color & 0xFF) / 255.0f,          // B
        ((color >> 24) & 0xFF) / 255.0f   // A
    };
}

inline D3DCOLOR D3DCOLORFromColorValue(const D3DCOLORVALUE& cv) {
    return (static_cast<DWORD>(cv.a * 255.0f) << 24) |
           (static_cast<DWORD>(cv.r * 255.0f) << 16) |
           (static_cast<DWORD>(cv.g * 255.0f) << 8) |
           (static_cast<DWORD>(cv.b * 255.0f));
}

// Vector types
struct D3DVECTOR {
    float x, y, z;
};

struct D3DVECTOR4 {
    float x, y, z, w;
};

// Matrix type (row-major)
struct D3DMATRIX {
    union {
        struct {
            float _11, _12, _13, _14;
            float _21, _22, _23, _24;
            float _31, _32, _33, _34;
            float _41, _42, _43, _44;
        };
        float m[4][4];
    };

    D3DMATRIX() {
        std::memset(m, 0, sizeof(m));
        _11 = _22 = _33 = _44 = 1.0f;
    }
};

// Material
struct D3DMATERIAL8 {
    D3DCOLORVALUE Diffuse;
    D3DCOLORVALUE Ambient;
    D3DCOLORVALUE Specular;
    D3DCOLORVALUE Emissive;
    float Power;
};

// Light types
enum D3DLIGHTTYPE {
    D3DLIGHT_POINT = 1,
    D3DLIGHT_SPOT = 2,
    D3DLIGHT_DIRECTIONAL = 3,
};

struct D3DLIGHT8 {
    D3DLIGHTTYPE Type;
    D3DCOLORVALUE Diffuse;
    D3DCOLORVALUE Specular;
    D3DCOLORVALUE Ambient;
    D3DVECTOR Position;
    D3DVECTOR Direction;
    float Range;
    float Falloff;
    float Attenuation0;
    float Attenuation1;
    float Attenuation2;
    float Theta;
    float Phi;
};

// Viewport
struct D3DVIEWPORT8 {
    DWORD X;
    DWORD Y;
    DWORD Width;
    DWORD Height;
    float MinZ;
    float MaxZ;
};

// Primitive types
enum D3DPRIMITIVETYPE {
    D3DPT_POINTLIST = 1,
    D3DPT_LINELIST = 2,
    D3DPT_LINESTRIP = 3,
    D3DPT_TRIANGLELIST = 4,
    D3DPT_TRIANGLESTRIP = 5,
    D3DPT_TRIANGLEFAN = 6,
};

// Transform state types
enum D3DTRANSFORMSTATETYPE {
    D3DTS_VIEW = 2,
    D3DTS_PROJECTION = 3,
    D3DTS_WORLD = 256,
    D3DTS_WORLD1 = 257,
    D3DTS_WORLD2 = 258,
    D3DTS_WORLD3 = 259,
    D3DTS_TEXTURE0 = 16,
    D3DTS_TEXTURE1 = 17,
    D3DTS_TEXTURE2 = 18,
    D3DTS_TEXTURE3 = 19,
    D3DTS_TEXTURE4 = 20,
    D3DTS_TEXTURE5 = 21,
    D3DTS_TEXTURE6 = 22,
    D3DTS_TEXTURE7 = 23,
};

// Flexible Vertex Format flags
constexpr DWORD D3DFVF_RESERVED0       = 0x0001;
constexpr DWORD D3DFVF_POSITION_MASK   = 0x000E;
constexpr DWORD D3DFVF_XYZ             = 0x0002;
constexpr DWORD D3DFVF_XYZRHW          = 0x0004;
constexpr DWORD D3DFVF_XYZB1           = 0x0006;
constexpr DWORD D3DFVF_XYZB2           = 0x0008;
constexpr DWORD D3DFVF_XYZB3           = 0x000A;
constexpr DWORD D3DFVF_XYZB4           = 0x000C;
constexpr DWORD D3DFVF_XYZB5           = 0x000E;
constexpr DWORD D3DFVF_NORMAL          = 0x0010;
constexpr DWORD D3DFVF_PSIZE           = 0x0020;
constexpr DWORD D3DFVF_DIFFUSE         = 0x0040;
constexpr DWORD D3DFVF_SPECULAR        = 0x0080;
constexpr DWORD D3DFVF_TEXCOUNT_MASK   = 0x0F00;
constexpr DWORD D3DFVF_TEXCOUNT_SHIFT  = 8;
constexpr DWORD D3DFVF_TEX0            = 0x0000;
constexpr DWORD D3DFVF_TEX1            = 0x0100;
constexpr DWORD D3DFVF_TEX2            = 0x0200;
constexpr DWORD D3DFVF_TEX3            = 0x0300;
constexpr DWORD D3DFVF_TEX4            = 0x0400;
constexpr DWORD D3DFVF_TEX5            = 0x0500;
constexpr DWORD D3DFVF_TEX6            = 0x0600;
constexpr DWORD D3DFVF_TEX7            = 0x0700;
constexpr DWORD D3DFVF_TEX8            = 0x0800;
constexpr DWORD D3DFVF_LASTBETA_UBYTE4 = 0x1000;
constexpr DWORD D3DFVF_LASTBETA_D3DCOLOR = 0x8000;

// Helper to get texture count from FVF
inline UINT GetTexCoordCount(DWORD fvf) {
    return (fvf & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
}

// Helper to check for transformed vertices
inline bool HasPositionT(DWORD fvf) {
    return (fvf & D3DFVF_POSITION_MASK) == D3DFVF_XYZRHW;
}

// Helper to get blend weight count
inline UINT GetBlendWeightCount(DWORD fvf) {
    DWORD pos = fvf & D3DFVF_POSITION_MASK;
    if (pos >= D3DFVF_XYZB1 && pos <= D3DFVF_XYZB5) {
        return (pos - D3DFVF_XYZB1) / 2 + 1;
    }
    return 0;
}

// Caps constants
constexpr UINT MaxTextureStages = 8;
constexpr UINT MaxLights = 8;
constexpr UINT MaxClipPlanes = 6;
constexpr UINT MaxWorldMatrices = 256;
constexpr UINT MaxStreams = 16;

// Internal light structure for GPU
struct GPULight {
    float Diffuse[4];
    float Specular[4];
    float Ambient[4];
    float Position[4];
    float Direction[4];
    DWORD Type;
    float Range;
    float Falloff;
    float Attenuation0;
    float Attenuation1;
    float Attenuation2;
    float Theta;  // cos(theta/2)
    float Phi;    // cos(phi/2)
};

// Internal material structure for GPU
struct GPUMaterial {
    float Diffuse[4];
    float Ambient[4];
    float Specular[4];
    float Emissive[4];
    float Power;
    float _pad[3];
};

} // namespace dx8bgfx
