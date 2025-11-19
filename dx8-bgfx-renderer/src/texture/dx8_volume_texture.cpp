// =============================================================================
// DX8-BGFX Volume Texture Implementation
// =============================================================================

#include "dx8bgfx/dx8_volume_texture.h"
#include "dx8bgfx/dx8_texture_utils.h"
#include <cstring>
#include <cmath>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace dx8bgfx {

// =============================================================================
// VolumeTextureUtils Implementation
// =============================================================================

bgfx::TextureHandle VolumeTextureUtils::CreateVolumeTexture(
    uint32_t width,
    uint32_t height,
    uint32_t depth,
    bool hasMips,
    bgfx::TextureFormat::Enum format,
    uint64_t flags) {

    return bgfx::createTexture3D(
        uint16_t(width),
        uint16_t(height),
        uint16_t(depth),
        hasMips,
        format,
        flags
    );
}

bgfx::TextureHandle VolumeTextureUtils::CreateVolumeTextureFromMemory(
    const void* data,
    uint32_t width,
    uint32_t height,
    uint32_t depth,
    bgfx::TextureFormat::Enum format,
    bool hasMips) {

    uint32_t size = CalculateDataSize(width, height, depth, format, hasMips);
    const bgfx::Memory* mem = bgfx::copy(data, size);

    return bgfx::createTexture3D(
        uint16_t(width),
        uint16_t(height),
        uint16_t(depth),
        hasMips,
        format,
        BGFX_TEXTURE_NONE,
        mem
    );
}

void VolumeTextureUtils::UpdateVolumeTexture(
    bgfx::TextureHandle handle,
    uint8_t mip,
    uint16_t x, uint16_t y, uint16_t z,
    uint16_t width, uint16_t height, uint16_t depth,
    const void* data,
    uint32_t pitch) {

    uint32_t size = width * height * depth * 4; // Assume RGBA8
    const bgfx::Memory* mem = bgfx::copy(data, size);

    bgfx::updateTexture3D(handle, mip, x, y, z, width, height, depth, mem, pitch);
}

void VolumeTextureUtils::GetMipDimensions(
    uint32_t level,
    uint32_t baseWidth,
    uint32_t baseHeight,
    uint32_t baseDepth,
    uint32_t& mipWidth,
    uint32_t& mipHeight,
    uint32_t& mipDepth) {

    mipWidth = std::max(1u, baseWidth >> level);
    mipHeight = std::max(1u, baseHeight >> level);
    mipDepth = std::max(1u, baseDepth >> level);
}

uint32_t VolumeTextureUtils::CalculateDataSize(
    uint32_t width,
    uint32_t height,
    uint32_t depth,
    bgfx::TextureFormat::Enum format,
    bool includeMips) {

    uint32_t bpp = GetBytesPerPixel(format);
    uint32_t size = width * height * depth * bpp;

    if (includeMips) {
        uint32_t w = width, h = height, d = depth;
        while (w > 1 || h > 1 || d > 1) {
            w = std::max(1u, w >> 1);
            h = std::max(1u, h >> 1);
            d = std::max(1u, d >> 1);
            size += w * h * d * bpp;
        }
    }

    return size;
}

uint32_t VolumeTextureUtils::GetBytesPerPixel(bgfx::TextureFormat::Enum format) {
    switch (format) {
        case bgfx::TextureFormat::R8:
            return 1;
        case bgfx::TextureFormat::RG8:
        case bgfx::TextureFormat::R16F:
            return 2;
        case bgfx::TextureFormat::RGBA8:
        case bgfx::TextureFormat::BGRA8:
        case bgfx::TextureFormat::RG16F:
        case bgfx::TextureFormat::R32F:
            return 4;
        case bgfx::TextureFormat::RGBA16F:
        case bgfx::TextureFormat::RG32F:
            return 8;
        case bgfx::TextureFormat::RGBA32F:
            return 16;
        default:
            return 4;
    }
}

