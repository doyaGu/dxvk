// =============================================================================
// DX8-BGFX Cube Texture Implementation
// =============================================================================

#include "dx8bgfx/dx8_cube_texture.h"
#include "dx8bgfx/dx8_texture_utils.h"
#include <cstring>
#include <cmath>

namespace dx8bgfx {

// =============================================================================
// CubeTextureUtils Implementation
// =============================================================================

bgfx::TextureHandle CubeTextureUtils::CreateCubeTexture(
    uint32_t size,
    bool hasMips,
    uint16_t numLayers,
    bgfx::TextureFormat::Enum format,
    uint64_t flags) {

    return bgfx::createTextureCube(
        uint16_t(size),
        hasMips,
        numLayers,
        format,
        flags
    );
}

bgfx::TextureHandle CubeTextureUtils::CreateCubeTextureFromFaces(
    const void* posX, const void* negX,
    const void* posY, const void* negY,
    const void* posZ, const void* negZ,
    uint32_t size,
    bgfx::TextureFormat::Enum format) {

    // Calculate face size in bytes
    uint32_t bpp = 4; // Assume RGBA8
    switch (format) {
        case bgfx::TextureFormat::R8:
            bpp = 1;
            break;
        case bgfx::TextureFormat::RG8:
            bpp = 2;
            break;
        case bgfx::TextureFormat::RGBA8:
        case bgfx::TextureFormat::BGRA8:
            bpp = 4;
            break;
        case bgfx::TextureFormat::RGBA16F:
            bpp = 8;
            break;
        case bgfx::TextureFormat::RGBA32F:
            bpp = 16;
            break;
        default:
            bpp = 4;
            break;
    }

    uint32_t faceSize = size * size * bpp;
    uint32_t totalSize = faceSize * 6;

    // Combine all faces into one buffer
    const bgfx::Memory* mem = bgfx::alloc(totalSize);
    uint8_t* dst = mem->data;

    const void* faces[6] = { posX, negX, posY, negY, posZ, negZ };
    for (int i = 0; i < 6; ++i) {
        if (faces[i]) {
            std::memcpy(dst + i * faceSize, faces[i], faceSize);
        } else {
            std::memset(dst + i * faceSize, 0, faceSize);
        }
    }

    return bgfx::createTextureCube(
        uint16_t(size),
        false, // No mipmaps
        1,
        format,
        BGFX_TEXTURE_NONE,
        mem
    );
}

bgfx::TextureHandle CubeTextureUtils::CreateCubeTextureFromCross(
    const void* data,
    uint32_t width, uint32_t height,
    bgfx::TextureFormat::Enum format) {

    // Determine face size from cross layout
    // Horizontal cross: width = 4*face, height = 3*face
    // Vertical cross: width = 3*face, height = 4*face

    uint32_t faceSize;
    bool horizontal;

    if (width > height) {
        // Horizontal cross
        faceSize = width / 4;
        horizontal = true;
    } else {
        // Vertical cross
        faceSize = height / 4;
        horizontal = false;
    }

    // Calculate bytes per pixel
    uint32_t bpp = 4;
    switch (format) {
        case bgfx::TextureFormat::R8: bpp = 1; break;
        case bgfx::TextureFormat::RG8: bpp = 2; break;
        case bgfx::TextureFormat::RGBA8:
        case bgfx::TextureFormat::BGRA8: bpp = 4; break;
        case bgfx::TextureFormat::RGBA16F: bpp = 8; break;
        case bgfx::TextureFormat::RGBA32F: bpp = 16; break;
        default: bpp = 4; break;
    }

    uint32_t srcPitch = width * bpp;
    uint32_t faceBytes = faceSize * faceSize * bpp;

    // Extract faces
    std::vector<uint8_t> faces[6];
    for (int i = 0; i < 6; ++i) {
        faces[i].resize(faceBytes);
    }

    const uint8_t* src = static_cast<const uint8_t*>(data);

    // Face offsets in cross layout (in face units)
    struct FaceOffset { int x, y; };
    FaceOffset offsets[6];

    if (horizontal) {
        // Horizontal cross:
        //   [+Y]
        // [-X][+Z][+X][-Z]
        //   [-Y]
        offsets[0] = {2, 1}; // +X
        offsets[1] = {0, 1}; // -X
        offsets[2] = {1, 0}; // +Y
        offsets[3] = {1, 2}; // -Y
        offsets[4] = {1, 1}; // +Z
        offsets[5] = {3, 1}; // -Z
    } else {
        // Vertical cross:
        //   [+Y]
        // [-X][+Z][+X]
        //   [-Y]
        //   [-Z]
        offsets[0] = {2, 1}; // +X
        offsets[1] = {0, 1}; // -X
        offsets[2] = {1, 0}; // +Y
        offsets[3] = {1, 2}; // -Y
        offsets[4] = {1, 1}; // +Z
        offsets[5] = {1, 3}; // -Z
    }

    // Copy each face
    for (int face = 0; face < 6; ++face) {
        uint32_t srcX = offsets[face].x * faceSize;
        uint32_t srcY = offsets[face].y * faceSize;

        for (uint32_t y = 0; y < faceSize; ++y) {
            const uint8_t* srcRow = src + (srcY + y) * srcPitch + srcX * bpp;
            uint8_t* dstRow = faces[face].data() + y * faceSize * bpp;
            std::memcpy(dstRow, srcRow, faceSize * bpp);
        }
    }

    return CreateCubeTextureFromFaces(
        faces[0].data(), faces[1].data(),
        faces[2].data(), faces[3].data(),
        faces[4].data(), faces[5].data(),
        faceSize, format
    );
}

bgfx::TextureHandle CubeTextureUtils::CreateCubeTextureFromMemory(
    const void* data,
    uint32_t faceSize,
    uint32_t size,
    bgfx::TextureFormat::Enum format,
    bool hasMips) {

    uint32_t totalSize = faceSize * 6;
    const bgfx::Memory* mem = bgfx::copy(data, totalSize);

    return bgfx::createTextureCube(
        uint16_t(size),
        hasMips,
        1,
        format,
        BGFX_TEXTURE_NONE,
        mem
    );
}

void CubeTextureUtils::UpdateCubeTextureFace(
    bgfx::TextureHandle handle,
    uint8_t face,
    uint8_t mip,
    uint16_t x, uint16_t y,
    uint16_t width, uint16_t height,
    const void* data,
    uint32_t pitch) {

    const bgfx::Memory* mem = bgfx::copy(data, pitch * height);
    bgfx::updateTextureCube(handle, 0, face, mip, x, y, width, height, mem, pitch);
}

void CubeTextureUtils::GenerateCubeTextureMips(
    bgfx::TextureHandle handle,
    uint32_t size,
    bgfx::TextureFormat::Enum format) {

    // Note: bgfx doesn't support automatic mipmap generation for cube textures
    // This would need to be done manually by reading back and downsampling
    (void)handle;
    (void)size;
    (void)format;
}

uint8_t CubeTextureUtils::D3DFaceToBgfx(D3DCUBEMAP_FACES face) {
    // D3D8 and bgfx use the same face ordering
    return static_cast<uint8_t>(face);
}

D3DCUBEMAP_FACES CubeTextureUtils::BgfxFaceToD3D(uint8_t face) {
    return static_cast<D3DCUBEMAP_FACES>(face);
}

void CubeTextureUtils::GetFaceDirections(
    D3DCUBEMAP_FACES face,
    float& dirX, float& dirY, float& dirZ,
    float& upX, float& upY, float& upZ) {

    switch (face) {
        case D3DCUBEMAP_FACE_POSITIVE_X:
            dirX = 1; dirY = 0; dirZ = 0;
            upX = 0; upY = 1; upZ = 0;
            break;
        case D3DCUBEMAP_FACE_NEGATIVE_X:
            dirX = -1; dirY = 0; dirZ = 0;
            upX = 0; upY = 1; upZ = 0;
            break;
        case D3DCUBEMAP_FACE_POSITIVE_Y:
            dirX = 0; dirY = 1; dirZ = 0;
            upX = 0; upY = 0; upZ = -1;
            break;
        case D3DCUBEMAP_FACE_NEGATIVE_Y:
            dirX = 0; dirY = -1; dirZ = 0;
            upX = 0; upY = 0; upZ = 1;
            break;
        case D3DCUBEMAP_FACE_POSITIVE_Z:
            dirX = 0; dirY = 0; dirZ = 1;
            upX = 0; upY = 1; upZ = 0;
            break;
        case D3DCUBEMAP_FACE_NEGATIVE_Z:
            dirX = 0; dirY = 0; dirZ = -1;
            upX = 0; upY = 1; upZ = 0;
            break;
        default:
            dirX = 0; dirY = 0; dirZ = 1;
            upX = 0; upY = 1; upZ = 0;
            break;
    }
}

// =============================================================================
// CubeTextureManager Implementation
// =============================================================================

CubeTextureManager::CubeTextureManager() {
    for (int i = 0; i < 8; ++i) {
        m_stages[i].texture = BGFX_INVALID_HANDLE;
        m_stages[i].isCube = false;
    }
}

CubeTextureManager::~CubeTextureManager() {
    for (auto& entry : m_textures) {
        if (bgfx::isValid(entry.handle)) {
            bgfx::destroy(entry.handle);
        }
    }
}

bgfx::TextureHandle CubeTextureManager::CreateTexture(const CubeTextureDesc& desc) {
    bgfx::TextureFormat::Enum format = TextureUtils::ConvertFormat(desc.format);
    uint64_t flags = BGFX_TEXTURE_NONE;

    if (desc.renderTarget) {
        flags |= BGFX_TEXTURE_RT;
    }

    bgfx::TextureHandle handle = CubeTextureUtils::CreateCubeTexture(
        desc.size,
        desc.levels != 1,
        1,
        format,
        flags
    );

    if (bgfx::isValid(handle)) {
        CubeTextureEntry entry;
        entry.handle = handle;
        entry.desc = desc;
        entry.locked = false;
        m_textures.push_back(entry);
    }

    return handle;
}

void CubeTextureManager::DestroyTexture(bgfx::TextureHandle handle) {
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

void* CubeTextureManager::LockFace(
    bgfx::TextureHandle handle,
    D3DCUBEMAP_FACES face,
    uint32_t level,
    uint32_t& pitch) {

    for (auto& entry : m_textures) {
        if (entry.handle.idx == handle.idx) {
            if (entry.locked) {
                return nullptr;
            }

            // Calculate mip size
            uint32_t mipSize = entry.desc.size >> level;
            if (mipSize < 1) mipSize = 1;

            // Calculate pitch based on format
            uint32_t bpp = 4;
            switch (entry.desc.format) {
                case D3DFMT_A8R8G8B8:
                case D3DFMT_X8R8G8B8:
                    bpp = 4;
                    break;
                case D3DFMT_R5G6B5:
                case D3DFMT_A1R5G5B5:
                    bpp = 2;
                    break;
                default:
                    bpp = 4;
                    break;
            }

            pitch = mipSize * bpp;
            entry.lockBuffer.resize(mipSize * pitch);
            entry.locked = true;
            entry.lockedFace = face;
            entry.lockedLevel = level;

            return entry.lockBuffer.data();
        }
    }

    return nullptr;
}

void CubeTextureManager::UnlockFace(
    bgfx::TextureHandle handle,
    D3DCUBEMAP_FACES face,
    uint32_t level) {

    for (auto& entry : m_textures) {
        if (entry.handle.idx == handle.idx && entry.locked) {
            if (entry.lockedFace == face && entry.lockedLevel == level) {
                // Upload data to texture
                uint32_t mipSize = entry.desc.size >> level;
                if (mipSize < 1) mipSize = 1;

                uint32_t bpp = 4;
                uint32_t pitch = mipSize * bpp;

                CubeTextureUtils::UpdateCubeTextureFace(
                    entry.handle,
                    CubeTextureUtils::D3DFaceToBgfx(face),
                    uint8_t(level),
                    0, 0, uint16_t(mipSize), uint16_t(mipSize),
                    entry.lockBuffer.data(),
                    pitch
                );

                entry.locked = false;
                entry.lockBuffer.clear();
            }
            return;
        }
    }
}

void CubeTextureManager::SetCubeTexture(uint32_t stage, bgfx::TextureHandle handle) {
    if (stage < 8) {
        m_stages[stage].texture = handle;
        m_stages[stage].isCube = true;
    }
}

bgfx::TextureHandle CubeTextureManager::GetCubeTexture(uint32_t stage) const {
    if (stage < 8) {
        return m_stages[stage].texture;
    }
    return BGFX_INVALID_HANDLE;
}

bool CubeTextureManager::IsCubeTexture(uint32_t stage) const {
    if (stage < 8) {
        return m_stages[stage].isCube;
    }
    return false;
}

// =============================================================================
// EnvironmentMapGenerator Implementation
// =============================================================================

EnvironmentMapGenerator::EnvironmentMapGenerator()
    : m_resolution(256)
    , m_frameBuffer(BGFX_INVALID_HANDLE)
    , m_depthBuffer(BGFX_INVALID_HANDLE)
    , m_initialized(false) {
}

EnvironmentMapGenerator::~EnvironmentMapGenerator() {
    Shutdown();
}

void EnvironmentMapGenerator::Initialize(uint32_t resolution) {
    if (m_initialized) {
        Shutdown();
    }

    m_resolution = resolution;

    // Create depth buffer for rendering
    m_depthBuffer = bgfx::createTexture2D(
        uint16_t(resolution), uint16_t(resolution),
        false, 1, bgfx::TextureFormat::D24S8,
        BGFX_TEXTURE_RT
    );

    m_initialized = true;
}

void EnvironmentMapGenerator::Shutdown() {
    if (!m_initialized) {
        return;
    }

    if (bgfx::isValid(m_frameBuffer)) {
        bgfx::destroy(m_frameBuffer);
        m_frameBuffer = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(m_depthBuffer)) {
        bgfx::destroy(m_depthBuffer);
        m_depthBuffer = BGFX_INVALID_HANDLE;
    }

    m_initialized = false;
}

bgfx::TextureHandle EnvironmentMapGenerator::GenerateEnvironmentMap(
    float posX, float posY, float posZ,
    RenderFaceCallback callback,
    void* userData,
    bgfx::TextureFormat::Enum format) {

    if (!m_initialized || !callback) {
        return BGFX_INVALID_HANDLE;
    }

    // Create cube texture
    bgfx::TextureHandle cubeTexture = bgfx::createTextureCube(
        uint16_t(m_resolution),
        false, 1, format,
        BGFX_TEXTURE_RT
    );

    if (!bgfx::isValid(cubeTexture)) {
        return BGFX_INVALID_HANDLE;
    }

    // Get projection matrix
    D3DMATRIX proj = GetFaceProjectionMatrix(0.1f, 1000.0f);

    // Render each face
    for (int face = 0; face < 6; ++face) {
        D3DCUBEMAP_FACES d3dFace = static_cast<D3DCUBEMAP_FACES>(face);

        // Create frame buffer for this face
        bgfx::Attachment attachments[2];
        attachments[0].init(cubeTexture, bgfx::Access::Write, uint16_t(face));
        attachments[1].init(m_depthBuffer);

        bgfx::FrameBufferHandle fb = bgfx::createFrameBuffer(2, attachments, true);

        if (bgfx::isValid(fb)) {
            // Get view matrix for this face
            D3DMATRIX view = GetFaceViewMatrix(d3dFace, posX, posY, posZ);

            // Set up view
            bgfx::setViewFrameBuffer(0, fb);
            bgfx::setViewRect(0, 0, 0, uint16_t(m_resolution), uint16_t(m_resolution));
            bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x000000FF, 1.0f, 0);

            // Invoke callback to render scene
            callback(d3dFace, view, proj, userData);

            // Submit frame
            bgfx::frame();

            bgfx::destroy(fb);
        }
    }

    return cubeTexture;
}

D3DMATRIX EnvironmentMapGenerator::GetFaceViewMatrix(
    D3DCUBEMAP_FACES face,
    float posX, float posY, float posZ) {

    float dirX, dirY, dirZ;
    float upX, upY, upZ;
    CubeTextureUtils::GetFaceDirections(face, dirX, dirY, dirZ, upX, upY, upZ);

    // Build look-at matrix
    // Right vector = up x dir
    float rx = upY * dirZ - upZ * dirY;
    float ry = upZ * dirX - upX * dirZ;
    float rz = upX * dirY - upY * dirX;

    D3DMATRIX view;
    view._11 = rx;   view._12 = upX;  view._13 = dirX; view._14 = 0;
    view._21 = ry;   view._22 = upY;  view._23 = dirY; view._24 = 0;
    view._31 = rz;   view._32 = upZ;  view._33 = dirZ; view._34 = 0;
    view._41 = -(rx * posX + ry * posY + rz * posZ);
    view._42 = -(upX * posX + upY * posY + upZ * posZ);
    view._43 = -(dirX * posX + dirY * posY + dirZ * posZ);
    view._44 = 1;

    return view;
}

D3DMATRIX EnvironmentMapGenerator::GetFaceProjectionMatrix(float nearZ, float farZ) {
    // 90 degree FOV, aspect 1:1
    float f = 1.0f; // tan(45 degrees) = 1

    D3DMATRIX proj;
    proj._11 = f;    proj._12 = 0;    proj._13 = 0;                            proj._14 = 0;
    proj._21 = 0;    proj._22 = f;    proj._23 = 0;                            proj._24 = 0;
    proj._31 = 0;    proj._32 = 0;    proj._33 = farZ / (farZ - nearZ);        proj._34 = 1;
    proj._41 = 0;    proj._42 = 0;    proj._43 = -nearZ * farZ / (farZ - nearZ); proj._44 = 0;

    return proj;
}

// =============================================================================
// SphericalHarmonics Implementation
// =============================================================================

SHCoefficients SphericalHarmonics::ProjectCubeMap(
    bgfx::TextureHandle cubeMap,
    uint32_t size) {

    SHCoefficients sh = {};

    // Note: This is a simplified implementation
    // Full implementation would read back cube map data and integrate

    (void)cubeMap;
    (void)size;

    // Return identity (white ambient)
    sh.coefficients[0][0] = 1.0f;
    sh.coefficients[0][1] = 1.0f;
    sh.coefficients[0][2] = 1.0f;

    return sh;
}

void SphericalHarmonics::Evaluate(
    const SHCoefficients& sh,
    float dirX, float dirY, float dirZ,
    float& r, float& g, float& b) {

    // SH basis functions
    const float c1 = 0.429043f;
    const float c2 = 0.511664f;
    const float c3 = 0.743125f;
    const float c4 = 0.886227f;
    const float c5 = 0.247708f;

    // Evaluate for each color channel
    for (int c = 0; c < 3; ++c) {
        float result =
            c4 * sh.coefficients[0][c] +
            2.0f * c2 * sh.coefficients[1][c] * dirY +
            2.0f * c2 * sh.coefficients[2][c] * dirZ +
            2.0f * c2 * sh.coefficients[3][c] * dirX +
            2.0f * c1 * sh.coefficients[4][c] * dirX * dirY +
            2.0f * c1 * sh.coefficients[5][c] * dirY * dirZ +
            c3 * sh.coefficients[6][c] * (3.0f * dirZ * dirZ - 1.0f) +
            2.0f * c1 * sh.coefficients[7][c] * dirX * dirZ +
            c1 * sh.coefficients[8][c] * (dirX * dirX - dirY * dirY);

        if (c == 0) r = result;
        else if (c == 1) g = result;
        else b = result;
    }
}

bgfx::TextureHandle SphericalHarmonics::CreateIrradianceMap(
    const SHCoefficients& sh,
    uint32_t size) {

    // Create a cube map with evaluated SH
    std::vector<uint8_t> faceData[6];
    uint32_t faceBytes = size * size * 4;

    for (int face = 0; face < 6; ++face) {
        faceData[face].resize(faceBytes);
        uint8_t* pixels = faceData[face].data();

        for (uint32_t y = 0; y < size; ++y) {
            for (uint32_t x = 0; x < size; ++x) {
                // Calculate direction for this texel
                float u = (x + 0.5f) / size * 2.0f - 1.0f;
                float v = (y + 0.5f) / size * 2.0f - 1.0f;

                float dirX, dirY, dirZ;
                switch (face) {
                    case 0: dirX = 1; dirY = -v; dirZ = -u; break;  // +X
                    case 1: dirX = -1; dirY = -v; dirZ = u; break;  // -X
                    case 2: dirX = u; dirY = 1; dirZ = v; break;    // +Y
                    case 3: dirX = u; dirY = -1; dirZ = -v; break;  // -Y
                    case 4: dirX = u; dirY = -v; dirZ = 1; break;   // +Z
                    case 5: dirX = -u; dirY = -v; dirZ = -1; break; // -Z
                    default: dirX = 0; dirY = 0; dirZ = 1; break;
                }

                // Normalize
                float len = std::sqrt(dirX*dirX + dirY*dirY + dirZ*dirZ);
                dirX /= len; dirY /= len; dirZ /= len;

                // Evaluate SH
                float r, g, b;
                Evaluate(sh, dirX, dirY, dirZ, r, g, b);

                // Clamp and convert to 8-bit
                uint32_t idx = (y * size + x) * 4;
                pixels[idx + 0] = uint8_t(std::min(std::max(r * 255.0f, 0.0f), 255.0f));
                pixels[idx + 1] = uint8_t(std::min(std::max(g * 255.0f, 0.0f), 255.0f));
                pixels[idx + 2] = uint8_t(std::min(std::max(b * 255.0f, 0.0f), 255.0f));
                pixels[idx + 3] = 255;
            }
        }
    }

    return CubeTextureUtils::CreateCubeTextureFromFaces(
        faceData[0].data(), faceData[1].data(),
        faceData[2].data(), faceData[3].data(),
        faceData[4].data(), faceData[5].data(),
        size, bgfx::TextureFormat::RGBA8
    );
}

// =============================================================================
// CubeMapTexGen Implementation
// =============================================================================

void CubeMapTexGen::GenerateReflectionCoords(
    const float* positions,
    const float* normals,
    const D3DMATRIX& world,
    const D3DVECTOR& eyePos,
    float* texCoords,
    uint32_t numVertices) {

    for (uint32_t i = 0; i < numVertices; ++i) {
        // Get position
        float px = positions[i * 3 + 0];
        float py = positions[i * 3 + 1];
        float pz = positions[i * 3 + 2];

        // Transform position to world space
        float wx = px * world._11 + py * world._21 + pz * world._31 + world._41;
        float wy = px * world._12 + py * world._22 + pz * world._32 + world._42;
        float wz = px * world._13 + py * world._23 + pz * world._33 + world._43;

        // Get normal
        float nx = normals[i * 3 + 0];
        float ny = normals[i * 3 + 1];
        float nz = normals[i * 3 + 2];

        // Transform normal to world space (ignore translation)
        float wnx = nx * world._11 + ny * world._21 + nz * world._31;
        float wny = nx * world._12 + ny * world._22 + nz * world._32;
        float wnz = nx * world._13 + ny * world._23 + nz * world._33;

        // Normalize
        float nlen = std::sqrt(wnx*wnx + wny*wny + wnz*wnz);
        if (nlen > 0.0001f) {
            wnx /= nlen;
            wny /= nlen;
            wnz /= nlen;
        }

        // View direction
        float vx = wx - eyePos.x;
        float vy = wy - eyePos.y;
        float vz = wz - eyePos.z;

        // Normalize view
        float vlen = std::sqrt(vx*vx + vy*vy + vz*vz);
        if (vlen > 0.0001f) {
            vx /= vlen;
            vy /= vlen;
            vz /= vlen;
        }

        // Reflection: R = I - 2(N.I)N
        float dot = vx * wnx + vy * wny + vz * wnz;
        texCoords[i * 3 + 0] = vx - 2.0f * dot * wnx;
        texCoords[i * 3 + 1] = vy - 2.0f * dot * wny;
        texCoords[i * 3 + 2] = vz - 2.0f * dot * wnz;
    }
}

void CubeMapTexGen::GenerateNormalCoords(
    const float* normals,
    const D3DMATRIX& world,
    float* texCoords,
    uint32_t numVertices) {

    for (uint32_t i = 0; i < numVertices; ++i) {
        float nx = normals[i * 3 + 0];
        float ny = normals[i * 3 + 1];
        float nz = normals[i * 3 + 2];

        // Transform to world space
        texCoords[i * 3 + 0] = nx * world._11 + ny * world._21 + nz * world._31;
        texCoords[i * 3 + 1] = nx * world._12 + ny * world._22 + nz * world._32;
        texCoords[i * 3 + 2] = nx * world._13 + ny * world._23 + nz * world._33;
    }
}

void CubeMapTexGen::GeneratePositionCoords(
    const float* positions,
    const D3DMATRIX& world,
    float* texCoords,
    uint32_t numVertices) {

    for (uint32_t i = 0; i < numVertices; ++i) {
        float px = positions[i * 3 + 0];
        float py = positions[i * 3 + 1];
        float pz = positions[i * 3 + 2];

        // Transform to world space
        texCoords[i * 3 + 0] = px * world._11 + py * world._21 + pz * world._31 + world._41;
        texCoords[i * 3 + 1] = px * world._12 + py * world._22 + pz * world._32 + world._42;
        texCoords[i * 3 + 2] = px * world._13 + py * world._23 + pz * world._33 + world._43;
    }
}

void CubeMapTexGen::TransformCubeMapDirection(
    float& u, float& v, float& w,
    const D3DMATRIX& textureMatrix) {

    float tu = u * textureMatrix._11 + v * textureMatrix._21 + w * textureMatrix._31 + textureMatrix._41;
    float tv = u * textureMatrix._12 + v * textureMatrix._22 + w * textureMatrix._32 + textureMatrix._42;
    float tw = u * textureMatrix._13 + v * textureMatrix._23 + w * textureMatrix._33 + textureMatrix._43;

    u = tu;
    v = tv;
    w = tw;
}

} // namespace dx8bgfx
