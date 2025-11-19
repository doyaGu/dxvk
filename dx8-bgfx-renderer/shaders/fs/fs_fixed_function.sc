$input v_color0, v_color1, v_normal, v_texcoord0, v_texcoord1, v_texcoord2, v_texcoord3, v_viewPos, v_fog

/*
 * DX8 Fixed Function Fragment Shader
 *
 * This is an "ubershader" that handles all texture stage states.
 * In production, specialized shaders would be generated for specific states.
 */

#include <bgfx_shader.sh>

// =============================================================================
// Samplers
// =============================================================================

SAMPLER2D(s_texture0, 0);
SAMPLER2D(s_texture1, 1);
SAMPLER2D(s_texture2, 2);
SAMPLER2D(s_texture3, 3);

// =============================================================================
// Uniforms
// =============================================================================

// Texture stage 0
uniform vec4 u_stage0ColorOp;   // x=colorOp, y=alphaOp, z=hasTexture, w=unused
uniform vec4 u_stage0ColorArgs; // x=arg0, y=arg1, z=arg2, w=unused
uniform vec4 u_stage0AlphaArgs; // x=arg0, y=arg1, z=arg2, w=unused

// Texture stage 1
uniform vec4 u_stage1ColorOp;
uniform vec4 u_stage1ColorArgs;
uniform vec4 u_stage1AlphaArgs;

// Texture stage 2
uniform vec4 u_stage2ColorOp;
uniform vec4 u_stage2ColorArgs;
uniform vec4 u_stage2AlphaArgs;

// Texture stage 3
uniform vec4 u_stage3ColorOp;
uniform vec4 u_stage3ColorArgs;
uniform vec4 u_stage3AlphaArgs;

// Texture factor
uniform vec4 u_textureFactor;

// Fog
uniform vec4 u_fogColor;

// Alpha test
uniform vec4 u_alphaTest;  // x=enabled, y=func, z=ref, w=unused

// Control flags
uniform vec4 u_psFlags;  // x=fogEnabled, y=specularEnabled, z=unused, w=unused

// =============================================================================
// Constants
// =============================================================================

// Texture Operations
#define D3DTOP_DISABLE           1.0
#define D3DTOP_SELECTARG1        2.0
#define D3DTOP_SELECTARG2        3.0
#define D3DTOP_MODULATE          4.0
#define D3DTOP_MODULATE2X        5.0
#define D3DTOP_MODULATE4X        6.0
#define D3DTOP_ADD               7.0
#define D3DTOP_ADDSIGNED         8.0
#define D3DTOP_ADDSIGNED2X       9.0
#define D3DTOP_SUBTRACT          10.0
#define D3DTOP_ADDSMOOTH         11.0
#define D3DTOP_BLENDDIFFUSEALPHA 12.0
#define D3DTOP_BLENDTEXTUREALPHA 13.0
#define D3DTOP_BLENDFACTORALPHA  14.0
#define D3DTOP_BLENDCURRENTALPHA 16.0
#define D3DTOP_DOTPRODUCT3       24.0
#define D3DTOP_MULTIPLYADD       25.0
#define D3DTOP_LERP              26.0

// Texture Arguments
#define D3DTA_DIFFUSE   0.0
#define D3DTA_CURRENT   1.0
#define D3DTA_TEXTURE   2.0
#define D3DTA_TFACTOR   3.0
#define D3DTA_SPECULAR  4.0
#define D3DTA_TEMP      5.0

// Alpha test functions
#define D3DCMP_NEVER        1.0
#define D3DCMP_LESS         2.0
#define D3DCMP_EQUAL        3.0
#define D3DCMP_LESSEQUAL    4.0
#define D3DCMP_GREATER      5.0
#define D3DCMP_NOTEQUAL     6.0
#define D3DCMP_GREATEREQUAL 7.0
#define D3DCMP_ALWAYS       8.0

// =============================================================================
// Helper Functions
// =============================================================================

vec4 getTextureArg(float arg, vec4 texture, vec4 current, vec4 diffuse, vec4 specular, vec4 tfactor, vec4 temp) {
    vec4 result;

    if (arg == D3DTA_DIFFUSE) {
        result = diffuse;
    } else if (arg == D3DTA_CURRENT) {
        result = current;
    } else if (arg == D3DTA_TEXTURE) {
        result = texture;
    } else if (arg == D3DTA_TFACTOR) {
        result = tfactor;
    } else if (arg == D3DTA_SPECULAR) {
        result = specular;
    } else if (arg == D3DTA_TEMP) {
        result = temp;
    } else {
        result = vec4(1.0, 1.0, 1.0, 1.0);
    }

    return result;
}

