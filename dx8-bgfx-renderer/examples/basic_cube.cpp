/*
 * DX8-bgfx-renderer Example: Basic Lit Cube
 *
 * This example demonstrates:
 * - Creating vertex and index buffers
 * - Setting up transforms (world, view, projection)
 * - Basic lighting with one directional light
 * - Material properties
 * - Texture mapping with modulate operation
 */

#include <dx8bgfx/dx8_renderer.h>
#include <dx8bgfx/dx8_math.h>

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

#include <cstdio>
#include <cmath>

using namespace dx8bgfx;

// =============================================================================
// Cube Vertex Data
// =============================================================================

struct Vertex {
    float x, y, z;       // Position
    float nx, ny, nz;    // Normal
    uint32_t color;      // ARGB color
    float u, v;          // Texture coordinates
};

// Cube vertices with normals
static Vertex s_cubeVertices[] = {
    // Front face
    {-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0xFFFFFFFF, 0.0f, 1.0f},
    { 1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0xFFFFFFFF, 1.0f, 1.0f},
    { 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0xFFFFFFFF, 1.0f, 0.0f},
    {-1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0xFFFFFFFF, 0.0f, 0.0f},

    // Back face
    { 1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0xFFFFFFFF, 0.0f, 1.0f},
    {-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0xFFFFFFFF, 1.0f, 1.0f},
    {-1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0xFFFFFFFF, 1.0f, 0.0f},
    { 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0xFFFFFFFF, 0.0f, 0.0f},

    // Top face
    {-1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0xFFFFFFFF, 0.0f, 1.0f},
    { 1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0xFFFFFFFF, 1.0f, 1.0f},
    { 1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0xFFFFFFFF, 1.0f, 0.0f},
    {-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0xFFFFFFFF, 0.0f, 0.0f},

    // Bottom face
    {-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0xFFFFFFFF, 0.0f, 1.0f},
    { 1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0xFFFFFFFF, 1.0f, 1.0f},
    { 1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0xFFFFFFFF, 1.0f, 0.0f},
    {-1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0xFFFFFFFF, 0.0f, 0.0f},

    // Right face
    { 1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0xFFFFFFFF, 0.0f, 1.0f},
    { 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0xFFFFFFFF, 1.0f, 1.0f},
    { 1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0xFFFFFFFF, 1.0f, 0.0f},
    { 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0xFFFFFFFF, 0.0f, 0.0f},

    // Left face
    {-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0xFFFFFFFF, 0.0f, 1.0f},
    {-1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0xFFFFFFFF, 1.0f, 1.0f},
    {-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0xFFFFFFFF, 1.0f, 0.0f},
    {-1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0xFFFFFFFF, 0.0f, 0.0f},
};

// Cube indices
static uint16_t s_cubeIndices[] = {
    0, 1, 2, 0, 2, 3,       // Front
    4, 5, 6, 4, 6, 7,       // Back
    8, 9, 10, 8, 10, 11,    // Top
    12, 13, 14, 12, 14, 15, // Bottom
    16, 17, 18, 16, 18, 19, // Right
    20, 21, 22, 20, 22, 23  // Left
};

// =============================================================================
// Main Application
// =============================================================================

class CubeExample {
public:
    bool Init(uint32_t width, uint32_t height) {
        m_width = width;
        m_height = height;

        // Initialize renderer
        if (m_renderer.Init(width, height) != D3D_OK) {
            printf("Failed to initialize renderer\n");
            return false;
        }

        // Create vertex buffer
        // FVF: Position, Normal, Diffuse Color, 1 Texture Coordinate
        DWORD fvf = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX1;
        m_vertexBuffer = m_renderer.CreateVertexBuffer(
            s_cubeVertices,
            sizeof(s_cubeVertices),
            fvf
        );

        // Create index buffer
        m_indexBuffer = m_renderer.CreateIndexBuffer(
            s_cubeIndices,
            sizeof(s_cubeIndices) / sizeof(s_cubeIndices[0]),
            false  // 16-bit indices
        );

        // Set up view matrix
        D3DVECTOR eye = {0.0f, 3.0f, -5.0f};
        D3DVECTOR at = {0.0f, 0.0f, 0.0f};
        D3DVECTOR up = {0.0f, 1.0f, 0.0f};
        m_viewMatrix = MatrixLookAtLH(eye, at, up);

        // Set up projection matrix
        float fov = 60.0f * DEG_TO_RAD;
        float aspect = static_cast<float>(width) / static_cast<float>(height);
        m_projMatrix = MatrixPerspectiveFovLH(fov, aspect, 0.1f, 100.0f);

        // Set up material
        m_material = {};
        m_material.Diffuse = {1.0f, 1.0f, 1.0f, 1.0f};
        m_material.Ambient = {0.2f, 0.2f, 0.2f, 1.0f};
        m_material.Specular = {1.0f, 1.0f, 1.0f, 1.0f};
        m_material.Power = 32.0f;

        // Set up light
        m_light = {};
        m_light.Type = D3DLIGHT_DIRECTIONAL;
        m_light.Diffuse = {1.0f, 1.0f, 0.8f, 1.0f};
        m_light.Specular = {1.0f, 1.0f, 1.0f, 1.0f};
        m_light.Ambient = {0.1f, 0.1f, 0.1f, 1.0f};
        m_light.Direction = {0.5f, -1.0f, 0.5f};

        return true;
    }

