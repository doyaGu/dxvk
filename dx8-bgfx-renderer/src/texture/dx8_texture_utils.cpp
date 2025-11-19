#include "dx8bgfx/dx8_texture_utils.h"
#include <algorithm>
#include <cstring>
#include <cmath>

namespace dx8bgfx {

// =============================================================================
// Format Conversion
// =============================================================================

bgfx::TextureFormat::Enum TextureUtils::D3DFormatToBgfx(D3DFORMAT format) {
    switch (format) {
        // RGBA formats
        case D3DFMT_A8R8G8B8:    return bgfx::TextureFormat::BGRA8;
        case D3DFMT_X8R8G8B8:    return bgfx::TextureFormat::BGRA8;
        case D3DFMT_R5G6B5:      return bgfx::TextureFormat::R5G6B5;
        case D3DFMT_A1R5G5B5:    return bgfx::TextureFormat::RGB5A1;
        case D3DFMT_X1R5G5B5:    return bgfx::TextureFormat::RGB5A1;
        case D3DFMT_A4R4G4B4:    return bgfx::TextureFormat::RGBA4;
        case D3DFMT_A8:          return bgfx::TextureFormat::A8;
        case D3DFMT_A2B10G10R10: return bgfx::TextureFormat::RGB10A2;

        // Luminance formats - convert to R8
        case D3DFMT_L8:          return bgfx::TextureFormat::R8;
        case D3DFMT_A8L8:        return bgfx::TextureFormat::RG8;

        // Bump mapping formats
        case D3DFMT_V8U8:        return bgfx::TextureFormat::RG8S;
        case D3DFMT_Q8W8V8U8:    return bgfx::TextureFormat::RGBA8S;
        case D3DFMT_V16U16:      return bgfx::TextureFormat::RG16S;

        // DXT compressed formats
        case D3DFMT_DXT1:        return bgfx::TextureFormat::BC1;
        case D3DFMT_DXT2:        return bgfx::TextureFormat::BC2;
        case D3DFMT_DXT3:        return bgfx::TextureFormat::BC2;
        case D3DFMT_DXT4:        return bgfx::TextureFormat::BC3;
        case D3DFMT_DXT5:        return bgfx::TextureFormat::BC3;

        // Depth formats
        case D3DFMT_D16:
        case D3DFMT_D16_LOCKABLE:return bgfx::TextureFormat::D16;
        case D3DFMT_D24S8:       return bgfx::TextureFormat::D24S8;
        case D3DFMT_D24X8:       return bgfx::TextureFormat::D24;
        case D3DFMT_D32:         return bgfx::TextureFormat::D32;

        // Fallback
        default:                 return bgfx::TextureFormat::BGRA8;
    }
}

D3DFORMAT TextureUtils::BgfxFormatToD3D(bgfx::TextureFormat::Enum format) {
    switch (format) {
        case bgfx::TextureFormat::BGRA8:     return D3DFMT_A8R8G8B8;
        case bgfx::TextureFormat::RGBA8:     return D3DFMT_A8R8G8B8;
        case bgfx::TextureFormat::R5G6B5:    return D3DFMT_R5G6B5;
        case bgfx::TextureFormat::RGB5A1:    return D3DFMT_A1R5G5B5;
        case bgfx::TextureFormat::RGBA4:     return D3DFMT_A4R4G4B4;
        case bgfx::TextureFormat::A8:        return D3DFMT_A8;
        case bgfx::TextureFormat::R8:        return D3DFMT_L8;
        case bgfx::TextureFormat::RG8:       return D3DFMT_A8L8;
        case bgfx::TextureFormat::RG8S:      return D3DFMT_V8U8;
        case bgfx::TextureFormat::RGBA8S:    return D3DFMT_Q8W8V8U8;
        case bgfx::TextureFormat::BC1:       return D3DFMT_DXT1;
        case bgfx::TextureFormat::BC2:       return D3DFMT_DXT3;
        case bgfx::TextureFormat::BC3:       return D3DFMT_DXT5;
        case bgfx::TextureFormat::D16:       return D3DFMT_D16;
        case bgfx::TextureFormat::D24S8:     return D3DFMT_D24S8;
        case bgfx::TextureFormat::D24:       return D3DFMT_D24X8;
        case bgfx::TextureFormat::D32:       return D3DFMT_D32;
        default:                             return D3DFMT_UNKNOWN;
    }
}

// =============================================================================
// Format Information
// =============================================================================

uint32_t TextureUtils::GetBitsPerPixel(D3DFORMAT format) {
    switch (format) {
        case D3DFMT_R8G8B8:         return 24;
        case D3DFMT_A8R8G8B8:       return 32;
        case D3DFMT_X8R8G8B8:       return 32;
        case D3DFMT_R5G6B5:         return 16;
        case D3DFMT_X1R5G5B5:       return 16;
        case D3DFMT_A1R5G5B5:       return 16;
        case D3DFMT_A4R4G4B4:       return 16;
        case D3DFMT_R3G3B2:         return 8;
        case D3DFMT_A8:             return 8;
        case D3DFMT_A8R3G3B2:       return 16;
        case D3DFMT_X4R4G4B4:       return 16;
        case D3DFMT_A2B10G10R10:    return 32;
        case D3DFMT_G16R16:         return 32;
        case D3DFMT_A8P8:           return 16;
        case D3DFMT_P8:             return 8;
        case D3DFMT_L8:             return 8;
        case D3DFMT_A8L8:           return 16;
        case D3DFMT_A4L4:           return 8;
        case D3DFMT_V8U8:           return 16;
        case D3DFMT_L6V5U5:         return 16;
        case D3DFMT_X8L8V8U8:       return 32;
        case D3DFMT_Q8W8V8U8:       return 32;
        case D3DFMT_V16U16:         return 32;
        case D3DFMT_D16_LOCKABLE:   return 16;
        case D3DFMT_D32:            return 32;
        case D3DFMT_D15S1:          return 16;
        case D3DFMT_D24S8:          return 32;
        case D3DFMT_D24X8:          return 32;
        case D3DFMT_D24X4S4:        return 32;
        case D3DFMT_D16:            return 16;
        case D3DFMT_DXT1:           return 4;  // 4 bits per pixel (compressed)
        case D3DFMT_DXT2:
        case D3DFMT_DXT3:
        case D3DFMT_DXT4:
        case D3DFMT_DXT5:           return 8;  // 8 bits per pixel (compressed)
        default:                    return 0;
    }
}

uint32_t TextureUtils::GetBlockSize(D3DFORMAT format) {
    switch (format) {
        case D3DFMT_DXT1:           return 8;
        case D3DFMT_DXT2:
        case D3DFMT_DXT3:
        case D3DFMT_DXT4:
        case D3DFMT_DXT5:           return 16;
        default:                    return 0;
    }
}

bool TextureUtils::IsCompressed(D3DFORMAT format) {
    switch (format) {
        case D3DFMT_DXT1:
        case D3DFMT_DXT2:
        case D3DFMT_DXT3:
        case D3DFMT_DXT4:
        case D3DFMT_DXT5:
            return true;
        default:
            return false;
    }
}

bool TextureUtils::HasAlpha(D3DFORMAT format) {
    switch (format) {
        case D3DFMT_A8R8G8B8:
        case D3DFMT_A1R5G5B5:
        case D3DFMT_A4R4G4B4:
        case D3DFMT_A8:
        case D3DFMT_A8R3G3B2:
        case D3DFMT_A2B10G10R10:
        case D3DFMT_A8P8:
        case D3DFMT_A8L8:
        case D3DFMT_A4L4:
        case D3DFMT_DXT2:
        case D3DFMT_DXT3:
        case D3DFMT_DXT4:
        case D3DFMT_DXT5:
            return true;
        default:
            return false;
    }
}

bool TextureUtils::IsDepthFormat(D3DFORMAT format) {
    switch (format) {
        case D3DFMT_D16_LOCKABLE:
        case D3DFMT_D32:
        case D3DFMT_D15S1:
        case D3DFMT_D24S8:
        case D3DFMT_D24X8:
        case D3DFMT_D24X4S4:
        case D3DFMT_D16:
            return true;
        default:
            return false;
    }
}

// =============================================================================
// Pitch Calculation
// =============================================================================

uint32_t TextureUtils::CalculatePitch(D3DFORMAT format, uint32_t width) {
    if (IsCompressed(format)) {
        // For compressed formats, pitch is for 4-row blocks
        uint32_t blockWidth = (width + 3) / 4;
        return blockWidth * GetBlockSize(format);
    } else {
        // For uncompressed formats
        uint32_t bpp = GetBitsPerPixel(format);
        return (width * bpp + 7) / 8;
    }
}

uint32_t TextureUtils::CalculateSlicePitch(D3DFORMAT format, uint32_t width, uint32_t height) {
    if (IsCompressed(format)) {
        uint32_t blockWidth = (width + 3) / 4;
        uint32_t blockHeight = (height + 3) / 4;
        return blockWidth * blockHeight * GetBlockSize(format);
    } else {
        return CalculatePitch(format, width) * height;
    }
}

// =============================================================================
// Mipmap Calculation
// =============================================================================

uint32_t TextureUtils::CalculateMipLevels(uint32_t width, uint32_t height) {
    uint32_t levels = 1;
    uint32_t size = std::max(width, height);
    while (size > 1) {
        size >>= 1;
        levels++;
    }
    return levels;
}

void TextureUtils::CalculateMipDimensions(uint32_t level, uint32_t& width, uint32_t& height) {
    width = std::max(1u, width >> level);
    height = std::max(1u, height >> level);
}

// =============================================================================
// Pixel Conversion
// =============================================================================

uint32_t TextureUtils::ConvertPixelToRGBA8(const uint8_t* src, D3DFORMAT format) {
    uint32_t r = 0, g = 0, b = 0, a = 255;

    switch (format) {
        case D3DFMT_A8R8G8B8:
            b = src[0]; g = src[1]; r = src[2]; a = src[3];
            break;
        case D3DFMT_X8R8G8B8:
            b = src[0]; g = src[1]; r = src[2]; a = 255;
            break;
        case D3DFMT_R8G8B8:
            b = src[0]; g = src[1]; r = src[2]; a = 255;
            break;
        case D3DFMT_R5G6B5: {
            uint16_t pixel = *reinterpret_cast<const uint16_t*>(src);
            r = ((pixel >> 11) & 0x1F) * 255 / 31;
            g = ((pixel >> 5) & 0x3F) * 255 / 63;
            b = (pixel & 0x1F) * 255 / 31;
            a = 255;
            break;
        }
        case D3DFMT_A1R5G5B5: {
            uint16_t pixel = *reinterpret_cast<const uint16_t*>(src);
            r = ((pixel >> 10) & 0x1F) * 255 / 31;
            g = ((pixel >> 5) & 0x1F) * 255 / 31;
            b = (pixel & 0x1F) * 255 / 31;
            a = (pixel >> 15) ? 255 : 0;
            break;
        }
        case D3DFMT_X1R5G5B5: {
            uint16_t pixel = *reinterpret_cast<const uint16_t*>(src);
            r = ((pixel >> 10) & 0x1F) * 255 / 31;
            g = ((pixel >> 5) & 0x1F) * 255 / 31;
            b = (pixel & 0x1F) * 255 / 31;
            a = 255;
            break;
        }
        case D3DFMT_A4R4G4B4: {
            uint16_t pixel = *reinterpret_cast<const uint16_t*>(src);
            r = ((pixel >> 8) & 0xF) * 255 / 15;
            g = ((pixel >> 4) & 0xF) * 255 / 15;
            b = (pixel & 0xF) * 255 / 15;
            a = ((pixel >> 12) & 0xF) * 255 / 15;
            break;
        }
        case D3DFMT_A8:
            r = g = b = 255;
            a = src[0];
            break;
        case D3DFMT_L8:
            r = g = b = src[0];
            a = 255;
            break;
        case D3DFMT_A8L8:
            r = g = b = src[0];
            a = src[1];
            break;
        default:
            break;
    }

    return (a << 24) | (b << 16) | (g << 8) | r;
}

void TextureUtils::ConvertRGBA8ToPixel(uint32_t rgba, uint8_t* dst, D3DFORMAT format) {
    uint8_t r = rgba & 0xFF;
    uint8_t g = (rgba >> 8) & 0xFF;
    uint8_t b = (rgba >> 16) & 0xFF;
    uint8_t a = (rgba >> 24) & 0xFF;

    switch (format) {
        case D3DFMT_A8R8G8B8:
            dst[0] = b; dst[1] = g; dst[2] = r; dst[3] = a;
            break;
        case D3DFMT_X8R8G8B8:
            dst[0] = b; dst[1] = g; dst[2] = r; dst[3] = 255;
            break;
        case D3DFMT_R8G8B8:
            dst[0] = b; dst[1] = g; dst[2] = r;
            break;
        case D3DFMT_R5G6B5: {
            uint16_t pixel = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
            *reinterpret_cast<uint16_t*>(dst) = pixel;
            break;
        }
        case D3DFMT_A1R5G5B5: {
            uint16_t pixel = ((a ? 1 : 0) << 15) | ((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3);
            *reinterpret_cast<uint16_t*>(dst) = pixel;
            break;
        }
        case D3DFMT_A4R4G4B4: {
            uint16_t pixel = ((a >> 4) << 12) | ((r >> 4) << 8) | ((g >> 4) << 4) | (b >> 4);
            *reinterpret_cast<uint16_t*>(dst) = pixel;
            break;
        }
        case D3DFMT_A8:
            dst[0] = a;
            break;
        case D3DFMT_L8:
            // Convert to grayscale
            dst[0] = static_cast<uint8_t>((r * 299 + g * 587 + b * 114) / 1000);
            break;
        case D3DFMT_A8L8:
            dst[0] = static_cast<uint8_t>((r * 299 + g * 587 + b * 114) / 1000);
            dst[1] = a;
            break;
        default:
            break;
    }
}

void TextureUtils::ConvertPixels(
    const void* srcData, D3DFORMAT srcFormat,
    void* dstData, D3DFORMAT dstFormat,
    uint32_t width, uint32_t height
) {
    if (srcFormat == dstFormat) {
        // Direct copy
        uint32_t size = CalculateSlicePitch(srcFormat, width, height);
        std::memcpy(dstData, srcData, size);
        return;
    }

    uint32_t srcBpp = GetBitsPerPixel(srcFormat) / 8;
    uint32_t dstBpp = GetBitsPerPixel(dstFormat) / 8;

    const uint8_t* src = static_cast<const uint8_t*>(srcData);
    uint8_t* dst = static_cast<uint8_t*>(dstData);

    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            uint32_t rgba = ConvertPixelToRGBA8(src, srcFormat);
            ConvertRGBA8ToPixel(rgba, dst, dstFormat);
            src += srcBpp;
            dst += dstBpp;
        }
    }
}

