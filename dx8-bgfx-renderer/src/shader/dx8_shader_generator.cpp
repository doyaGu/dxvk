#include "dx8bgfx/dx8_shader_generator.h"
#include "dx8bgfx/dx8_constants.h"

namespace dx8bgfx {

// =============================================================================
// Vertex Shader Generation
// =============================================================================

std::string ShaderGenerator::GenerateVertexShader(const VertexShaderKey& key) {
    m_code.str("");
    m_code.clear();

    EmitVSHeader();
    EmitVSInputs(key);
    EmitVSOutputs(key);
    EmitVSUniforms(key);
    EmitVSHelpers(key);
    EmitVSMain(key);

    return m_code.str();
}

void ShaderGenerator::EmitVSHeader() {
    Line("$input a_position, a_normal, a_color0, a_color1, a_texcoord0, a_texcoord1, a_texcoord2, a_texcoord3, a_texcoord4, a_texcoord5, a_texcoord6, a_texcoord7, a_weight, a_indices");
    Line("$output v_color0, v_color1, v_normal, v_texcoord0, v_texcoord1, v_texcoord2, v_texcoord3, v_texcoord4, v_texcoord5, v_texcoord6, v_texcoord7, v_viewPos, v_fog");
    Line();
    Line("/*");
    Line(" * DX8 Fixed Function Vertex Shader - Generated");
    Line(" */");
    Line();
    Line("#include <bgfx_shader.sh>");
    Line();
}

void ShaderGenerator::EmitVSInputs(const VertexShaderKey& key) {
    // Inputs are defined in varying.def.sc
}

void ShaderGenerator::EmitVSOutputs(const VertexShaderKey& key) {
    // Outputs are defined in varying.def.sc
}

void ShaderGenerator::EmitVSUniforms(const VertexShaderKey& key) {
    Line("// Transform matrices");
    Line("uniform mat4 u_worldView;");
    Line("uniform mat4 u_worldViewProj;");
    Line("uniform mat4 u_normalMatrix;");
    Line("uniform mat4 u_invView;");
    Line();

    // Texture matrices - only emit what we need
    for (int i = 0; i < 8; i++) {
        uint32_t transformFlags = (key.Data.bits.TransformFlags >> (i * 3)) & 0x7;
        if (transformFlags != D3DTTFF_DISABLE) {
            m_code << "uniform mat4 u_texMatrix" << i << ";\n";
        }
    }
    Line();

    // Lighting uniforms
    if (key.Data.bits.UseLighting) {
        Line("// Material");
        Line("uniform vec4 u_materialDiffuse;");
        Line("uniform vec4 u_materialAmbient;");
        Line("uniform vec4 u_materialSpecular;");
        Line("uniform vec4 u_materialEmissive;");
        Line("uniform vec4 u_materialPower;");
        Line();
        Line("// Global ambient");
        Line("uniform vec4 u_globalAmbient;");
        Line();

        // Light uniforms - only emit enabled lights
        uint32_t lightCount = key.Data.bits.LightCount;
        for (uint32_t i = 0; i < lightCount; i++) {
            m_code << "// Light " << i << "\n";
            m_code << "uniform vec4 u_light" << i << "Diffuse;\n";
            m_code << "uniform vec4 u_light" << i << "Specular;\n";
            m_code << "uniform vec4 u_light" << i << "Ambient;\n";
            m_code << "uniform vec4 u_light" << i << "Position;\n";
            m_code << "uniform vec4 u_light" << i << "Direction;\n";
            m_code << "uniform vec4 u_light" << i << "Attenuation;\n";
            m_code << "uniform vec4 u_light" << i << "SpotParams;\n";
            Line();
        }
    }

    // Fog parameters
    if (key.Data.bits.FogMode != D3DFOG_NONE) {
        Line("// Fog");
        Line("uniform vec4 u_fogParams;");
        Line();
    }

    // Vertex blending
    if (key.Data.bits.VertexBlendMode == 1) {
        Line("// Vertex blend matrices");
        m_code << "uniform mat4 u_blendMatrices[" << (key.Data.bits.VertexBlendCount + 1) << "];\n";
        Line();
    } else if (key.Data.bits.VertexBlendMode == 2) {
        Line("// Tween");
        Line("uniform vec4 u_tweenFactor;");
        Line();
    }

    // Viewport info for Position T
    if (key.Data.bits.HasPositionT) {
        Line("// Viewport for pre-transformed vertices");
        Line("uniform vec4 u_viewportInvOffset;");
        Line("uniform vec4 u_viewportInvExtent;");
        Line();
    }
}

void ShaderGenerator::EmitVSHelpers(const VertexShaderKey& key) {
    Line("// =============================================================================");
    Line("// Constants");
    Line("// =============================================================================");
    Line();
    Line("#define LIGHT_POINT       1.0");
    Line("#define LIGHT_SPOT        2.0");
    Line("#define LIGHT_DIRECTIONAL 3.0");
    Line();

    if (key.Data.bits.UseLighting && key.Data.bits.LightCount > 0) {
        Line("// =============================================================================");
        Line("// Light Calculation");
        Line("// =============================================================================");
        Line();
        Line("void computeLight(");
        Line("    vec3 position,");
        Line("    vec3 normal,");
        Line("    vec4 lightDiffuse,");
        Line("    vec4 lightSpecular,");
        Line("    vec4 lightAmbient,");
        Line("    vec4 lightPosition,");
        Line("    vec4 lightDirection,");
        Line("    vec4 lightAttenuation,");
        Line("    vec4 lightSpotParams,");
        Line("    float materialPower,");
        Line("    bool localViewer,");
        Line("    inout vec3 diffuseAccum,");
        Line("    inout vec3 specularAccum,");
        Line("    inout vec3 ambientAccum");
        Line(") {");
        Line("    float lightType = lightPosition.w;");
        Line("    float range = lightDirection.w;");
        Line("    vec3 lightPos = lightPosition.xyz;");
        Line("    vec3 lightDir = lightDirection.xyz;");
        Line();
        Line("    vec3 L;");
        Line("    float attenuation = 1.0;");
        Line();
        Line("    if (lightType == LIGHT_DIRECTIONAL) {");
        Line("        L = -lightDir;");
        Line("    } else {");
        Line("        vec3 lightVec = lightPos - position;");
        Line("        float dist = length(lightVec);");
        Line("        L = lightVec / max(dist, 0.0001);");
        Line();
        Line("        float atten0 = lightAttenuation.x;");
        Line("        float atten1 = lightAttenuation.y;");
        Line("        float atten2 = lightAttenuation.z;");
        Line("        attenuation = 1.0 / (atten0 + atten1 * dist + atten2 * dist * dist);");
        Line("        attenuation = dist > range ? 0.0 : attenuation;");
        Line();
        Line("        if (lightType == LIGHT_SPOT) {");
        Line("            float rho = dot(-L, lightDir);");
        Line("            float theta = lightSpotParams.x;");
        Line("            float phi = lightSpotParams.y;");
        Line("            float falloff = lightAttenuation.w;");
        Line("            float spotFactor = clamp((rho - phi) / (theta - phi), 0.0, 1.0);");
        Line("            spotFactor = pow(spotFactor, falloff);");
        Line("            attenuation *= spotFactor;");
        Line("        }");
        Line("    }");
        Line();
        Line("    // Diffuse");
        Line("    float NdotL = max(dot(normal, L), 0.0);");
        Line("    diffuseAccum += lightDiffuse.rgb * NdotL * attenuation;");
        Line("    ambientAccum += lightAmbient.rgb * attenuation;");
        Line();

        if (key.Data.bits.SpecularEnabled) {
            Line("    // Specular");
            Line("    if (NdotL > 0.0 && materialPower > 0.0) {");
            if (key.Data.bits.LocalViewer) {
                Line("        vec3 V = normalize(position);");
            } else {
                Line("        vec3 V = vec3(0.0, 0.0, 1.0);");
            }
            Line("        vec3 H = normalize(L + V);");
            Line("        float NdotH = max(dot(normal, H), 0.0);");
            Line("        specularAccum += lightSpecular.rgb * pow(NdotH, materialPower) * attenuation;");
            Line("    }");
        }

        Line("}");
        Line();
    }

    // Fog helper
    if (key.Data.bits.FogMode != D3DFOG_NONE) {
        Line("float computeFog(vec3 position) {");
        Line("    float fogStart = u_fogParams.x;");
        Line("    float fogEnd = u_fogParams.y;");
        Line("    float fogDensity = u_fogParams.z;");

        if (key.Data.bits.RangeFog) {
            Line("    float dist = length(position);");
        } else {
            Line("    float dist = abs(position.z);");
        }

        switch (key.Data.bits.FogMode) {
            case D3DFOG_LINEAR:
                Line("    return clamp((fogEnd - dist) / (fogEnd - fogStart), 0.0, 1.0);");
                break;
            case D3DFOG_EXP:
                Line("    return clamp(exp(-fogDensity * dist), 0.0, 1.0);");
                break;
            case D3DFOG_EXP2:
                Line("    return clamp(exp(-fogDensity * fogDensity * dist * dist), 0.0, 1.0);");
                break;
        }

        Line("}");
        Line();
    }
}

void ShaderGenerator::EmitVSMain(const VertexShaderKey& key) {
    Line("// =============================================================================");
    Line("// Main");
    Line("// =============================================================================");
    Line();
    Line("void main() {");

    // Position transformation
    EmitVertexTransform(key);

    // Normal transformation
    if (!key.Data.bits.HasPositionT) {
        Line("    // Transform normal");
        Line("    vec3 normal = mul(u_normalMatrix, vec4(a_normal, 0.0)).xyz;");
        if (key.Data.bits.NormalizeNormals) {
            Line("    normal = normalize(normal);");
        }
        Line("    v_normal = normal;");
        Line();
    }

    // Lighting
    EmitLighting(key);

    // Texture coordinates
    EmitTexCoordGen(key);

    // Fog
    EmitFog(key);

    Line("}");
}

void ShaderGenerator::EmitVertexTransform(const VertexShaderKey& key) {
    if (key.Data.bits.HasPositionT) {
        // Pre-transformed vertices
        Line("    // Pre-transformed vertex");
        Line("    gl_Position.xy = a_position.xy * u_viewportInvExtent.xy + u_viewportInvOffset.xy;");
        Line("    gl_Position.z = a_position.z;");
        Line("    gl_Position.w = 1.0;");
        Line("    v_viewPos = vec3(0.0, 0.0, 0.0);");
    } else if (key.Data.bits.VertexBlendMode == 2) {
        // Tween blending
        Line("    // Tween vertex blending");
        Line("    vec3 position = mix(a_position.xyz, a_position1.xyz, u_tweenFactor.x);");
        Line("    vec4 worldPos = mul(u_worldView, vec4(position, 1.0));");
        Line("    gl_Position = mul(u_worldViewProj, vec4(position, 1.0));");
        Line("    v_viewPos = worldPos.xyz;");
    } else if (key.Data.bits.VertexBlendMode == 1) {
        // Hardware vertex blending
        Line("    // Hardware vertex blending");
        Line("    vec4 blendedPos = vec4(0.0, 0.0, 0.0, 0.0);");
        Line("    float weightSum = 0.0;");
        Line();

        uint32_t blendCount = key.Data.bits.VertexBlendCount;
        for (uint32_t i = 0; i <= blendCount; i++) {
            if (i < blendCount) {
                m_code << "    float w" << i << " = a_weight[" << i << "];\n";
                m_code << "    weightSum += w" << i << ";\n";
            } else {
                m_code << "    float w" << i << " = 1.0 - weightSum;\n";
            }

            if (key.Data.bits.VertexBlendIndexed) {
                m_code << "    uint idx" << i << " = uint(a_indices[" << i << "]);\n";
                m_code << "    blendedPos += mul(u_blendMatrices[idx" << i << "], vec4(a_position, 1.0)) * w" << i << ";\n";
            } else {
                m_code << "    blendedPos += mul(u_blendMatrices[" << i << "], vec4(a_position, 1.0)) * w" << i << ";\n";
            }
        }

        Line();
        Line("    gl_Position = mul(u_worldViewProj, blendedPos);");
        Line("    v_viewPos = blendedPos.xyz;");
    } else {
        // Standard transform
        Line("    // Standard transform");
        Line("    vec4 worldPos = mul(u_worldView, vec4(a_position, 1.0));");
        Line("    gl_Position = mul(u_worldViewProj, vec4(a_position, 1.0));");
        Line("    v_viewPos = worldPos.xyz;");
    }
    Line();
}

void ShaderGenerator::EmitVertexBlending(const VertexShaderKey& key) {
    // Already handled in EmitVertexTransform
}

void ShaderGenerator::EmitLighting(const VertexShaderKey& key) {
    if (!key.Data.bits.UseLighting) {
        // Pass through vertex colors
        Line("    // Pass through vertex colors");
        if (key.Data.bits.HasColor0) {
            Line("    v_color0 = a_color0;");
        } else {
            Line("    v_color0 = vec4(1.0, 1.0, 1.0, 1.0);");
        }
        if (key.Data.bits.HasColor1) {
            Line("    v_color1 = a_color1;");
        } else {
            Line("    v_color1 = vec4(0.0, 0.0, 0.0, 1.0);");
        }
        Line();
        return;
    }

    Line("    // Lighting calculation");
    Line("    vec3 diffuseAccum = vec3_splat(0.0);");
    Line("    vec3 specularAccum = vec3_splat(0.0);");
    Line("    vec3 ambientAccum = vec3_splat(0.0);");
    Line("    float power = u_materialPower.x;");
    Line();

    // Generate light calculations
    uint32_t lightCount = key.Data.bits.LightCount;
    for (uint32_t i = 0; i < lightCount; i++) {
        m_code << "    // Light " << i << "\n";
        m_code << "    computeLight(\n";
        m_code << "        v_viewPos, normal,\n";
        m_code << "        u_light" << i << "Diffuse,\n";
        m_code << "        u_light" << i << "Specular,\n";
        m_code << "        u_light" << i << "Ambient,\n";
        m_code << "        u_light" << i << "Position,\n";
        m_code << "        u_light" << i << "Direction,\n";
        m_code << "        u_light" << i << "Attenuation,\n";
        m_code << "        u_light" << i << "SpotParams,\n";
        m_code << "        power,\n";
        m_code << "        " << (key.Data.bits.LocalViewer ? "true" : "false") << ",\n";
        m_code << "        diffuseAccum, specularAccum, ambientAccum\n";
        m_code << "    );\n";
        Line();
    }

    // Final color calculation
    Line("    // Final color");

    // Get material colors based on source
    auto emitColorSource = [&](const char* name, uint32_t source, const char* matColor) {
        switch (source) {
            case D3DMCS_MATERIAL:
                m_code << "    vec4 " << name << " = " << matColor << ";\n";
                break;
            case D3DMCS_COLOR1:
                m_code << "    vec4 " << name << " = a_color0;\n";
                break;
            case D3DMCS_COLOR2:
                m_code << "    vec4 " << name << " = a_color1;\n";
                break;
        }
    };

    emitColorSource("matDiffuse", key.Data.bits.DiffuseSource, "u_materialDiffuse");
    emitColorSource("matAmbient", key.Data.bits.AmbientSource, "u_materialAmbient");
    emitColorSource("matSpecular", key.Data.bits.SpecularSource, "u_materialSpecular");
    emitColorSource("matEmissive", key.Data.bits.EmissiveSource, "u_materialEmissive");

    Line();
    Line("    v_color0.rgb = matEmissive.rgb + matAmbient.rgb * u_globalAmbient.rgb + matAmbient.rgb * ambientAccum + matDiffuse.rgb * diffuseAccum;");
    Line("    v_color0.a = matDiffuse.a;");
    Line("    v_color0 = clamp(v_color0, 0.0, 1.0);");
    Line();

    if (key.Data.bits.SpecularEnabled) {
        Line("    v_color1.rgb = matSpecular.rgb * specularAccum;");
        Line("    v_color1.a = 1.0;");
        Line("    v_color1 = clamp(v_color1, 0.0, 1.0);");
    } else {
        if (key.Data.bits.HasColor1) {
            Line("    v_color1 = a_color1;");
        } else {
            Line("    v_color1 = vec4(0.0, 0.0, 0.0, 1.0);");
        }
    }
    Line();
}

void ShaderGenerator::EmitTexCoordGen(const VertexShaderKey& key) {
    Line("    // Texture coordinates");

    for (int i = 0; i < 8; i++) {
        uint32_t tciIndex = (key.Data.bits.TexcoordIndices >> (i * 3)) & 0x7;
        uint32_t tciGen = (key.Data.bits.TexcoordFlags >> (i * 3)) & 0x7;
        uint32_t transformFlags = (key.Data.bits.TransformFlags >> (i * 3)) & 0x7;

        std::string texCoordVar = "v_texcoord" + std::to_string(i);
        std::string inputCoord;

        // Determine source
        switch (tciGen) {
            case 0: // Pass-through
                inputCoord = "vec4(a_texcoord" + std::to_string(tciIndex) + ", 0.0, 1.0)";
                break;
            case 1: // Camera space normal
                inputCoord = "vec4(normal, 1.0)";
                break;
            case 2: // Camera space position
                inputCoord = "vec4(v_viewPos, 1.0)";
                break;
            case 3: // Reflection vector
                m_code << "    {\n";
                m_code << "        vec3 eyeVec = normalize(v_viewPos);\n";
                m_code << "        vec3 reflection = eyeVec - 2.0 * normal * dot(eyeVec, normal);\n";
                inputCoord = "vec4(reflection, 1.0)";
                break;
            case 4: // Sphere map
                m_code << "    {\n";
                m_code << "        vec3 eyeVec = normalize(v_viewPos);\n";
                m_code << "        vec3 reflection = eyeVec - 2.0 * normal * dot(eyeVec, normal);\n";
                m_code << "        float m = 2.0 * sqrt(reflection.x*reflection.x + reflection.y*reflection.y + (reflection.z+1.0)*(reflection.z+1.0));\n";
                inputCoord = "vec4(reflection.x/m + 0.5, reflection.y/m + 0.5, 0.0, 1.0)";
                break;
        }

        // Apply transform if needed
        if (transformFlags != D3DTTFF_DISABLE) {
            m_code << "    " << texCoordVar << " = mul(u_texMatrix" << i << ", " << inputCoord << ");\n";
        } else {
            m_code << "    " << texCoordVar << " = " << inputCoord << ";\n";
        }

        // Close block for reflection/sphere map
        if (tciGen == 3 || tciGen == 4) {
            m_code << "    }\n";
        }
    }
    Line();
}

void ShaderGenerator::EmitFog(const VertexShaderKey& key) {
    if (key.Data.bits.FogMode != D3DFOG_NONE) {
        Line("    // Fog");
        Line("    v_fog = computeFog(v_viewPos);");
    } else {
        Line("    v_fog = 1.0;");
    }
}

// =============================================================================
// Fragment Shader Generation
// =============================================================================

std::string ShaderGenerator::GenerateFragmentShader(const FragmentShaderKey& key) {
    m_code.str("");
    m_code.clear();

    EmitFSHeader();
    EmitFSInputs(key);
    EmitFSUniforms(key);
    EmitFSHelpers(key);
    EmitFSMain(key);

    return m_code.str();
}

void ShaderGenerator::EmitFSHeader() {
    Line("$input v_color0, v_color1, v_normal, v_texcoord0, v_texcoord1, v_texcoord2, v_texcoord3, v_texcoord4, v_texcoord5, v_texcoord6, v_texcoord7, v_viewPos, v_fog");
    Line();
    Line("/*");
    Line(" * DX8 Fixed Function Fragment Shader - Generated");
    Line(" */");
    Line();
    Line("#include <bgfx_shader.sh>");
    Line();
}

void ShaderGenerator::EmitFSInputs(const FragmentShaderKey& key) {
    // Declare samplers for used stages
    Line("// Samplers");
    for (int i = 0; i < 8; i++) {
        if (key.Data.Stages[i].bits.HasTexture &&
            key.Data.Stages[i].bits.ColorOp != D3DTOP_DISABLE) {
            m_code << "SAMPLER2D(s_texture" << i << ", " << i << ");\n";
        }
    }
    Line();
}

void ShaderGenerator::EmitFSUniforms(const FragmentShaderKey& key) {
    Line("// Uniforms");
    Line("uniform vec4 u_textureFactor;");

    // Per-stage constants for bump mapping
    for (int i = 0; i < 8; i++) {
        uint8_t colorOp = key.Data.Stages[i].bits.ColorOp;
        if (colorOp == D3DTOP_BUMPENVMAP || colorOp == D3DTOP_BUMPENVMAPLUMINANCE) {
            m_code << "uniform vec4 u_bumpEnvMat" << i << ";\n";
            if (colorOp == D3DTOP_BUMPENVMAPLUMINANCE) {
                m_code << "uniform vec4 u_bumpEnvLum" << i << ";\n";
            }
        }
    }

    if (key.Data.bits.FogEnabled) {
        Line("uniform vec4 u_fogColor;");
    }

    if (key.Data.bits.AlphaTestEnabled) {
        Line("uniform vec4 u_alphaTest;");
    }

    Line();
}

void ShaderGenerator::EmitFSHelpers(const FragmentShaderKey& key) {
    // Alpha test helper
    if (key.Data.bits.AlphaTestEnabled) {
        Line("bool alphaTest(float alpha) {");
        Line("    float ref = u_alphaTest.y;");

        switch (key.Data.bits.AlphaTestFunc) {
            case D3DCMP_NEVER:
                Line("    return false;");
                break;
            case D3DCMP_LESS:
                Line("    return alpha < ref;");
                break;
            case D3DCMP_EQUAL:
                Line("    return abs(alpha - ref) < 0.004;");
                break;
            case D3DCMP_LESSEQUAL:
                Line("    return alpha <= ref;");
                break;
            case D3DCMP_GREATER:
                Line("    return alpha > ref;");
                break;
            case D3DCMP_NOTEQUAL:
                Line("    return abs(alpha - ref) >= 0.004;");
                break;
            case D3DCMP_GREATEREQUAL:
                Line("    return alpha >= ref;");
                break;
            case D3DCMP_ALWAYS:
            default:
                Line("    return true;");
                break;
        }

        Line("}");
        Line();
    }
}

void ShaderGenerator::EmitFSMain(const FragmentShaderKey& key) {
    Line("void main() {");
    Line("    vec4 diffuse = v_color0;");
    Line("    vec4 specular = v_color1;");
    Line("    vec4 current = diffuse;");
    Line("    vec4 temp = vec4(0.0, 0.0, 0.0, 0.0);");
    Line();

    // Generate texture stage operations
    for (int i = 0; i < 8; i++) {
        if (key.Data.Stages[i].bits.ColorOp == D3DTOP_DISABLE) {
            break;
        }
        EmitTextureStage(key, i);
    }

    // Add specular
    if (key.Data.bits.SpecularEnabled) {
        Line("    // Add specular");
        Line("    current.rgb += specular.rgb;");
        Line();
    }

    // Fog
    EmitFogBlend(key);

    // Alpha test
    EmitAlphaTest(key);

    Line("    gl_FragColor = current;");
    Line("}");
}

void ShaderGenerator::EmitTextureStage(const FragmentShaderKey& key, int stage) {
    const auto& s = key.Data.Stages[stage];

    m_code << "    // Stage " << stage << "\n";

    // Sample texture if needed
    if (s.bits.HasTexture) {
        m_code << "    vec4 tex" << stage << " = texture2D(s_texture" << stage
               << ", v_texcoord" << stage << ".xy);\n";
    } else {
        m_code << "    vec4 tex" << stage << " = vec4(1.0, 1.0, 1.0, 1.0);\n";
    }

    // Get arguments
    std::string arg0 = GetTextureArgCode(s.bits.ColorArg0, stage);
    std::string arg1 = GetTextureArgCode(s.bits.ColorArg1, stage);
    std::string arg2 = GetTextureArgCode(s.bits.ColorArg2, stage);

    // Color operation
    std::string colorResult = GetColorOpCode(s.bits.ColorOp, arg0, arg1, arg2);
    m_code << "    vec3 colorResult" << stage << " = " << colorResult << ";\n";

    // Alpha arguments
    std::string aArg0 = GetTextureArgCode(s.bits.AlphaArg0, stage) + ".a";
    std::string aArg1 = GetTextureArgCode(s.bits.AlphaArg1, stage) + ".a";
    std::string aArg2 = GetTextureArgCode(s.bits.AlphaArg2, stage) + ".a";

    // Alpha operation
    std::string alphaResult = GetAlphaOpCode(s.bits.AlphaOp, aArg0, aArg1, aArg2);
    m_code << "    float alphaResult" << stage << " = " << alphaResult << ";\n";

    // Write result
    if (s.bits.ResultIsTemp) {
        m_code << "    temp = vec4(colorResult" << stage << ", alphaResult" << stage << ");\n";
    } else {
        m_code << "    current = vec4(colorResult" << stage << ", alphaResult" << stage << ");\n";
    }
    Line();
}

std::string ShaderGenerator::GetTextureArgCode(uint8_t arg, int stage) {
    uint8_t source = arg & D3DTA_SELECTMASK;
    bool complement = arg & D3DTA_COMPLEMENT;
    bool alphaReplicate = arg & D3DTA_ALPHAREPLICATE;

    std::string result;
    switch (source) {
        case D3DTA_DIFFUSE:   result = "diffuse"; break;
        case D3DTA_CURRENT:   result = "current"; break;
        case D3DTA_TEXTURE:   result = "tex" + std::to_string(stage); break;
        case D3DTA_TFACTOR:   result = "u_textureFactor"; break;
        case D3DTA_SPECULAR:  result = "specular"; break;
        case D3DTA_TEMP:      result = "temp"; break;
        default:              result = "vec4(1.0, 1.0, 1.0, 1.0)"; break;
    }

    if (alphaReplicate) {
        result = "vec4(" + result + ".a, " + result + ".a, " + result + ".a, " + result + ".a)";
    }

    if (complement) {
        result = "(vec4(1.0) - " + result + ")";
    }

    return result;
}

std::string ShaderGenerator::GetColorOpCode(uint8_t op, const std::string& arg0,
                                           const std::string& arg1, const std::string& arg2) {
    switch (op) {
        case D3DTOP_SELECTARG1:
            return arg1 + ".rgb";
        case D3DTOP_SELECTARG2:
            return arg2 + ".rgb";
        case D3DTOP_MODULATE:
            return arg1 + ".rgb * " + arg2 + ".rgb";
        case D3DTOP_MODULATE2X:
            return "clamp(" + arg1 + ".rgb * " + arg2 + ".rgb * 2.0, 0.0, 1.0)";
        case D3DTOP_MODULATE4X:
            return "clamp(" + arg1 + ".rgb * " + arg2 + ".rgb * 4.0, 0.0, 1.0)";
        case D3DTOP_ADD:
            return "clamp(" + arg1 + ".rgb + " + arg2 + ".rgb, 0.0, 1.0)";
        case D3DTOP_ADDSIGNED:
            return "clamp(" + arg1 + ".rgb + " + arg2 + ".rgb - 0.5, 0.0, 1.0)";
        case D3DTOP_ADDSIGNED2X:
            return "clamp((" + arg1 + ".rgb + " + arg2 + ".rgb - 0.5) * 2.0, 0.0, 1.0)";
        case D3DTOP_SUBTRACT:
            return "clamp(" + arg1 + ".rgb - " + arg2 + ".rgb, 0.0, 1.0)";
        case D3DTOP_ADDSMOOTH:
            return "clamp(" + arg1 + ".rgb + " + arg2 + ".rgb - " + arg1 + ".rgb * " + arg2 + ".rgb, 0.0, 1.0)";
        case D3DTOP_BLENDDIFFUSEALPHA:
            return "mix(" + arg2 + ".rgb, " + arg1 + ".rgb, diffuse.a)";
        case D3DTOP_BLENDTEXTUREALPHA:
            return "mix(" + arg2 + ".rgb, " + arg1 + ".rgb, " + arg1 + ".a)";
        case D3DTOP_BLENDFACTORALPHA:
            return "mix(" + arg2 + ".rgb, " + arg1 + ".rgb, u_textureFactor.a)";
        case D3DTOP_BLENDCURRENTALPHA:
            return "mix(" + arg2 + ".rgb, " + arg1 + ".rgb, current.a)";
        case D3DTOP_DOTPRODUCT3:
            return "vec3_splat(clamp(dot(" + arg1 + ".rgb - 0.5, " + arg2 + ".rgb - 0.5) * 4.0, 0.0, 1.0))";
        case D3DTOP_MULTIPLYADD:
            return "clamp(" + arg1 + ".rgb * " + arg2 + ".rgb + " + arg0 + ".rgb, 0.0, 1.0)";
        case D3DTOP_LERP:
            return "mix(" + arg2 + ".rgb, " + arg1 + ".rgb, " + arg0 + ".rgb)";
        default:
            return arg1 + ".rgb";
    }
}

std::string ShaderGenerator::GetAlphaOpCode(uint8_t op, const std::string& arg0,
                                           const std::string& arg1, const std::string& arg2) {
    switch (op) {
        case D3DTOP_SELECTARG1:
            return arg1;
        case D3DTOP_SELECTARG2:
            return arg2;
        case D3DTOP_MODULATE:
            return arg1 + " * " + arg2;
        case D3DTOP_MODULATE2X:
            return "clamp(" + arg1 + " * " + arg2 + " * 2.0, 0.0, 1.0)";
        case D3DTOP_MODULATE4X:
            return "clamp(" + arg1 + " * " + arg2 + " * 4.0, 0.0, 1.0)";
        case D3DTOP_ADD:
            return "clamp(" + arg1 + " + " + arg2 + ", 0.0, 1.0)";
        case D3DTOP_SUBTRACT:
            return "clamp(" + arg1 + " - " + arg2 + ", 0.0, 1.0)";
        case D3DTOP_MULTIPLYADD:
            return "clamp(" + arg1 + " * " + arg2 + " + " + arg0 + ", 0.0, 1.0)";
        case D3DTOP_LERP:
            return "mix(" + arg2 + ", " + arg1 + ", " + arg0 + ")";
        default:
            return arg1;
    }
}

void ShaderGenerator::EmitAlphaTest(const FragmentShaderKey& key) {
    if (key.Data.bits.AlphaTestEnabled) {
        Line("    // Alpha test");
        Line("    if (!alphaTest(current.a)) {");
        Line("        discard;");
        Line("    }");
        Line();
    }
}

void ShaderGenerator::EmitFogBlend(const FragmentShaderKey& key) {
    if (key.Data.bits.FogEnabled) {
        Line("    // Fog");
        Line("    current.rgb = mix(u_fogColor.rgb, current.rgb, v_fog);");
        Line();
    }
}

// =============================================================================
// Ubershader Source
// =============================================================================

std::string ShaderGenerator::GetUbershaderVertexSource() {
    // Return the contents of vs_fixed_function.sc
    return R"(
$input a_position, a_normal, a_color0, a_color1, a_texcoord0, a_texcoord1, a_texcoord2, a_texcoord3
$output v_color0, v_color1, v_normal, v_texcoord0, v_texcoord1, v_texcoord2, v_texcoord3, v_viewPos, v_fog

#include <bgfx_shader.sh>

uniform mat4 u_worldView;
uniform mat4 u_worldViewProj;
uniform mat4 u_normalMatrix;
uniform vec4 u_flags;

void main() {
    vec4 worldPos = mul(u_worldView, vec4(a_position, 1.0));
    gl_Position = mul(u_worldViewProj, vec4(a_position, 1.0));
    v_viewPos = worldPos.xyz;
    v_normal = mul(u_normalMatrix, vec4(a_normal, 0.0)).xyz;
    v_color0 = a_color0;
    v_color1 = a_color1;
    v_texcoord0 = vec4(a_texcoord0, 0.0, 1.0);
    v_texcoord1 = vec4(a_texcoord1, 0.0, 1.0);
    v_texcoord2 = vec4(a_texcoord2, 0.0, 1.0);
    v_texcoord3 = vec4(a_texcoord3, 0.0, 1.0);
    v_fog = 1.0;
}
)";
}

std::string ShaderGenerator::GetUbershaderFragmentSource() {
    return R"(
$input v_color0, v_color1, v_normal, v_texcoord0, v_texcoord1, v_texcoord2, v_texcoord3, v_viewPos, v_fog

#include <bgfx_shader.sh>

SAMPLER2D(s_texture0, 0);

void main() {
    vec4 color = v_color0;
    vec4 tex = texture2D(s_texture0, v_texcoord0.xy);
    color.rgb *= tex.rgb;
    color.a *= tex.a;
    gl_FragColor = color;
}
)";
}

} // namespace dx8bgfx
