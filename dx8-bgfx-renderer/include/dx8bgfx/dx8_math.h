#pragma once

#include "dx8_types.h"
#include <cmath>

namespace dx8bgfx {

// =============================================================================
// Constants
// =============================================================================

constexpr float PI = 3.14159265358979323846f;
constexpr float DEG_TO_RAD = PI / 180.0f;
constexpr float RAD_TO_DEG = 180.0f / PI;

// =============================================================================
// Matrix Functions
// =============================================================================

// Identity matrix
inline D3DMATRIX MatrixIdentity() {
    D3DMATRIX m = {};
    m._11 = m._22 = m._33 = m._44 = 1.0f;
    return m;
}

// Zero matrix
inline D3DMATRIX MatrixZero() {
    return D3DMATRIX{};
}

// Matrix multiplication
inline D3DMATRIX MatrixMultiply(const D3DMATRIX& a, const D3DMATRIX& b) {
    D3DMATRIX result = {};
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            result.m[i][j] = a.m[i][0] * b.m[0][j] +
                             a.m[i][1] * b.m[1][j] +
                             a.m[i][2] * b.m[2][j] +
                             a.m[i][3] * b.m[3][j];
        }
    }
    return result;
}

// Matrix transpose
inline D3DMATRIX MatrixTranspose(const D3DMATRIX& m) {
    D3DMATRIX result;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            result.m[i][j] = m.m[j][i];
        }
    }
    return result;
}

// Matrix determinant (3x3 submatrix)
inline float Matrix3x3Determinant(const D3DMATRIX& m) {
    return m._11 * (m._22 * m._33 - m._23 * m._32) -
           m._12 * (m._21 * m._33 - m._23 * m._31) +
           m._13 * (m._21 * m._32 - m._22 * m._31);
}

// Matrix determinant (4x4)
inline float MatrixDeterminant(const D3DMATRIX& m) {
    float det = 0.0f;

    det += m._11 * (m._22 * (m._33 * m._44 - m._34 * m._43) -
                    m._23 * (m._32 * m._44 - m._34 * m._42) +
                    m._24 * (m._32 * m._43 - m._33 * m._42));

    det -= m._12 * (m._21 * (m._33 * m._44 - m._34 * m._43) -
                    m._23 * (m._31 * m._44 - m._34 * m._41) +
                    m._24 * (m._31 * m._43 - m._33 * m._41));

    det += m._13 * (m._21 * (m._32 * m._44 - m._34 * m._42) -
                    m._22 * (m._31 * m._44 - m._34 * m._41) +
                    m._24 * (m._31 * m._42 - m._32 * m._41));

    det -= m._14 * (m._21 * (m._32 * m._43 - m._33 * m._42) -
                    m._22 * (m._31 * m._43 - m._33 * m._41) +
                    m._23 * (m._31 * m._42 - m._32 * m._41));

    return det;
}

