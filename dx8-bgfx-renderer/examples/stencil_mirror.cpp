// =============================================================================
// DX8-BGFX Stencil Mirror Example
// =============================================================================
// Demonstrates stencil buffer usage for planar reflections
//

#include <dx8bgfx/dx8bgfx.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <cmath>

using namespace dx8bgfx;

// Application state
static Renderer* g_renderer = nullptr;
static float g_time = 0.0f;

// Geometry
static std::vector<GeometryGenerator::Vertex> g_cubeVertices;
static std::vector<uint16_t> g_cubeIndices;
static std::vector<GeometryGenerator::Vertex> g_planeVertices;
static std::vector<uint16_t> g_planeIndices;

static bgfx::VertexBufferHandle g_cubeVB = BGFX_INVALID_HANDLE;
static bgfx::IndexBufferHandle g_cubeIB = BGFX_INVALID_HANDLE;
static bgfx::VertexBufferHandle g_planeVB = BGFX_INVALID_HANDLE;
static bgfx::IndexBufferHandle g_planeIB = BGFX_INVALID_HANDLE;

// Initialize geometry
void InitGeometry() {
    // Generate cube
    GeometryGenerator::GenerateCube(g_cubeVertices, g_cubeIndices);

    // Generate mirror plane
    GeometryGenerator::GeneratePlane(g_planeVertices, g_planeIndices, 1);

    // Create buffers
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

    g_planeVB = VertexBufferUtils::CreateVertexBuffer(
        g_planeVertices.data(),
        static_cast<uint32_t>(g_planeVertices.size()),
        layout
    );

    g_planeIB = IndexBufferUtils::CreateIndexBuffer16(
        g_planeIndices.data(),
        static_cast<uint32_t>(g_planeIndices.size())
    );
}

