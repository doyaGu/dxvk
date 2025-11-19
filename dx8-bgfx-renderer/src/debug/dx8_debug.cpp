#include "dx8bgfx/dx8_debug.h"
#include <cstdio>
#include <cstring>
#include <chrono>
#include <sstream>

namespace dx8bgfx {

// =============================================================================
// Debug Utils Implementation
// =============================================================================

std::string DebugUtils::RenderStateToString(D3DRENDERSTATETYPE state) {
    switch (state) {
        case D3DRS_ZENABLE: return "ZENABLE";
        case D3DRS_FILLMODE: return "FILLMODE";
        case D3DRS_SHADEMODE: return "SHADEMODE";
        case D3DRS_ZWRITEENABLE: return "ZWRITEENABLE";
        case D3DRS_ALPHATESTENABLE: return "ALPHATESTENABLE";
        case D3DRS_SRCBLEND: return "SRCBLEND";
        case D3DRS_DESTBLEND: return "DESTBLEND";
        case D3DRS_CULLMODE: return "CULLMODE";
        case D3DRS_ZFUNC: return "ZFUNC";
        case D3DRS_ALPHAREF: return "ALPHAREF";
        case D3DRS_ALPHAFUNC: return "ALPHAFUNC";
        case D3DRS_DITHERENABLE: return "DITHERENABLE";
        case D3DRS_ALPHABLENDENABLE: return "ALPHABLENDENABLE";
        case D3DRS_FOGENABLE: return "FOGENABLE";
        case D3DRS_SPECULARENABLE: return "SPECULARENABLE";
        case D3DRS_FOGCOLOR: return "FOGCOLOR";
        case D3DRS_FOGTABLEMODE: return "FOGTABLEMODE";
        case D3DRS_FOGSTART: return "FOGSTART";
        case D3DRS_FOGEND: return "FOGEND";
        case D3DRS_FOGDENSITY: return "FOGDENSITY";
        case D3DRS_LIGHTING: return "LIGHTING";
        case D3DRS_AMBIENT: return "AMBIENT";
        case D3DRS_COLORVERTEX: return "COLORVERTEX";
        case D3DRS_NORMALIZENORMALS: return "NORMALIZENORMALS";
        case D3DRS_STENCILENABLE: return "STENCILENABLE";
        default: return "UNKNOWN";
    }
}

std::string DebugUtils::PrimitiveTypeToString(D3DPRIMITIVETYPE type) {
    switch (type) {
        case D3DPT_POINTLIST: return "POINTLIST";
        case D3DPT_LINELIST: return "LINELIST";
        case D3DPT_LINESTRIP: return "LINESTRIP";
        case D3DPT_TRIANGLELIST: return "TRIANGLELIST";
        case D3DPT_TRIANGLESTRIP: return "TRIANGLESTRIP";
        case D3DPT_TRIANGLEFAN: return "TRIANGLEFAN";
        default: return "UNKNOWN";
    }
}

std::string DebugUtils::BlendModeToString(D3DBLEND blend) {
    switch (blend) {
        case D3DBLEND_ZERO: return "ZERO";
        case D3DBLEND_ONE: return "ONE";
        case D3DBLEND_SRCCOLOR: return "SRCCOLOR";
        case D3DBLEND_INVSRCCOLOR: return "INVSRCCOLOR";
        case D3DBLEND_SRCALPHA: return "SRCALPHA";
        case D3DBLEND_INVSRCALPHA: return "INVSRCALPHA";
        case D3DBLEND_DESTALPHA: return "DESTALPHA";
        case D3DBLEND_INVDESTALPHA: return "INVDESTALPHA";
        case D3DBLEND_DESTCOLOR: return "DESTCOLOR";
        case D3DBLEND_INVDESTCOLOR: return "INVDESTCOLOR";
        case D3DBLEND_SRCALPHASAT: return "SRCALPHASAT";
        default: return "UNKNOWN";
    }
}

std::string DebugUtils::FogModeToString(D3DFOGMODE mode) {
    switch (mode) {
        case D3DFOG_NONE: return "NONE";
        case D3DFOG_EXP: return "EXP";
        case D3DFOG_EXP2: return "EXP2";
        case D3DFOG_LINEAR: return "LINEAR";
        default: return "UNKNOWN";
    }
}

std::string DebugUtils::TextureOpToString(DWORD op) {
    switch (op) {
        case D3DTOP_DISABLE: return "DISABLE";
        case D3DTOP_SELECTARG1: return "SELECTARG1";
        case D3DTOP_SELECTARG2: return "SELECTARG2";
        case D3DTOP_MODULATE: return "MODULATE";
        case D3DTOP_MODULATE2X: return "MODULATE2X";
        case D3DTOP_MODULATE4X: return "MODULATE4X";
        case D3DTOP_ADD: return "ADD";
        case D3DTOP_ADDSIGNED: return "ADDSIGNED";
        case D3DTOP_SUBTRACT: return "SUBTRACT";
        case D3DTOP_BLENDDIFFUSEALPHA: return "BLENDDIFFUSEALPHA";
        case D3DTOP_BLENDTEXTUREALPHA: return "BLENDTEXTUREALPHA";
        case D3DTOP_DOTPRODUCT3: return "DOTPRODUCT3";
        default: return "UNKNOWN";
    }
}

std::string DebugUtils::FVFToString(DWORD fvf) {
    std::ostringstream ss;

    DWORD posType = fvf & D3DFVF_POSITION_MASK;
    switch (posType) {
        case D3DFVF_XYZ: ss << "XYZ"; break;
        case D3DFVF_XYZRHW: ss << "XYZRHW"; break;
        case D3DFVF_XYZB1: ss << "XYZB1"; break;
        case D3DFVF_XYZB2: ss << "XYZB2"; break;
        case D3DFVF_XYZB3: ss << "XYZB3"; break;
        case D3DFVF_XYZB4: ss << "XYZB4"; break;
        case D3DFVF_XYZB5: ss << "XYZB5"; break;
        default: ss << "UNK_POS"; break;
    }

    if (fvf & D3DFVF_NORMAL) ss << " | NORMAL";
    if (fvf & D3DFVF_PSIZE) ss << " | PSIZE";
    if (fvf & D3DFVF_DIFFUSE) ss << " | DIFFUSE";
    if (fvf & D3DFVF_SPECULAR) ss << " | SPECULAR";

    DWORD texCount = (fvf & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
    if (texCount > 0) {
        ss << " | TEX" << texCount;
    }

    return ss.str();
}

std::string DebugUtils::MatrixToString(const D3DMATRIX& m) {
    char buf[512];
    snprintf(buf, sizeof(buf),
        "[%.3f, %.3f, %.3f, %.3f]\n"
        "[%.3f, %.3f, %.3f, %.3f]\n"
        "[%.3f, %.3f, %.3f, %.3f]\n"
        "[%.3f, %.3f, %.3f, %.3f]",
        m._11, m._12, m._13, m._14,
        m._21, m._22, m._23, m._24,
        m._31, m._32, m._33, m._34,
        m._41, m._42, m._43, m._44);
    return buf;
}

void DebugUtils::PrintMatrix(const char* name, const D3DMATRIX& m) {
    printf("%s:\n%s\n", name, MatrixToString(m).c_str());
}

std::string DebugUtils::ColorToString(D3DCOLOR color) {
    char buf[32];
    snprintf(buf, sizeof(buf), "0x%08X (A=%d R=%d G=%d B=%d)",
        color,
        (color >> 24) & 0xFF,
        (color >> 16) & 0xFF,
        (color >> 8) & 0xFF,
        color & 0xFF);
    return buf;
}

std::string DebugUtils::ColorValueToString(const D3DCOLORVALUE& cv) {
    char buf[64];
    snprintf(buf, sizeof(buf), "(%.3f, %.3f, %.3f, %.3f)", cv.r, cv.g, cv.b, cv.a);
    return buf;
}

std::string DebugUtils::LightTypeToString(D3DLIGHTTYPE type) {
    switch (type) {
        case D3DLIGHT_POINT: return "POINT";
        case D3DLIGHT_SPOT: return "SPOT";
        case D3DLIGHT_DIRECTIONAL: return "DIRECTIONAL";
        default: return "UNKNOWN";
    }
}

std::string DebugUtils::LightToString(const D3DLIGHT8& light) {
    std::ostringstream ss;
    ss << "Type: " << LightTypeToString(light.Type) << "\n";
    ss << "Diffuse: " << ColorValueToString(light.Diffuse) << "\n";
    ss << "Specular: " << ColorValueToString(light.Specular) << "\n";
    ss << "Ambient: " << ColorValueToString(light.Ambient) << "\n";
    ss << "Position: (" << light.Position.x << ", " << light.Position.y << ", " << light.Position.z << ")\n";
    ss << "Direction: (" << light.Direction.x << ", " << light.Direction.y << ", " << light.Direction.z << ")\n";
    ss << "Range: " << light.Range << "\n";
    ss << "Attenuation: " << light.Attenuation0 << ", " << light.Attenuation1 << ", " << light.Attenuation2;
    return ss.str();
}

// =============================================================================
// Profiler Implementation
// =============================================================================

Profiler::Profiler()
    : m_enabled(false)
    , m_frameStartTime(0)
    , m_maxHistory(300)
{
    Reset();
}

Profiler::~Profiler() {
}

void Profiler::BeginFrame() {
    if (!m_enabled) return;

    auto now = std::chrono::high_resolution_clock::now();
    m_frameStartTime = std::chrono::duration_cast<std::chrono::microseconds>(
        now.time_since_epoch()
    ).count();

    // Reset per-frame counters
    m_stats.drawCalls = 0;
    m_stats.primitives = 0;
    m_stats.vertices = 0;
    m_stats.indices = 0;
    m_stats.stateChanges = 0;
    m_stats.textureChanges = 0;
    m_stats.shaderChanges = 0;
}

void Profiler::EndFrame() {
    if (!m_enabled) return;

    auto now = std::chrono::high_resolution_clock::now();
    uint64_t endTime = std::chrono::duration_cast<std::chrono::microseconds>(
        now.time_since_epoch()
    ).count();

    m_stats.frameTime = endTime - m_frameStartTime;
    m_stats.cpuTime = m_stats.frameTime; // Simplified
    m_stats.fps = m_stats.frameTime > 0 ? 1000000.0f / m_stats.frameTime : 0.0f;

    // Store in history
    m_history.push_back(m_stats);
    if (m_history.size() > m_maxHistory) {
        m_history.erase(m_history.begin());
    }
}

void Profiler::AddDrawCall(uint32_t primitives, uint32_t vertices, uint32_t indices) {
    if (!m_enabled) return;
    m_stats.drawCalls++;
    m_stats.primitives += primitives;
    m_stats.vertices += vertices;
    m_stats.indices += indices;
}

void Profiler::AddStateChange() {
    if (!m_enabled) return;
    m_stats.stateChanges++;
}

void Profiler::AddTextureChange() {
    if (!m_enabled) return;
    m_stats.textureChanges++;
}

void Profiler::AddShaderChange() {
    if (!m_enabled) return;
    m_stats.shaderChanges++;
}

ProfilerStats Profiler::GetAverageStats(uint32_t numFrames) const {
    ProfilerStats avg = {};
    if (m_history.empty()) return avg;

    uint32_t count = std::min(numFrames, static_cast<uint32_t>(m_history.size()));
    size_t start = m_history.size() - count;

    for (size_t i = start; i < m_history.size(); i++) {
        avg.frameTime += m_history[i].frameTime;
        avg.drawCalls += m_history[i].drawCalls;
        avg.primitives += m_history[i].primitives;
        avg.vertices += m_history[i].vertices;
        avg.indices += m_history[i].indices;
        avg.stateChanges += m_history[i].stateChanges;
        avg.textureChanges += m_history[i].textureChanges;
        avg.shaderChanges += m_history[i].shaderChanges;
    }

    avg.frameTime /= count;
    avg.drawCalls /= count;
    avg.primitives /= count;
    avg.vertices /= count;
    avg.indices /= count;
    avg.stateChanges /= count;
    avg.textureChanges /= count;
    avg.shaderChanges /= count;
    avg.fps = avg.frameTime > 0 ? 1000000.0f / avg.frameTime : 0.0f;
    avg.avgFrameTime = static_cast<float>(avg.frameTime) / 1000.0f;

    return avg;
}

void Profiler::Reset() {
    std::memset(&m_stats, 0, sizeof(m_stats));
    m_history.clear();
}

void Profiler::PrintStats() {
    ProfilerStats avg = GetAverageStats(60);
    printf("=== Profiler Stats (60 frame avg) ===\n");
    printf("FPS: %.1f (%.2f ms)\n", avg.fps, avg.avgFrameTime);
    printf("Draw calls: %u\n", avg.drawCalls);
    printf("Primitives: %u\n", avg.primitives);
    printf("Vertices: %u\n", avg.vertices);
    printf("State changes: %u\n", avg.stateChanges);
    printf("Texture changes: %u\n", avg.textureChanges);
    printf("Shader changes: %u\n", avg.shaderChanges);
    printf("=====================================\n");
}

// =============================================================================
// Debug Draw Implementation
// =============================================================================

DebugDraw::DebugDraw()
    : m_initialized(false)
{
}

DebugDraw::~DebugDraw() {
    Shutdown();
}

void DebugDraw::Initialize() {
    if (m_initialized) return;

    m_layout.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
        .end();

    m_initialized = true;
}

void DebugDraw::Shutdown() {
    m_lines.clear();
    m_initialized = false;
}

void DebugDraw::Begin(const D3DMATRIX& viewProj) {
    m_lines.clear();
    m_viewProj = viewProj;
}

void DebugDraw::End() {
    if (m_lines.empty()) return;

    // Submit lines using transient buffers
    bgfx::TransientVertexBuffer tvb;
    if (bgfx::getAvailTransientVertexBuffer(static_cast<uint32_t>(m_lines.size()), m_layout)
        < m_lines.size()) {
        return;
    }

    bgfx::allocTransientVertexBuffer(&tvb, static_cast<uint32_t>(m_lines.size()), m_layout);
    std::memcpy(tvb.data, m_lines.data(), m_lines.size() * sizeof(DebugVertex));

    bgfx::setVertexBuffer(0, &tvb);
    // Note: Would need a simple shader program for debug lines
    // bgfx::submit(0, debugProgram);

    m_lines.clear();
}

void DebugDraw::DrawLine(const D3DVECTOR& start, const D3DVECTOR& end, uint32_t color) {
    DebugVertex v0 = {{start.x, start.y, start.z}, color};
    DebugVertex v1 = {{end.x, end.y, end.z}, color};
    m_lines.push_back(v0);
    m_lines.push_back(v1);
}

void DebugDraw::DrawWireBox(const D3DVECTOR& min, const D3DVECTOR& max, uint32_t color) {
    // Bottom face
    DrawLine({min.x, min.y, min.z}, {max.x, min.y, min.z}, color);
    DrawLine({max.x, min.y, min.z}, {max.x, min.y, max.z}, color);
    DrawLine({max.x, min.y, max.z}, {min.x, min.y, max.z}, color);
    DrawLine({min.x, min.y, max.z}, {min.x, min.y, min.z}, color);

    // Top face
    DrawLine({min.x, max.y, min.z}, {max.x, max.y, min.z}, color);
    DrawLine({max.x, max.y, min.z}, {max.x, max.y, max.z}, color);
    DrawLine({max.x, max.y, max.z}, {min.x, max.y, max.z}, color);
    DrawLine({min.x, max.y, max.z}, {min.x, max.y, min.z}, color);

    // Vertical edges
    DrawLine({min.x, min.y, min.z}, {min.x, max.y, min.z}, color);
    DrawLine({max.x, min.y, min.z}, {max.x, max.y, min.z}, color);
    DrawLine({max.x, min.y, max.z}, {max.x, max.y, max.z}, color);
    DrawLine({min.x, min.y, max.z}, {min.x, max.y, max.z}, color);
}

void DebugDraw::DrawAxes(const D3DMATRIX& transform, float size) {
    D3DVECTOR origin = {transform._41, transform._42, transform._43};
    D3DVECTOR xAxis = {
        origin.x + transform._11 * size,
        origin.y + transform._12 * size,
        origin.z + transform._13 * size
    };
    D3DVECTOR yAxis = {
        origin.x + transform._21 * size,
        origin.y + transform._22 * size,
        origin.z + transform._23 * size
    };
    D3DVECTOR zAxis = {
        origin.x + transform._31 * size,
        origin.y + transform._32 * size,
        origin.z + transform._33 * size
    };

    DrawLine(origin, xAxis, 0xFF0000FF); // Red = X
    DrawLine(origin, yAxis, 0xFF00FF00); // Green = Y
    DrawLine(origin, zAxis, 0xFFFF0000); // Blue = Z
}

void DebugDraw::DrawGrid(float size, uint32_t divisions, uint32_t color) {
    float step = size * 2.0f / divisions;
    float start = -size;

    for (uint32_t i = 0; i <= divisions; i++) {
        float pos = start + i * step;
        // Lines along X
        DrawLine({pos, 0, -size}, {pos, 0, size}, color);
        // Lines along Z
        DrawLine({-size, 0, pos}, {size, 0, pos}, color);
    }
}

// =============================================================================
// Validator Implementation
// =============================================================================

bool Validator::ValidateFVF(DWORD fvf, std::string& errorMsg) {
    DWORD posType = fvf & D3DFVF_POSITION_MASK;

    // Must have position
    if (posType == 0) {
        errorMsg = "FVF must have position";
        return false;
    }

    // Can't have both XYZ and XYZRHW
    if ((fvf & D3DFVF_XYZRHW) && (fvf & D3DFVF_NORMAL)) {
        errorMsg = "XYZRHW cannot be combined with NORMAL";
        return false;
    }

    // Texture count check
    DWORD texCount = (fvf & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
    if (texCount > 8) {
        errorMsg = "Too many texture coordinates (max 8)";
        return false;
    }

    return true;
}

bool Validator::ValidateDrawCall(
    D3DPRIMITIVETYPE primType,
    UINT startVertex,
    UINT primitiveCount,
    std::string& errorMsg
) {
    if (primitiveCount == 0) {
        errorMsg = "Primitive count cannot be zero";
        return false;
    }

    if (primType < D3DPT_POINTLIST || primType > D3DPT_TRIANGLEFAN) {
        errorMsg = "Invalid primitive type";
        return false;
    }

    (void)startVertex;
    return true;
}

} // namespace dx8bgfx