vec3 applyColorOp(float op, vec3 arg0, vec3 arg1, vec3 arg2) {
    vec3 result;

    if (op == D3DTOP_SELECTARG1) {
        result = arg1;
    } else if (op == D3DTOP_SELECTARG2) {
        result = arg2;
    } else if (op == D3DTOP_MODULATE) {
        result = arg1 * arg2;
    } else if (op == D3DTOP_MODULATE2X) {
        result = arg1 * arg2 * 2.0;
    } else if (op == D3DTOP_MODULATE4X) {
        result = arg1 * arg2 * 4.0;
    } else if (op == D3DTOP_ADD) {
        result = arg1 + arg2;
    } else if (op == D3DTOP_ADDSIGNED) {
        result = arg1 + arg2 - 0.5;
    } else if (op == D3DTOP_ADDSIGNED2X) {
        result = (arg1 + arg2 - 0.5) * 2.0;
    } else if (op == D3DTOP_SUBTRACT) {
        result = arg1 - arg2;
    } else if (op == D3DTOP_ADDSMOOTH) {
        result = arg1 + arg2 - arg1 * arg2;
    } else if (op == D3DTOP_BLENDDIFFUSEALPHA) {
        result = mix(arg2, arg1, v_color0.a);
    } else if (op == D3DTOP_BLENDCURRENTALPHA) {
        result = mix(arg2, arg1, arg0.x);  // Current alpha passed in arg0.x
    } else if (op == D3DTOP_DOTPRODUCT3) {
        float d = dot(arg1 - 0.5, arg2 - 0.5) * 4.0;
        result = vec3_splat(d);
    } else if (op == D3DTOP_MULTIPLYADD) {
        result = arg0 * arg1 + arg2;  // Actually arg1 * arg2 + arg0
    } else if (op == D3DTOP_LERP) {
        result = mix(arg2, arg1, arg0);
    } else {
        result = arg1;
    }

    return clamp(result, 0.0, 1.0);
}

float applyAlphaOp(float op, float arg0, float arg1, float arg2) {
    float result;

    if (op == D3DTOP_SELECTARG1) {
        result = arg1;
    } else if (op == D3DTOP_SELECTARG2) {
        result = arg2;
    } else if (op == D3DTOP_MODULATE) {
        result = arg1 * arg2;
    } else if (op == D3DTOP_MODULATE2X) {
        result = arg1 * arg2 * 2.0;
    } else if (op == D3DTOP_MODULATE4X) {
        result = arg1 * arg2 * 4.0;
    } else if (op == D3DTOP_ADD) {
        result = arg1 + arg2;
    } else if (op == D3DTOP_SUBTRACT) {
        result = arg1 - arg2;
    } else if (op == D3DTOP_MULTIPLYADD) {
        result = arg1 * arg2 + arg0;
    } else if (op == D3DTOP_LERP) {
        result = mix(arg2, arg1, arg0);
    } else {
        result = arg1;
    }

    return clamp(result, 0.0, 1.0);
}

bool alphaTest(float alpha, float func, float ref) {
    bool result = true;

    if (func == D3DCMP_NEVER) {
        result = false;
    } else if (func == D3DCMP_LESS) {
        result = alpha < ref;
    } else if (func == D3DCMP_EQUAL) {
        result = abs(alpha - ref) < 0.001;
    } else if (func == D3DCMP_LESSEQUAL) {
        result = alpha <= ref;
    } else if (func == D3DCMP_GREATER) {
        result = alpha > ref;
    } else if (func == D3DCMP_NOTEQUAL) {
        result = abs(alpha - ref) >= 0.001;
    } else if (func == D3DCMP_GREATEREQUAL) {
        result = alpha >= ref;
    } else if (func == D3DCMP_ALWAYS) {
        result = true;
    }

    return result;
}

// =============================================================================
// Main
// =============================================================================