// Matrix inverse
inline D3DMATRIX MatrixInverse(const D3DMATRIX& m) {
    D3DMATRIX result;
    float det = MatrixDeterminant(m);

    if (std::abs(det) < 0.0001f) {
        return MatrixIdentity();
    }

    float invDet = 1.0f / det;

    result._11 = invDet * (m._22 * (m._33 * m._44 - m._34 * m._43) -
                           m._23 * (m._32 * m._44 - m._34 * m._42) +
                           m._24 * (m._32 * m._43 - m._33 * m._42));
    result._12 = -invDet * (m._12 * (m._33 * m._44 - m._34 * m._43) -
                            m._13 * (m._32 * m._44 - m._34 * m._42) +
                            m._14 * (m._32 * m._43 - m._33 * m._42));
    result._13 = invDet * (m._12 * (m._23 * m._44 - m._24 * m._43) -
                           m._13 * (m._22 * m._44 - m._24 * m._42) +
                           m._14 * (m._22 * m._43 - m._23 * m._42));
    result._14 = -invDet * (m._12 * (m._23 * m._34 - m._24 * m._33) -
                            m._13 * (m._22 * m._34 - m._24 * m._32) +
                            m._14 * (m._22 * m._33 - m._23 * m._32));

    result._21 = -invDet * (m._21 * (m._33 * m._44 - m._34 * m._43) -
                            m._23 * (m._31 * m._44 - m._34 * m._41) +
                            m._24 * (m._31 * m._43 - m._33 * m._41));
    result._22 = invDet * (m._11 * (m._33 * m._44 - m._34 * m._43) -
                           m._13 * (m._31 * m._44 - m._34 * m._41) +
                           m._14 * (m._31 * m._43 - m._33 * m._41));
    result._23 = -invDet * (m._11 * (m._23 * m._44 - m._24 * m._43) -
                            m._13 * (m._21 * m._44 - m._24 * m._41) +
                            m._14 * (m._21 * m._43 - m._23 * m._41));
    result._24 = invDet * (m._11 * (m._23 * m._34 - m._24 * m._33) -
                           m._13 * (m._21 * m._34 - m._24 * m._31) +
                           m._14 * (m._21 * m._33 - m._23 * m._31));

    result._31 = invDet * (m._21 * (m._32 * m._44 - m._34 * m._42) -
                           m._22 * (m._31 * m._44 - m._34 * m._41) +
                           m._24 * (m._31 * m._42 - m._32 * m._41));
    result._32 = -invDet * (m._11 * (m._32 * m._44 - m._34 * m._42) -
                            m._12 * (m._31 * m._44 - m._34 * m._41) +
                            m._14 * (m._31 * m._42 - m._32 * m._41));
    result._33 = invDet * (m._11 * (m._22 * m._44 - m._24 * m._42) -
                           m._12 * (m._21 * m._44 - m._24 * m._41) +
                           m._14 * (m._21 * m._42 - m._22 * m._41));
    result._34 = -invDet * (m._11 * (m._22 * m._34 - m._24 * m._32) -
                            m._12 * (m._21 * m._34 - m._24 * m._31) +
                            m._14 * (m._21 * m._32 - m._22 * m._31));

    result._41 = -invDet * (m._21 * (m._32 * m._43 - m._33 * m._42) -
                            m._22 * (m._31 * m._43 - m._33 * m._41) +
                            m._23 * (m._31 * m._42 - m._32 * m._41));
    result._42 = invDet * (m._11 * (m._32 * m._43 - m._33 * m._42) -
                           m._12 * (m._31 * m._43 - m._33 * m._41) +
                           m._13 * (m._31 * m._42 - m._32 * m._41));
    result._43 = -invDet * (m._11 * (m._22 * m._43 - m._23 * m._42) -
                            m._12 * (m._21 * m._43 - m._23 * m._41) +
                            m._13 * (m._21 * m._42 - m._22 * m._41));
    result._44 = invDet * (m._11 * (m._22 * m._33 - m._23 * m._32) -
                           m._12 * (m._21 * m._33 - m._23 * m._31) +
                           m._13 * (m._21 * m._32 - m._22 * m._31));

    return result;
}

// Translation matrix
inline D3DMATRIX MatrixTranslation(float x, float y, float z) {
    D3DMATRIX m = MatrixIdentity();
    m._41 = x;
    m._42 = y;
    m._43 = z;
    return m;
}

// Scaling matrix
inline D3DMATRIX MatrixScaling(float sx, float sy, float sz) {
    D3DMATRIX m = {};
    m._11 = sx;
    m._22 = sy;
    m._33 = sz;
    m._44 = 1.0f;
    return m;
}

// Rotation around X axis
inline D3DMATRIX MatrixRotationX(float angle) {
    float c = std::cos(angle);
    float s = std::sin(angle);

    D3DMATRIX m = MatrixIdentity();
    m._22 = c;
    m._23 = s;
    m._32 = -s;
    m._33 = c;
    return m;
}

// Rotation around Y axis
inline D3DMATRIX MatrixRotationY(float angle) {
    float c = std::cos(angle);
    float s = std::sin(angle);

    D3DMATRIX m = MatrixIdentity();
    m._11 = c;
    m._13 = -s;
    m._31 = s;
    m._33 = c;
    return m;
}

