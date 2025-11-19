// =============================================================================
// DX8-BGFX Lighting Demo
// =============================================================================
// Demonstrates multiple light types:
// - Directional light
// - Point lights (orbiting)
// - Spot light
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
static std::vector<GeometryGenerator::Vertex> g_sphereVertices;
static std::vector<uint16_t> g_sphereIndices;
static std::vector<GeometryGenerator::Vertex> g_planeVertices;
static std::vector<uint16_t> g_planeIndices;
static std::vector<GeometryGenerator::Vertex> g_torusVertices;
static std::vector<uint16_t> g_torusIndices;

static bgfx::VertexBufferHandle g_sphereVB = BGFX_INVALID_HANDLE;
static bgfx::IndexBufferHandle g_sphereIB = BGFX_INVALID_HANDLE;
static bgfx::VertexBufferHandle g_planeVB = BGFX_INVALID_HANDLE;
static bgfx::IndexBufferHandle g_planeIB = BGFX_INVALID_HANDLE;
static bgfx::VertexBufferHandle g_torusVB = BGFX_INVALID_HANDLE;
static bgfx::IndexBufferHandle g_torusIB = BGFX_INVALID_HANDLE;

// Initialize geometry
void InitGeometry() {
    bgfx::VertexLayout layout = GeometryGenerator::GetGeneratedVertexLayout();

    // Sphere
    GeometryGenerator::GenerateSphere(g_sphereVertices, g_sphereIndices, 24, 24);
    g_sphereVB = VertexBufferUtils::CreateVertexBuffer(
        g_sphereVertices.data(),
        static_cast<uint32_t>(g_sphereVertices.size()),
        layout
    );
    g_sphereIB = IndexBufferUtils::CreateIndexBuffer16(
        g_sphereIndices.data(),
        static_cast<uint32_t>(g_sphereIndices.size())
    );

    // Plane
    GeometryGenerator::GeneratePlane(g_planeVertices, g_planeIndices, 4);
    g_planeVB = VertexBufferUtils::CreateVertexBuffer(
        g_planeVertices.data(),
        static_cast<uint32_t>(g_planeVertices.size()),
        layout
    );
    g_planeIB = IndexBufferUtils::CreateIndexBuffer16(
        g_planeIndices.data(),
        static_cast<uint32_t>(g_planeIndices.size())
    );

    // Torus
    GeometryGenerator::GenerateTorus(g_torusVertices, g_torusIndices, 0.3f, 1.0f, 24, 16);
    g_torusVB = VertexBufferUtils::CreateVertexBuffer(
        g_torusVertices.data(),
        static_cast<uint32_t>(g_torusVertices.size()),
        layout
    );
    g_torusIB = IndexBufferUtils::CreateIndexBuffer16(
        g_torusIndices.data(),
        static_cast<uint32_t>(g_torusIndices.size())
    );
}

// Draw sphere
void DrawSphere(Renderer& renderer, float x, float y, float z, float scale = 1.0f) {
    D3DMATRIX world;
    world._11 = scale; world._12 = 0;     world._13 = 0;     world._14 = 0;
    world._21 = 0;     world._22 = scale; world._23 = 0;     world._24 = 0;
    world._31 = 0;     world._32 = 0;     world._33 = scale; world._34 = 0;
    world._41 = x;     world._42 = y;     world._43 = z;     world._44 = 1;

    renderer.SetTransform(D3DTS_WORLD, &world);

    DWORD fvf = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1 | D3DFVF_DIFFUSE;
    renderer.SetFVF(fvf);

    bgfx::setVertexBuffer(0, g_sphereVB);
    bgfx::setIndexBuffer(g_sphereIB);
    renderer.DrawIndexedPrimitive(
        D3DPT_TRIANGLELIST, 0, 0,
        static_cast<UINT>(g_sphereVertices.size()),
        0, static_cast<UINT>(g_sphereIndices.size() / 3)
    );
}

// Draw torus
void DrawTorus(Renderer& renderer, float x, float y, float z, float rotY, float scale = 1.0f) {
    D3DMATRIX world;
    float c = std::cos(rotY);
    float s = std::sin(rotY);

    world._11 = c * scale;  world._12 = 0;     world._13 = s * scale; world._14 = 0;
    world._21 = 0;          world._22 = scale; world._23 = 0;         world._24 = 0;
    world._31 = -s * scale; world._32 = 0;     world._33 = c * scale; world._34 = 0;
    world._41 = x;          world._42 = y;     world._43 = z;         world._44 = 1;

    renderer.SetTransform(D3DTS_WORLD, &world);

    DWORD fvf = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1 | D3DFVF_DIFFUSE;
    renderer.SetFVF(fvf);

    bgfx::setVertexBuffer(0, g_torusVB);
    bgfx::setIndexBuffer(g_torusIB);
    renderer.DrawIndexedPrimitive(
        D3DPT_TRIANGLELIST, 0, 0,
        static_cast<UINT>(g_torusVertices.size()),
        0, static_cast<UINT>(g_torusIndices.size() / 3)
    );
}

