#include "dx8bgfx/dx8_sprite_batch.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace dx8bgfx {

// =============================================================================
// Sprite Batch Implementation
// =============================================================================

SpriteBatch::SpriteBatch()
    : m_renderer(nullptr)
    , m_texture(BGFX_INVALID_HANDLE)
    , m_vertexBuffer(BGFX_INVALID_HANDLE)
    , m_indexBuffer(BGFX_INVALID_HANDLE)
    , m_maxSprites(0)
    , m_spriteCount(0)
    , m_drawCalls(0)
    , m_inBatch(false)
    , m_sortMode(SortMode::None)
{
}

SpriteBatch::~SpriteBatch() {
    Shutdown();
}

void SpriteBatch::Initialize(uint32_t maxSprites) {
    m_maxSprites = maxSprites;

    // Setup vertex layout for sprites (pre-transformed vertices)
    m_layout.begin()
        .add(bgfx::Attrib::Position, 4, bgfx::AttribType::Float)  // x, y, z, rhw
        .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .end();

    // Allocate buffers
    m_sprites.reserve(maxSprites);
    m_vertices.resize(maxSprites * 4);
    m_indices.resize(maxSprites * 6);

    // Generate index buffer (shared pattern for all sprites)
    for (uint32_t i = 0; i < maxSprites; i++) {
        uint16_t base = static_cast<uint16_t>(i * 4);
        m_indices[i * 6 + 0] = base + 0;
        m_indices[i * 6 + 1] = base + 1;
        m_indices[i * 6 + 2] = base + 2;
        m_indices[i * 6 + 3] = base + 0;
        m_indices[i * 6 + 4] = base + 2;
        m_indices[i * 6 + 5] = base + 3;
    }

    // Create dynamic buffers
    m_vertexBuffer = bgfx::createDynamicVertexBuffer(
        maxSprites * 4, m_layout, BGFX_BUFFER_ALLOW_RESIZE
    );

    m_indexBuffer = bgfx::createDynamicIndexBuffer(
        bgfx::copy(m_indices.data(), static_cast<uint32_t>(m_indices.size() * sizeof(uint16_t))),
        BGFX_BUFFER_ALLOW_RESIZE
    );
}

void SpriteBatch::Shutdown() {
    if (bgfx::isValid(m_vertexBuffer)) {
        bgfx::destroy(m_vertexBuffer);
        m_vertexBuffer = BGFX_INVALID_HANDLE;
    }
    if (bgfx::isValid(m_indexBuffer)) {
        bgfx::destroy(m_indexBuffer);
        m_indexBuffer = BGFX_INVALID_HANDLE;
    }

    m_sprites.clear();
    m_vertices.clear();
    m_indices.clear();
}

void SpriteBatch::Begin(Renderer& renderer, bgfx::TextureHandle texture) {
    if (m_inBatch) {
        End();
    }

    m_renderer = &renderer;
    m_texture = texture;
    m_sprites.clear();
    m_spriteCount = 0;
    m_drawCalls = 0;
    m_inBatch = true;

    // Save current render states
    renderer.GetRenderState(D3DRS_ALPHABLENDENABLE, &m_savedAlphaBlend);
    renderer.GetRenderState(D3DRS_SRCBLEND, &m_savedSrcBlend);
    renderer.GetRenderState(D3DRS_DESTBLEND, &m_savedDstBlend);
    renderer.GetRenderState(D3DRS_ZENABLE, &m_savedZEnable);
    renderer.GetRenderState(D3DRS_ZWRITEENABLE, &m_savedZWriteEnable);
    renderer.GetRenderState(D3DRS_CULLMODE, &m_savedCullMode);

    SetupRenderStates();
}

void SpriteBatch::End() {
    if (!m_inBatch) return;

    if (m_spriteCount > 0) {
        Flush();
    }

    RestoreRenderStates();
    m_inBatch = false;
    m_renderer = nullptr;
}

void SpriteBatch::Draw(const Sprite& sprite) {
    if (!m_inBatch) return;

    if (m_spriteCount >= m_maxSprites) {
        Flush();
    }

    m_sprites.push_back(sprite);
    m_spriteCount++;
}

void SpriteBatch::Draw(float x, float y, float width, float height, uint32_t color) {
    Draw(x, y, width, height, 0.0f, 0.0f, 1.0f, 1.0f, color);
}

