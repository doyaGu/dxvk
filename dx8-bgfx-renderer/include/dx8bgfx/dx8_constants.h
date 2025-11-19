#pragma once

#include "dx8_types.h"

namespace dx8bgfx {

// =============================================================================
// Render States (D3DRENDERSTATETYPE)
// =============================================================================
enum D3DRENDERSTATETYPE {
    D3DRS_ZENABLE = 7,
    D3DRS_FILLMODE = 8,
    D3DRS_SHADEMODE = 9,
    D3DRS_ZWRITEENABLE = 14,
    D3DRS_ALPHATESTENABLE = 15,
    D3DRS_LASTPIXEL = 16,
    D3DRS_SRCBLEND = 19,
    D3DRS_DESTBLEND = 20,
    D3DRS_CULLMODE = 22,
    D3DRS_ZFUNC = 23,
    D3DRS_ALPHAREF = 24,
    D3DRS_ALPHAFUNC = 25,
    D3DRS_DITHERENABLE = 26,
    D3DRS_ALPHABLENDENABLE = 27,
    D3DRS_FOGENABLE = 28,
    D3DRS_SPECULARENABLE = 29,
    D3DRS_FOGCOLOR = 34,
    D3DRS_FOGTABLEMODE = 35,
    D3DRS_FOGSTART = 36,
    D3DRS_FOGEND = 37,
    D3DRS_FOGDENSITY = 38,
    D3DRS_RANGEFOGENABLE = 48,
    D3DRS_STENCILENABLE = 52,
    D3DRS_STENCILFAIL = 53,
    D3DRS_STENCILZFAIL = 54,
    D3DRS_STENCILPASS = 55,
    D3DRS_STENCILFUNC = 56,
    D3DRS_STENCILREF = 57,
    D3DRS_STENCILMASK = 58,
    D3DRS_STENCILWRITEMASK = 59,
    D3DRS_TEXTUREFACTOR = 60,
    D3DRS_WRAP0 = 128,
    D3DRS_WRAP1 = 129,
    D3DRS_WRAP2 = 130,
    D3DRS_WRAP3 = 131,
    D3DRS_WRAP4 = 132,
    D3DRS_WRAP5 = 133,
    D3DRS_WRAP6 = 134,
    D3DRS_WRAP7 = 135,
    D3DRS_CLIPPING = 136,
    D3DRS_LIGHTING = 137,
    D3DRS_AMBIENT = 139,
    D3DRS_FOGVERTEXMODE = 140,
    D3DRS_COLORVERTEX = 141,
    D3DRS_LOCALVIEWER = 142,
    D3DRS_NORMALIZENORMALS = 143,
    D3DRS_DIFFUSEMATERIALSOURCE = 145,
    D3DRS_SPECULARMATERIALSOURCE = 146,
    D3DRS_AMBIENTMATERIALSOURCE = 147,
    D3DRS_EMISSIVEMATERIALSOURCE = 148,
    D3DRS_VERTEXBLEND = 151,
    D3DRS_CLIPPLANEENABLE = 152,
    D3DRS_POINTSIZE = 154,
    D3DRS_POINTSIZE_MIN = 155,
    D3DRS_POINTSPRITEENABLE = 156,
    D3DRS_POINTSCALEENABLE = 157,
    D3DRS_POINTSCALE_A = 158,
    D3DRS_POINTSCALE_B = 159,
    D3DRS_POINTSCALE_C = 160,
    D3DRS_MULTISAMPLEANTIALIAS = 161,
    D3DRS_MULTISAMPLEMASK = 162,
    D3DRS_PATCHEDGESTYLE = 163,
    D3DRS_DEBUGMONITORTOKEN = 165,
    D3DRS_POINTSIZE_MAX = 166,
    D3DRS_INDEXEDVERTEXBLENDENABLE = 167,
    D3DRS_COLORWRITEENABLE = 168,
    D3DRS_TWEENFACTOR = 170,
    D3DRS_BLENDOP = 171,