// Draw floor
void DrawFloor(Renderer& renderer) {
    D3DMATRIX world;
    float scale = 8.0f;

    world._11 = scale; world._12 = 0;     world._13 = 0;     world._14 = 0;
    world._21 = 0;     world._22 = scale; world._23 = 0;     world._24 = 0;
    world._31 = 0;     world._32 = 0;     world._33 = scale; world._34 = 0;
    world._41 = 0;     world._42 = -1;    world._43 = 0;     world._44 = 1;

    renderer.SetTransform(D3DTS_WORLD, &world);

    DWORD fvf = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1 | D3DFVF_DIFFUSE;
    renderer.SetFVF(fvf);

    bgfx::setVertexBuffer(0, g_planeVB);
    bgfx::setIndexBuffer(g_planeIB);
    renderer.DrawIndexedPrimitive(
        D3DPT_TRIANGLELIST, 0, 0,
        static_cast<UINT>(g_planeVertices.size()),
        0, static_cast<UINT>(g_planeIndices.size() / 3)
    );
}

// Render frame
void RenderFrame(uint32_t width, uint32_t height) {
    Renderer& renderer = *g_renderer;

    g_time += 0.016f;

    // Clear
    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x101020FF, 1.0f, 0);

    // Camera
    D3DMATRIX view, proj;

    float eyeX = std::sin(g_time * 0.2f) * 10.0f;
    float eyeY = 6.0f;
    float eyeZ = std::cos(g_time * 0.2f) * 10.0f;

    // Look-at
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

    // Projection
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

    // ==========================================================================
    // Setup Lights
    // ==========================================================================

    // Enable lighting
    renderer.SetRenderState(D3DRS_LIGHTING, TRUE);
    renderer.SetRenderState(D3DRS_AMBIENT, 0x00202020);
    renderer.SetRenderState(D3DRS_SPECULARENABLE, TRUE);

    // Light 0: Directional (sun)
    D3DLIGHT8 dirLight = CreateDirectionalLight(0.3f, -1.0f, 0.5f, 0.6f, 0.6f, 0.5f);
    renderer.SetLight(0, &dirLight);
    renderer.LightEnable(0, TRUE);

    // Light 1: Red point light (orbiting)
    float redAngle = g_time * 1.5f;
    D3DLIGHT8 redPoint = CreatePointLight(
        std::cos(redAngle) * 4.0f, 2.0f, std::sin(redAngle) * 4.0f,
        1.0f, 0.2f, 0.2f, 15.0f, 0.0f, 0.1f, 0.02f
    );
    renderer.SetLight(1, &redPoint);
    renderer.LightEnable(1, TRUE);

    // Light 2: Blue point light (orbiting opposite direction)
    float blueAngle = -g_time * 1.2f + 3.14159f;
    D3DLIGHT8 bluePoint = CreatePointLight(
        std::cos(blueAngle) * 4.0f, 2.5f, std::sin(blueAngle) * 4.0f,
        0.2f, 0.3f, 1.0f, 15.0f, 0.0f, 0.1f, 0.02f
    );
    renderer.SetLight(2, &bluePoint);
    renderer.LightEnable(2, TRUE);

    // Light 3: Green point light (bouncing)
    float greenY = 3.0f + std::sin(g_time * 2.0f) * 2.0f;
    D3DLIGHT8 greenPoint = CreatePointLight(
        0.0f, greenY, 0.0f,
        0.3f, 1.0f, 0.3f, 10.0f, 0.0f, 0.15f, 0.03f
    );
    renderer.SetLight(3, &greenPoint);
    renderer.LightEnable(3, TRUE);

    // Light 4: Spot light (rotating)
    float spotAngle = g_time * 0.5f;
    D3DLIGHT8 spotLight;
    spotLight.Type = D3DLIGHT_SPOT;
    spotLight.Diffuse = {1.0f, 1.0f, 0.8f, 1.0f};
    spotLight.Specular = {1.0f, 1.0f, 0.8f, 1.0f};
    spotLight.Ambient = {0.0f, 0.0f, 0.0f, 1.0f};
    spotLight.Position = {std::cos(spotAngle) * 6.0f, 5.0f, std::sin(spotAngle) * 6.0f};
    spotLight.Direction = {-std::cos(spotAngle), -0.7f, -std::sin(spotAngle)};
    spotLight.Range = 20.0f;
    spotLight.Theta = 0.2f;  // Inner cone
    spotLight.Phi = 0.4f;    // Outer cone
    spotLight.Falloff = 1.0f;
    spotLight.Attenuation0 = 1.0f;
    spotLight.Attenuation1 = 0.0f;
    spotLight.Attenuation2 = 0.0f;
    renderer.SetLight(4, &spotLight);
    renderer.LightEnable(4, TRUE);

    // ==========================================================================
    // Draw Objects
    // ==========================================================================

    ApplySolidRenderStates(renderer);

    // Floor
    D3DMATERIAL8 material = CreateDefaultMaterial();
    material.Diffuse = {0.5f, 0.5f, 0.5f, 1.0f};
    material.Specular = {0.3f, 0.3f, 0.3f, 1.0f};
    material.Power = 20.0f;
    renderer.SetMaterial(&material);
    DrawFloor(renderer);

    // Central sphere (white, shiny)
    material.Diffuse = {0.9f, 0.9f, 0.9f, 1.0f};
    material.Specular = {1.0f, 1.0f, 1.0f, 1.0f};
    material.Power = 50.0f;
    renderer.SetMaterial(&material);
    DrawSphere(renderer, 0.0f, 0.5f, 0.0f, 1.0f);

    // Surrounding spheres
    for (int i = 0; i < 6; i++) {
        float angle = i * 3.14159f * 2.0f / 6.0f;
        float x = std::cos(angle) * 3.5f;
        float z = std::sin(angle) * 3.5f;

        // Vary colors
        float r = 0.5f + 0.5f * std::sin(angle);
        float g = 0.5f + 0.5f * std::sin(angle + 2.0f);
        float b = 0.5f + 0.5f * std::sin(angle + 4.0f);

        material.Diffuse = {r, g, b, 1.0f};
        material.Specular = {0.5f, 0.5f, 0.5f, 1.0f};
        material.Power = 30.0f;
        renderer.SetMaterial(&material);
        DrawSphere(renderer, x, 0.3f, z, 0.6f);
    }

    // Rotating torus
    material.Diffuse = {0.8f, 0.6f, 0.2f, 1.0f};
    material.Specular = {1.0f, 0.8f, 0.4f, 1.0f};
    material.Power = 40.0f;
    renderer.SetMaterial(&material);
    DrawTorus(renderer, 0.0f, 2.5f, 0.0f, g_time, 1.2f);

    // Small indicator spheres at light positions
    material.Diffuse = {1.0f, 0.3f, 0.3f, 1.0f};
    material.Emissive = {0.8f, 0.2f, 0.2f, 1.0f};
    material.Power = 0.0f;
    renderer.SetMaterial(&material);
    DrawSphere(renderer, redPoint.Position.x, redPoint.Position.y, redPoint.Position.z, 0.15f);

    material.Diffuse = {0.3f, 0.4f, 1.0f, 1.0f};
    material.Emissive = {0.2f, 0.3f, 0.8f, 1.0f};
    renderer.SetMaterial(&material);
    DrawSphere(renderer, bluePoint.Position.x, bluePoint.Position.y, bluePoint.Position.z, 0.15f);

    material.Diffuse = {0.4f, 1.0f, 0.4f, 1.0f};
    material.Emissive = {0.3f, 0.8f, 0.3f, 1.0f};
    renderer.SetMaterial(&material);
    DrawSphere(renderer, greenPoint.Position.x, greenPoint.Position.y, greenPoint.Position.z, 0.15f);

    // Display info
    bgfx::dbgTextClear();
    bgfx::dbgTextPrintf(0, 1, 0x0f, "Lighting Demo - Multiple Light Types");
    bgfx::dbgTextPrintf(0, 2, 0x0f, "1 Directional + 3 Point + 1 Spot = 5 Lights");

    renderer.EndFrame();
    bgfx::frame();
}

// Cleanup
void Cleanup() {
    if (bgfx::isValid(g_sphereVB)) bgfx::destroy(g_sphereVB);
    if (bgfx::isValid(g_sphereIB)) bgfx::destroy(g_sphereIB);
    if (bgfx::isValid(g_planeVB)) bgfx::destroy(g_planeVB);
    if (bgfx::isValid(g_planeIB)) bgfx::destroy(g_planeIB);
    if (bgfx::isValid(g_torusVB)) bgfx::destroy(g_torusVB);
    if (bgfx::isValid(g_torusIB)) bgfx::destroy(g_torusIB);

    delete g_renderer;
    g_renderer = nullptr;
}

int main(int argc, char** argv) {
    uint32_t width = 1280;
    uint32_t height = 720;

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

    g_renderer = new Renderer();
    g_renderer->Initialize(width, height);

    InitGeometry();

    bool running = true;
    while (running) {
        RenderFrame(width, height);
        if (g_time > 60.0f) running = false;
    }

    Cleanup();
    bgfx::shutdown();

    return 0;
}
