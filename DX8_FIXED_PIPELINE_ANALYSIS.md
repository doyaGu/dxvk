# DX8 固定管线渲染器技术分析与 bgfx 重新实现方案

## 目录
1. [DXVK DX8 固定管线架构分析](#架构分析)
2. [核心功能模块详解](#核心功能模块)
3. [状态管理机制](#状态管理机制)
4. [Shader 动态生成策略](#shader动态生成)
5. [bgfx 重新实现方案](#bgfx实现方案)
6. [实现路线图](#实现路线图)

---

## 架构分析

### DXVK 分层架构

```
┌─────────────────────────────────────────┐
│         D3D8 API Layer (d3d8/)          │
│  - d3d8_device.cpp/h                    │
│  - d3d8_shader.cpp/h (VS8 → VS9 转换)  │
└──────────────────┬──────────────────────┘
                   ↓ (转换层)
┌─────────────────────────────────────────┐
│         D3D9 API Layer (d3d9/)          │
│  - 固定管线展开为可编程管线              │
└──────────────────┬──────────────────────┘
                   ↓ (动态生成)
┌─────────────────────────────────────────┐
│    GLSL Shaders (d3d9/shaders/)         │
│  - d3d9_fixed_function_vert.vert (711行)│
│  - d3d9_fixed_function_frag.frag        │
└──────────────────┬──────────────────────┘
                   ↓ (编译)
┌─────────────────────────────────────────┐
│         SPIR-V Bytecode                 │
└──────────────────┬──────────────────────┘
                   ↓
┌─────────────────────────────────────────┐
│            Vulkan API                   │
└─────────────────────────────────────────┘
```

### 关键文件清单

| 文件路径 | 行数/大小 | 功能描述 |
|---------|----------|---------|
| `src/d3d8/d3d8_shader.cpp` | 12535 bytes | D3D8→D3D9 顶点着色器转换 |
| `src/d3d8/d3d8_device.cpp` | ~2000行 | D3D8 设备实现，状态设置 API |
| `src/d3d9/d3d9_fixed_function.cpp` | 3017行 | 固定管线核心实现 |
| `src/d3d9/d3d9_fixed_function.h` | 217行 | Shader 模块管理接口 |
| `src/d3d9/d3d9_state.h` | ~600行 | 状态管理数据结构 |
| `src/d3d9/shaders/d3d9_fixed_function_vert.vert` | 711行 | 顶点着色器 GLSL |
| `src/d3d9/shaders/d3d9_fixed_function_frag.frag` | 23585 bytes | 片段着色器 GLSL |

---

## 核心功能模块

### 1. 顶点处理管线 (Vertex Pipeline)

#### 1.1 顶点输入映射
**文件**: `src/d3d8/d3d8_shader.cpp`

D3D8 顶点寄存器 → D3D9 语义映射：
```cpp
v0  → POSITION 0
v1  → BLENDWEIGHT 0
v2  → BLENDINDICES 0
v3  → NORMAL 0
v4  → PSIZE 0
v5  → COLOR 0 (Diffuse)
v6  → COLOR 1 (Specular)
v7-v14 → TEXCOORD 0-7
v15 → FOG
v16 → DEPTH
```

#### 1.2 顶点变换和混合
**文件**: `src/d3d9/shaders/d3d9_fixed_function_vert.vert:358-415`

支持三种模式：
- **Standard Mode** (标准模式)
  ```glsl
  vtx = vtx * data.WorldView;
  normal = mat3(data.NormalMatrix) * normal;
  gl_Position = vtx * data.Projection;
  ```

- **Tween Mode** (补间模式)
  ```glsl
  vtx = mix(in_Position0, in_Position1, data.TweenFactor);
  normal = mix(in_Normal0.xyz, in_Normal1.xyz, data.TweenFactor);
  ```

- **Vertex Blending** (顶点混合，最多8个骨骼)
  ```glsl
  for (uint i = 0; i <= vertexBlendCount(); i++) {
      uint arrayIndex = vertexBlendIndexed()
          ? uint(round(in_BlendIndices[i]))
          : i;
      mat4 worldView = WorldViewArray[arrayIndex];
      vtxSum += (vtx * worldView) * weight;
      nrmSum += (normal * mat3(worldView)) * weight;
  }
  ```

#### 1.3 纹理坐标生成
**文件**: `src/d3d9/shaders/d3d9_fixed_function_vert.vert:449-578`

支持5种生成模式：
```glsl
// 1. Pass-through (直通)
DXVK_TSS_TCI_PASSTHRU

// 2. Camera Space Normal (相机空间法线)
DXVK_TSS_TCI_CAMERASPACENORMAL
texCoord = vec4(normal, 1.0);

// 3. Camera Space Position (相机空间位置)
DXVK_TSS_TCI_CAMERASPACEPOSITION
texCoord = vec4(vtx.xyz, 1.0);

// 4. Reflection Vector (反射向量)
DXVK_TSS_TCI_CAMERASPACEREFLECTIONVECTOR
vec3 eyeVec = normalize(vtx.xyz);
vec3 reflection = eyeVec - 2.0 * normal * dot(eyeVec, normal);
texCoord = vec4(reflection, 1.0);

// 5. Sphere Map (球面映射)
DXVK_TSS_TCI_SPHEREMAP
vec3 reflection = /* 同上 */;
float m = 2.0 * sqrt(pow(reflection.x, 2) +
                     pow(reflection.y, 2) +
                     pow(reflection.z + 1.0, 2));
texCoord = vec4(reflection.x / m + 0.5,
                reflection.y / m + 0.5, 0.0, 1.0);
```

纹理坐标变换：
```glsl
// 支持投影纹理坐标
if (transformFlags & D3DTTFF_PROJECTED) {
    texCoord.xyz /= texCoord.w;
}
```

### 2. 光照系统 (Lighting)

**文件**: `src/d3d9/shaders/d3d9_fixed_function_vert.vert:580-702`

#### 2.1 光源类型
```glsl
const uint D3DLIGHT_POINT       = 1;  // 点光源
const uint D3DLIGHT_SPOT        = 2;  // 聚光灯
const uint D3DLIGHT_DIRECTIONAL = 3;  // 平行光
```

#### 2.2 光照计算公式

**点光源衰减**:
```glsl
float d = length(position - vtx.xyz);
float atten = 1.0 / (atten0 + d * atten1 + d * d * atten2);
atten = (d > range) ? 0.0 : atten;
```

**聚光灯衰减**:
```glsl
float rho = dot(-hitDir, direction);
float spotAtten = (rho - phi) / (theta - phi);
spotAtten = pow(spotAtten, falloff);
spotAtten = clamp(spotAtten, 0.0, 1.0);
```

**漫反射 (Lambertian)**:
```glsl
float hitDot = clamp(dot(normal, hitDir), 0.0, 1.0);
vec4 lightDiffuse = diffuse * (hitDot * atten);
```

**镜面反射 (Phong)**:
```glsl
vec3 mid = localViewer()
    ? normalize(hitDir - normalize(vtx.xyz))
    : normalize(hitDir - vec3(0.0, 0.0, 1.0));
float midDot = clamp(dot(normal, mid), 0.0, 1.0);
float specularness = pow(midDot, Material.Power) * atten;
vec4 lightSpecular = specular * specularness;
```

**最终颜色**:
```glsl
vec4 finalColor0 = matEmissive
                 + matAmbient * GlobalAmbient
                 + matAmbient * ambientValue
                 + matDiffuse * diffuseValue;
finalColor0.a = matDiffuse.a;

vec4 finalColor1 = matSpecular * specularValue;
```

#### 2.3 材质颜色源
```glsl
enum D3DMATERIALCOLORSOURCE {
    D3DMCS_MATERIAL = 0,  // 使用材质
    D3DMCS_COLOR1   = 1,  // 使用顶点漫反射色
    D3DMCS_COLOR2   = 2   // 使用顶点镜面反射色
};
```

### 3. 纹理阶段操作 (Texture Stage Operations)

**文件**: `src/d3d9/shaders/d3d9_fixed_function_frag.frag`

#### 3.1 支持的26种纹理操作
```glsl
D3DTOP_DISABLE                   = 1   // 禁用
D3DTOP_SELECTARG1                = 2   // arg1
D3DTOP_SELECTARG2                = 3   // arg2
D3DTOP_MODULATE                  = 4   // arg1 * arg2
D3DTOP_MODULATE2X                = 5   // arg1 * arg2 * 2
D3DTOP_MODULATE4X                = 6   // arg1 * arg2 * 4
D3DTOP_ADD                       = 7   // arg1 + arg2
D3DTOP_ADDSIGNED                 = 8   // arg1 + arg2 - 0.5
D3DTOP_ADDSIGNED2X               = 9   // (arg1 + arg2 - 0.5) * 2
D3DTOP_SUBTRACT                  = 10  // arg1 - arg2
D3DTOP_ADDSMOOTH                 = 11  // arg1 + arg2 - arg1*arg2
D3DTOP_BLENDDIFFUSEALPHA         = 12  // lerp(arg2, arg1, diffuse.a)
D3DTOP_BLENDTEXTUREALPHA         = 13  // lerp(arg2, arg1, texture.a)
D3DTOP_BLENDFACTORALPHA          = 14  // lerp(arg2, arg1, factor.a)
D3DTOP_BLENDTEXTUREALPHAPM       = 15  // arg1 + arg2 * (1-texture.a)
D3DTOP_BLENDCURRENTALPHA         = 16  // lerp(arg2, arg1, current.a)
D3DTOP_PREMODULATE               = 17  // arg1 * prevStage
D3DTOP_MODULATEALPHA_ADDCOLOR    = 18  // arg1.rgb + arg1.a * arg2.rgb
D3DTOP_MODULATECOLOR_ADDALPHA    = 19  // arg1.rgb * arg2.rgb + arg1.a
D3DTOP_MODULATEINVALPHA_ADDCOLOR = 20  // (1-arg1.a) * arg2.rgb + arg1.rgb
D3DTOP_MODULATEINVCOLOR_ADDALPHA = 21  // (1-arg1.rgb) * arg2.rgb + arg1.a
D3DTOP_BUMPENVMAP                = 22  // 环境贴图凹凸映射
D3DTOP_BUMPENVMAPLUMINANCE       = 23  // 带亮度的凹凸映射
D3DTOP_DOTPRODUCT3               = 24  // dot3(arg1, arg2)
D3DTOP_MULTIPLYADD               = 25  // arg1 * arg2 + arg0
D3DTOP_LERP                      = 26  // lerp(arg2, arg1, arg0)
```

#### 3.2 纹理参数源
```glsl
D3DTA_DIFFUSE   = 0  // 顶点漫反射色
D3DTA_CURRENT   = 1  // 当前阶段输出
D3DTA_TEXTURE   = 2  // 当前纹理采样
D3DTA_TFACTOR   = 3  // 纹理因子 (D3DRS_TEXTUREFACTOR)
D3DTA_SPECULAR  = 4  // 顶点镜面反射色
D3DTA_TEMP      = 5  // 临时寄存器
D3DTA_CONSTANT  = 6  // 阶段常量

// 修饰符
D3DTA_COMPLEMENT     = 0x10  // 1 - value
D3DTA_ALPHAREPLICATE = 0x20  // value.aaaa
```

#### 3.3 Bump Mapping 实现
```glsl
// BUMPENVMAP 计算偏移纹理坐标
vec2 bumpCoord = texCoord.xy;
bumpCoord.x += dot(prevColor.xy, bumpEnvMat[0]);
bumpCoord.y += dot(prevColor.xy, bumpEnvMat[1]);
vec4 sampledColor = texture(sampler2D(...), bumpCoord);

// BUMPENVMAPLUMINANCE 应用亮度调制
float luminance = prevColor.z * bumpEnvLScale + bumpEnvLOffset;
sampledColor.rgb *= luminance;
```

### 4. 雾化效果 (Fog)

**文件**: `src/d3d9/shaders/d3d9_fixed_function_vert.vert:245-301` (VS)
**文件**: `src/d3d9/shaders/d3d9_fixed_function_frag.frag:178-241` (PS)

#### 4.1 雾化类型
```glsl
D3DFOG_NONE   = 0  // 使用顶点雾因子
D3DFOG_EXP    = 1  // 指数雾: f = exp(-density * z)
D3DFOG_EXP2   = 2  // 指数平方雾: f = exp(-(density * z)^2)
D3DFOG_LINEAR = 3  // 线性雾: f = (end - z) / (end - start)
```

#### 4.2 Range Fog (距离雾)
```glsl
// 标准雾使用深度 z
float depth = vPos.z / vPos.w;

// Range fog 使用相机距离
if (RangeFog) {
    depth = length(vPos.xyz) / vPos.w;
}
```

### 5. Alpha 测试 (Alpha Test)

**文件**: `src/d3d9/d3d9_fixed_function.cpp`

```glsl
bool alphaTest(float alpha, uint func, uint ref) {
    switch (func) {
        case VK_COMPARE_OP_NEVER:   return false;
        case VK_COMPARE_OP_LESS:    return alpha <  ref;
        case VK_COMPARE_OP_EQUAL:   return alpha == ref;
        case VK_COMPARE_OP_LEQUAL:  return alpha <= ref;
        case VK_COMPARE_OP_GREATER: return alpha >  ref;
        case VK_COMPARE_OP_NOTEQUAL:return alpha != ref;
        case VK_COMPARE_OP_GEQUAL:  return alpha >= ref;
        case VK_COMPARE_OP_ALWAYS:  return true;
    }
}

// 失败则 discard
if (!alphaTest(color.a, alphaFunc, alphaRef)) {
    discard;
}
```

---

## 状态管理机制

### 1. Shader Key 系统

**文件**: `src/d3d9/d3d9_state.h`

#### 1.1 顶点 Shader Key
```cpp
struct D3D9FFShaderKeyVSData {
    uint32_t TexcoordIndices : 24;      // 纹理坐标索引
    uint32_t VertexHasPositionT : 1;    // 预变换顶点
    uint32_t VertexHasColor0 : 1;       // 有漫反射色
    uint32_t VertexHasColor1 : 1;       // 有镜面反射色
    uint32_t VertexHasPointSize : 1;    // 有点大小
    uint32_t UseLighting : 1;           // 启用光照
    uint32_t NormalizeNormals : 1;      // 规范化法线
    uint32_t LocalViewer : 1;           // 局部观察者
    uint32_t RangeFog : 1;              // 距离雾

    uint32_t TexcoordFlags : 24;        // 纹理坐标标志
    uint32_t DiffuseSource : 2;         // 漫反射颜色源
    uint32_t AmbientSource : 2;         // 环境光颜色源
    uint32_t SpecularSource : 2;        // 镜面反射颜色源
    uint32_t EmissiveSource : 2;        // 自发光颜色源

    uint32_t TransformFlags : 24;       // 纹理变换标志
    uint32_t LightCount : 4;            // 光源数量 (0-8)
    uint32_t SpecularEnabled : 1;       // 启用镜面反射

    uint32_t VertexTexcoordDeclMask : 24;
    uint32_t VertexHasFog : 1;
    uint32_t VertexBlendMode : 2;       // 顶点混合模式
    uint32_t VertexBlendIndexed : 1;    // 索引顶点混合
    uint32_t VertexBlendCount : 2;      // 混合数量
    uint32_t VertexClipping : 1;        // 顶点裁剪
};
```

#### 1.2 片段 Shader Key
```cpp
struct D3D9FFShaderStage {
    uint32_t ColorOp   : 5;    // 颜色操作 (D3DTOP_*)
    uint32_t ColorArg0 : 6;    // 颜色参数0
    uint32_t ColorArg1 : 6;    // 颜色参数1
    uint32_t ColorArg2 : 6;    // 颜色参数2
    uint32_t AlphaOp   : 5;    // Alpha 操作
    uint32_t AlphaArg0 : 6;    // Alpha 参数0
    uint32_t AlphaArg1 : 6;    // Alpha 参数1
    uint32_t AlphaArg2 : 6;    // Alpha 参数2
    uint32_t ResultIsTemp : 1; // 结果写入临时寄存器
};

struct D3D9FFShaderKeyFS {
    D3D9FFShaderStage Stages[8];  // 8个纹理阶段
    // + 其他标志位
};
```

### 2. Shader 缓存管理

**文件**: `src/d3d9/d3d9_fixed_function.h:161-208`

```cpp
class D3D9FFShaderModuleSet : public RcObject {
    // 使用哈希表缓存 shader 变体
    std::unordered_map<
        D3D9FFShaderKeyVS,
        D3D9FFShader,
        D3D9FFShaderKeyHash,
        D3D9FFShaderKeyEq> m_vsModules;

    std::unordered_map<
        D3D9FFShaderKeyFS,
        D3D9FFShader,
        D3D9FFShaderKeyHash,
        D3D9FFShaderKeyEq> m_fsModules;

    // Ubershader (超级着色器) - 用于所有状态组合
    D3D9FFShader m_vsUbershader;
    D3D9FFShader m_fsUbershader;
};
```

### 3. 数据传递机制

#### 3.1 Uniform Buffer Objects (UBO)
```glsl
// 顶点着色器常量
layout(set = 0, binding = 4) uniform ShaderData {
    mat4 WorldView;
    mat4 NormalMatrix;
    mat4 InverseView;
    mat4 Projection;
    mat4 TexcoordMatrices[8];
    D3D9ViewportInfo ViewportInfo;
    vec4 GlobalAmbient;
    D3D9Light Lights[8];
    D3DMATERIAL9 Material;
    float TweenFactor;
    // ...
} data;
```

#### 3.2 Push Constants
```glsl
layout(push_constant) uniform RenderStates {
    vec3 fogColor;
    float fogScale;
    float fogEnd;
    float fogDensity;
    uint alphaRef;
    float pointSize;
    float pointSizeMin;
    float pointSizeMax;
    // ...
} rs;
```

#### 3.3 Specialization Constants
用于编译时优化的特化常量：
- 光源数量
- 启用的纹理阶段
- 雾化模式
- Alpha 测试函数
- 纹理操作类型

---

## Shader动态生成

### 1. 生成触发时机

**文件**: `src/d3d9/d3d9_fixed_function.cpp`

当渲染状态改变时：
```cpp
// 1. 计算当前状态的 Shader Key
D3D9FFShaderKeyVS vsKey = ComputeVertexShaderKey(state);
D3D9FFShaderKeyFS fsKey = ComputeFragmentShaderKey(state);

// 2. 在缓存中查找
D3D9FFShader vs = m_ffModules->GetShaderModule(device, vsKey);
D3D9FFShader fs = m_ffModules->GetShaderModule(device, fsKey);

// 3. 如果未找到，动态生成并编译
if (!vs.exists()) {
    vs = CompileVertexShader(vsKey);
    m_ffModules->m_vsModules.insert({vsKey, vs});
}
```

### 2. Ubershader 策略

Ubershader 是包含所有可能代码路径的超级着色器：
```glsl
// 使用运行时分支处理所有状态
if (specUint(LightCount) > 0) {
    for (uint i = 0; i < specUint(LightCount); i++) {
        // 光照计算
    }
}

if (specBool(UseLighting)) {
    // 启用光照路径
} else {
    // 直接使用顶点颜色
}
```

优缺点：
- **优点**: 零编译延迟，状态切换快
- **缺点**: 性能较低（分支开销），占用更多寄存器

### 3. Specialized Shader 策略

为特定状态组合生成优化的着色器：
```glsl
// 编译时已知有2个光源
for (uint i = 0; i < 2; i++) {
    // 光照计算（循环展开）
}

// 编译时已知启用光照
// 直接移除 if 分支
```

优缺点：
- **优点**: 最优性能（无分支），更少寄存器使用
- **缺点**: 首次使用编译延迟，可能产生大量变体

---

## bgfx实现方案

### 方案概述

bgfx 是一个跨平台的低层渲染 API 抽象层，支持 Direct3D 9/11/12、OpenGL、Vulkan、Metal 等后端。以下是使用 bgfx 重新实现 DX8 固定管线的完整方案。

### 架构设计

```
┌────────────────────────────────────────────────┐
│         DX8 Fixed Pipeline API                 │
│  - SetTransform(), SetLight(), SetMaterial()  │
│  - SetTexture(), SetTextureStageState()       │
│  - SetRenderState()                            │
└────────────────┬───────────────────────────────┘
                 ↓
┌────────────────────────────────────────────────┐
│      State Tracking & Shader Key Builder      │
│  - 跟踪所有渲染状态变化                         │
│  - 生成 Shader Key 哈希                        │
└────────────────┬───────────────────────────────┘
                 ↓
┌────────────────────────────────────────────────┐
│         Shader Cache Manager                   │
│  - 查找已编译的 Shader 变体                     │
│  - 触发新 Shader 编译 (如果需要)                │
└────────────────┬───────────────────────────────┘
                 ↓
┌────────────────────────────────────────────────┐
│      Shader Compiler (shaderc)                │
│  - 将 GLSL/HLSL 编译为 bgfx shader             │
│  - 适配不同后端 (DX11, GL, Vulkan, Metal)      │
└────────────────┬───────────────────────────────┘
                 ↓
┌────────────────────────────────────────────────┐
│              bgfx API                          │
│  - bgfx::setUniform()                          │
│  - bgfx::setVertexBuffer()                     │
│  - bgfx::setState()                            │
│  - bgfx::submit()                              │
└────────────────────────────────────────────────┘
```

---

## 实现路线图

### 阶段1: 核心框架 (2-3周)

#### 1.1 状态管理器
```cpp
// dx8_state_manager.h
class DX8StateManager {
public:
    // 变换矩阵
    void SetTransform(D3DTRANSFORMSTATETYPE type, const D3DMATRIX* matrix);

    // 光照
    void SetLight(DWORD index, const D3DLIGHT8* light);
    void LightEnable(DWORD index, BOOL enable);
    void SetMaterial(const D3DMATERIAL8* material);

    // 纹理阶段
    void SetTexture(DWORD stage, IDirect3DBaseTexture8* texture);
    void SetTextureStageState(DWORD stage, D3DTEXTURESTAGESTATETYPE type, DWORD value);

    // 渲染状态
    void SetRenderState(D3DRENDERSTATETYPE state, DWORD value);

    // 生成 Shader Key
    ShaderKey GetVertexShaderKey() const;
    ShaderKey GetFragmentShaderKey() const;

private:
    // 状态存储
    D3DMATRIX m_transforms[256];
    D3DLIGHT8 m_lights[8];
    bool m_lightEnabled[8];
    D3DMATERIAL8 m_material;
    TextureStageState m_textureStages[8];
    RenderState m_renderStates;
};
```

#### 1.2 Shader Key 结构
```cpp
// shader_key.h
struct VertexShaderKey {
    uint64_t hash;

    struct {
        uint8_t lightCount : 4;
        uint8_t useLighting : 1;
        uint8_t normalizeNormals : 1;
        uint8_t localViewer : 1;
        uint8_t rangeFog : 1;
        uint8_t vertexBlendMode : 2;
        uint8_t vertexBlendCount : 3;
        uint8_t hasColor0 : 1;
        uint8_t hasColor1 : 1;
        uint8_t hasPositionT : 1;
        uint8_t specularEnabled : 1;
        uint8_t fogMode : 2;
        // ... 更多标志
    } flags;

    uint32_t texcoordGenModes[8];
    uint32_t texcoordTransformFlags[8];
};

struct FragmentShaderKey {
    uint64_t hash;

    struct TextureStage {
        uint8_t colorOp : 5;
        uint8_t colorArg0 : 6;
        uint8_t colorArg1 : 6;
        uint8_t colorArg2 : 6;
        uint8_t alphaOp : 5;
        uint8_t alphaArg0 : 6;
        uint8_t alphaArg1 : 6;
        uint8_t alphaArg2 : 6;
    } stages[8];

    struct {
        uint8_t alphaTestEnabled : 1;
        uint8_t alphaTestFunc : 3;
        uint8_t fogEnabled : 1;
        uint8_t fogMode : 2;
    } flags;
};
```

### 阶段2: Shader 生成器 (3-4周)

#### 2.1 GLSL 代码生成
```cpp
// shader_generator.h
class ShaderGenerator {
public:
    std::string GenerateVertexShader(const VertexShaderKey& key);
    std::string GenerateFragmentShader(const FragmentShaderKey& key);

private:
    // 顶点着色器模块
    void EmitVertexInputs(const VertexShaderKey& key);
    void EmitVertexTransform(const VertexShaderKey& key);
    void EmitVertexBlending(const VertexShaderKey& key);
    void EmitLighting(const VertexShaderKey& key);
    void EmitTexCoordGen(const VertexShaderKey& key);
    void EmitFog(const VertexShaderKey& key);

    // 片段着色器模块
    void EmitTextureStage(const FragmentShaderKey& key, int stage);
    void EmitAlphaTest(const FragmentShaderKey& key);
    void EmitFogBlend(const FragmentShaderKey& key);

    std::stringstream m_code;
};
```

#### 2.2 示例：生成光照代码
```cpp
void ShaderGenerator::EmitLighting(const VertexShaderKey& key) {
    if (!key.flags.useLighting) {
        m_code << "    out_Color0 = in_Color0;\n";
        m_code << "    out_Color1 = in_Color1;\n";
        return;
    }

    m_code << "    vec3 diffuseAccum = vec3(0.0);\n";
    m_code << "    vec3 specularAccum = vec3(0.0);\n";
    m_code << "    vec3 ambientAccum = vec3(0.0);\n";
    m_code << "\n";

    for (int i = 0; i < key.flags.lightCount; i++) {
        m_code << "    // Light " << i << "\n";
        m_code << "    {\n";
        m_code << "        vec3 lightDir;\n";
        m_code << "        float attenuation = 1.0;\n";
        m_code << "\n";

        // 光源类型判断
        m_code << "        if (u_Lights[" << i << "].type == LIGHT_DIRECTIONAL) {\n";
        m_code << "            lightDir = -u_Lights[" << i << "].direction;\n";
        m_code << "        } else {\n";
        m_code << "            vec3 lightVec = u_Lights[" << i << "].position - v_ViewPos.xyz;\n";
        m_code << "            float dist = length(lightVec);\n";
        m_code << "            lightDir = lightVec / dist;\n";
        m_code << "            attenuation = 1.0 / (u_Lights[" << i << "].atten0 + \n";
        m_code << "                                  u_Lights[" << i << "].atten1 * dist + \n";
        m_code << "                                  u_Lights[" << i << "].atten2 * dist * dist);\n";

        // 聚光灯计算
        m_code << "            if (u_Lights[" << i << "].type == LIGHT_SPOT) {\n";
        m_code << "                float rho = dot(-lightDir, u_Lights[" << i << "].direction);\n";
        m_code << "                float spotFactor = (rho - u_Lights[" << i << "].phi) / \n";
        m_code << "                                   (u_Lights[" << i << "].theta - u_Lights[" << i << "].phi);\n";
        m_code << "                spotFactor = pow(clamp(spotFactor, 0.0, 1.0), u_Lights[" << i << "].falloff);\n";
        m_code << "                attenuation *= spotFactor;\n";
        m_code << "            }\n";
        m_code << "        }\n";
        m_code << "\n";

        // 漫反射
        m_code << "        float NdotL = max(dot(v_Normal, lightDir), 0.0);\n";
        m_code << "        diffuseAccum += u_Lights[" << i << "].diffuse.rgb * NdotL * attenuation;\n";
        m_code << "\n";

        // 镜面反射
        if (key.flags.specularEnabled) {
            if (key.flags.localViewer) {
                m_code << "        vec3 viewDir = normalize(v_ViewPos.xyz);\n";
            } else {
                m_code << "        vec3 viewDir = vec3(0.0, 0.0, 1.0);\n";
            }
            m_code << "        vec3 halfVec = normalize(lightDir + viewDir);\n";
            m_code << "        float NdotH = max(dot(v_Normal, halfVec), 0.0);\n";
            m_code << "        if (NdotL > 0.0 && NdotH > 0.0) {\n";
            m_code << "            specularAccum += u_Lights[" << i << "].specular.rgb * \n";
            m_code << "                             pow(NdotH, u_Material.power) * attenuation;\n";
            m_code << "        }\n";
        }

        m_code << "    }\n";
    }

    // 最终颜色计算
    m_code << "\n";
    m_code << "    vec3 ambient = u_Material.ambient.rgb * u_GlobalAmbient.rgb;\n";
    m_code << "    vec3 emissive = u_Material.emissive.rgb;\n";
    m_code << "    out_Color0.rgb = emissive + ambient + u_Material.diffuse.rgb * diffuseAccum;\n";
    m_code << "    out_Color0.a = u_Material.diffuse.a;\n";

    if (key.flags.specularEnabled) {
        m_code << "    out_Color1.rgb = u_Material.specular.rgb * specularAccum;\n";
        m_code << "    out_Color1.a = 1.0;\n";
    }
}
```

### 阶段3: bgfx 集成 (2-3周)

#### 3.1 Shader 编译和缓存
```cpp
// bgfx_shader_cache.h
class BGFXShaderCache {
public:
    bgfx::ProgramHandle GetOrCompileShader(
        const VertexShaderKey& vsKey,
        const FragmentShaderKey& fsKey);

private:
    struct ShaderProgram {
        bgfx::VertexShaderHandle vs;
        bgfx::FragmentShaderHandle fs;
        bgfx::ProgramHandle program;
    };

    // 使用 Key 哈希作为缓存键
    std::unordered_map<uint64_t, ShaderProgram> m_cache;

    bgfx::ShaderHandle CompileShader(
        const std::string& code,
        bgfx::ShaderStage stage);
};
```

#### 3.2 Uniform 管理
```cpp
// uniform_manager.h
class UniformManager {
public:
    void UpdateUniforms(const DX8StateManager& state);

private:
    // bgfx uniform handles
    bgfx::UniformHandle u_WorldViewProj;
    bgfx::UniformHandle u_WorldView;
    bgfx::UniformHandle u_NormalMatrix;
    bgfx::UniformHandle u_TexcoordMatrices;

    bgfx::UniformHandle u_Material;
    bgfx::UniformHandle u_Lights;
    bgfx::UniformHandle u_GlobalAmbient;

    bgfx::UniformHandle u_FogColor;
    bgfx::UniformHandle u_FogParams;

    bgfx::UniformHandle u_TextureFactor;
    bgfx::UniformHandle u_BumpEnvMat;

    bgfx::UniformHandle s_Textures[8];
};
```

#### 3.3 渲染循环
```cpp
void DX8Renderer::DrawPrimitive(
    D3DPRIMITIVETYPE primitiveType,
    UINT startVertex,
    UINT primitiveCount)
{
    // 1. 获取当前渲染状态
    VertexShaderKey vsKey = m_stateManager.GetVertexShaderKey();
    FragmentShaderKey fsKey = m_stateManager.GetFragmentShaderKey();

    // 2. 获取或编译 Shader
    bgfx::ProgramHandle program = m_shaderCache.GetOrCompileShader(vsKey, fsKey);

    // 3. 更新 Uniforms
    m_uniformManager.UpdateUniforms(m_stateManager);

    // 4. 设置顶点缓冲区
    bgfx::setVertexBuffer(0, m_currentVB);

    // 5. 设置渲染状态
    uint64_t state = BGFX_STATE_WRITE_RGB
                   | BGFX_STATE_WRITE_A
                   | BGFX_STATE_WRITE_Z;

    if (m_stateManager.GetRenderState(D3DRS_ALPHABLENDENABLE)) {
        state |= BGFX_STATE_BLEND_FUNC(
            TranslateBlendOp(m_stateManager.GetRenderState(D3DRS_SRCBLEND)),
            TranslateBlendOp(m_stateManager.GetRenderState(D3DRS_DESTBLEND)));
    }

    bgfx::setState(state);

    // 6. 提交绘制
    bgfx::submit(0, program);
}
```

### 阶段4: 纹理阶段实现 (3-4周)

#### 4.1 纹理阶段代码生成
```cpp
void ShaderGenerator::EmitTextureStage(const FragmentShaderKey& key, int stage) {
    const auto& s = key.stages[stage];

    if (s.colorOp == D3DTOP_DISABLE) {
        return;
    }

    m_code << "    // Texture Stage " << stage << "\n";

    // 采样纹理
    m_code << "    vec4 texColor" << stage << " = texture(s_Texture" << stage << ", v_TexCoord" << stage << ".xy);\n";

    // 获取参数
    m_code << "    vec4 colorArg0 = " << GetTextureArg(s.colorArg0, stage) << ";\n";
    m_code << "    vec4 colorArg1 = " << GetTextureArg(s.colorArg1, stage) << ";\n";
    m_code << "    vec4 colorArg2 = " << GetTextureArg(s.colorArg2, stage) << ";\n";

    // 执行操作
    m_code << "    vec3 colorResult;\n";
    switch (s.colorOp) {
        case D3DTOP_SELECTARG1:
            m_code << "    colorResult = colorArg1.rgb;\n";
            break;
        case D3DTOP_MODULATE:
            m_code << "    colorResult = colorArg1.rgb * colorArg2.rgb;\n";
            break;
        case D3DTOP_ADD:
            m_code << "    colorResult = colorArg1.rgb + colorArg2.rgb;\n";
            break;
        case D3DTOP_DOTPRODUCT3:
            m_code << "    float dp3 = dot(colorArg1.rgb - 0.5, colorArg2.rgb - 0.5);\n";
            m_code << "    colorResult = vec3(dp3 * 4.0);\n";
            break;
        // ... 实现所有26种操作
    }

    // Alpha 通道同理
    // ...

    // 写入当前或临时寄存器
    if (key.stages[stage].resultIsTemp) {
        m_code << "    tempReg = vec4(colorResult, alphaResult);\n";
    } else {
        m_code << "    currentColor = vec4(colorResult, alphaResult);\n";
    }
}

std::string ShaderGenerator::GetTextureArg(uint8_t arg, int stage) {
    uint8_t source = arg & D3DTA_SELECTMASK;
    bool complement = arg & D3DTA_COMPLEMENT;
    bool alphaReplicate = arg & D3DTA_ALPHAREPLICATE;

    std::string result;
    switch (source) {
        case D3DTA_DIFFUSE:   result = "v_Color0"; break;
        case D3DTA_CURRENT:   result = "currentColor"; break;
        case D3DTA_TEXTURE:   result = "texColor" + std::to_string(stage); break;
        case D3DTA_TFACTOR:   result = "u_TextureFactor"; break;
        case D3DTA_SPECULAR:  result = "v_Color1"; break;
        case D3DTA_TEMP:      result = "tempReg"; break;
        case D3DTA_CONSTANT:  result = "u_StageConstant" + std::to_string(stage); break;
    }

    if (alphaReplicate) {
        result += ".aaaa";
    }

    if (complement) {
        result = "vec4(1.0) - " + result;
    }

    return result;
}
```

### 阶段5: 优化和测试 (2-3周)

#### 5.1 性能优化
1. **Shader 预编译**: 离线编译常用的 Shader 变体
2. **异步编译**: 后台线程编译新 Shader，避免卡顿
3. **LRU 缓存**: 限制缓存大小，淘汰最少使用的 Shader
4. **Ubershader 回退**: 编译时使用 Ubershader 避免延迟

```cpp
class AsyncShaderCompiler {
    std::thread m_compileThread;
    std::queue<CompileRequest> m_requests;
    std::mutex m_mutex;

    void CompileWorker() {
        while (m_running) {
            CompileRequest req;
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                if (m_requests.empty()) continue;
                req = m_requests.front();
                m_requests.pop();
            }

            auto program = CompileShader(req.vsKey, req.fsKey);
            m_cache.Insert(req.hash, program);
        }
    }
};
```

#### 5.2 测试套件
1. **单元测试**: 测试每个纹理操作、光照模式
2. **集成测试**: 使用实际游戏场景
3. **参考渲染**: 与 D3D8 原生渲染结果对比
4. **性能测试**: 帧率、编译时间、内存占用

```cpp
// 参考渲染对比
void TestTextureOp(D3DTEXTUREOP op) {
    // 1. 使用真实 D3D8 渲染
    RenderWithD3D8(op);
    SaveFramebuffer("d3d8_reference.png");

    // 2. 使用 bgfx 实现渲染
    RenderWithBGFX(op);
    SaveFramebuffer("bgfx_result.png");

    // 3. 像素级对比
    float mse = CompareImages("d3d8_reference.png", "bgfx_result.png");
    EXPECT_LT(mse, 0.01f);  // MSE < 1%
}
```

---

## 关键实现细节

### 1. 坐标系统转换

DX8 使用左手坐标系，OpenGL/Vulkan 使用右手坐标系：
```glsl
// 在投影矩阵中处理
mat4 proj = u_Projection;
#ifdef BGFX_SHADER_LANGUAGE_GLSL
    proj[2][2] = -proj[2][2];  // 翻转 Z
    proj[3][2] = -proj[3][2];
#endif
```

### 2. 预变换顶点 (Position T)

DX8 支持预变换顶点（已经在屏幕空间）：
```glsl
if (u_HasPositionT) {
    // 屏幕坐标 → NDC
    gl_Position.xy = in_Position.xy * u_ViewportInvExtent.xy + u_ViewportInvOffset.xy;
    gl_Position.z = in_Position.z;
    gl_Position.w = 1.0 / in_Position.w;  // RHW → W
    gl_Position.xyz *= gl_Position.w;      // 透视校正
} else {
    // 标准变换管线
    gl_Position = in_Position * u_WorldViewProj;
}
```

### 3. 颜色精度

DX8 使用 D3DCOLOR (ARGB8888)，需要正确处理：
```cpp
// D3DCOLOR → vec4
vec4 UnpackD3DCOLOR(uint32_t color) {
    return vec4(
        ((color >> 16) & 0xFF) / 255.0f,  // R
        ((color >> 8)  & 0xFF) / 255.0f,  // G
        ((color >> 0)  & 0xFF) / 255.0f,  // B
        ((color >> 24) & 0xFF) / 255.0f   // A
    );
}
```

### 4. Alpha 测试精度

DX8 使用 8-bit alpha 参考值：
```glsl
// 转换为 [0, 255] 范围进行比较
float alpha8 = floor(color.a * 255.0 + 0.5);
float ref8 = float(u_AlphaRef);
if (alpha8 < ref8) discard;  // D3DCMP_LESS
```

---

## 完整代码示例

### 示例：完整的顶点着色器生成
```glsl
#version 450

// 输入
layout(location = 0) in vec3 in_Position;
layout(location = 1) in vec3 in_Normal;
layout(location = 2) in vec4 in_Color0;
layout(location = 3) in vec4 in_Color1;
layout(location = 4) in vec2 in_TexCoord0;
layout(location = 5) in vec2 in_TexCoord1;
layout(location = 6) in vec4 in_BlendWeights;
layout(location = 7) in uvec4 in_BlendIndices;

// 输出
layout(location = 0) out vec4 v_Color0;
layout(location = 1) out vec4 v_Color1;
layout(location = 2) out vec2 v_TexCoord0;
layout(location = 3) out vec2 v_TexCoord1;
layout(location = 4) out vec3 v_ViewPos;
layout(location = 5) out vec3 v_Normal;
layout(location = 6) out float v_Fog;

// Uniforms
uniform mat4 u_WorldViewProj;
uniform mat4 u_WorldView;
uniform mat4 u_NormalMatrix;
uniform mat4 u_TexCoordMatrix[8];

struct Light {
    vec4 diffuse;
    vec4 specular;
    vec4 ambient;
    vec3 position;
    vec3 direction;
    uint type;
    float range;
    float falloff;
    float atten0;
    float atten1;
    float atten2;
    float theta;
    float phi;
};

uniform Light u_Lights[8];
uniform vec4 u_GlobalAmbient;

struct Material {
    vec4 diffuse;
    vec4 ambient;
    vec4 specular;
    vec4 emissive;
    float power;
};

uniform Material u_Material;

// 配置（Specialization Constants）
const bool USE_LIGHTING = true;
const int LIGHT_COUNT = 2;
const bool NORMALIZE_NORMALS = true;
const bool LOCAL_VIEWER = false;
const int VERTEX_BLEND_MODE = 0;  // 0=None, 1=Normal, 2=Tween

void main() {
    vec4 position = vec4(in_Position, 1.0);
    vec3 normal = in_Normal;

    // 顶点混合
    if (VERTEX_BLEND_MODE == 1) {
        // 硬件顶点混合
        mat4 blendMatrix = mat4(0.0);
        for (int i = 0; i < 4; i++) {
            blendMatrix += u_WorldView * in_BlendWeights[i];
        }
        position = position * blendMatrix;
        normal = mat3(blendMatrix) * normal;
    } else {
        position = position * u_WorldView;
        normal = mat3(u_NormalMatrix) * normal;
    }

    // 法线规范化
    if (NORMALIZE_NORMALS) {
        normal = normalize(normal);
    }

    v_ViewPos = position.xyz;
    v_Normal = normal;

    // 光照计算
    if (USE_LIGHTING) {
        vec3 diffuseAccum = vec3(0.0);
        vec3 specularAccum = vec3(0.0);
        vec3 ambientAccum = vec3(0.0);

        for (int i = 0; i < LIGHT_COUNT; i++) {
            Light light = u_Lights[i];

            vec3 lightDir;
            float attenuation = 1.0;

            if (light.type == 3) {  // Directional
                lightDir = -light.direction;
            } else {
                vec3 lightVec = light.position - position.xyz;
                float dist = length(lightVec);
                lightDir = lightVec / dist;

                attenuation = 1.0 / (light.atten0 +
                                     light.atten1 * dist +
                                     light.atten2 * dist * dist);

                if (light.type == 2) {  // Spot
                    float rho = dot(-lightDir, light.direction);
                    float spotFactor = (rho - light.phi) / (light.theta - light.phi);
                    spotFactor = pow(clamp(spotFactor, 0.0, 1.0), light.falloff);
                    attenuation *= spotFactor;
                }
            }

            float NdotL = max(dot(normal, lightDir), 0.0);
            diffuseAccum += light.diffuse.rgb * NdotL * attenuation;
            ambientAccum += light.ambient.rgb * attenuation;

            // 镜面反射
            vec3 viewDir = LOCAL_VIEWER ? normalize(position.xyz) : vec3(0.0, 0.0, 1.0);
            vec3 halfVec = normalize(lightDir + viewDir);
            float NdotH = max(dot(normal, halfVec), 0.0);
            if (NdotL > 0.0) {
                specularAccum += light.specular.rgb * pow(NdotH, u_Material.power) * attenuation;
            }
        }

        v_Color0.rgb = u_Material.emissive.rgb +
                       u_Material.ambient.rgb * u_GlobalAmbient.rgb +
                       u_Material.ambient.rgb * ambientAccum +
                       u_Material.diffuse.rgb * diffuseAccum;
        v_Color0.a = u_Material.diffuse.a;
        v_Color1 = vec4(u_Material.specular.rgb * specularAccum, 1.0);
    } else {
        v_Color0 = in_Color0;
        v_Color1 = in_Color1;
    }

    // 纹理坐标变换
    v_TexCoord0 = (vec4(in_TexCoord0, 0.0, 1.0) * u_TexCoordMatrix[0]).xy;
    v_TexCoord1 = (vec4(in_TexCoord1, 0.0, 1.0) * u_TexCoordMatrix[1]).xy;

    // 雾化（顶点雾）
    float fogDist = length(position.xyz);
    v_Fog = fogDist;

    gl_Position = position * u_WorldViewProj;
}
```

---

## 性能考虑

### Shader 变体爆炸问题
理论上的 Shader 变体数量：
```
顶点着色器变体 =
    光源数量 (0-8) *
    顶点格式组合 (32种) *
    混合模式 (3种) *
    纹理坐标生成 (5^8 种) *
    雾化模式 (4种) *
    其他标志 (2^10)
    ≈ 数百万种

片段着色器变体 =
    纹理操作 (26^8 种) *
    纹理参数 (7^24 种) *
    雾化模式 (4种)
    ≈ 数十亿种
```

### 变体数量优化策略

1. **合并不常用状态**：使用动态分支处理罕见状态
2. **限制缓存大小**：只缓存最常用的 1000-5000 个变体
3. **预编译热点**：分析游戏使用模式，预编译常用组合
4. **Ubershader 回退**：首次遇到新状态时使用 Ubershader

---

## 参考资料

### Microsoft DirectX 8 文档
- Direct3D 8 Fixed Function Pipeline Specification
- D3D8 Texture Blending Operations Reference
- D3D8 Lighting and Materials Guide

### DXVK 相关
- DXVK GitHub: https://github.com/doitsujin/dxvk
- DXVK Fixed Function Implementation (本分析基于此)

### bgfx 相关
- bgfx GitHub: https://github.com/bkaradzic/bgfx
- bgfx Examples: https://github.com/bkaradzic/bgfx/tree/master/examples
- shaderc (bgfx shader compiler)

### OpenGL 固定管线
- OpenGL 1.x Fixed Function Reference (兼容参考)
- OpenGL ES 1.1 Specification

---

## 总结

本文档详细分析了 DXVK 中 DX8 固定管线的实现，并提供了使用 bgfx 重新实现的完整方案。关键要点：

1. **状态驱动**: 固定管线本质上是状态机，通过状态组合动态生成 Shader
2. **Shader 缓存**: 使用哈希表缓存已编译的 Shader 变体，避免重复编译
3. **模块化设计**: 将光照、纹理、雾化等功能模块化，便于生成和维护
4. **性能优化**: 通过 Ubershader、异步编译、预编译等策略平衡编译延迟和运行性能
5. **跨平台**: bgfx 提供统一的抽象层，自动适配不同图形 API

实现这样的渲染器需要 12-16 周的开发时间，但可以获得与原生 DX8 完全一致的渲染效果，同时支持现代图形 API 和跨平台运行。