// =============================================================================
// VolumeTextureManager Implementation
// =============================================================================

VolumeTextureManager::VolumeTextureManager() {
    for (int i = 0; i < 8; ++i) {
        m_stages[i].texture = BGFX_INVALID_HANDLE;
        m_stages[i].isVolume = false;
    }
}

VolumeTextureManager::~VolumeTextureManager() {
    for (auto& entry : m_textures) {
        if (bgfx::isValid(entry.handle)) {
            bgfx::destroy(entry.handle);
        }
    }
}

bgfx::TextureHandle VolumeTextureManager::CreateTexture(const VolumeTextureDesc& desc) {
    bgfx::TextureFormat::Enum format = TextureUtils::ConvertFormat(desc.format);
    uint64_t flags = BGFX_TEXTURE_NONE;

    if (desc.dynamic) {
        flags |= BGFX_TEXTURE_RT;
    }

    bgfx::TextureHandle handle = VolumeTextureUtils::CreateVolumeTexture(
        desc.width,
        desc.height,
        desc.depth,
        desc.levels != 1,
        format,
        flags
    );

    if (bgfx::isValid(handle)) {
        VolumeTextureEntry entry;
        entry.handle = handle;
        entry.desc = desc;
        entry.locked = false;
        m_textures.push_back(entry);
    }

    return handle;
}

void VolumeTextureManager::DestroyTexture(bgfx::TextureHandle handle) {
    for (auto it = m_textures.begin(); it != m_textures.end(); ++it) {
        if (it->handle.idx == handle.idx) {
            if (bgfx::isValid(it->handle)) {
                bgfx::destroy(it->handle);
            }
            m_textures.erase(it);
            return;
        }
    }
}

bool VolumeTextureManager::LockBox(
    bgfx::TextureHandle handle,
    uint32_t level,
    D3DLOCKED_BOX* lockedBox,
    const D3DBOX* box,
    DWORD flags) {

    (void)flags;

    for (auto& entry : m_textures) {
        if (entry.handle.idx == handle.idx) {
            if (entry.locked) {
                return false;
            }

            // Calculate mip dimensions
            uint32_t mipW, mipH, mipD;
            VolumeTextureUtils::GetMipDimensions(
                level,
                entry.desc.width,
                entry.desc.height,
                entry.desc.depth,
                mipW, mipH, mipD
            );

            // Determine lock region
            D3DBOX lockBox;
            if (box) {
                lockBox = *box;
            } else {
                lockBox.Left = 0;
                lockBox.Top = 0;
                lockBox.Front = 0;
                lockBox.Right = mipW;
                lockBox.Bottom = mipH;
                lockBox.Back = mipD;
            }

            // Calculate pitches
            uint32_t bpp = 4; // Assume RGBA8
            uint32_t width = lockBox.Right - lockBox.Left;
            uint32_t height = lockBox.Bottom - lockBox.Top;
            uint32_t depth = lockBox.Back - lockBox.Front;

            uint32_t rowPitch = width * bpp;
            uint32_t slicePitch = rowPitch * height;

            // Allocate buffer
            entry.lockBuffer.resize(slicePitch * depth);
            entry.locked = true;
            entry.lockedLevel = level;
            entry.lockedBox = lockBox;

            lockedBox->pBits = entry.lockBuffer.data();
            lockedBox->RowPitch = rowPitch;
            lockedBox->SlicePitch = slicePitch;

            return true;
        }
    }

    return false;
}

