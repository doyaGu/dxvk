#pragma once

#include "dx8_types.h"
#include "dx8_constants.h"
#include "dx8_state_manager.h"

#include <bgfx/bgfx.h>

namespace dx8bgfx {

// =============================================================================
// Stencil/Depth State Utilities
// =============================================================================

class StencilUtils {
public:
    // Convert D3D8 stencil operation to bgfx
    static uint32_t D3DStencilOpToBgfx(D3DSTENCILOP op);

    // Convert D3D8 comparison function to bgfx stencil test
    static uint32_t D3DCmpFuncToBgfxStencil(D3DCMPFUNC func);

    // Convert D3D8 comparison function to bgfx depth test
    static uint64_t D3DCmpFuncToBgfxDepth(D3DCMPFUNC func);

    // Build complete bgfx depth state from state manager
    static uint64_t BuildDepthState(const StateManager& state);

    // Build complete bgfx stencil state from state manager
    static uint32_t BuildStencilState(const StateManager& state);

    // Get combined depth-stencil state
    static uint64_t BuildDepthStencilState(const StateManager& state);

    // Helper to create stencil function
    static uint32_t MakeStencilFunc(
        uint32_t test,
        uint32_t stencilFail,
        uint32_t depthFail,
        uint32_t depthPass
    );
};

// =============================================================================
// Blend State Utilities
// =============================================================================

class BlendUtils {
public:
    // Convert D3D8 blend factor to bgfx
    static uint64_t D3DBlendToBgfx(D3DBLEND blend);

    // Convert D3D8 blend operation to bgfx
    static uint64_t D3DBlendOpToBgfx(D3DBLENDOP op);

    // Build complete bgfx blend state from state manager
    static uint64_t BuildBlendState(const StateManager& state);

    // Build write mask state
    static uint64_t BuildWriteMaskState(const StateManager& state);
};

// =============================================================================
// Rasterizer State Utilities
// =============================================================================

class RasterizerUtils {
public:
    // Build cull mode state
    static uint64_t BuildCullState(const StateManager& state);

    // Build complete rasterizer state (cull, fill, scissors, etc.)
    static uint64_t BuildRasterizerState(const StateManager& state);
};

// =============================================================================
// Complete State Builder
// =============================================================================

class StateBuilder {
public:
    // Build all bgfx states from D3D8 state manager
    static void BuildAllStates(
        const StateManager& state,
        uint64_t& outState,
        uint32_t& outStencil,
        uint32_t& outRgba
    );

    // Calculate alpha reference value for bgfx
    static uint32_t BuildAlphaRef(const StateManager& state);
};

} // namespace dx8bgfx
