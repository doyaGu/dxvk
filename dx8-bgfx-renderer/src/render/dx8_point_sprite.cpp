// =============================================================================
// DX8-BGFX Point Sprite Implementation
// =============================================================================

#include "dx8bgfx/dx8_point_sprite.h"
#include <cmath>
#include <cstring>
#include <algorithm>
#include <random>

namespace dx8bgfx {

// =============================================================================
// PointSpriteRenderer Implementation
// =============================================================================

PointSpriteRenderer::PointSpriteRenderer()
    : m_viewportWidth(1280)
    , m_viewportHeight(720)
    , m_texture(BGFX_INVALID_HANDLE)
    , m_initialized(false) {

    // Default parameters
    m_params.pointSize = 1.0f;
    m_params.pointSizeMin = 1.0f;
    m_params.pointSizeMax = 64.0f;
    m_params.pointScaleA = 1.0f;
    m_params.pointScaleB = 0.0f;
    m_params.pointScaleC = 0.0f;
    m_params.pointScaleEnable = false;
    m_params.pointSpriteEnable = false;

    // Identity matrices
    std::memset(&m_view, 0, sizeof(m_view));
    std::memset(&m_projection, 0, sizeof(m_projection));
    m_view._11 = m_view._22 = m_view._33 = m_view._44 = 1.0f;
    m_projection._11 = m_projection._22 = m_projection._33 = m_projection._44 = 1.0f;
}

PointSpriteRenderer::~PointSpriteRenderer() {
    Shutdown();
}

void PointSpriteRenderer::Initialize() {
    if (m_initialized) {
        return;
    }

    m_quadVertices.reserve(4096);
    m_quadIndices.reserve(6144);

    m_initialized = true;
}

void PointSpriteRenderer::Shutdown() {
    if (!m_initialized) {
        return;
    }

    m_quadVertices.clear();
    m_quadIndices.clear();
    m_initialized = false;
}

void PointSpriteRenderer::SetParams(const PointSpriteParams& params) {
    m_params = params;
}

void PointSpriteRenderer::SetViewport(uint32_t width, uint32_t height) {
    m_viewportWidth = width;
    m_viewportHeight = height;
}

void PointSpriteRenderer::SetViewMatrix(const D3DMATRIX& view) {
    m_view = view;
}

void PointSpriteRenderer::SetProjectionMatrix(const D3DMATRIX& projection) {
    m_projection = projection;
}

void PointSpriteRenderer::SetTexture(bgfx::TextureHandle texture) {
    m_texture = texture;
}

bgfx::VertexLayout PointSpriteRenderer::GetQuadVertexLayout() {
    bgfx::VertexLayout layout;
    layout.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
        .end();
    return layout;
}

float PointSpriteRenderer::CalculatePointSize(float viewZ) const {
    float size = m_params.pointSize;

    if (m_params.pointScaleEnable && viewZ > 0.0f) {
        // D3D8 point size attenuation formula:
        // size = size * sqrt(1 / (A + B*d + C*d^2))
        // where d is the distance from the eye
        float d = viewZ;
        float attenuation = m_params.pointScaleA +
                           m_params.pointScaleB * d +
                           m_params.pointScaleC * d * d;

        if (attenuation > 0.0f) {
            size = size * std::sqrt(1.0f / attenuation);
        }

        // Scale by viewport height (D3D8 behavior)
        size *= m_viewportHeight;
    }

    // Clamp to min/max
    size = std::max(m_params.pointSizeMin, std::min(m_params.pointSizeMax, size));

    return size;
}

void PointSpriteRenderer::ExpandPointToQuad(
    float x, float y, float z,
    uint32_t color,
    float size,
    float rotation) {

    // Get camera right and up vectors from view matrix
    float rx = m_view._11;
    float ry = m_view._21;
    float rz = m_view._31;
    float ux = m_view._12;
    float uy = m_view._22;
    float uz = m_view._32;

    // Apply rotation if needed
    if (rotation != 0.0f) {
        float c = std::cos(rotation);
        float s = std::sin(rotation);

        float newRx = c * rx - s * ux;
        float newRy = c * ry - s * uy;
        float newRz = c * rz - s * uz;
        float newUx = s * rx + c * ux;
        float newUy = s * ry + c * uy;
        float newUz = s * rz + c * uz;

        rx = newRx; ry = newRy; rz = newRz;
        ux = newUx; uy = newUy; uz = newUz;
    }

    float halfSize = size * 0.5f;

    // Calculate quad corners
    // Bottom-left
    PointQuadVertex v0;
    v0.x = x - rx * halfSize - ux * halfSize;
    v0.y = y - ry * halfSize - uy * halfSize;
    v0.z = z - rz * halfSize - uz * halfSize;
    v0.u = 0.0f;
    v0.v = 1.0f;
    v0.color = color;

    // Bottom-right
    PointQuadVertex v1;
    v1.x = x + rx * halfSize - ux * halfSize;
    v1.y = y + ry * halfSize - uy * halfSize;
    v1.z = z + rz * halfSize - uz * halfSize;
    v1.u = 1.0f;
    v1.v = 1.0f;
    v1.color = color;

    // Top-right
    PointQuadVertex v2;
    v2.x = x + rx * halfSize + ux * halfSize;
    v2.y = y + ry * halfSize + uy * halfSize;
    v2.z = z + rz * halfSize + uz * halfSize;
    v2.u = 1.0f;
    v2.v = 0.0f;
    v2.color = color;

    // Top-left
    PointQuadVertex v3;
    v3.x = x - rx * halfSize + ux * halfSize;
    v3.y = y - ry * halfSize + uy * halfSize;
    v3.z = z - rz * halfSize + uz * halfSize;
    v3.u = 0.0f;
    v3.v = 0.0f;
    v3.color = color;

    // Add vertices
    uint16_t baseIdx = static_cast<uint16_t>(m_quadVertices.size());
    m_quadVertices.push_back(v0);
    m_quadVertices.push_back(v1);
    m_quadVertices.push_back(v2);
    m_quadVertices.push_back(v3);

    // Add indices (two triangles)
    m_quadIndices.push_back(baseIdx + 0);
    m_quadIndices.push_back(baseIdx + 1);
    m_quadIndices.push_back(baseIdx + 2);
    m_quadIndices.push_back(baseIdx + 0);
    m_quadIndices.push_back(baseIdx + 2);
    m_quadIndices.push_back(baseIdx + 3);
}

void PointSpriteRenderer::RenderPoints(
    const void* vertices,
    uint32_t numVertices,
    DWORD fvf,
    uint32_t stride) {

    if (!m_initialized || numVertices == 0) {
        return;
    }

    m_quadVertices.clear();
    m_quadIndices.clear();

    const uint8_t* ptr = static_cast<const uint8_t*>(vertices);

    // Calculate offsets based on FVF
    uint32_t posOffset = 0;
    uint32_t colorOffset = 0;
    uint32_t sizeOffset = -1;

    bool hasColor = (fvf & D3DFVF_DIFFUSE) != 0;
    bool hasSize = (fvf & D3DFVF_PSIZE) != 0;

    uint32_t offset = 0;

    // Position
    if (fvf & D3DFVF_XYZ) {
        posOffset = offset;
        offset += 12;
    } else if (fvf & D3DFVF_XYZRHW) {
        posOffset = offset;
        offset += 16;
    }

    // Normal
    if (fvf & D3DFVF_NORMAL) {
        offset += 12;
    }

    // Point size
    if (hasSize) {
        sizeOffset = offset;
        offset += 4;
    }

    // Diffuse color
    if (hasColor) {
        colorOffset = offset;
        offset += 4;
    }

    for (uint32_t i = 0; i < numVertices; ++i) {
        const uint8_t* v = ptr + i * stride;

        // Get position
        const float* pos = reinterpret_cast<const float*>(v + posOffset);
        float x = pos[0];
        float y = pos[1];
        float z = pos[2];

        // Get color
        uint32_t color = 0xFFFFFFFF;
        if (hasColor) {
            color = *reinterpret_cast<const uint32_t*>(v + colorOffset);
        }

        // Get size
        float size = m_params.pointSize;
        if (hasSize) {
            size = *reinterpret_cast<const float*>(v + sizeOffset);
        }

        // Transform to view space for size calculation
        float viewX = x * m_view._11 + y * m_view._21 + z * m_view._31 + m_view._41;
        float viewY = x * m_view._12 + y * m_view._22 + z * m_view._32 + m_view._42;
        float viewZ = x * m_view._13 + y * m_view._23 + z * m_view._33 + m_view._43;

        (void)viewX;
        (void)viewY;

        // Calculate attenuated size
        float finalSize = CalculatePointSize(viewZ);
        if (hasSize) {
            finalSize = size * (finalSize / m_params.pointSize);
        }

        // Expand to quad
        ExpandPointToQuad(x, y, z, color, finalSize, 0.0f);
    }

    // Render quads
    if (!m_quadVertices.empty()) {
        bgfx::TransientVertexBuffer tvb;
        bgfx::TransientIndexBuffer tib;

        bgfx::VertexLayout layout = GetQuadVertexLayout();

        uint32_t numVerts = static_cast<uint32_t>(m_quadVertices.size());
        uint32_t numInds = static_cast<uint32_t>(m_quadIndices.size());

        if (bgfx::allocTransientBuffers(&tvb, layout, numVerts, &tib, numInds)) {
            std::memcpy(tvb.data, m_quadVertices.data(), numVerts * sizeof(PointQuadVertex));
            std::memcpy(tib.data, m_quadIndices.data(), numInds * sizeof(uint16_t));

            bgfx::setVertexBuffer(0, &tvb);
            bgfx::setIndexBuffer(&tib);

            if (bgfx::isValid(m_texture)) {
                bgfx::setTexture(0, BGFX_INVALID_HANDLE, m_texture);
            }
        }
    }
}

void PointSpriteRenderer::RenderPointSprites(
    const PointSpriteVertex* points,
    uint32_t numPoints) {

    if (!m_initialized || numPoints == 0) {
        return;
    }

    m_quadVertices.clear();
    m_quadIndices.clear();

    for (uint32_t i = 0; i < numPoints; ++i) {
        const PointSpriteVertex& p = points[i];

        // Transform to view space
        float viewZ = p.x * m_view._13 + p.y * m_view._23 + p.z * m_view._33 + m_view._43;

        // Calculate final size
        float finalSize = CalculatePointSize(viewZ);
        finalSize = p.size * (finalSize / m_params.pointSize);

        ExpandPointToQuad(p.x, p.y, p.z, p.color, finalSize, p.rotation);
    }

    // Render (same as above)
    if (!m_quadVertices.empty()) {
        bgfx::TransientVertexBuffer tvb;
        bgfx::TransientIndexBuffer tib;

        bgfx::VertexLayout layout = GetQuadVertexLayout();

        uint32_t numVerts = static_cast<uint32_t>(m_quadVertices.size());
        uint32_t numInds = static_cast<uint32_t>(m_quadIndices.size());

        if (bgfx::allocTransientBuffers(&tvb, layout, numVerts, &tib, numInds)) {
            std::memcpy(tvb.data, m_quadVertices.data(), numVerts * sizeof(PointQuadVertex));
            std::memcpy(tib.data, m_quadIndices.data(), numInds * sizeof(uint16_t));

            bgfx::setVertexBuffer(0, &tvb);
            bgfx::setIndexBuffer(&tib);

            if (bgfx::isValid(m_texture)) {
                bgfx::setTexture(0, BGFX_INVALID_HANDLE, m_texture);
            }
        }
    }
}

// =============================================================================
// ParticleSystem Implementation
// =============================================================================

static std::mt19937 g_rng(12345);

ParticleSystem::ParticleSystem()
    : m_activeCount(0)
    , m_emitAccumulator(0.0f)
    , m_emitting(true)
    , m_initialized(false) {
}

ParticleSystem::~ParticleSystem() {
    Shutdown();
}

void ParticleSystem::Initialize(const ParticleEmitter& emitter) {
    m_emitter = emitter;
    m_particles.resize(emitter.maxParticles);
    m_activeCount = 0;
    m_emitAccumulator = 0.0f;
    m_initialized = true;

    // Initialize all particles as dead
    for (auto& p : m_particles) {
        p.life = 0.0f;
    }
}

void ParticleSystem::Shutdown() {
    m_particles.clear();
    m_activeCount = 0;
    m_initialized = false;
}

void ParticleSystem::Update(float deltaTime) {
    if (!m_initialized) {
        return;
    }

    // Update existing particles
    m_activeCount = 0;
    for (auto& p : m_particles) {
        if (p.life <= 0.0f) {
            continue;
        }

        // Update life
        p.life -= deltaTime / p.maxLife;

        if (p.life <= 0.0f) {
            continue;
        }

        // Update velocity (gravity and drag)
        p.vx += m_emitter.gravityX * deltaTime;
        p.vy += m_emitter.gravityY * deltaTime;
        p.vz += m_emitter.gravityZ * deltaTime;

        float drag = std::pow(1.0f - m_emitter.drag, deltaTime);
        p.vx *= drag;
        p.vy *= drag;
        p.vz *= drag;

        // Update position
        p.x += p.vx * deltaTime;
        p.y += p.vy * deltaTime;
        p.z += p.vz * deltaTime;

        // Update rotation
        p.rotation += p.angularVelocity * deltaTime;

        // Update size
        float t = 1.0f - p.life;
        float startSize = RandomFloat(m_emitter.sizeMin, m_emitter.sizeMax);
        p.size = startSize + (m_emitter.sizeEnd - startSize) * t;

        // Update color
        p.color = LerpColor(m_emitter.colorStart, m_emitter.colorEnd, t);

        m_activeCount++;
    }

    // Emit new particles
    if (m_emitting) {
        m_emitAccumulator += m_emitter.emitRate * deltaTime;

        while (m_emitAccumulator >= 1.0f) {
            EmitParticle();
            m_emitAccumulator -= 1.0f;
        }
    }
}

void ParticleSystem::Reset() {
    for (auto& p : m_particles) {
        p.life = 0.0f;
    }
    m_activeCount = 0;
    m_emitAccumulator = 0.0f;
}

void ParticleSystem::SetPosition(float x, float y, float z) {
    m_emitter.x = x;
    m_emitter.y = y;
    m_emitter.z = z;
}

void ParticleSystem::Burst(uint32_t count) {
    for (uint32_t i = 0; i < count; ++i) {
        EmitParticle();
    }
}

void ParticleSystem::EmitParticle() {
    // Find dead particle
    for (auto& p : m_particles) {
        if (p.life <= 0.0f) {
            // Initialize particle
            p.x = m_emitter.x;
            p.y = m_emitter.y;
            p.z = m_emitter.z;

            // Random velocity within cone
            float speed = RandomFloat(m_emitter.speedMin, m_emitter.speedMax);

            // Random direction within spread cone
            float phi = RandomFloat(0.0f, 2.0f * 3.14159f);
            float cosTheta = RandomFloat(std::cos(m_emitter.spread), 1.0f);
            float sinTheta = std::sqrt(1.0f - cosTheta * cosTheta);

            float localX = sinTheta * std::cos(phi);
            float localY = sinTheta * std::sin(phi);
            float localZ = cosTheta;

            // Rotate to emitter direction
            // Simple rotation assuming direction is normalized
            float dx = m_emitter.dirX;
            float dy = m_emitter.dirY;
            float dz = m_emitter.dirZ;

            // Find perpendicular vectors
            float ax, ay, az;
            if (std::abs(dx) < 0.9f) {
                ax = 1; ay = 0; az = 0;
            } else {
                ax = 0; ay = 1; az = 0;
            }

            // Cross products to get basis
            float rx = ay * dz - az * dy;
            float ry = az * dx - ax * dz;
            float rz = ax * dy - ay * dx;
            float rlen = std::sqrt(rx*rx + ry*ry + rz*rz);
            rx /= rlen; ry /= rlen; rz /= rlen;

            float ux = dy * rz - dz * ry;
            float uy = dz * rx - dx * rz;
            float uz = dx * ry - dy * rx;

            p.vx = (localX * rx + localY * ux + localZ * dx) * speed;
            p.vy = (localX * ry + localY * uy + localZ * dy) * speed;
            p.vz = (localX * rz + localY * uz + localZ * dz) * speed;

            p.life = 1.0f;
            p.maxLife = RandomFloat(m_emitter.lifeMin, m_emitter.lifeMax);
            p.size = RandomFloat(m_emitter.sizeMin, m_emitter.sizeMax);
            p.color = m_emitter.colorStart;
            p.rotation = RandomFloat(m_emitter.rotationMin, m_emitter.rotationMax);
            p.angularVelocity = RandomFloat(m_emitter.angularVelocityMin, m_emitter.angularVelocityMax);

            return;
        }
    }
}

float ParticleSystem::RandomFloat(float min, float max) {
    std::uniform_real_distribution<float> dist(min, max);
    return dist(g_rng);
}

uint32_t ParticleSystem::LerpColor(uint32_t c1, uint32_t c2, float t) {
    uint8_t a1 = (c1 >> 24) & 0xFF;
    uint8_t r1 = (c1 >> 16) & 0xFF;
    uint8_t g1 = (c1 >> 8) & 0xFF;
    uint8_t b1 = c1 & 0xFF;

    uint8_t a2 = (c2 >> 24) & 0xFF;
    uint8_t r2 = (c2 >> 16) & 0xFF;
    uint8_t g2 = (c2 >> 8) & 0xFF;
    uint8_t b2 = c2 & 0xFF;

    uint8_t a = uint8_t(a1 + (a2 - a1) * t);
    uint8_t r = uint8_t(r1 + (r2 - r1) * t);
    uint8_t g = uint8_t(g1 + (g2 - g1) * t);
    uint8_t b = uint8_t(b1 + (b2 - b1) * t);

    return (a << 24) | (r << 16) | (g << 8) | b;
}

// =============================================================================
// ParticleRenderer Implementation
// =============================================================================

ParticleRenderer::ParticleRenderer()
    : m_texture(BGFX_INVALID_HANDLE)
    , m_srcBlend(D3DBLEND_SRCALPHA)
    , m_destBlend(D3DBLEND_ONE)
    , m_softParticles(false)
    , m_softScale(1.0f)
    , m_sortParticles(false)
    , m_initialized(false) {
}

ParticleRenderer::~ParticleRenderer() {
    Shutdown();
}

void ParticleRenderer::Initialize() {
    if (m_initialized) {
        return;
    }

    m_spriteRenderer.Initialize();
    m_initialized = true;
}

void ParticleRenderer::Shutdown() {
    if (!m_initialized) {
        return;
    }

    m_spriteRenderer.Shutdown();
    m_initialized = false;
}

void ParticleRenderer::SetTexture(bgfx::TextureHandle texture) {
    m_texture = texture;
    m_spriteRenderer.SetTexture(texture);
}

void ParticleRenderer::SetBlendMode(D3DBLEND srcBlend, D3DBLEND destBlend) {
    m_srcBlend = srcBlend;
    m_destBlend = destBlend;
}

void ParticleRenderer::SetSoftParticles(bool enabled, float scale) {
    m_softParticles = enabled;
    m_softScale = scale;
}

void ParticleRenderer::Render(
    const ParticleSystem& system,
    const D3DMATRIX& view,
    const D3DMATRIX& projection) {

    if (!m_initialized) {
        return;
    }

    const auto& particles = system.GetParticles();

    // Convert to point sprites
    std::vector<PointSpriteVertex> points;
    points.reserve(system.GetActiveCount());

    for (const auto& p : particles) {
        if (p.life > 0.0f) {
            PointSpriteVertex v;
            v.x = p.x;
            v.y = p.y;
            v.z = p.z;
            v.color = p.color;
            v.size = p.size;
            v.rotation = p.rotation;
            points.push_back(v);
        }
    }

    // Sort if needed
    if (m_sortParticles && !points.empty()) {
        // Sort back to front
        std::sort(points.begin(), points.end(),
            [&view](const PointSpriteVertex& a, const PointSpriteVertex& b) {
                float za = a.x * view._13 + a.y * view._23 + a.z * view._33 + view._43;
                float zb = b.x * view._13 + b.y * view._23 + b.z * view._33 + view._43;
                return za > zb;
            });
    }

    // Set matrices
    m_spriteRenderer.SetViewMatrix(view);
    m_spriteRenderer.SetProjectionMatrix(projection);

    // Render
    if (!points.empty()) {
        m_spriteRenderer.RenderPointSprites(points.data(), static_cast<uint32_t>(points.size()));
    }
}

// =============================================================================
// GPUPointSprites Implementation
// =============================================================================

GPUPointSprites::GPUPointSprites()
    : m_pointParams(BGFX_INVALID_HANDLE)
    , m_attenuation(BGFX_INVALID_HANDLE)
    , m_initialized(false) {
}

GPUPointSprites::~GPUPointSprites() {
    Shutdown();
}

void GPUPointSprites::Initialize() {
    if (m_initialized) {
        return;
    }

    m_pointParams = bgfx::createUniform("u_pointParams", bgfx::UniformType::Vec4);
    m_attenuation = bgfx::createUniform("u_pointAttenuation", bgfx::UniformType::Vec4);

    m_initialized = true;
}

void GPUPointSprites::Shutdown() {
    if (!m_initialized) {
        return;
    }

    if (bgfx::isValid(m_pointParams)) {
        bgfx::destroy(m_pointParams);
        m_pointParams = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(m_attenuation)) {
        bgfx::destroy(m_attenuation);
        m_attenuation = BGFX_INVALID_HANDLE;
    }

    m_initialized = false;
}

void GPUPointSprites::SetPointSize(float size) {
    if (m_initialized) {
        float params[4] = { size, 1.0f, 64.0f, 0.0f };
        bgfx::setUniform(m_pointParams, params);
    }
}

void GPUPointSprites::SetPointSizeRange(float min, float max) {
    (void)min;
    (void)max;
    // Would update uniform
}

void GPUPointSprites::SetPointAttenuation(float a, float b, float c) {
    if (m_initialized) {
        float atten[4] = { a, b, c, 0.0f };
        bgfx::setUniform(m_attenuation, atten);
    }
}

bgfx::VertexLayout GPUPointSprites::GetVertexLayout() {
    bgfx::VertexLayout layout;
    layout.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
        .add(bgfx::Attrib::TexCoord0, 1, bgfx::AttribType::Float) // Point size
        .end();
    return layout;
}

bgfx::VertexBufferHandle GPUPointSprites::CreatePointBuffer(
    const float* positions,
    const uint32_t* colors,
    const float* sizes,
    uint32_t numPoints) {

    bgfx::VertexLayout layout = GetVertexLayout();

    uint32_t stride = layout.getStride();
    std::vector<uint8_t> data(numPoints * stride);

    for (uint32_t i = 0; i < numPoints; ++i) {
        uint8_t* ptr = data.data() + i * stride;

        // Position
        std::memcpy(ptr, &positions[i * 3], 12);
        ptr += 12;

        // Color
        std::memcpy(ptr, &colors[i], 4);
        ptr += 4;

        // Size
        float size = sizes ? sizes[i] : 1.0f;
        std::memcpy(ptr, &size, 4);
    }

    const bgfx::Memory* mem = bgfx::copy(data.data(), static_cast<uint32_t>(data.size()));
    return bgfx::createVertexBuffer(mem, layout);
}

void GPUPointSprites::Render(
    bgfx::VertexBufferHandle vb,
    uint32_t numPoints,
    const D3DMATRIX& view,
    const D3DMATRIX& projection) {

    (void)view;
    (void)projection;

    bgfx::setVertexBuffer(0, vb, 0, numPoints);
    // Would submit with point sprite shader
}

// =============================================================================
// Utility Functions
// =============================================================================

float ExtractPointSize(const void* vertex, DWORD fvf, uint32_t stride) {
    (void)stride;

    int offset = GetPointSizeOffset(fvf);
    if (offset < 0) {
        return 1.0f;
    }

    const float* ptr = reinterpret_cast<const float*>(
        static_cast<const uint8_t*>(vertex) + offset
    );
    return *ptr;
}

int GetPointSizeOffset(DWORD fvf) {
    if (!(fvf & D3DFVF_PSIZE)) {
        return -1;
    }

    int offset = 0;

    // Position
    if (fvf & D3DFVF_XYZ) {
        offset += 12;
    } else if (fvf & D3DFVF_XYZRHW) {
        offset += 16;
    }

    // Normal
    if (fvf & D3DFVF_NORMAL) {
        offset += 12;
    }

    return offset;
}

float CalculateAttenuatedPointSize(
    float baseSize,
    float distance,
    float a, float b, float c,
    float viewportHeight) {

    float attenuation = a + b * distance + c * distance * distance;
    if (attenuation <= 0.0f) {
        return baseSize;
    }

    return baseSize * std::sqrt(viewportHeight * viewportHeight / attenuation);
}

} // namespace dx8bgfx