void VolumeTextureManager::UnlockBox(bgfx::TextureHandle handle, uint32_t level) {
    for (auto& entry : m_textures) {
        if (entry.handle.idx == handle.idx && entry.locked) {
            if (entry.lockedLevel == level) {
                // Upload data
                uint32_t width = entry.lockedBox.Right - entry.lockedBox.Left;
                uint32_t height = entry.lockedBox.Bottom - entry.lockedBox.Top;
                uint32_t depth = entry.lockedBox.Back - entry.lockedBox.Front;

                VolumeTextureUtils::UpdateVolumeTexture(
                    entry.handle,
                    uint8_t(level),
                    uint16_t(entry.lockedBox.Left),
                    uint16_t(entry.lockedBox.Top),
                    uint16_t(entry.lockedBox.Front),
                    uint16_t(width),
                    uint16_t(height),
                    uint16_t(depth),
                    entry.lockBuffer.data(),
                    width * 4
                );

                entry.locked = false;
                entry.lockBuffer.clear();
            }
            return;
        }
    }
}

void VolumeTextureManager::SetVolumeTexture(uint32_t stage, bgfx::TextureHandle handle) {
    if (stage < 8) {
        m_stages[stage].texture = handle;
        m_stages[stage].isVolume = true;
    }
}

bgfx::TextureHandle VolumeTextureManager::GetVolumeTexture(uint32_t stage) const {
    if (stage < 8) {
        return m_stages[stage].texture;
    }
    return BGFX_INVALID_HANDLE;
}

bool VolumeTextureManager::IsVolumeTexture(uint32_t stage) const {
    if (stage < 8) {
        return m_stages[stage].isVolume;
    }
    return false;
}

// =============================================================================
// VolumeDataGenerator Implementation
// =============================================================================

// Simple noise function
static float Noise3D(float x, float y, float z) {
    // Simple hash-based noise
    int ix = int(std::floor(x)) & 255;
    int iy = int(std::floor(y)) & 255;
    int iz = int(std::floor(z)) & 255;

    float fx = x - std::floor(x);
    float fy = y - std::floor(y);
    float fz = z - std::floor(z);

    // Smoothstep
    fx = fx * fx * (3.0f - 2.0f * fx);
    fy = fy * fy * (3.0f - 2.0f * fy);
    fz = fz * fz * (3.0f - 2.0f * fz);

    // Hash
    auto hash = [](int x, int y, int z) -> float {
        int n = x + y * 57 + z * 113;
        n = (n << 13) ^ n;
        return (1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f);
    };

    // Trilinear interpolation
    float v000 = hash(ix, iy, iz);
    float v100 = hash(ix + 1, iy, iz);
    float v010 = hash(ix, iy + 1, iz);
    float v110 = hash(ix + 1, iy + 1, iz);
    float v001 = hash(ix, iy, iz + 1);
    float v101 = hash(ix + 1, iy, iz + 1);
    float v011 = hash(ix, iy + 1, iz + 1);
    float v111 = hash(ix + 1, iy + 1, iz + 1);

    float v00 = v000 + fx * (v100 - v000);
    float v10 = v010 + fx * (v110 - v010);
    float v01 = v001 + fx * (v101 - v001);
    float v11 = v011 + fx * (v111 - v011);

    float v0 = v00 + fy * (v10 - v00);
    float v1 = v01 + fy * (v11 - v01);

    return v0 + fz * (v1 - v0);
}

std::vector<uint8_t> VolumeDataGenerator::GeneratePerlinNoise(
    uint32_t width,
    uint32_t height,
    uint32_t depth,
    float scale,
    uint32_t octaves) {

    std::vector<uint8_t> data(width * height * depth);

    for (uint32_t z = 0; z < depth; ++z) {
        for (uint32_t y = 0; y < height; ++y) {
            for (uint32_t x = 0; x < width; ++x) {
                float value = 0.0f;
                float amplitude = 1.0f;
                float frequency = scale;
                float maxValue = 0.0f;

                for (uint32_t o = 0; o < octaves; ++o) {
                    value += Noise3D(
                        x * frequency / width,
                        y * frequency / height,
                        z * frequency / depth
                    ) * amplitude;

                    maxValue += amplitude;
                    amplitude *= 0.5f;
                    frequency *= 2.0f;
                }

                value = (value / maxValue + 1.0f) * 0.5f;
                data[z * width * height + y * width + x] = uint8_t(value * 255.0f);
            }
        }
    }

    return data;
}