// Draw the cube at a given position
void DrawCube(Renderer& renderer, float x, float y, float z, float scale = 0.5f) {
    D3DMATRIX world;
    float s = scale;
    float c = std::cos(g_time);
    float sn = std::sin(g_time);

    // Rotation around Y axis + translation
    world._11 = c * s;  world._12 = 0;    world._13 = sn * s; world._14 = 0;
    world._21 = 0;      world._22 = s;    world._23 = 0;      world._24 = 0;
    world._31 = -sn * s; world._32 = 0;   world._33 = c * s;  world._34 = 0;
    world._41 = x;      world._42 = y;    world._43 = z;      world._44 = 1;

    renderer.SetTransform(D3DTS_WORLD, &world);

    // Set FVF for generated geometry
    DWORD fvf = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1 | D3DFVF_DIFFUSE;
    renderer.SetFVF(fvf);

    // Draw
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

// Draw the mirror plane
void DrawMirrorPlane(Renderer& renderer, float y = 0.0f, float scale = 3.0f) {
    D3DMATRIX world;
    float s = scale;

    // Scale and position at y
    world._11 = s;  world._12 = 0;  world._13 = 0;  world._14 = 0;
    world._21 = 0;  world._22 = s;  world._23 = 0;  world._24 = 0;
    world._31 = 0;  world._32 = 0;  world._33 = s;  world._34 = 0;
    world._41 = 0;  world._42 = y;  world._43 = 0;  world._44 = 1;

    renderer.SetTransform(D3DTS_WORLD, &world);

    DWORD fvf = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1 | D3DFVF_DIFFUSE;
    renderer.SetFVF(fvf);

    bgfx::setVertexBuffer(0, g_planeVB);
    bgfx::setIndexBuffer(g_planeIB);
    renderer.DrawIndexedPrimitive(
        D3DPT_TRIANGLELIST,
        0, 0,
        static_cast<UINT>(g_planeVertices.size()),
        0,
        static_cast<UINT>(g_planeIndices.size() / 3)
    );
}

// Render frame
void RenderFrame(uint32_t width, uint32_t height) {
    Renderer& renderer = *g_renderer;

    // Update time
    g_time += 0.016f;

    // Setup camera
    D3DMATRIX view, proj;

    // View matrix - looking at origin from above
    float eyeX = std::sin(g_time * 0.3f) * 5.0f;
    float eyeY = 4.0f;
    float eyeZ = std::cos(g_time * 0.3f) * 5.0f;

    // Simple look-at
    float fx = -eyeX, fy = -eyeY + 1.0f, fz = -eyeZ;
    float len = std::sqrt(fx*fx + fy*fy + fz*fz);
    fx /= len; fy /= len; fz /= len;

    float ux = 0, uy = 1, uz = 0;
    float rx = uy*fz - uz*fy, ry = uz*fx - ux*fz, rz = ux*fy - uy*fx;
    len = std::sqrt(rx*rx + ry*ry + rz*rz);
    rx /= len; ry /= len; rz /= len;

    ux = fy*rz - fz*ry; uy = fz*rx - fx*rz; uz = fx*ry - fy*rx;

    view._11 = rx; view._12 = ux; view._13 = fx; view._14 = 0;
    view._21 = ry; view._22 = uy; view._23 = fy; view._24 = 0;
    view._31 = rz; view._32 = uz; view._33 = fz; view._34 = 0;
    view._41 = -(rx*eyeX + ry*eyeY + rz*eyeZ);
    view._42 = -(ux*eyeX + uy*eyeY + uz*eyeZ);
    view._43 = -(fx*eyeX + fy*eyeY + fz*eyeZ);
    view._44 = 1;

    // Perspective projection
    float fov = 45.0f * 3.14159f / 180.0f;
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
    D3DLIGHT8 light = CreateDirectionalLight(0.5f, -1.0f, 0.5f);
    renderer.SetLight(0, &light);
    renderer.LightEnable(0, TRUE);
    renderer.SetRenderState(D3DRS_LIGHTING, TRUE);
    renderer.SetRenderState(D3DRS_AMBIENT, 0x00404040);

    // Material
    D3DMATERIAL8 material = CreateDefaultMaterial();
    material.Diffuse = {0.8f, 0.2f, 0.2f, 1.0f};
    renderer.SetMaterial(&material);

    // ==========================================================================
    // PASS 1: Draw mirror plane to stencil buffer only
    // ==========================================================================

    // Clear framebuffer and stencil
    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH | BGFX_CLEAR_STENCIL,
                       0x303030FF, 1.0f, 0);

    // Setup stencil to write 1 where mirror is drawn
    renderer.SetRenderState(D3DRS_STENCILENABLE, TRUE);
    renderer.SetRenderState(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
    renderer.SetRenderState(D3DRS_STENCILREF, 1);
    renderer.SetRenderState(D3DRS_STENCILMASK, 0xFF);
    renderer.SetRenderState(D3DRS_STENCILWRITEMASK, 0xFF);
    renderer.SetRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP);
    renderer.SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP);
    renderer.SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE);

    // Don't write to color or depth, only stencil
    renderer.SetRenderState(D3DRS_COLORWRITEENABLE, 0);
    renderer.SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

    // Draw mirror plane geometry
    material.Diffuse = {0.5f, 0.5f, 0.8f, 0.3f};
    renderer.SetMaterial(&material);
    DrawMirrorPlane(renderer, 0.0f, 3.0f);

    // ==========================================================================
    // PASS 2: Draw reflected cube where stencil == 1
    // ==========================================================================

    // Re-enable color and depth writes
    renderer.SetRenderState(D3DRS_COLORWRITEENABLE,
        D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN |
        D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);
    renderer.SetRenderState(D3DRS_ZWRITEENABLE, TRUE);

    // Only draw where stencil == 1
    renderer.SetRenderState(D3DRS_STENCILFUNC, D3DCMP_EQUAL);
    renderer.SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);

    // Flip culling for reflection
    renderer.SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);

    // Draw reflected cube (mirrored in Y)
    material.Diffuse = {0.8f, 0.2f, 0.2f, 1.0f};
    renderer.SetMaterial(&material);
    DrawCube(renderer, 0.0f, -1.5f, 0.0f, 0.5f);  // Reflected position

    // ==========================================================================
    // PASS 3: Draw mirror surface with blending
    // ==========================================================================

    // Disable stencil test
    renderer.SetRenderState(D3DRS_STENCILENABLE, FALSE);

    // Enable alpha blending for semi-transparent mirror
    ApplyTransparentRenderStates(renderer);

    // Draw mirror plane
    material.Diffuse = {0.5f, 0.5f, 0.8f, 0.3f};
    renderer.SetMaterial(&material);
    DrawMirrorPlane(renderer, 0.0f, 3.0f);

    // ==========================================================================
    // PASS 4: Draw actual cube
    // ==========================================================================

    // Reset to solid rendering
    ApplySolidRenderStates(renderer);

    // Draw cube above the mirror
    material.Diffuse = {0.8f, 0.2f, 0.2f, 1.0f};
    renderer.SetMaterial(&material);
    DrawCube(renderer, 0.0f, 1.5f, 0.0f, 0.5f);

    // Draw a second cube
    material.Diffuse = {0.2f, 0.8f, 0.2f, 1.0f};
    renderer.SetMaterial(&material);
    DrawCube(renderer, 2.0f, 1.0f, 0.0f, 0.3f);

    // End frame
    renderer.EndFrame();
    bgfx::frame();
}

// Cleanup
void Cleanup() {
    if (bgfx::isValid(g_cubeVB)) bgfx::destroy(g_cubeVB);
    if (bgfx::isValid(g_cubeIB)) bgfx::destroy(g_cubeIB);
    if (bgfx::isValid(g_planeVB)) bgfx::destroy(g_planeVB);
    if (bgfx::isValid(g_planeIB)) bgfx::destroy(g_planeIB);

    delete g_renderer;
    g_renderer = nullptr;
}

// Main entry point
int main(int argc, char** argv) {
    // Window dimensions
    uint32_t width = 1280;
    uint32_t height = 720;

    // Initialize platform/window here (platform-specific)
    // ...

    // Initialize bgfx
    bgfx::Init init;
    init.type = bgfx::RendererType::Count; // Auto-detect
    init.resolution.width = width;
    init.resolution.height = height;
    init.resolution.reset = BGFX_RESET_VSYNC;

    if (!bgfx::init(init)) {
        return 1;
    }

    // Enable debug text
    bgfx::setDebug(BGFX_DEBUG_TEXT);

    // Set view rect
    bgfx::setViewRect(0, 0, 0, uint16_t(width), uint16_t(height));

    // Create renderer
    g_renderer = new Renderer();
    g_renderer->Initialize(width, height);

    // Initialize geometry
    InitGeometry();

    // Main loop
    bool running = true;
    while (running) {
        // Poll events (platform-specific)
        // ...

        // Render
        RenderFrame(width, height);

        // For demo purposes, exit after some time
        if (g_time > 30.0f) {
            running = false;
        }
    }

    // Cleanup
    Cleanup();
    bgfx::shutdown();

    return 0;
}
