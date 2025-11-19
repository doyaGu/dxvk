#pragma once

#include "dx8_types.h"
#include "dx8_constants.h"
#include "dx8_renderer.h"

#include <bgfx/bgfx.h>
#include <vector>

namespace dx8bgfx {

// =============================================================================
// Sprite Vertex
// =============================================================================

struct SpriteVertex {
    float x, y, z;      // Position (screen space or transformed)
    float rhw;          // Reciprocal of homogeneous W (for pre-transformed)
    uint32_t color;     // Diffuse color (BGRA)
    float u, v;         // Texture coordinates
};

// =============================================================================
// Sprite Definition
// =============================================================================

struct Sprite {
    float x, y;                 // Screen position
    float width, height;        // Size in pixels
    float rotation;             // Rotation in radians
    float originX, originY;     // Rotation origin (0-1 normalized)
    uint32_t color;             // Tint color (ARGB)
    float u0, v0, u1, v1;       // Texture coordinates
    float depth;                // Z depth for sorting
};

// =============================================================================
// Sprite Batch
// =============================================================================

class SpriteBatch {
public:
    SpriteBatch();
    ~SpriteBatch();

    // Initialize with maximum sprite count
    void Initialize(uint32_t maxSprites = 4096);

    // Shutdown and release resources
    void Shutdown();

    // Begin sprite batch
    void Begin(Renderer& renderer, bgfx::TextureHandle texture);

    // End sprite batch and flush to GPU
    void End();

    // Draw a sprite
    void Draw(const Sprite& sprite);

    // Convenience draw methods
    void Draw(float x, float y, float width, float height, uint32_t color = 0xFFFFFFFF);
    void Draw(float x, float y, float width, float height,
              float u0, float v0, float u1, float v1,
              uint32_t color = 0xFFFFFFFF);
    void DrawRotated(float x, float y, float width, float height,
                     float rotation, float originX = 0.5f, float originY = 0.5f,
                     uint32_t color = 0xFFFFFFFF);

    // Set sorting mode
    enum class SortMode {
        None,           // Draw in order
        BackToFront,    // Sort by depth (farthest first)
        FrontToBack,    // Sort by depth (nearest first)
        Texture         // Sort by texture (batching optimization)
    };
    void SetSortMode(SortMode mode) { m_sortMode = mode; }

    // Statistics
    uint32_t GetSpriteCount() const { return m_spriteCount; }
    uint32_t GetDrawCallCount() const { return m_drawCalls; }

private:
    // Flush current batch to GPU
    void Flush();

    // Sort sprites if needed
    void SortSprites();

    // Generate vertices for a sprite
    void GenerateVertices(const Sprite& sprite, SpriteVertex* out);

    // Setup render states for sprite rendering
    void SetupRenderStates();

    // Restore render states after sprite rendering
    void RestoreRenderStates();

private:
    Renderer* m_renderer;
    bgfx::TextureHandle m_texture;

    std::vector<Sprite> m_sprites;
    std::vector<SpriteVertex> m_vertices;
    std::vector<uint16_t> m_indices;

    bgfx::DynamicVertexBufferHandle m_vertexBuffer;
    bgfx::DynamicIndexBufferHandle m_indexBuffer;
    bgfx::VertexLayout m_layout;

    uint32_t m_maxSprites;
    uint32_t m_spriteCount;
    uint32_t m_drawCalls;
    bool m_inBatch;
    SortMode m_sortMode;

    // Saved render states
    DWORD m_savedAlphaBlend;
    DWORD m_savedSrcBlend;
    DWORD m_savedDstBlend;
    DWORD m_savedZEnable;
    DWORD m_savedZWriteEnable;
    DWORD m_savedCullMode;
};

// =============================================================================
// Text Renderer (Simple bitmap font support)
// =============================================================================

struct BitmapFont {
    bgfx::TextureHandle texture;
    uint32_t charWidth;
    uint32_t charHeight;
    uint32_t charsPerRow;
    uint32_t firstChar;
    uint32_t numChars;
    float* charWidths;  // Per-character widths (optional, for proportional fonts)
};

class TextRenderer {
public:
    TextRenderer();
    ~TextRenderer();

    // Initialize with a bitmap font
    void Initialize(SpriteBatch& spriteBatch);

    // Set current font
    void SetFont(const BitmapFont& font);

    // Draw text at position
    void DrawText(float x, float y, const char* text, uint32_t color = 0xFFFFFFFF, float scale = 1.0f);

    // Measure text dimensions
    void MeasureText(const char* text, float scale, float& outWidth, float& outHeight);

private:
    SpriteBatch* m_spriteBatch;
    BitmapFont m_font;
    bool m_hasFont;
};

} // namespace dx8bgfx