std::vector<uint8_t> VolumeDataGenerator::GenerateDensityField(
    uint32_t width,
    uint32_t height,
    uint32_t depth,
    float density,
    float falloff) {

    std::vector<uint8_t> data(width * height * depth);

    float cx = width * 0.5f;
    float cy = height * 0.5f;
    float cz = depth * 0.5f;

    for (uint32_t z = 0; z < depth; ++z) {
        for (uint32_t y = 0; y < height; ++y) {
            for (uint32_t x = 0; x < width; ++x) {
                float dx = (x - cx) / cx;
                float dy = (y - cy) / cy;
                float dz = (z - cz) / cz;
                float dist = std::sqrt(dx*dx + dy*dy + dz*dz);

                // Add noise
                float noise = Noise3D(x * 0.1f, y * 0.1f, z * 0.1f) * 0.3f;
                float value = density * (1.0f - std::pow(dist, falloff)) + noise;
                value = std::max(0.0f, std::min(1.0f, value));

                data[z * width * height + y * width + x] = uint8_t(value * 255.0f);
            }
        }
    }

    return data;
}

std::vector<uint8_t> VolumeDataGenerator::GenerateGradient(
    uint32_t size,
    const uint32_t* colors,
    const float* positions,
    uint32_t numStops) {

    std::vector<uint8_t> data(size * 4);

    for (uint32_t i = 0; i < size; ++i) {
        float t = float(i) / float(size - 1);

        // Find surrounding stops
        uint32_t idx0 = 0, idx1 = 0;
        for (uint32_t s = 0; s < numStops - 1; ++s) {
            if (t >= positions[s] && t <= positions[s + 1]) {
                idx0 = s;
                idx1 = s + 1;
                break;
            }
        }

        // Interpolate
        float range = positions[idx1] - positions[idx0];
        float localT = (range > 0.0001f) ? (t - positions[idx0]) / range : 0.0f;

        uint32_t c0 = colors[idx0];
        uint32_t c1 = colors[idx1];

        uint8_t r0 = (c0 >> 16) & 0xFF;
        uint8_t g0 = (c0 >> 8) & 0xFF;
        uint8_t b0 = c0 & 0xFF;
        uint8_t a0 = (c0 >> 24) & 0xFF;

        uint8_t r1 = (c1 >> 16) & 0xFF;
        uint8_t g1 = (c1 >> 8) & 0xFF;
        uint8_t b1 = c1 & 0xFF;
        uint8_t a1 = (c1 >> 24) & 0xFF;

        data[i * 4 + 0] = uint8_t(r0 + (r1 - r0) * localT);
        data[i * 4 + 1] = uint8_t(g0 + (g1 - g0) * localT);
        data[i * 4 + 2] = uint8_t(b0 + (b1 - b0) * localT);
        data[i * 4 + 3] = uint8_t(a0 + (a1 - a0) * localT);
    }

    return data;
}

std::vector<uint8_t> VolumeDataGenerator::GenerateSphereDensity(
    uint32_t size,
    float radius,
    float falloff) {

    std::vector<uint8_t> data(size * size * size);

    float center = size * 0.5f;
    float radiusPixels = size * radius;

    for (uint32_t z = 0; z < size; ++z) {
        for (uint32_t y = 0; y < size; ++y) {
            for (uint32_t x = 0; x < size; ++x) {
                float dx = x - center;
                float dy = y - center;
                float dz = z - center;
                float dist = std::sqrt(dx*dx + dy*dy + dz*dz);

                float value = 1.0f - std::pow(dist / radiusPixels, falloff);
                value = std::max(0.0f, value);

                data[z * size * size + y * size + x] = uint8_t(value * 255.0f);
            }
        }
    }

    return data;
}

