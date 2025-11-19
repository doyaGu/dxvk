#include "dx8bgfx/dx8_stencil_utils.h"

namespace dx8bgfx {

// =============================================================================
// Stencil Utils Implementation
// =============================================================================

uint32_t StencilUtils::D3DStencilOpToBgfx(D3DSTENCILOP op) {
    switch (op) {
        case D3DSTENCILOP_KEEP:    return BGFX_STENCIL_OP_FAIL_S_KEEP;
        case D3DSTENCILOP_ZERO:    return BGFX_STENCIL_OP_FAIL_S_ZERO;
        case D3DSTENCILOP_REPLACE: return BGFX_STENCIL_OP_FAIL_S_REPLACE;
        case D3DSTENCILOP_INCRSAT: return BGFX_STENCIL_OP_FAIL_S_INCR;
        case D3DSTENCILOP_DECRSAT: return BGFX_STENCIL_OP_FAIL_S_DECR;
        case D3DSTENCILOP_INVERT:  return BGFX_STENCIL_OP_FAIL_S_INVERT;
        case D3DSTENCILOP_INCR:    return BGFX_STENCIL_OP_FAIL_S_INCRSAT;
        case D3DSTENCILOP_DECR:    return BGFX_STENCIL_OP_FAIL_S_DECRSAT;
        default:                   return BGFX_STENCIL_OP_FAIL_S_KEEP;
    }
}

uint32_t StencilUtils::D3DCmpFuncToBgfxStencil(D3DCMPFUNC func) {
    switch (func) {
        case D3DCMP_NEVER:        return BGFX_STENCIL_TEST_NEVER;
        case D3DCMP_LESS:         return BGFX_STENCIL_TEST_LESS;
        case D3DCMP_EQUAL:        return BGFX_STENCIL_TEST_EQUAL;
        case D3DCMP_LESSEQUAL:    return BGFX_STENCIL_TEST_LEQUAL;
        case D3DCMP_GREATER:      return BGFX_STENCIL_TEST_GREATER;
        case D3DCMP_NOTEQUAL:     return BGFX_STENCIL_TEST_NOTEQUAL;
        case D3DCMP_GREATEREQUAL: return BGFX_STENCIL_TEST_GEQUAL;
        case D3DCMP_ALWAYS:       return BGFX_STENCIL_TEST_ALWAYS;
        default:                  return BGFX_STENCIL_TEST_ALWAYS;
    }
}

uint64_t StencilUtils::D3DCmpFuncToBgfxDepth(D3DCMPFUNC func) {
    switch (func) {
        case D3DCMP_NEVER:        return BGFX_STATE_DEPTH_TEST_NEVER;
        case D3DCMP_LESS:         return BGFX_STATE_DEPTH_TEST_LESS;
        case D3DCMP_EQUAL:        return BGFX_STATE_DEPTH_TEST_EQUAL;
        case D3DCMP_LESSEQUAL:    return BGFX_STATE_DEPTH_TEST_LEQUAL;
        case D3DCMP_GREATER:      return BGFX_STATE_DEPTH_TEST_GREATER;
        case D3DCMP_NOTEQUAL:     return BGFX_STATE_DEPTH_TEST_NOTEQUAL;
        case D3DCMP_GREATEREQUAL: return BGFX_STATE_DEPTH_TEST_GEQUAL;
        case D3DCMP_ALWAYS:       return BGFX_STATE_DEPTH_TEST_ALWAYS;
        default:                  return BGFX_STATE_DEPTH_TEST_LESS;
    }
}

uint64_t StencilUtils::BuildDepthState(const StateManager& state) {
    uint64_t bgfxState = 0;

    DWORD zEnable = 0;
    DWORD zWriteEnable = 0;
    DWORD zFunc = D3DCMP_LESSEQUAL;

    state.GetRenderState(D3DRS_ZENABLE, &zEnable);
    state.GetRenderState(D3DRS_ZWRITEENABLE, &zWriteEnable);
    state.GetRenderState(D3DRS_ZFUNC, &zFunc);

    if (zEnable) {
        bgfxState |= D3DCmpFuncToBgfxDepth(static_cast<D3DCMPFUNC>(zFunc));
    }

    if (zWriteEnable) {
        bgfxState |= BGFX_STATE_WRITE_Z;
    }

    return bgfxState;
}

uint32_t StencilUtils::MakeStencilFunc(
    uint32_t test,
    uint32_t stencilFail,
    uint32_t depthFail,
    uint32_t depthPass
) {
    // Convert stencil fail op to correct position
    uint32_t sfail = 0;
    switch (stencilFail) {
        case BGFX_STENCIL_OP_FAIL_S_KEEP:    sfail = BGFX_STENCIL_OP_FAIL_S_KEEP; break;
        case BGFX_STENCIL_OP_FAIL_S_ZERO:    sfail = BGFX_STENCIL_OP_FAIL_S_ZERO; break;
        case BGFX_STENCIL_OP_FAIL_S_REPLACE: sfail = BGFX_STENCIL_OP_FAIL_S_REPLACE; break;
        case BGFX_STENCIL_OP_FAIL_S_INCR:    sfail = BGFX_STENCIL_OP_FAIL_S_INCR; break;
        case BGFX_STENCIL_OP_FAIL_S_INCRSAT: sfail = BGFX_STENCIL_OP_FAIL_S_INCRSAT; break;
        case BGFX_STENCIL_OP_FAIL_S_DECR:    sfail = BGFX_STENCIL_OP_FAIL_S_DECR; break;
        case BGFX_STENCIL_OP_FAIL_S_DECRSAT: sfail = BGFX_STENCIL_OP_FAIL_S_DECRSAT; break;
        case BGFX_STENCIL_OP_FAIL_S_INVERT:  sfail = BGFX_STENCIL_OP_FAIL_S_INVERT; break;
        default: sfail = BGFX_STENCIL_OP_FAIL_S_KEEP; break;
    }

    // Convert depth fail to correct position
    uint32_t zfail = (depthFail >> 4) & 0xF;
    zfail = zfail << 8;

    // Convert depth pass to correct position
    uint32_t zpass = (depthPass >> 4) & 0xF;
    zpass = zpass << 12;

    return test | sfail | zfail | zpass;
}

uint32_t StencilUtils::BuildStencilState(const StateManager& state) {
    DWORD stencilEnable = 0;
    state.GetRenderState(D3DRS_STENCILENABLE, &stencilEnable);

    if (!stencilEnable) {
        return BGFX_STENCIL_NONE;
    }

    DWORD stencilFunc = D3DCMP_ALWAYS;
    DWORD stencilFail = D3DSTENCILOP_KEEP;
    DWORD stencilZFail = D3DSTENCILOP_KEEP;
    DWORD stencilPass = D3DSTENCILOP_KEEP;
    DWORD stencilRef = 0;
    DWORD stencilMask = 0xFF;
    DWORD stencilWriteMask = 0xFF;

    state.GetRenderState(D3DRS_STENCILFUNC, &stencilFunc);
    state.GetRenderState(D3DRS_STENCILFAIL, &stencilFail);
    state.GetRenderState(D3DRS_STENCILZFAIL, &stencilZFail);
    state.GetRenderState(D3DRS_STENCILPASS, &stencilPass);
    state.GetRenderState(D3DRS_STENCILREF, &stencilRef);
    state.GetRenderState(D3DRS_STENCILMASK, &stencilMask);
    state.GetRenderState(D3DRS_STENCILWRITEMASK, &stencilWriteMask);

    uint32_t bgfxStencil = 0;

    // Stencil test function
    bgfxStencil |= D3DCmpFuncToBgfxStencil(static_cast<D3DCMPFUNC>(stencilFunc));

    // Stencil fail operation
    switch (static_cast<D3DSTENCILOP>(stencilFail)) {
        case D3DSTENCILOP_KEEP:    bgfxStencil |= BGFX_STENCIL_OP_FAIL_S_KEEP; break;
        case D3DSTENCILOP_ZERO:    bgfxStencil |= BGFX_STENCIL_OP_FAIL_S_ZERO; break;
        case D3DSTENCILOP_REPLACE: bgfxStencil |= BGFX_STENCIL_OP_FAIL_S_REPLACE; break;
        case D3DSTENCILOP_INCRSAT: bgfxStencil |= BGFX_STENCIL_OP_FAIL_S_INCR; break;
        case D3DSTENCILOP_DECRSAT: bgfxStencil |= BGFX_STENCIL_OP_FAIL_S_DECR; break;
        case D3DSTENCILOP_INVERT:  bgfxStencil |= BGFX_STENCIL_OP_FAIL_S_INVERT; break;
        case D3DSTENCILOP_INCR:    bgfxStencil |= BGFX_STENCIL_OP_FAIL_S_INCRSAT; break;
        case D3DSTENCILOP_DECR:    bgfxStencil |= BGFX_STENCIL_OP_FAIL_S_DECRSAT; break;
        default: bgfxStencil |= BGFX_STENCIL_OP_FAIL_S_KEEP; break;
    }

    // Depth fail operation
    switch (static_cast<D3DSTENCILOP>(stencilZFail)) {
        case D3DSTENCILOP_KEEP:    bgfxStencil |= BGFX_STENCIL_OP_FAIL_Z_KEEP; break;
        case D3DSTENCILOP_ZERO:    bgfxStencil |= BGFX_STENCIL_OP_FAIL_Z_ZERO; break;
        case D3DSTENCILOP_REPLACE: bgfxStencil |= BGFX_STENCIL_OP_FAIL_Z_REPLACE; break;
        case D3DSTENCILOP_INCRSAT: bgfxStencil |= BGFX_STENCIL_OP_FAIL_Z_INCR; break;
        case D3DSTENCILOP_DECRSAT: bgfxStencil |= BGFX_STENCIL_OP_FAIL_Z_DECR; break;
        case D3DSTENCILOP_INVERT:  bgfxStencil |= BGFX_STENCIL_OP_FAIL_Z_INVERT; break;
        case D3DSTENCILOP_INCR:    bgfxStencil |= BGFX_STENCIL_OP_FAIL_Z_INCRSAT; break;
        case D3DSTENCILOP_DECR:    bgfxStencil |= BGFX_STENCIL_OP_FAIL_Z_DECRSAT; break;
        default: bgfxStencil |= BGFX_STENCIL_OP_FAIL_Z_KEEP; break;
    }

    // Depth pass operation
    switch (static_cast<D3DSTENCILOP>(stencilPass)) {
        case D3DSTENCILOP_KEEP:    bgfxStencil |= BGFX_STENCIL_OP_PASS_Z_KEEP; break;
        case D3DSTENCILOP_ZERO:    bgfxStencil |= BGFX_STENCIL_OP_PASS_Z_ZERO; break;
        case D3DSTENCILOP_REPLACE: bgfxStencil |= BGFX_STENCIL_OP_PASS_Z_REPLACE; break;
        case D3DSTENCILOP_INCRSAT: bgfxStencil |= BGFX_STENCIL_OP_PASS_Z_INCR; break;
        case D3DSTENCILOP_DECRSAT: bgfxStencil |= BGFX_STENCIL_OP_PASS_Z_DECR; break;
        case D3DSTENCILOP_INVERT:  bgfxStencil |= BGFX_STENCIL_OP_PASS_Z_INVERT; break;
        case D3DSTENCILOP_INCR:    bgfxStencil |= BGFX_STENCIL_OP_PASS_Z_INCRSAT; break;
        case D3DSTENCILOP_DECR:    bgfxStencil |= BGFX_STENCIL_OP_PASS_Z_DECRSAT; break;
        default: bgfxStencil |= BGFX_STENCIL_OP_PASS_Z_KEEP; break;
    }

    // Reference value and masks
    bgfxStencil |= BGFX_STENCIL_FUNC_REF(stencilRef);
    bgfxStencil |= BGFX_STENCIL_FUNC_RMASK(stencilMask);

    return bgfxStencil;
}

uint64_t StencilUtils::BuildDepthStencilState(const StateManager& state) {
    return BuildDepthState(state);
}

// =============================================================================
// Blend Utils Implementation
// =============================================================================

uint64_t BlendUtils::D3DBlendToBgfx(D3DBLEND blend) {
    switch (blend) {
        case D3DBLEND_ZERO:            return BGFX_STATE_BLEND_ZERO;
        case D3DBLEND_ONE:             return BGFX_STATE_BLEND_ONE;
        case D3DBLEND_SRCCOLOR:        return BGFX_STATE_BLEND_SRC_COLOR;
        case D3DBLEND_INVSRCCOLOR:     return BGFX_STATE_BLEND_INV_SRC_COLOR;
        case D3DBLEND_SRCALPHA:        return BGFX_STATE_BLEND_SRC_ALPHA;
        case D3DBLEND_INVSRCALPHA:     return BGFX_STATE_BLEND_INV_SRC_ALPHA;
        case D3DBLEND_DESTALPHA:       return BGFX_STATE_BLEND_DST_ALPHA;
        case D3DBLEND_INVDESTALPHA:    return BGFX_STATE_BLEND_INV_DST_ALPHA;
        case D3DBLEND_DESTCOLOR:       return BGFX_STATE_BLEND_DST_COLOR;
        case D3DBLEND_INVDESTCOLOR:    return BGFX_STATE_BLEND_INV_DST_COLOR;
        case D3DBLEND_SRCALPHASAT:     return BGFX_STATE_BLEND_SRC_ALPHA_SAT;
        case D3DBLEND_BOTHSRCALPHA:    return BGFX_STATE_BLEND_SRC_ALPHA;
        case D3DBLEND_BOTHINVSRCALPHA: return BGFX_STATE_BLEND_INV_SRC_ALPHA;
        default:                       return BGFX_STATE_BLEND_ONE;
    }
}

uint64_t BlendUtils::D3DBlendOpToBgfx(D3DBLENDOP op) {
    switch (op) {
        case D3DBLENDOP_ADD:         return BGFX_STATE_BLEND_EQUATION_ADD;
        case D3DBLENDOP_SUBTRACT:    return BGFX_STATE_BLEND_EQUATION_SUB;
        case D3DBLENDOP_REVSUBTRACT: return BGFX_STATE_BLEND_EQUATION_REVSUB;
        case D3DBLENDOP_MIN:         return BGFX_STATE_BLEND_EQUATION_MIN;
        case D3DBLENDOP_MAX:         return BGFX_STATE_BLEND_EQUATION_MAX;
        default:                     return BGFX_STATE_BLEND_EQUATION_ADD;
    }
}

uint64_t BlendUtils::BuildBlendState(const StateManager& state) {
    uint64_t bgfxState = 0;

    DWORD alphaBlendEnable = 0;
    state.GetRenderState(D3DRS_ALPHABLENDENABLE, &alphaBlendEnable);

    if (alphaBlendEnable) {
        DWORD srcBlend = D3DBLEND_ONE;
        DWORD dstBlend = D3DBLEND_ZERO;

        state.GetRenderState(D3DRS_SRCBLEND, &srcBlend);
        state.GetRenderState(D3DRS_DESTBLEND, &dstBlend);

        uint64_t src = D3DBlendToBgfx(static_cast<D3DBLEND>(srcBlend));
        uint64_t dst = D3DBlendToBgfx(static_cast<D3DBLEND>(dstBlend));

        bgfxState |= BGFX_STATE_BLEND_FUNC(src, dst);
    }

    return bgfxState;
}

uint64_t BlendUtils::BuildWriteMaskState(const StateManager& state) {
    uint64_t bgfxState = 0;

    DWORD colorWriteEnable = 0xF; // All channels
    state.GetRenderState(D3DRS_COLORWRITEENABLE, &colorWriteEnable);

    if (colorWriteEnable & D3DCOLORWRITEENABLE_RED)
        bgfxState |= BGFX_STATE_WRITE_R;
    if (colorWriteEnable & D3DCOLORWRITEENABLE_GREEN)
        bgfxState |= BGFX_STATE_WRITE_G;
    if (colorWriteEnable & D3DCOLORWRITEENABLE_BLUE)
        bgfxState |= BGFX_STATE_WRITE_B;
    if (colorWriteEnable & D3DCOLORWRITEENABLE_ALPHA)
        bgfxState |= BGFX_STATE_WRITE_A;

    return bgfxState;
}

// =============================================================================
// Rasterizer Utils Implementation
// =============================================================================

uint64_t RasterizerUtils::BuildCullState(const StateManager& state) {
    DWORD cullMode = D3DCULL_CCW;
    state.GetRenderState(D3DRS_CULLMODE, &cullMode);

    switch (static_cast<D3DCULL>(cullMode)) {
        case D3DCULL_NONE: return 0;
        case D3DCULL_CW:   return BGFX_STATE_CULL_CW;
        case D3DCULL_CCW:  return BGFX_STATE_CULL_CCW;
        default:           return BGFX_STATE_CULL_CCW;
    }
}

uint64_t RasterizerUtils::BuildRasterizerState(const StateManager& state) {
    uint64_t bgfxState = 0;

    // Cull mode
    bgfxState |= BuildCullState(state);

    // Fill mode
    DWORD fillMode = D3DFILL_SOLID;
    state.GetRenderState(D3DRS_FILLMODE, &fillMode);

    if (fillMode == D3DFILL_WIREFRAME) {
        bgfxState |= BGFX_STATE_PT_LINES;
    } else if (fillMode == D3DFILL_POINT) {
        bgfxState |= BGFX_STATE_PT_POINTS;
    }

    // MSAA
    DWORD msaaEnable = 0;
    state.GetRenderState(D3DRS_MULTISAMPLEANTIALIAS, &msaaEnable);
    if (msaaEnable) {
        bgfxState |= BGFX_STATE_MSAA;
    }

    return bgfxState;
}

// =============================================================================
// State Builder Implementation
// =============================================================================

void StateBuilder::BuildAllStates(
    const StateManager& state,
    uint64_t& outState,
    uint32_t& outStencil,
    uint32_t& outRgba
) {
    outState = 0;

    // Depth state
    outState |= StencilUtils::BuildDepthState(state);

    // Blend state
    outState |= BlendUtils::BuildBlendState(state);

    // Write mask
    outState |= BlendUtils::BuildWriteMaskState(state);

    // Rasterizer state
    outState |= RasterizerUtils::BuildRasterizerState(state);

    // Stencil state (separate)
    outStencil = StencilUtils::BuildStencilState(state);

    // Blend color (rgba)
    DWORD textureFactor = 0xFFFFFFFF;
    state.GetRenderState(D3DRS_TEXTUREFACTOR, &textureFactor);
    outRgba = textureFactor;
}

uint32_t StateBuilder::BuildAlphaRef(const StateManager& state) {
    DWORD alphaRef = 0;
    state.GetRenderState(D3DRS_ALPHAREF, &alphaRef);
    return alphaRef & 0xFF;
}

} // namespace dx8bgfx