    D3DRS_MAX = 256
};

// =============================================================================
// Texture Stage States (D3DTEXTURESTAGESTATETYPE)
// =============================================================================
enum D3DTEXTURESTAGESTATETYPE {
    D3DTSS_COLOROP = 1,
    D3DTSS_COLORARG1 = 2,
    D3DTSS_COLORARG2 = 3,
    D3DTSS_ALPHAOP = 4,
    D3DTSS_ALPHAARG1 = 5,
    D3DTSS_ALPHAARG2 = 6,
    D3DTSS_BUMPENVMAT00 = 7,
    D3DTSS_BUMPENVMAT01 = 8,
    D3DTSS_BUMPENVMAT10 = 9,
    D3DTSS_BUMPENVMAT11 = 10,
    D3DTSS_TEXCOORDINDEX = 11,
    D3DTSS_BORDERCOLOR = 12,  // D3D9 sampler state
    D3DTSS_MAGFILTER = 13,    // D3D9 sampler state
    D3DTSS_MINFILTER = 14,    // D3D9 sampler state
    D3DTSS_MIPFILTER = 15,    // D3D9 sampler state
    D3DTSS_MIPMAPLODBIAS = 16,// D3D9 sampler state
    D3DTSS_MAXMIPLEVEL = 17,  // D3D9 sampler state
    D3DTSS_MAXANISOTROPY = 18,// D3D9 sampler state
    D3DTSS_BUMPENVLSCALE = 19,
    D3DTSS_BUMPENVLOFFSET = 20,
    D3DTSS_TEXTURETRANSFORMFLAGS = 21,
    D3DTSS_ADDRESSU = 22,     // D3D9 sampler state
    D3DTSS_ADDRESSV = 23,     // D3D9 sampler state
    D3DTSS_ADDRESSW = 24,     // D3D9 sampler state
    D3DTSS_COLORARG0 = 25,
    D3DTSS_ALPHAARG0 = 26,
    D3DTSS_RESULTARG = 27,