std::vector<uint8_t> VolumeDataGenerator::GenerateBoxDensity(
    uint32_t width,
    uint32_t height,
    uint32_t depth,
    float minX, float maxX,
    float minY, float maxY,
    float minZ, float maxZ,
    float falloff) {

    std::vector<uint8_t> data(width * height * depth);

    for (uint32_t z = 0; z < depth; ++z) {
        for (uint32_t y = 0; y < height; ++y) {
            for (uint32_t x = 0; x < width; ++x) {
                float px = float(x) / float(width - 1);
                float py = float(y) / float(height - 1);
                float pz = float(z) / float(depth - 1);

                // Distance to box
                float dx = std::max(minX - px, std::max(0.0f, px - maxX));
                float dy = std::max(minY - py, std::max(0.0f, py - maxY));
                float dz = std::max(minZ - pz, std::max(0.0f, pz - maxZ));

                float dist = std::sqrt(dx*dx + dy*dy + dz*dz);
                float value = 1.0f - std::pow(dist * 4.0f, falloff);
                value = std::max(0.0f, value);

                data[z * width * height + y * width + x] = uint8_t(value * 255.0f);
            }
        }
    }

    return data;
}

// =============================================================================
// VolumeRenderer Implementation
// =============================================================================

VolumeRenderer::VolumeRenderer()
    : m_volume(BGFX_INVALID_HANDLE)
    , m_transfer(BGFX_INVALID_HANDLE)
    , m_stepSize(0.01f)
    , m_densityScale(1.0f)
    , m_brightness(1.0f)
    , m_cubeVB(BGFX_INVALID_HANDLE)
    , m_cubeIB(BGFX_INVALID_HANDLE)
    , m_initialized(false) {
}

VolumeRenderer::~VolumeRenderer() {
    Shutdown();
}

void VolumeRenderer::Initialize() {
    if (m_initialized) {
        return;
    }

    // Create proxy cube geometry
    // Note: Actual volume rendering would use ray marching shaders

    m_initialized = true;
}

void VolumeRenderer::Shutdown() {
    if (!m_initialized) {
        return;
    }

    if (bgfx::isValid(m_cubeVB)) {
        bgfx::destroy(m_cubeVB);
        m_cubeVB = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(m_cubeIB)) {
        bgfx::destroy(m_cubeIB);
        m_cubeIB = BGFX_INVALID_HANDLE;
    }

    m_initialized = false;
}

void VolumeRenderer::SetVolume(bgfx::TextureHandle volume) {
    m_volume = volume;
}

void VolumeRenderer::SetTransferFunction(bgfx::TextureHandle transfer) {
    m_transfer = transfer;
}

void VolumeRenderer::SetStepSize(float stepSize) {
    m_stepSize = stepSize;
}

void VolumeRenderer::SetDensityScale(float scale) {
    m_densityScale = scale;
}

void VolumeRenderer::SetBrightness(float brightness) {
    m_brightness = brightness;
}

void VolumeRenderer::Render(
    const D3DMATRIX& world,
    const D3DMATRIX& view,
    const D3DMATRIX& projection,
    const D3DVECTOR& eyePos) {

    // Note: Full implementation would use ray marching shader
    // This is a placeholder

    (void)world;
    (void)view;
    (void)projection;
    (void)eyePos;
}

// =============================================================================
// LUT3D Implementation
// =============================================================================

LUT3D::LUT3D()
    : m_texture(BGFX_INVALID_HANDLE)
    , m_size(0) {
}

LUT3D::~LUT3D() {
    if (bgfx::isValid(m_texture)) {
        bgfx::destroy(m_texture);
    }
}

void LUT3D::CreateIdentity(uint32_t size) {
    m_size = size;
    m_data.resize(size * size * size * 3);

    for (uint32_t z = 0; z < size; ++z) {
        for (uint32_t y = 0; y < size; ++y) {
            for (uint32_t x = 0; x < size; ++x) {
                uint32_t idx = (z * size * size + y * size + x) * 3;
                m_data[idx + 0] = float(x) / float(size - 1);
                m_data[idx + 1] = float(y) / float(size - 1);
                m_data[idx + 2] = float(z) / float(size - 1);
            }
        }
    }

    Rebuild();
}