// =============================================================================
// BGRA <-> RGBA Swizzle
// =============================================================================

void TextureUtils::SwizzleBGRAtoRGBA(void* data, uint32_t width, uint32_t height, uint32_t bpp) {
    if (bpp != 4) return;

    uint8_t* pixels = static_cast<uint8_t*>(data);
    uint32_t count = width * height;

    for (uint32_t i = 0; i < count; i++) {
        std::swap(pixels[0], pixels[2]); // Swap B and R
        pixels += 4;
    }
}

void TextureUtils::SwizzleRGBAtoBGRA(void* data, uint32_t width, uint32_t height, uint32_t bpp) {
    // Same operation - just swapping R and B
    SwizzleBGRAtoRGBA(data, width, height, bpp);
}

// =============================================================================
// Texture Creation
// =============================================================================

bgfx::TextureHandle TextureUtils::CreateTexture2D(
    uint32_t width, uint32_t height,
    bool hasMips, uint16_t numLayers,
    D3DFORMAT format,
    uint64_t flags,
    const void* data, uint32_t dataSize
) {
    bgfx::TextureFormat::Enum bgfxFormat = D3DFormatToBgfx(format);

    // Create memory reference if data provided
    const bgfx::Memory* mem = nullptr;
    if (data && dataSize > 0) {
        mem = bgfx::copy(data, dataSize);
    }

    return bgfx::createTexture2D(
        static_cast<uint16_t>(width),
        static_cast<uint16_t>(height),
        hasMips,
        numLayers,
        bgfxFormat,
        flags,
        mem
    );
}

