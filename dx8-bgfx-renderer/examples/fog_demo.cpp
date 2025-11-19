// =============================================================================
// DX8-BGFX Fog Demo Example
// =============================================================================
// Demonstrates different fog modes: Linear, Exponential, Exponential Squared
//

#include <dx8bgfx/dx8bgfx.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <cmath>

using namespace dx8bgfx;

// Application state
static Renderer* g_renderer = nullptr;
static float g_time = 0.0f;
static int g_fogMode = 0; // 0=Linear, 1=Exp, 2=Exp2

// Geometry
static std::vector<GeometryGenerator::Vertex> g_cubeVertices;
static std::vector<uint16_t> g_cubeIndices;
static bgfx::VertexBufferHandle g_cubeVB = BGFX_INVALID_HANDLE;
static bgfx::IndexBufferHandle g_cubeIB = BGFX_INVALID_HANDLE;

// Initialize geometry
void InitGeometry() {
    GeometryGenerator::GenerateCube(g_cubeVertices, g_cubeIndices);

    bgfx::VertexLayout layout = GeometryGenerator::GetGeneratedVertexLayout();

    g_cubeVB = VertexBufferUtils::CreateVertexBuffer(
        g_cubeVertices.data(),
        static_cast<uint32_t>(g_cubeVertices.size()),
        layout
    );

    g_cubeIB = IndexBufferUtils::CreateIndexBuffer16(
        g_cubeIndices.data(),
        static_cast<uint32_t>(g_cubeIndices.size())
    );
}

// Draw a cube at position
void DrawCube(Renderer& renderer, float x, float y, float z, float scale = 1.0f) {
    D3DMATRIX world;
    float s = scale;

    // Simple translation matrix
    world._11 = s;  world._12 = 0;  world._13 = 0;  world._14 = 0;
    world._21 = 0;  world._22 = s;  world._23 = 0;  world._24 = 0;
    world._31 = 0;  world._32 = 0;  world._33 = s;  world._34 = 0;
    world._41 = x;  world._42 = y;  world._43 = z;  world._44 = 1;

    renderer.SetTransform(D3DTS_WORLD, &world);

    DWORD fvf = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1 | D3DFVF_DIFFUSE;
    renderer.SetFVF(fvf);

    bgfx::setVertexBuffer(0, g_cubeVB);
    bgfx::setIndexBuffer(g_cubeIB);
    renderer.DrawIndexedPrimitive(
        D3DPT_TRIANGLELIST,
        0, 0,
        static_cast<UINT>(g_cubeVertices.size()),
        0,
        static_cast<UINT>(g_cubeIndices.size() / 3)
    );
}