bool LUT3D::LoadFromCube(const char* path) {
    std::ifstream file(path);
    if (!file) {
        return false;
    }

    std::string line;
    uint32_t size = 0;
    std::vector<float> data;

    while (std::getline(file, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#') {
            continue;
        }

        // Parse LUT_3D_SIZE
        if (line.find("LUT_3D_SIZE") != std::string::npos) {
            std::istringstream iss(line);
            std::string token;
            iss >> token >> size;
            data.reserve(size * size * size * 3);
            continue;
        }

        // Parse RGB values
        std::istringstream iss(line);
        float r, g, b;
        if (iss >> r >> g >> b) {
            data.push_back(r);
            data.push_back(g);
            data.push_back(b);
        }
    }

    if (size == 0 || data.size() != size * size * size * 3) {
        return false;
    }

    m_size = size;
    m_data = std::move(data);
    Rebuild();

    return true;
}

void LUT3D::LoadFromData(const float* data, uint32_t size) {
    m_size = size;
    m_data.resize(size * size * size * 3);
    std::memcpy(m_data.data(), data, m_data.size() * sizeof(float));
    Rebuild();
}

void LUT3D::SetBrightness(float brightness) {
    for (size_t i = 0; i < m_data.size(); ++i) {
        m_data[i] *= brightness;
    }
}

void LUT3D::SetContrast(float contrast) {
    for (size_t i = 0; i < m_data.size(); ++i) {
        m_data[i] = (m_data[i] - 0.5f) * contrast + 0.5f;
    }
}

void LUT3D::SetSaturation(float saturation) {
    for (uint32_t i = 0; i < m_data.size() / 3; ++i) {
        float r = m_data[i * 3 + 0];
        float g = m_data[i * 3 + 1];
        float b = m_data[i * 3 + 2];

        float gray = r * 0.299f + g * 0.587f + b * 0.114f;

        m_data[i * 3 + 0] = gray + (r - gray) * saturation;
        m_data[i * 3 + 1] = gray + (g - gray) * saturation;
        m_data[i * 3 + 2] = gray + (b - gray) * saturation;
    }
}

void LUT3D::SetHueShift(float degrees) {
    float rad = degrees * 3.14159f / 180.0f;
    float c = std::cos(rad);
    float s = std::sin(rad);

    // Hue rotation matrix
    float m00 = 0.299f + 0.701f * c + 0.168f * s;
    float m01 = 0.587f - 0.587f * c + 0.330f * s;
    float m02 = 0.114f - 0.114f * c - 0.497f * s;
    float m10 = 0.299f - 0.299f * c - 0.328f * s;
    float m11 = 0.587f + 0.413f * c + 0.035f * s;
    float m12 = 0.114f - 0.114f * c + 0.292f * s;
    float m20 = 0.299f - 0.300f * c + 1.250f * s;
    float m21 = 0.587f - 0.588f * c - 1.050f * s;
    float m22 = 0.114f + 0.886f * c - 0.203f * s;

    for (uint32_t i = 0; i < m_data.size() / 3; ++i) {
        float r = m_data[i * 3 + 0];
        float g = m_data[i * 3 + 1];
        float b = m_data[i * 3 + 2];

        m_data[i * 3 + 0] = r * m00 + g * m01 + b * m02;
        m_data[i * 3 + 1] = r * m10 + g * m11 + b * m12;
        m_data[i * 3 + 2] = r * m20 + g * m21 + b * m22;
    }
}