void SpriteBatch::Draw(float x, float y, float width, float height,
                       float u0, float v0, float u1, float v1,
                       uint32_t color) {
    Sprite sprite;
    sprite.x = x;
    sprite.y = y;
    sprite.width = width;
    sprite.height = height;
    sprite.rotation = 0.0f;
    sprite.originX = 0.0f;
    sprite.originY = 0.0f;
    sprite.color = color;
    sprite.u0 = u0;
    sprite.v0 = v0;
    sprite.u1 = u1;
    sprite.v1 = v1;
    sprite.depth = 0.0f;

    Draw(sprite);
}

void SpriteBatch::DrawRotated(float x, float y, float width, float height,
                              float rotation, float originX, float originY,
                              uint32_t color) {
    Sprite sprite;
    sprite.x = x;
    sprite.y = y;
    sprite.width = width;
    sprite.height = height;
    sprite.rotation = rotation;
    sprite.originX = originX;
    sprite.originY = originY;
    sprite.color = color;
    sprite.u0 = 0.0f;
    sprite.v0 = 0.0f;
    sprite.u1 = 1.0f;
    sprite.v1 = 1.0f;
    sprite.depth = 0.0f;

    Draw(sprite);
}

void SpriteBatch::Flush() {
    if (m_spriteCount == 0) return;

    // Sort sprites if needed
    SortSprites();

    // Generate vertices
    for (uint32_t i = 0; i < m_spriteCount; i++) {
        GenerateVertices(m_sprites[i], &m_vertices[i * 4]);
    }

    // Update vertex buffer
    const bgfx::Memory* mem = bgfx::copy(
        m_vertices.data(),
        m_spriteCount * 4 * sizeof(SpriteVertex)
    );
    bgfx::update(m_vertexBuffer, 0, mem);

    // Set buffers and draw
    bgfx::setVertexBuffer(0, m_vertexBuffer, 0, m_spriteCount * 4);
    bgfx::setIndexBuffer(m_indexBuffer, 0, m_spriteCount * 6);

    // Set texture
    if (bgfx::isValid(m_texture)) {
        m_renderer->SetTextureHandle(0, m_texture.idx);
    }

    // Set FVF for pre-transformed vertices
    m_renderer->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1);

    // Submit draw call
    m_renderer->DrawIndexedPrimitive(
        D3DPT_TRIANGLELIST,
        0, 0,
        m_spriteCount * 4,
        0,
        m_spriteCount * 2
    );

    m_drawCalls++;
    m_sprites.clear();
    m_spriteCount = 0;
}

void SpriteBatch::SortSprites() {
    switch (m_sortMode) {
        case SortMode::BackToFront:
            std::sort(m_sprites.begin(), m_sprites.end(),
                [](const Sprite& a, const Sprite& b) {
                    return a.depth > b.depth;
                });
            break;
        case SortMode::FrontToBack:
            std::sort(m_sprites.begin(), m_sprites.end(),
                [](const Sprite& a, const Sprite& b) {
                    return a.depth < b.depth;
                });
            break;
        default:
            break;
    }
}

void SpriteBatch::GenerateVertices(const Sprite& sprite, SpriteVertex* out) {
    float x = sprite.x;
    float y = sprite.y;
    float w = sprite.width;
    float h = sprite.height;

    // Calculate corner positions
    float x0 = x;
    float y0 = y;
    float x1 = x + w;
    float y1 = y + h;

    // Apply rotation if needed
    if (sprite.rotation != 0.0f) {
        float cx = x + w * sprite.originX;
        float cy = y + h * sprite.originY;
        float cosR = std::cos(sprite.rotation);
        float sinR = std::sin(sprite.rotation);

        auto rotate = [&](float& px, float& py) {
            float dx = px - cx;
            float dy = py - cy;
            px = cx + dx * cosR - dy * sinR;
            py = cy + dx * sinR + dy * cosR;
        };

        // Rotate corners
        float corners[4][2] = {
            {x0, y0}, {x1, y0}, {x1, y1}, {x0, y1}
        };
        for (int i = 0; i < 4; i++) {
            rotate(corners[i][0], corners[i][1]);
        }

        // Top-left
        out[0].x = corners[0][0];
        out[0].y = corners[0][1];
        // Top-right
        out[1].x = corners[1][0];
        out[1].y = corners[1][1];
        // Bottom-right
        out[2].x = corners[2][0];
        out[2].y = corners[2][1];
        // Bottom-left
        out[3].x = corners[3][0];
        out[3].y = corners[3][1];
    } else {
        out[0].x = x0; out[0].y = y0;
        out[1].x = x1; out[1].y = y0;
        out[2].x = x1; out[2].y = y1;
        out[3].x = x0; out[3].y = y1;
    }

    // Set common attributes
    for (int i = 0; i < 4; i++) {
        out[i].z = sprite.depth;
        out[i].rhw = 1.0f;
        out[i].color = sprite.color;
    }

    // Texture coordinates
    out[0].u = sprite.u0; out[0].v = sprite.v0;
    out[1].u = sprite.u1; out[1].v = sprite.v0;
    out[2].u = sprite.u1; out[2].v = sprite.v1;
    out[3].u = sprite.u0; out[3].v = sprite.v1;
}