// Rotation around Z axis
inline D3DMATRIX MatrixRotationZ(float angle) {
    float c = std::cos(angle);
    float s = std::sin(angle);

    D3DMATRIX m = MatrixIdentity();
    m._11 = c;
    m._12 = s;
    m._21 = -s;
    m._22 = c;
    return m;
}

// Rotation around arbitrary axis
inline D3DMATRIX MatrixRotationAxis(const D3DVECTOR& axis, float angle) {
    float c = std::cos(angle);
    float s = std::sin(angle);
    float t = 1.0f - c;

    // Normalize axis
    float len = std::sqrt(axis.x * axis.x + axis.y * axis.y + axis.z * axis.z);
    float x = axis.x / len;
    float y = axis.y / len;
    float z = axis.z / len;

    D3DMATRIX m = {};
    m._11 = t * x * x + c;
    m._12 = t * x * y + s * z;
    m._13 = t * x * z - s * y;
    m._14 = 0.0f;

    m._21 = t * x * y - s * z;
    m._22 = t * y * y + c;
    m._23 = t * y * z + s * x;
    m._24 = 0.0f;

    m._31 = t * x * z + s * y;
    m._32 = t * y * z - s * x;
    m._33 = t * z * z + c;
    m._34 = 0.0f;

    m._41 = 0.0f;
    m._42 = 0.0f;
    m._43 = 0.0f;
    m._44 = 1.0f;

    return m;
}

// Look-at matrix (left-handed)
inline D3DMATRIX MatrixLookAtLH(const D3DVECTOR& eye, const D3DVECTOR& at, const D3DVECTOR& up) {
    D3DVECTOR zaxis = {at.x - eye.x, at.y - eye.y, at.z - eye.z};
    float zlen = std::sqrt(zaxis.x * zaxis.x + zaxis.y * zaxis.y + zaxis.z * zaxis.z);
    zaxis = {zaxis.x / zlen, zaxis.y / zlen, zaxis.z / zlen};

    D3DVECTOR xaxis = {
        up.y * zaxis.z - up.z * zaxis.y,
        up.z * zaxis.x - up.x * zaxis.z,
        up.x * zaxis.y - up.y * zaxis.x
    };
    float xlen = std::sqrt(xaxis.x * xaxis.x + xaxis.y * xaxis.y + xaxis.z * xaxis.z);
    xaxis = {xaxis.x / xlen, xaxis.y / xlen, xaxis.z / xlen};

    D3DVECTOR yaxis = {
        zaxis.y * xaxis.z - zaxis.z * xaxis.y,
        zaxis.z * xaxis.x - zaxis.x * xaxis.z,
        zaxis.x * xaxis.y - zaxis.y * xaxis.x
    };

    D3DMATRIX m = {};
    m._11 = xaxis.x; m._12 = yaxis.x; m._13 = zaxis.x; m._14 = 0.0f;
    m._21 = xaxis.y; m._22 = yaxis.y; m._23 = zaxis.y; m._24 = 0.0f;
    m._31 = xaxis.z; m._32 = yaxis.z; m._33 = zaxis.z; m._34 = 0.0f;
    m._41 = -(xaxis.x * eye.x + xaxis.y * eye.y + xaxis.z * eye.z);
    m._42 = -(yaxis.x * eye.x + yaxis.y * eye.y + yaxis.z * eye.z);
    m._43 = -(zaxis.x * eye.x + zaxis.y * eye.y + zaxis.z * eye.z);
    m._44 = 1.0f;

    return m;
}

// Perspective projection matrix (left-handed)
inline D3DMATRIX MatrixPerspectiveFovLH(float fovY, float aspect, float zn, float zf) {
    float h = 1.0f / std::tan(fovY * 0.5f);
    float w = h / aspect;

    D3DMATRIX m = {};
    m._11 = w;
    m._22 = h;
    m._33 = zf / (zf - zn);
    m._34 = 1.0f;
    m._43 = -zn * zf / (zf - zn);

    return m;
}