// =============================================================================
// Surface Operations
// =============================================================================

bool TextureUtils::CopyRects(
    const void* srcData, D3DFORMAT srcFormat, uint32_t srcPitch,
    void* dstData, D3DFORMAT dstFormat, uint32_t dstPitch,
    uint32_t width, uint32_t height,
    uint32_t srcX, uint32_t srcY,
    uint32_t dstX, uint32_t dstY
) {
    if (srcFormat != dstFormat) {
        // Format conversion needed
        return false; // TODO: Implement format conversion during copy
    }

    uint32_t bpp = GetBitsPerPixel(srcFormat) / 8;
    uint32_t rowBytes = width * bpp;

    const uint8_t* src = static_cast<const uint8_t*>(srcData) + srcY * srcPitch + srcX * bpp;
    uint8_t* dst = static_cast<uint8_t*>(dstData) + dstY * dstPitch + dstX * bpp;

    for (uint32_t y = 0; y < height; y++) {
        std::memcpy(dst, src, rowBytes);
        src += srcPitch;
        dst += dstPitch;
    }

    return true;
}

// =============================================================================
// Color Key
// =============================================================================

void TextureUtils::ApplyColorKey(
    void* data, D3DFORMAT format,
    uint32_t width, uint32_t height,
    uint32_t colorKey
) {
    // Only supported for 32-bit formats
    if (format != D3DFMT_A8R8G8B8 && format != D3DFMT_X8R8G8B8) {
        return;
    }

    uint32_t* pixels = static_cast<uint32_t*>(data);
    uint32_t count = width * height;

    // Color key is in ARGB format
    uint32_t keyRGB = colorKey & 0x00FFFFFF;

    for (uint32_t i = 0; i < count; i++) {
        if ((pixels[i] & 0x00FFFFFF) == keyRGB) {
            pixels[i] = 0; // Set to transparent black
        }
    }
}