void SpriteBatch::SetupRenderStates() {
    if (!m_renderer) return;

    // Enable alpha blending
    m_renderer->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    m_renderer->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    m_renderer->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

    // Disable depth testing for 2D
    m_renderer->SetRenderState(D3DRS_ZENABLE, FALSE);
    m_renderer->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

    // No culling
    m_renderer->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

    // Setup texture stage for modulate
    m_renderer->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    m_renderer->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    m_renderer->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    m_renderer->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    m_renderer->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    m_renderer->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

    // Disable second stage
    m_renderer->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    m_renderer->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
}

void SpriteBatch::RestoreRenderStates() {
    if (!m_renderer) return;

    m_renderer->SetRenderState(D3DRS_ALPHABLENDENABLE, m_savedAlphaBlend);
    m_renderer->SetRenderState(D3DRS_SRCBLEND, m_savedSrcBlend);
    m_renderer->SetRenderState(D3DRS_DESTBLEND, m_savedDstBlend);
    m_renderer->SetRenderState(D3DRS_ZENABLE, m_savedZEnable);
    m_renderer->SetRenderState(D3DRS_ZWRITEENABLE, m_savedZWriteEnable);
    m_renderer->SetRenderState(D3DRS_CULLMODE, m_savedCullMode);
}

// =============================================================================
// Text Renderer Implementation
// =============================================================================

TextRenderer::TextRenderer()
    : m_spriteBatch(nullptr)
    , m_hasFont(false)
{
    m_font.texture = BGFX_INVALID_HANDLE;
    m_font.charWidths = nullptr;
}

TextRenderer::~TextRenderer() {
}

void TextRenderer::Initialize(SpriteBatch& spriteBatch) {
    m_spriteBatch = &spriteBatch;
}

void TextRenderer::SetFont(const BitmapFont& font) {
    m_font = font;
    m_hasFont = true;
}

void TextRenderer::DrawText(float x, float y, const char* text, uint32_t color, float scale) {
    if (!m_spriteBatch || !m_hasFont || !text) return;

    float cursorX = x;
    float cursorY = y;
    float charW = m_font.charWidth * scale;
    float charH = m_font.charHeight * scale;
    float texCharW = 1.0f / m_font.charsPerRow;
    float texCharH = texCharW; // Assume square texture

    while (*text) {
        char c = *text++;

        if (c == '\n') {
            cursorX = x;
            cursorY += charH;
            continue;
        }

        if (c == ' ') {
            cursorX += charW;
            continue;
        }

        uint32_t charIndex = static_cast<uint32_t>(c) - m_font.firstChar;
        if (charIndex >= m_font.numChars) {
            cursorX += charW;
            continue;
        }

        // Calculate texture coordinates
        uint32_t col = charIndex % m_font.charsPerRow;
        uint32_t row = charIndex / m_font.charsPerRow;

        float u0 = col * texCharW;
        float v0 = row * texCharH;
        float u1 = u0 + texCharW;
        float v1 = v0 + texCharH;

        // Get character width
        float w = charW;
        if (m_font.charWidths) {
            w = m_font.charWidths[charIndex] * scale;
        }

        // Draw character sprite
        m_spriteBatch->Draw(cursorX, cursorY, w, charH, u0, v0, u1, v1, color);

        cursorX += w;
    }
}

void TextRenderer::MeasureText(const char* text, float scale, float& outWidth, float& outHeight) {
    outWidth = 0.0f;
    outHeight = 0.0f;

    if (!m_hasFont || !text) return;

    float charW = m_font.charWidth * scale;
    float charH = m_font.charHeight * scale;
    float lineWidth = 0.0f;
    float maxWidth = 0.0f;
    int lines = 1;

    while (*text) {
        char c = *text++;

        if (c == '\n') {
            maxWidth = std::max(maxWidth, lineWidth);
            lineWidth = 0.0f;
            lines++;
            continue;
        }

        float w = charW;
        if (m_font.charWidths) {
            uint32_t charIndex = static_cast<uint32_t>(c) - m_font.firstChar;
            if (charIndex < m_font.numChars) {
                w = m_font.charWidths[charIndex] * scale;
            }
        }

        lineWidth += w;
    }

    maxWidth = std::max(maxWidth, lineWidth);
    outWidth = maxWidth;
    outHeight = lines * charH;
}

} // namespace dx8bgfx