// Orthographic projection matrix (left-handed)
inline D3DMATRIX MatrixOrthoLH(float w, float h, float zn, float zf) {
    D3DMATRIX m = {};
    m._11 = 2.0f / w;
    m._22 = 2.0f / h;
    m._33 = 1.0f / (zf - zn);
    m._43 = -zn / (zf - zn);
    m._44 = 1.0f;

    return m;
}

// =============================================================================
// Vector Functions
// =============================================================================

// Vector length
inline float VectorLength(const D3DVECTOR& v) {
    return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

// Vector length squared
inline float VectorLengthSq(const D3DVECTOR& v) {
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

// Normalize vector
inline D3DVECTOR VectorNormalize(const D3DVECTOR& v) {
    float len = VectorLength(v);
    if (len < 0.0001f) return {0.0f, 0.0f, 0.0f};
    return {v.x / len, v.y / len, v.z / len};
}

// Dot product
inline float VectorDot(const D3DVECTOR& a, const D3DVECTOR& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

// Cross product
inline D3DVECTOR VectorCross(const D3DVECTOR& a, const D3DVECTOR& b) {
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

// Vector addition
inline D3DVECTOR VectorAdd(const D3DVECTOR& a, const D3DVECTOR& b) {
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}

// Vector subtraction
inline D3DVECTOR VectorSubtract(const D3DVECTOR& a, const D3DVECTOR& b) {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

// Vector scale
inline D3DVECTOR VectorScale(const D3DVECTOR& v, float s) {
    return {v.x * s, v.y * s, v.z * s};
}

// Vector negate
inline D3DVECTOR VectorNegate(const D3DVECTOR& v) {
    return {-v.x, -v.y, -v.z};
}

// Lerp
inline D3DVECTOR VectorLerp(const D3DVECTOR& a, const D3DVECTOR& b, float t) {
    return {
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t
    };
}

// Transform vector by matrix (w=1)
inline D3DVECTOR VectorTransformCoord(const D3DVECTOR& v, const D3DMATRIX& m) {
    float w = v.x * m._14 + v.y * m._24 + v.z * m._34 + m._44;
    return {
        (v.x * m._11 + v.y * m._21 + v.z * m._31 + m._41) / w,
        (v.x * m._12 + v.y * m._22 + v.z * m._32 + m._42) / w,
        (v.x * m._13 + v.y * m._23 + v.z * m._33 + m._43) / w
    };
}

// Transform vector by matrix (w=0, for normals)
inline D3DVECTOR VectorTransformNormal(const D3DVECTOR& v, const D3DMATRIX& m) {
    return {
        v.x * m._11 + v.y * m._21 + v.z * m._31,
        v.x * m._12 + v.y * m._22 + v.z * m._32,
        v.x * m._13 + v.y * m._23 + v.z * m._33
    };
}

// =============================================================================
// Color Functions
// =============================================================================

// Color lerp
inline D3DCOLORVALUE ColorLerp(const D3DCOLORVALUE& a, const D3DCOLORVALUE& b, float t) {
    return {
        a.r + (b.r - a.r) * t,
        a.g + (b.g - a.g) * t,
        a.b + (b.b - a.b) * t,
        a.a + (b.a - a.a) * t
    };
}

// Color modulate
inline D3DCOLORVALUE ColorModulate(const D3DCOLORVALUE& a, const D3DCOLORVALUE& b) {
    return {
        a.r * b.r,
        a.g * b.g,
        a.b * b.b,
        a.a * b.a
    };
}

// Color add
inline D3DCOLORVALUE ColorAdd(const D3DCOLORVALUE& a, const D3DCOLORVALUE& b) {
    return {
        std::min(a.r + b.r, 1.0f),
        std::min(a.g + b.g, 1.0f),
        std::min(a.b + b.b, 1.0f),
        std::min(a.a + b.a, 1.0f)
    };
}

} // namespace dx8bgfx