// =============================================================================
// Mipmap Generation
// =============================================================================

void TextureUtils::BoxFilter2D(
    const uint8_t* src, uint8_t* dst,
    uint32_t srcWidth, uint32_t srcHeight,
    uint32_t bpp
) {
    uint32_t dstWidth = std::max(1u, srcWidth / 2);
    uint32_t dstHeight = std::max(1u, srcHeight / 2);

    for (uint32_t y = 0; y < dstHeight; y++) {
        for (uint32_t x = 0; x < dstWidth; x++) {
            uint32_t srcX = x * 2;
            uint32_t srcY = y * 2;

            for (uint32_t c = 0; c < bpp; c++) {
                uint32_t sum = 0;
                uint32_t count = 0;

                // Sample 2x2 block
                for (uint32_t dy = 0; dy < 2 && srcY + dy < srcHeight; dy++) {
                    for (uint32_t dx = 0; dx < 2 && srcX + dx < srcWidth; dx++) {
                        uint32_t idx = ((srcY + dy) * srcWidth + (srcX + dx)) * bpp + c;
                        sum += src[idx];
                        count++;
                    }
                }

                uint32_t dstIdx = (y * dstWidth + x) * bpp + c;
                dst[dstIdx] = static_cast<uint8_t>(sum / count);
            }
        }
    }
}

