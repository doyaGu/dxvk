#pragma once

#include "dx8_types.h"
#include "dx8_constants.h"

#include <bgfx/bgfx.h>
#include <vector>

namespace dx8bgfx {

// =============================================================================
// Point Sprite Types
// =============================================================================

// Point sprite vertex (for CPU expansion)
struct PointSpriteVertex {
    float x, y, z;      // Position
    uint32_t color;     // Diffuse color
    float size;         // Point size
    float rotation;     // Rotation angle (radians)
};

// Expanded quad vertex
struct PointQuadVertex {
    float x, y, z;
    float u, v;
    uint32_t color;
};

// Point sprite parameters
struct PointSpriteParams {
    float pointSize;
    float pointSizeMin;
    float pointSizeMax;
    float pointScaleA;   // Constant attenuation
    float pointScaleB;   // Linear attenuation
    float pointScaleC;   // Quadratic attenuation
    bool  pointScaleEnable;
    bool  pointSpriteEnable;
};

// =============================================================================
// Point Sprite Renderer
// =============================================================================

class PointSpriteRenderer {
public:
    PointSpriteRenderer();
    ~PointSpriteRenderer();

    // Initialize renderer
    void Initialize();
    void Shutdown();

    // Set parameters
    void SetParams(const PointSpriteParams& params);
    const PointSpriteParams& GetParams() const { return m_params; }

    // Set view parameters for point scaling
    void SetViewport(uint32_t width, uint32_t height);
    void SetViewMatrix(const D3DMATRIX& view);
    void SetProjectionMatrix(const D3DMATRIX& projection);

    // Set texture
    void SetTexture(bgfx::TextureHandle texture);

    // Render points as sprites
    // Points are expanded to quads on CPU
    void RenderPoints(
        const void* vertices,
        uint32_t numVertices,
        DWORD fvf,
        uint32_t stride
    );

    // Render pre-expanded point sprites
    void RenderPointSprites(
        const PointSpriteVertex* points,
        uint32_t numPoints
    );

    // Get vertex layout for PointQuadVertex
    static bgfx::VertexLayout GetQuadVertexLayout();

private:
    PointSpriteParams m_params;

    D3DMATRIX m_view;
    D3DMATRIX m_projection;
    uint32_t m_viewportWidth;
    uint32_t m_viewportHeight;

    bgfx::TextureHandle m_texture;

    std::vector<PointQuadVertex> m_quadVertices;
    std::vector<uint16_t> m_quadIndices;

    bool m_initialized;

    // Calculate point size based on distance and parameters
    float CalculatePointSize(float viewZ) const;

    // Expand a single point to quad vertices
    void ExpandPointToQuad(
        float x, float y, float z,
        uint32_t color,
        float size,
        float rotation
    );
};

// =============================================================================
// Particle System
// =============================================================================

struct Particle {
    float x, y, z;          // Position
    float vx, vy, vz;       // Velocity
    uint32_t color;         // Current color
    float size;             // Current size
    float life;             // Remaining life (0-1)
    float maxLife;          // Maximum life
    float rotation;         // Rotation
    float angularVelocity;  // Rotation speed
};

struct ParticleEmitter {
    // Position
    float x, y, z;

    // Emission
    float emitRate;         // Particles per second
    uint32_t maxParticles;

    // Initial values
    float lifeMin, lifeMax;
    float sizeMin, sizeMax;
    float speedMin, speedMax;

    // Velocity direction (cone)
    float dirX, dirY, dirZ;
    float spread;           // Cone angle (radians)

    // Colors
    uint32_t colorStart;
    uint32_t colorEnd;

    // Physics
    float gravityX, gravityY, gravityZ;
    float drag;

    // Size over life
    float sizeEnd;

    // Rotation
    float rotationMin, rotationMax;
    float angularVelocityMin, angularVelocityMax;
};

class ParticleSystem {
public:
    ParticleSystem();
    ~ParticleSystem();

    // Initialize with emitter settings
    void Initialize(const ParticleEmitter& emitter);
    void Shutdown();

    // Update particles
    void Update(float deltaTime);

    // Reset all particles
    void Reset();

    // Set emitter position
    void SetPosition(float x, float y, float z);

    // Get particles for rendering
    const std::vector<Particle>& GetParticles() const { return m_particles; }

    // Get active particle count
    uint32_t GetActiveCount() const { return m_activeCount; }

    // Enable/disable emission
    void SetEmitting(bool emit) { m_emitting = emit; }
    bool IsEmitting() const { return m_emitting; }

    // Emit a burst of particles
    void Burst(uint32_t count);

private:
    ParticleEmitter m_emitter;
    std::vector<Particle> m_particles;
    uint32_t m_activeCount;
    float m_emitAccumulator;
    bool m_emitting;
    bool m_initialized;

    // Emit a single particle
    void EmitParticle();

    // Random helpers
    float RandomFloat(float min, float max);
    uint32_t LerpColor(uint32_t c1, uint32_t c2, float t);
};

// =============================================================================
// Particle Renderer
// =============================================================================

class ParticleRenderer {
public:
    ParticleRenderer();
    ~ParticleRenderer();

    // Initialize
    void Initialize();
    void Shutdown();

    // Set rendering parameters
    void SetTexture(bgfx::TextureHandle texture);
    void SetBlendMode(D3DBLEND srcBlend, D3DBLEND destBlend);
    void SetSoftParticles(bool enabled, float scale = 1.0f);

    // Render particles
    void Render(
        const ParticleSystem& system,
        const D3DMATRIX& view,
        const D3DMATRIX& projection
    );

    // Sort particles back-to-front
    void SetSortParticles(bool sort) { m_sortParticles = sort; }

private:
    PointSpriteRenderer m_spriteRenderer;
    bgfx::TextureHandle m_texture;

    D3DBLEND m_srcBlend;
    D3DBLEND m_destBlend;

    bool m_softParticles;
    float m_softScale;
    bool m_sortParticles;
    bool m_initialized;
};

// =============================================================================
// GPU Point Sprites (bgfx point rendering)
// =============================================================================

class GPUPointSprites {
public:
    GPUPointSprites();
    ~GPUPointSprites();

    // Initialize
    void Initialize();
    void Shutdown();

    // Set uniforms
    void SetPointSize(float size);
    void SetPointSizeRange(float min, float max);
    void SetPointAttenuation(float a, float b, float c);

    // Create point vertex buffer
    static bgfx::VertexBufferHandle CreatePointBuffer(
        const float* positions,  // x,y,z
        const uint32_t* colors,
        const float* sizes,      // Optional per-point sizes
        uint32_t numPoints
    );

    // Render points
    void Render(
        bgfx::VertexBufferHandle vb,
        uint32_t numPoints,
        const D3DMATRIX& view,
        const D3DMATRIX& projection
    );

    // Get vertex layout for GPU point sprites
    static bgfx::VertexLayout GetVertexLayout();

private:
    bgfx::UniformHandle m_pointParams;  // x=size, y=min, z=max, w=unused
    bgfx::UniformHandle m_attenuation;  // x=a, y=b, z=c
    bool m_initialized;
};

// =============================================================================
// Utility Functions
// =============================================================================

// Extract point size from FVF data
float ExtractPointSize(const void* vertex, DWORD fvf, uint32_t stride);

// Get point size offset in FVF
int GetPointSizeOffset(DWORD fvf);

// Calculate attenuated point size
float CalculateAttenuatedPointSize(
    float baseSize,
    float distance,
    float a, float b, float c,
    float viewportHeight
);

} // namespace dx8bgfx
