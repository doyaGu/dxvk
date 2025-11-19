#include "dx8bgfx/dx8_shader_key.h"

namespace dx8bgfx {

// =============================================================================
// Hash Implementation
// =============================================================================

// FNV-1a hash constants
constexpr uint64_t FNV_OFFSET_BASIS = 14695981039346656037ULL;
constexpr uint64_t FNV_PRIME = 1099511628211ULL;

static uint64_t HashBytes(const void* data, size_t size) {
    uint64_t hash = FNV_OFFSET_BASIS;
    const uint8_t* bytes = static_cast<const uint8_t*>(data);

    for (size_t i = 0; i < size; i++) {
        hash ^= bytes[i];
        hash *= FNV_PRIME;
    }

    return hash;
}

uint64_t VertexShaderKey::GetHash() const {
    return HashBytes(Data.data, sizeof(Data.data));
}

uint64_t FragmentShaderKey::GetHash() const {
    uint64_t hash = FNV_OFFSET_BASIS;

    // Hash stages
    for (int i = 0; i < MaxTextureStages; i++) {
        hash ^= HashBytes(Data.Stages[i].data, sizeof(Data.Stages[i].data));
        hash *= FNV_PRIME;
    }

    // Hash flags
    hash ^= Data.flags;
    hash *= FNV_PRIME;

    return hash;
}

} // namespace dx8bgfx