std::vector<uint8_t> TextureUtils::GenerateMipmaps(
    const void* data, D3DFORMAT format,
    uint32_t width, uint32_t height,
    uint32_t& outMipLevels
) {
    if (IsCompressed(format)) {
        // Cannot generate mipmaps for compressed formats
        outMipLevels = 1;
        uint32_t size = CalculateSlicePitch(format, width, height);
        std::vector<uint8_t> result(size);
        std::memcpy(result.data(), data, size);
        return result;
    }

    uint32_t bpp = GetBitsPerPixel(format) / 8;
    outMipLevels = CalculateMipLevels(width, height);

    // Calculate total size
    uint32_t totalSize = 0;
    uint32_t w = width, h = height;
    for (uint32_t i = 0; i < outMipLevels; i++) {
        totalSize += w * h * bpp;
        w = std::max(1u, w / 2);
        h = std::max(1u, h / 2);
    }

    std::vector<uint8_t> result(totalSize);

    // Copy mip 0
    uint32_t mip0Size = width * height * bpp;
    std::memcpy(result.data(), data, mip0Size);

    // Generate subsequent mips
    uint32_t srcOffset = 0;
    uint32_t dstOffset = mip0Size;
    w = width;
    h = height;

    for (uint32_t i = 1; i < outMipLevels; i++) {
        uint32_t srcW = w, srcH = h;
        w = std::max(1u, w / 2);
        h = std::max(1u, h / 2);

        BoxFilter2D(
            result.data() + srcOffset,
            result.data() + dstOffset,
            srcW, srcH, bpp
        );

        srcOffset = dstOffset;
        dstOffset += w * h * bpp;
    }

    return result;
}

} // namespace dx8bgfx