    void Shutdown() {
        m_renderer.DestroyVertexBuffer(m_vertexBuffer);
        m_renderer.DestroyIndexBuffer(m_indexBuffer);
        m_renderer.Shutdown();
    }

    void Update(float deltaTime) {
        m_rotation += deltaTime * 0.5f;
    }

    void Render() {
        m_renderer.BeginFrame();

        // Clear
        m_renderer.Clear(0, nullptr, 3, 0xFF404040, 1.0f, 0);

        // Set transforms
        D3DMATRIX world = MatrixMultiply(
            MatrixRotationY(m_rotation),
            MatrixRotationX(m_rotation * 0.7f)
        );
        m_renderer.SetTransform(D3DTS_WORLD, &world);
        m_renderer.SetTransform(D3DTS_VIEW, &m_viewMatrix);
        m_renderer.SetTransform(D3DTS_PROJECTION, &m_projMatrix);

        // Set material
        m_renderer.SetMaterial(&m_material);

        // Set light
        m_renderer.SetLight(0, &m_light);
        m_renderer.LightEnable(0, TRUE);

        // Enable lighting
        m_renderer.SetRenderState(D3DRS_LIGHTING, TRUE);
        m_renderer.SetRenderState(D3DRS_SPECULARENABLE, TRUE);
        m_renderer.SetRenderState(D3DRS_NORMALIZENORMALS, TRUE);

        // Set global ambient
        m_renderer.SetRenderState(D3DRS_AMBIENT, 0xFF202020);

        // Set up texture stage (no texture, just vertex colors)
        m_renderer.SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
        m_renderer.SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
        m_renderer.SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
        m_renderer.SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);

        // Disable second stage
        m_renderer.SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);

        // Set buffers
        m_renderer.SetStreamSource(0, &m_vertexBuffer, sizeof(Vertex));
        m_renderer.SetIndices(&m_indexBuffer);

        // Draw
        m_renderer.DrawIndexedPrimitive(
            D3DPT_TRIANGLELIST,
            0,                                              // Min vertex index
            sizeof(s_cubeVertices) / sizeof(s_cubeVertices[0]), // Num vertices
            0,                                              // Start index
            sizeof(s_cubeIndices) / sizeof(s_cubeIndices[0]) / 3  // Primitive count
        );

        m_renderer.EndFrame();
    }

private:
    Renderer m_renderer;
    VertexBufferHandle m_vertexBuffer;
    IndexBufferHandle m_indexBuffer;

    D3DMATRIX m_viewMatrix;
    D3DMATRIX m_projMatrix;

    D3DMATERIAL8 m_material;
    D3DLIGHT8 m_light;

    uint32_t m_width = 0;
    uint32_t m_height = 0;
    float m_rotation = 0.0f;
};

// =============================================================================
// Entry Point (Platform-specific code would go here)
// =============================================================================

/*
 * To use this example, you would need to:
 * 1. Initialize a window (SDL, GLFW, Win32, etc.)
 * 2. Initialize bgfx with bgfx::init()
 * 3. Create a CubeExample instance
 * 4. Call Init(), then Update() and Render() in your main loop
 * 5. Call Shutdown() when done
 *
 * Example with GLFW:
 *
 * int main() {
 *     // Init window
 *     glfwInit();
 *     GLFWwindow* window = glfwCreateWindow(1280, 720, "DX8 Cube", NULL, NULL);
 *
 *     // Init bgfx
 *     bgfx::Init init;
 *     init.platformData.nwh = glfwGetWin32Window(window);
 *     init.resolution.width = 1280;
 *     init.resolution.height = 720;
 *     bgfx::init(init);
 *
 *     // Init example
 *     CubeExample example;
 *     example.Init(1280, 720);
 *
 *     // Main loop
 *     while (!glfwWindowShouldClose(window)) {
 *         glfwPollEvents();
 *         example.Update(1.0f / 60.0f);
 *         example.Render();
 *     }
 *
 *     // Cleanup
 *     example.Shutdown();
 *     bgfx::shutdown();
 *     glfwTerminate();
 *
 *     return 0;
 * }
 */

#ifndef DX8BGFX_NO_MAIN

int main(int argc, char** argv) {
    printf("DX8-bgfx-renderer Basic Cube Example\n");
    printf("=====================================\n");
    printf("\n");
    printf("This is a header-only example demonstrating renderer usage.\n");
    printf("To run it, you need to:\n");
    printf("1. Link with bgfx, bx, bimg\n");
    printf("2. Initialize a window (GLFW, SDL, etc.)\n");
    printf("3. Initialize bgfx\n");
    printf("4. Uncomment the main loop code above\n");
    printf("\n");
    printf("See the source code for details.\n");

    return 0;
}

#endif // DX8BGFX_NO_MAIN