    D3DTSS_MAX = 32
};

// =============================================================================
// Texture Operations (D3DTEXTUREOP)
// =============================================================================
enum D3DTEXTUREOP {
    D3DTOP_DISABLE = 1,
    D3DTOP_SELECTARG1 = 2,
    D3DTOP_SELECTARG2 = 3,
    D3DTOP_MODULATE = 4,
    D3DTOP_MODULATE2X = 5,
    D3DTOP_MODULATE4X = 6,
    D3DTOP_ADD = 7,
    D3DTOP_ADDSIGNED = 8,
    D3DTOP_ADDSIGNED2X = 9,
    D3DTOP_SUBTRACT = 10,
    D3DTOP_ADDSMOOTH = 11,
    D3DTOP_BLENDDIFFUSEALPHA = 12,
    D3DTOP_BLENDTEXTUREALPHA = 13,
    D3DTOP_BLENDFACTORALPHA = 14,
    D3DTOP_BLENDTEXTUREALPHAPM = 15,
    D3DTOP_BLENDCURRENTALPHA = 16,
    D3DTOP_PREMODULATE = 17,
    D3DTOP_MODULATEALPHA_ADDCOLOR = 18,
    D3DTOP_MODULATECOLOR_ADDALPHA = 19,
    D3DTOP_MODULATEINVALPHA_ADDCOLOR = 20,
    D3DTOP_MODULATEINVCOLOR_ADDALPHA = 21,
    D3DTOP_BUMPENVMAP = 22,
    D3DTOP_BUMPENVMAPLUMINANCE = 23,
    D3DTOP_DOTPRODUCT3 = 24,
    D3DTOP_MULTIPLYADD = 25,
    D3DTOP_LERP = 26,
};

// =============================================================================
// Texture Arguments (D3DTA_*)
// =============================================================================
constexpr DWORD D3DTA_SELECTMASK = 0x0000000F;
constexpr DWORD D3DTA_DIFFUSE = 0x00000000;
constexpr DWORD D3DTA_CURRENT = 0x00000001;
constexpr DWORD D3DTA_TEXTURE = 0x00000002;
constexpr DWORD D3DTA_TFACTOR = 0x00000003;
constexpr DWORD D3DTA_SPECULAR = 0x00000004;
constexpr DWORD D3DTA_TEMP = 0x00000005;
constexpr DWORD D3DTA_CONSTANT = 0x00000006;
constexpr DWORD D3DTA_COMPLEMENT = 0x00000010;
constexpr DWORD D3DTA_ALPHAREPLICATE = 0x00000020;

// =============================================================================
// Texture Coordinate Index flags
// =============================================================================
constexpr DWORD D3DTSS_TCI_PASSTHRU = 0x00000000;
constexpr DWORD D3DTSS_TCI_CAMERASPACENORMAL = 0x00010000;
constexpr DWORD D3DTSS_TCI_CAMERASPACEPOSITION = 0x00020000;
constexpr DWORD D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR = 0x00030000;
constexpr DWORD D3DTSS_TCI_SPHEREMAP = 0x00040000;

// =============================================================================
// Texture Transform Flags
// =============================================================================
enum D3DTEXTURETRANSFORMFLAGS {
    D3DTTFF_DISABLE = 0,
    D3DTTFF_COUNT1 = 1,
    D3DTTFF_COUNT2 = 2,
    D3DTTFF_COUNT3 = 3,
    D3DTTFF_COUNT4 = 4,
    D3DTTFF_PROJECTED = 256,
};

// =============================================================================
// Blend Operations
// =============================================================================
enum D3DBLEND {
    D3DBLEND_ZERO = 1,
    D3DBLEND_ONE = 2,
    D3DBLEND_SRCCOLOR = 3,
    D3DBLEND_INVSRCCOLOR = 4,
    D3DBLEND_SRCALPHA = 5,
    D3DBLEND_INVSRCALPHA = 6,
    D3DBLEND_DESTALPHA = 7,
    D3DBLEND_INVDESTALPHA = 8,
    D3DBLEND_DESTCOLOR = 9,
    D3DBLEND_INVDESTCOLOR = 10,
    D3DBLEND_SRCALPHASAT = 11,
    D3DBLEND_BOTHSRCALPHA = 12,
    D3DBLEND_BOTHINVSRCALPHA = 13,
};

enum D3DBLENDOP {
    D3DBLENDOP_ADD = 1,
    D3DBLENDOP_SUBTRACT = 2,
    D3DBLENDOP_REVSUBTRACT = 3,
    D3DBLENDOP_MIN = 4,
    D3DBLENDOP_MAX = 5,
};

// =============================================================================
// Comparison Functions
// =============================================================================
enum D3DCMPFUNC {
    D3DCMP_NEVER = 1,
    D3DCMP_LESS = 2,
    D3DCMP_EQUAL = 3,
    D3DCMP_LESSEQUAL = 4,
    D3DCMP_GREATER = 5,
    D3DCMP_NOTEQUAL = 6,
    D3DCMP_GREATEREQUAL = 7,
    D3DCMP_ALWAYS = 8,
};

// =============================================================================
// Cull Modes
// =============================================================================
enum D3DCULL {
    D3DCULL_NONE = 1,
    D3DCULL_CW = 2,
    D3DCULL_CCW = 3,
};

// =============================================================================
// Fill Modes
// =============================================================================
enum D3DFILLMODE {
    D3DFILL_POINT = 1,
    D3DFILL_WIREFRAME = 2,
    D3DFILL_SOLID = 3,
};

// =============================================================================
// Shade Modes
// =============================================================================
enum D3DSHADEMODE {
    D3DSHADE_FLAT = 1,
    D3DSHADE_GOURAUD = 2,
    D3DSHADE_PHONG = 3,  // Not supported
};

// =============================================================================
// Fog Modes
// =============================================================================
enum D3DFOGMODE {
    D3DFOG_NONE = 0,
    D3DFOG_EXP = 1,
    D3DFOG_EXP2 = 2,
    D3DFOG_LINEAR = 3,
};

// =============================================================================
// Material Color Source
// =============================================================================
enum D3DMATERIALCOLORSOURCE {
    D3DMCS_MATERIAL = 0,
    D3DMCS_COLOR1 = 1,
    D3DMCS_COLOR2 = 2,
};

// =============================================================================
// Vertex Blend Flags
// =============================================================================
enum D3DVERTEXBLENDFLAGS {
    D3DVBF_DISABLE = 0,
    D3DVBF_1WEIGHTS = 1,
    D3DVBF_2WEIGHTS = 2,
    D3DVBF_3WEIGHTS = 3,
    D3DVBF_TWEENING = 255,
    D3DVBF_0WEIGHTS = 256,
};

// =============================================================================
// Texture Address Modes
// =============================================================================
enum D3DTEXTUREADDRESS {
    D3DTADDRESS_WRAP = 1,
    D3DTADDRESS_MIRROR = 2,
    D3DTADDRESS_CLAMP = 3,
    D3DTADDRESS_BORDER = 4,
    D3DTADDRESS_MIRRORONCE = 5,
};

// =============================================================================
// Texture Filter Types
// =============================================================================
enum D3DTEXTUREFILTERTYPE {
    D3DTEXF_NONE = 0,
    D3DTEXF_POINT = 1,
    D3DTEXF_LINEAR = 2,
    D3DTEXF_ANISOTROPIC = 3,
    D3DTEXF_PYRAMIDALQUAD = 6,
    D3DTEXF_GAUSSIANQUAD = 7,
};

// =============================================================================
// Stencil Operations
// =============================================================================
enum D3DSTENCILOP {
    D3DSTENCILOP_KEEP = 1,
    D3DSTENCILOP_ZERO = 2,
    D3DSTENCILOP_REPLACE = 3,
    D3DSTENCILOP_INCRSAT = 4,
    D3DSTENCILOP_DECRSAT = 5,
    D3DSTENCILOP_INVERT = 6,
    D3DSTENCILOP_INCR = 7,
    D3DSTENCILOP_DECR = 8,
};

// =============================================================================
// Z Buffer Types
// =============================================================================
enum D3DZBUFFERTYPE {
    D3DZB_FALSE = 0,
    D3DZB_TRUE = 1,
    D3DZB_USEW = 2,
};

// =============================================================================
// Default render state values
// =============================================================================
namespace DefaultRenderState {
    constexpr DWORD ZEnable = D3DZB_TRUE;
    constexpr DWORD FillMode = D3DFILL_SOLID;
    constexpr DWORD ShadeMode = D3DSHADE_GOURAUD;
    constexpr DWORD ZWriteEnable = TRUE;
    constexpr DWORD AlphaTestEnable = FALSE;
    constexpr DWORD SrcBlend = D3DBLEND_ONE;
    constexpr DWORD DestBlend = D3DBLEND_ZERO;
    constexpr DWORD CullMode = D3DCULL_CCW;
    constexpr DWORD ZFunc = D3DCMP_LESSEQUAL;
    constexpr DWORD AlphaRef = 0;
    constexpr DWORD AlphaFunc = D3DCMP_ALWAYS;
    constexpr DWORD AlphaBlendEnable = FALSE;
    constexpr DWORD FogEnable = FALSE;
    constexpr DWORD SpecularEnable = FALSE;
    constexpr DWORD FogColor = 0;
    constexpr DWORD FogTableMode = D3DFOG_NONE;
    constexpr DWORD FogVertexMode = D3DFOG_NONE;
    constexpr DWORD RangeFogEnable = FALSE;
    constexpr DWORD Lighting = TRUE;
    constexpr DWORD Ambient = 0;
    constexpr DWORD ColorVertex = TRUE;
    constexpr DWORD LocalViewer = TRUE;
    constexpr DWORD NormalizeNormals = FALSE;
    constexpr DWORD DiffuseMaterialSource = D3DMCS_COLOR1;
    constexpr DWORD SpecularMaterialSource = D3DMCS_COLOR2;
    constexpr DWORD AmbientMaterialSource = D3DMCS_MATERIAL;
    constexpr DWORD EmissiveMaterialSource = D3DMCS_MATERIAL;
    constexpr DWORD VertexBlend = D3DVBF_DISABLE;
    constexpr DWORD Clipping = TRUE;
}

// =============================================================================
// Default texture stage state values
// =============================================================================
namespace DefaultTextureStageState {
    constexpr DWORD ColorOp = D3DTOP_DISABLE;  // Stage 0: MODULATE, others: DISABLE
    constexpr DWORD ColorArg1 = D3DTA_TEXTURE;
    constexpr DWORD ColorArg2 = D3DTA_CURRENT;
    constexpr DWORD ColorArg0 = D3DTA_CURRENT;
    constexpr DWORD AlphaOp = D3DTOP_DISABLE;  // Stage 0: SELECTARG1, others: DISABLE
    constexpr DWORD AlphaArg1 = D3DTA_TEXTURE;
    constexpr DWORD AlphaArg2 = D3DTA_CURRENT;
    constexpr DWORD AlphaArg0 = D3DTA_CURRENT;
    constexpr DWORD ResultArg = D3DTA_CURRENT;
    constexpr DWORD TexCoordIndex = 0;  // Same as stage index
    constexpr DWORD TextureTransformFlags = D3DTTFF_DISABLE;
}

} // namespace dx8bgfx