void LUT3D::Rebuild() {
    if (m_size == 0 || m_data.empty()) {
        return;
    }

    // Destroy old texture
    if (bgfx::isValid(m_texture)) {
        bgfx::destroy(m_texture);
    }

    // Convert to RGBA8
    std::vector<uint8_t> rgba(m_size * m_size * m_size * 4);
    for (uint32_t i = 0; i < m_data.size() / 3; ++i) {
        rgba[i * 4 + 0] = uint8_t(std::max(0.0f, std::min(1.0f, m_data[i * 3 + 0])) * 255.0f);
        rgba[i * 4 + 1] = uint8_t(std::max(0.0f, std::min(1.0f, m_data[i * 3 + 1])) * 255.0f);
        rgba[i * 4 + 2] = uint8_t(std::max(0.0f, std::min(1.0f, m_data[i * 3 + 2])) * 255.0f);
        rgba[i * 4 + 3] = 255;
    }

    m_texture = VolumeTextureUtils::CreateVolumeTextureFromMemory(
        rgba.data(),
        m_size, m_size, m_size,
        bgfx::TextureFormat::RGBA8,
        false
    );
}

// =============================================================================
// VolumeTexGen Implementation
// =============================================================================

void VolumeTexGen::GenerateObjectCoords(
    const float* positions,
    const D3DMATRIX& worldInverse,
    const D3DVECTOR& volumeMin,
    const D3DVECTOR& volumeMax,
    float* texCoords,
    uint32_t numVertices) {

    float sizeX = volumeMax.x - volumeMin.x;
    float sizeY = volumeMax.y - volumeMin.y;
    float sizeZ = volumeMax.z - volumeMin.z;

    for (uint32_t i = 0; i < numVertices; ++i) {
        float px = positions[i * 3 + 0];
        float py = positions[i * 3 + 1];
        float pz = positions[i * 3 + 2];

        // Transform to object space
        float ox = px * worldInverse._11 + py * worldInverse._21 + pz * worldInverse._31 + worldInverse._41;
        float oy = px * worldInverse._12 + py * worldInverse._22 + pz * worldInverse._32 + worldInverse._42;
        float oz = px * worldInverse._13 + py * worldInverse._23 + pz * worldInverse._33 + worldInverse._43;

        // Normalize to 0-1 range based on volume bounds
        texCoords[i * 3 + 0] = (ox - volumeMin.x) / sizeX;
        texCoords[i * 3 + 1] = (oy - volumeMin.y) / sizeY;
        texCoords[i * 3 + 2] = (oz - volumeMin.z) / sizeZ;
    }
}

void VolumeTexGen::GenerateAnimatedCoords(
    const float* positions,
    const D3DMATRIX& world,
    float time,
    float speed,
    float* texCoords,
    uint32_t numVertices) {

    float offset = time * speed;

    for (uint32_t i = 0; i < numVertices; ++i) {
        float px = positions[i * 3 + 0];
        float py = positions[i * 3 + 1];
        float pz = positions[i * 3 + 2];

        // Transform to world space
        float wx = px * world._11 + py * world._21 + pz * world._31 + world._41;
        float wy = px * world._12 + py * world._22 + pz * world._32 + world._42;
        float wz = px * world._13 + py * world._23 + pz * world._33 + world._43;

        // Add time-based offset
        texCoords[i * 3 + 0] = wx + offset;
        texCoords[i * 3 + 1] = wy;
        texCoords[i * 3 + 2] = wz;
    }
}

void VolumeTexGen::TransformCoords(
    float* texCoords,
    uint32_t numVertices,
    const D3DMATRIX& textureMatrix) {

    for (uint32_t i = 0; i < numVertices; ++i) {
        float u = texCoords[i * 3 + 0];
        float v = texCoords[i * 3 + 1];
        float w = texCoords[i * 3 + 2];

        texCoords[i * 3 + 0] = u * textureMatrix._11 + v * textureMatrix._21 + w * textureMatrix._31 + textureMatrix._41;
        texCoords[i * 3 + 1] = u * textureMatrix._12 + v * textureMatrix._22 + w * textureMatrix._32 + textureMatrix._42;
        texCoords[i * 3 + 2] = u * textureMatrix._13 + v * textureMatrix._23 + w * textureMatrix._33 + textureMatrix._43;
    }
}

} // namespace dx8bgfx
