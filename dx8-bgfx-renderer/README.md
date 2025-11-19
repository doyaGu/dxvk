# DX8-bgfx-renderer

A bgfx-based renderer that replicates the Direct3D 8 fixed-function pipeline exactly.

## Overview

This project implements the complete DX8 fixed-function pipeline using bgfx, providing:

- **Vertex Processing**: Transforms, vertex blending (up to 8 bones), lighting (8 lights)
- **Texture Stages**: All 26 texture operations across 8 stages
- **Fog**: Linear, exponential, exponential squared, range fog
- **Alpha Testing**: All 8 comparison functions
- **Material System**: Diffuse, ambient, specular, emissive colors

## Architecture

```
┌─────────────────────────────────────┐
│     DX8 Fixed Pipeline API          │
│  SetTransform, SetLight, SetTexture │
└──────────────┬──────────────────────┘
               ↓
┌─────────────────────────────────────┐
│      State Manager                  │
│  Track all render state changes     │
└──────────────┬──────────────────────┘
               ↓
┌─────────────────────────────────────┐
│     Shader Key Builder              │
│  Generate hash from current state   │
└──────────────┬──────────────────────┘
               ↓
┌─────────────────────────────────────┐
│     Shader Cache                    │
│  Lookup or compile shader variant   │
└──────────────┬──────────────────────┘
               ↓
┌─────────────────────────────────────┐
│          bgfx API                   │
└─────────────────────────────────────┘
```

## Building

### Prerequisites

- CMake 3.16+
- C++17 compiler
- bgfx (with bx and bimg)

### Build Steps

```bash
mkdir build && cd build
cmake -DBGFX_DIR=/path/to/bgfx ..
make -j$(nproc)
```

### Build Options

- `DX8BGFX_BUILD_EXAMPLES` - Build example applications (default: ON)
- `DX8BGFX_BUILD_TESTS` - Build unit tests (default: OFF)

## Usage

```cpp
#include <dx8bgfx/dx8_renderer.h>

// Initialize
dx8bgfx::Renderer renderer;
renderer.Init(width, height);

// Set transforms
D3DMATRIX world, view, proj;
renderer.SetTransform(D3DTS_WORLD, &world);
renderer.SetTransform(D3DTS_VIEW, &view);
renderer.SetTransform(D3DTS_PROJECTION, &proj);

// Set material
D3DMATERIAL8 material = {};
material.Diffuse = {1.0f, 1.0f, 1.0f, 1.0f};
material.Ambient = {0.2f, 0.2f, 0.2f, 1.0f};
material.Power = 32.0f;
renderer.SetMaterial(&material);

// Set light
D3DLIGHT8 light = {};
light.Type = D3DLIGHT_DIRECTIONAL;
light.Direction = {0.0f, -1.0f, 0.0f};
light.Diffuse = {1.0f, 1.0f, 1.0f, 1.0f};
renderer.SetLight(0, &light);
renderer.LightEnable(0, TRUE);

// Set texture and stage state
renderer.SetTexture(0, textureHandle);
renderer.SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
renderer.SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
renderer.SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

// Draw
renderer.SetVertexBuffer(vb);
renderer.DrawPrimitive(D3DPT_TRIANGLELIST, 0, triangleCount);

// Submit frame
renderer.EndFrame();
```

## Project Structure

```
dx8-bgfx-renderer/
├── include/dx8bgfx/       # Public headers
│   ├── dx8_types.h        # D3D8 type definitions
│   ├── dx8_constants.h    # Enums and constants
│   ├── dx8_state_manager.h
│   ├── dx8_shader_key.h
│   ├── dx8_shader_generator.h
│   ├── dx8_shader_cache.h
│   └── dx8_renderer.h
├── src/
│   ├── core/              # Core implementation
│   ├── state/             # State management
│   └── shader/            # Shader generation
├── shaders/               # bgfx shader sources
│   ├── vs/                # Vertex shaders
│   ├── fs/                # Fragment shaders
│   └── varying/           # Varying definitions
└── examples/              # Example applications
```

## Implementation Status

### Phase 1: Core Framework
- [ ] State manager
- [ ] Shader key system
- [ ] Basic bgfx integration

### Phase 2: Shader Generator
- [ ] Vertex transform
- [ ] Lighting (8 lights)
- [ ] Texture coordinate generation

### Phase 3: Texture Stages
- [ ] All 26 texture operations
- [ ] Bump mapping
- [ ] Multi-texture blending

### Phase 4: Additional Features
- [ ] Fog (all modes)
- [ ] Alpha testing
- [ ] Point sprites
- [ ] Vertex blending

### Phase 5: Optimization
- [ ] Shader caching
- [ ] Async compilation
- [ ] Ubershader fallback

## References

- [DXVK](https://github.com/doitsujin/dxvk) - Reference implementation
- [bgfx](https://github.com/bkaradzic/bgfx) - Rendering library
- [Direct3D 8 Documentation](https://docs.microsoft.com/en-us/windows/win32/direct3d9/dx9-graphics)

## License

MIT License - See LICENSE file for details