void main() {
    vec4 diffuse = v_color0;
    vec4 specular = v_color1;
    vec4 current = diffuse;
    vec4 temp = vec4(0.0, 0.0, 0.0, 0.0);

    // Sample textures
    vec4 tex0 = texture2D(s_texture0, v_texcoord0.xy);
    vec4 tex1 = texture2D(s_texture1, v_texcoord1.xy);
    vec4 tex2 = texture2D(s_texture2, v_texcoord2.xy);
    vec4 tex3 = texture2D(s_texture3, v_texcoord3.xy);

    // Stage 0
    float colorOp0 = u_stage0ColorOp.x;
    float alphaOp0 = u_stage0ColorOp.y;

    if (colorOp0 != D3DTOP_DISABLE) {
        vec4 cArg0 = getTextureArg(u_stage0ColorArgs.x, tex0, current, diffuse, specular, u_textureFactor, temp);
        vec4 cArg1 = getTextureArg(u_stage0ColorArgs.y, tex0, current, diffuse, specular, u_textureFactor, temp);
        vec4 cArg2 = getTextureArg(u_stage0ColorArgs.z, tex0, current, diffuse, specular, u_textureFactor, temp);
        current.rgb = applyColorOp(colorOp0, cArg0.rgb, cArg1.rgb, cArg2.rgb);

        vec4 aArg0 = getTextureArg(u_stage0AlphaArgs.x, tex0, current, diffuse, specular, u_textureFactor, temp);
        vec4 aArg1 = getTextureArg(u_stage0AlphaArgs.y, tex0, current, diffuse, specular, u_textureFactor, temp);
        vec4 aArg2 = getTextureArg(u_stage0AlphaArgs.z, tex0, current, diffuse, specular, u_textureFactor, temp);
        current.a = applyAlphaOp(alphaOp0, aArg0.a, aArg1.a, aArg2.a);
    }

    // Stage 1
    float colorOp1 = u_stage1ColorOp.x;
    float alphaOp1 = u_stage1ColorOp.y;

    if (colorOp1 != D3DTOP_DISABLE) {
        vec4 cArg0 = getTextureArg(u_stage1ColorArgs.x, tex1, current, diffuse, specular, u_textureFactor, temp);
        vec4 cArg1 = getTextureArg(u_stage1ColorArgs.y, tex1, current, diffuse, specular, u_textureFactor, temp);
        vec4 cArg2 = getTextureArg(u_stage1ColorArgs.z, tex1, current, diffuse, specular, u_textureFactor, temp);
        current.rgb = applyColorOp(colorOp1, cArg0.rgb, cArg1.rgb, cArg2.rgb);

        vec4 aArg0 = getTextureArg(u_stage1AlphaArgs.x, tex1, current, diffuse, specular, u_textureFactor, temp);
        vec4 aArg1 = getTextureArg(u_stage1AlphaArgs.y, tex1, current, diffuse, specular, u_textureFactor, temp);
        vec4 aArg2 = getTextureArg(u_stage1AlphaArgs.z, tex1, current, diffuse, specular, u_textureFactor, temp);
        current.a = applyAlphaOp(alphaOp1, aArg0.a, aArg1.a, aArg2.a);
    }

    // Stage 2
    float colorOp2 = u_stage2ColorOp.x;
    float alphaOp2 = u_stage2ColorOp.y;

    if (colorOp2 != D3DTOP_DISABLE) {
        vec4 cArg0 = getTextureArg(u_stage2ColorArgs.x, tex2, current, diffuse, specular, u_textureFactor, temp);
        vec4 cArg1 = getTextureArg(u_stage2ColorArgs.y, tex2, current, diffuse, specular, u_textureFactor, temp);
        vec4 cArg2 = getTextureArg(u_stage2ColorArgs.z, tex2, current, diffuse, specular, u_textureFactor, temp);
        current.rgb = applyColorOp(colorOp2, cArg0.rgb, cArg1.rgb, cArg2.rgb);

        vec4 aArg0 = getTextureArg(u_stage2AlphaArgs.x, tex2, current, diffuse, specular, u_textureFactor, temp);
        vec4 aArg1 = getTextureArg(u_stage2AlphaArgs.y, tex2, current, diffuse, specular, u_textureFactor, temp);
        vec4 aArg2 = getTextureArg(u_stage2AlphaArgs.z, tex2, current, diffuse, specular, u_textureFactor, temp);
        current.a = applyAlphaOp(alphaOp2, aArg0.a, aArg1.a, aArg2.a);
    }

    // Stage 3
    float colorOp3 = u_stage3ColorOp.x;
    float alphaOp3 = u_stage3ColorOp.y;

    if (colorOp3 != D3DTOP_DISABLE) {
        vec4 cArg0 = getTextureArg(u_stage3ColorArgs.x, tex3, current, diffuse, specular, u_textureFactor, temp);
        vec4 cArg1 = getTextureArg(u_stage3ColorArgs.y, tex3, current, diffuse, specular, u_textureFactor, temp);
        vec4 cArg2 = getTextureArg(u_stage3ColorArgs.z, tex3, current, diffuse, specular, u_textureFactor, temp);
        current.rgb = applyColorOp(colorOp3, cArg0.rgb, cArg1.rgb, cArg2.rgb);

        vec4 aArg0 = getTextureArg(u_stage3AlphaArgs.x, tex3, current, diffuse, specular, u_textureFactor, temp);
        vec4 aArg1 = getTextureArg(u_stage3AlphaArgs.y, tex3, current, diffuse, specular, u_textureFactor, temp);
        vec4 aArg2 = getTextureArg(u_stage3AlphaArgs.z, tex3, current, diffuse, specular, u_textureFactor, temp);
        current.a = applyAlphaOp(alphaOp3, aArg0.a, aArg1.a, aArg2.a);
    }

    // Add specular
    if (u_psFlags.y > 0.5) {
        current.rgb += specular.rgb;
    }

    // Apply fog
    if (u_psFlags.x > 0.5) {
        current.rgb = mix(u_fogColor.rgb, current.rgb, v_fog);
    }

    // Alpha test
    if (u_alphaTest.x > 0.5) {
        if (!alphaTest(current.a, u_alphaTest.y, u_alphaTest.z)) {
            discard;
        }
    }

    gl_FragColor = current;
}