// Render frame
void RenderFrame(uint32_t width, uint32_t height) {
    Renderer& renderer = *g_renderer;

    // Update time
    g_time += 0.016f;

    // Cycle fog modes every 5 seconds
    int newFogMode = static_cast<int>(g_time / 5.0f) % 3;
    if (newFogMode != g_fogMode) {
        g_fogMode = newFogMode;
    }

    // Clear with fog color
    D3DCOLOR fogColor = 0xFF808080; // Gray fog
    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
                       (fogColor & 0x00FFFFFF) | 0xFF000000, 1.0f, 0);

    // Setup camera - looking down a corridor of cubes
    D3DMATRIX view, proj;

    // Camera position moves slightly
    float eyeX = std::sin(g_time * 0.2f) * 0.5f;
    float eyeY = 1.5f;
    float eyeZ = -5.0f + std::sin(g_time * 0.1f) * 2.0f;

    // Look forward
    float fx = 0, fy = 0, fz = 1;
    float ux = 0, uy = 1, uz = 0;
    float rx = 1, ry = 0, rz = 0;

    view._11 = rx; view._12 = ux; view._13 = fx; view._14 = 0;
    view._21 = ry; view._22 = uy; view._23 = fy; view._24 = 0;
    view._31 = rz; view._32 = uz; view._33 = fz; view._34 = 0;
    view._41 = -(rx*eyeX + ry*eyeY + rz*eyeZ);
    view._42 = -(ux*eyeX + uy*eyeY + uz*eyeZ);
    view._43 = -(fx*eyeX + fy*eyeY + fz*eyeZ);
    view._44 = 1;

    // Perspective projection
    float fov = 60.0f * 3.14159f / 180.0f;
    float aspect = float(width) / float(height);
    float nearZ = 0.1f, farZ = 100.0f;
    float yScale = 1.0f / std::tan(fov * 0.5f);
    float xScale = yScale / aspect;

    proj._11 = xScale; proj._12 = 0;      proj._13 = 0;                       proj._14 = 0;
    proj._21 = 0;      proj._22 = yScale; proj._23 = 0;                       proj._24 = 0;
    proj._31 = 0;      proj._32 = 0;      proj._33 = farZ/(farZ-nearZ);       proj._34 = 1;
    proj._41 = 0;      proj._42 = 0;      proj._43 = -nearZ*farZ/(farZ-nearZ); proj._44 = 0;

    renderer.SetTransform(D3DTS_VIEW, &view);
    renderer.SetTransform(D3DTS_PROJECTION, &proj);

    // Setup lighting
    D3DLIGHT8 light = CreateDirectionalLight(0.0f, -0.5f, 1.0f);
    renderer.SetLight(0, &light);
    renderer.LightEnable(0, TRUE);
    renderer.SetRenderState(D3DRS_LIGHTING, TRUE);
    renderer.SetRenderState(D3DRS_AMBIENT, 0x00404040);

    // Enable fog
    renderer.SetRenderState(D3DRS_FOGENABLE, TRUE);
    renderer.SetRenderState(D3DRS_FOGCOLOR, fogColor);

    // Set fog mode based on current selection
    switch (g_fogMode) {
        case 0: // Linear fog
            renderer.SetRenderState(D3DRS_FOGTABLEMODE, D3DFOG_LINEAR);
            {
                float fogStart = 5.0f;
                float fogEnd = 30.0f;
                renderer.SetRenderState(D3DRS_FOGSTART, *reinterpret_cast<DWORD*>(&fogStart));
                renderer.SetRenderState(D3DRS_FOGEND, *reinterpret_cast<DWORD*>(&fogEnd));
            }
            break;

        case 1: // Exponential fog
            renderer.SetRenderState(D3DRS_FOGTABLEMODE, D3DFOG_EXP);
            {
                float fogDensity = 0.05f;
                renderer.SetRenderState(D3DRS_FOGDENSITY, *reinterpret_cast<DWORD*>(&fogDensity));
            }
            break;

        case 2: // Exponential squared fog
            renderer.SetRenderState(D3DRS_FOGTABLEMODE, D3DFOG_EXP2);
            {
                float fogDensity = 0.03f;
                renderer.SetRenderState(D3DRS_FOGDENSITY, *reinterpret_cast<DWORD*>(&fogDensity));
            }
            break;
    }

    // Material
    D3DMATERIAL8 material = CreateDefaultMaterial();

    // Draw a corridor of cubes
    const int numRows = 8;
    const int cubesPerRow = 5;
    const float spacing = 6.0f;
    const float rowSpacing = 4.0f;

    // Floor cubes
    material.Diffuse = {0.3f, 0.3f, 0.4f, 1.0f};
    renderer.SetMaterial(&material);

    for (int row = 0; row < numRows; row++) {
        float z = row * rowSpacing;
        for (int i = 0; i < cubesPerRow; i++) {
            float x = (i - cubesPerRow / 2) * spacing;
            DrawCube(renderer, x, -1.0f, z, 0.8f);
        }
    }

    // Wall cubes (left and right)
    material.Diffuse = {0.6f, 0.4f, 0.3f, 1.0f};
    renderer.SetMaterial(&material);

    for (int row = 0; row < numRows; row++) {
        float z = row * rowSpacing;
        // Left wall
        DrawCube(renderer, -((cubesPerRow / 2 + 1) * spacing), 1.0f, z, 0.8f);
        // Right wall
        DrawCube(renderer, (cubesPerRow / 2 + 1) * spacing, 1.0f, z, 0.8f);
    }

    // Floating cubes in the corridor
    material.Diffuse = {0.8f, 0.2f, 0.2f, 1.0f};
    renderer.SetMaterial(&material);

    for (int row = 0; row < numRows; row++) {
        float z = row * rowSpacing + 2.0f;
        float y = 1.0f + std::sin(g_time + row * 0.5f) * 0.3f;
        float rotation = g_time + row * 0.3f;

        // Rotating cube
        D3DMATRIX world;
        float c = std::cos(rotation);
        float s = std::sin(rotation);
        float scale = 0.5f;

        world._11 = c * scale;  world._12 = 0;     world._13 = s * scale; world._14 = 0;
        world._21 = 0;          world._22 = scale; world._23 = 0;         world._24 = 0;
        world._31 = -s * scale; world._32 = 0;     world._33 = c * scale; world._34 = 0;
        world._41 = 0;          world._42 = y;     world._43 = z;         world._44 = 1;

        renderer.SetTransform(D3DTS_WORLD, &world);

        DWORD fvf = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1 | D3DFVF_DIFFUSE;
        renderer.SetFVF(fvf);

        bgfx::setVertexBuffer(0, g_cubeVB);
        bgfx::setIndexBuffer(g_cubeIB);
        renderer.DrawIndexedPrimitive(
            D3DPT_TRIANGLELIST,
            0, 0,
            static_cast<UINT>(g_cubeVertices.size()),
            0,
            static_cast<UINT>(g_cubeIndices.size() / 3)
        );
    }

    // Display fog mode text (using debug text)
    bgfx::dbgTextClear();
    bgfx::dbgTextPrintf(0, 1, 0x0f, "Fog Demo - Press SPACE to change mode");

    const char* modeNames[] = {"LINEAR", "EXP", "EXP2"};
    bgfx::dbgTextPrintf(0, 2, 0x0f, "Current Mode: %s", modeNames[g_fogMode]);

    // End frame
    renderer.EndFrame();
    bgfx::frame();
}

// Cleanup
void Cleanup() {
    if (bgfx::isValid(g_cubeVB)) bgfx::destroy(g_cubeVB);
    if (bgfx::isValid(g_cubeIB)) bgfx::destroy(g_cubeIB);

    delete g_renderer;
    g_renderer = nullptr;
}

// Main entry point
int main(int argc, char** argv) {
    uint32_t width = 1280;
    uint32_t height = 720;

    // Initialize bgfx
    bgfx::Init init;
    init.type = bgfx::RendererType::Count;
    init.resolution.width = width;
    init.resolution.height = height;
    init.resolution.reset = BGFX_RESET_VSYNC;

    if (!bgfx::init(init)) {
        return 1;
    }

    bgfx::setDebug(BGFX_DEBUG_TEXT);
    bgfx::setViewRect(0, 0, 0, uint16_t(width), uint16_t(height));

    // Create renderer
    g_renderer = new Renderer();
    g_renderer->Initialize(width, height);

    // Initialize geometry
    InitGeometry();

    // Main loop
    bool running = true;
    while (running) {
        RenderFrame(width, height);

        if (g_time > 30.0f) {
            running = false;
        }
    }

    Cleanup();
    bgfx::shutdown();

    return 0;
}
