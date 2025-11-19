$input a_position, a_normal, a_color0, a_color1, a_texcoord0, a_texcoord1, a_texcoord2, a_texcoord3
$output v_color0, v_color1, v_normal, v_texcoord0, v_texcoord1, v_texcoord2, v_texcoord3, v_viewPos, v_fog

/*
 * DX8 Fixed Function Vertex Shader
 *
 * This is an "ubershader" that handles all fixed-function states.
 * In production, specialized shaders would be generated for specific states.
 */

#include <bgfx_shader.sh>

// =============================================================================
// Uniforms
// =============================================================================

// Transform matrices
uniform mat4 u_worldView;
uniform mat4 u_worldViewProj;
uniform mat4 u_normalMatrix;
uniform mat4 u_texMatrix0;
uniform mat4 u_texMatrix1;
uniform mat4 u_texMatrix2;
uniform mat4 u_texMatrix3;

// Material
uniform vec4 u_materialDiffuse;
uniform vec4 u_materialAmbient;
uniform vec4 u_materialSpecular;
uniform vec4 u_materialEmissive;
uniform vec4 u_materialPower;  // x = power

// Global ambient
uniform vec4 u_globalAmbient;

// Light 0
uniform vec4 u_light0Diffuse;
uniform vec4 u_light0Specular;
uniform vec4 u_light0Ambient;
uniform vec4 u_light0Position;   // xyz = position, w = type (1=point, 2=spot, 3=dir)
uniform vec4 u_light0Direction;  // xyz = direction, w = range
uniform vec4 u_light0Attenuation; // x=atten0, y=atten1, z=atten2, w=falloff
uniform vec4 u_light0SpotParams;  // x=theta (cos), y=phi (cos), z=unused, w=unused

// Light 1
uniform vec4 u_light1Diffuse;
uniform vec4 u_light1Specular;
uniform vec4 u_light1Ambient;
uniform vec4 u_light1Position;
uniform vec4 u_light1Direction;
uniform vec4 u_light1Attenuation;
uniform vec4 u_light1SpotParams;

// Fog parameters
uniform vec4 u_fogParams;  // x=start, y=end, z=density, w=mode

// Control flags
uniform vec4 u_flags;  // x=useLighting, y=specularEnabled, z=normalizeNormals, w=localViewer

// =============================================================================
// Constants
// =============================================================================

#define LIGHT_POINT       1.0
#define LIGHT_SPOT        2.0
#define LIGHT_DIRECTIONAL 3.0

#define FOG_NONE   0.0
#define FOG_EXP    1.0
#define FOG_EXP2   2.0
#define FOG_LINEAR 3.0

// =============================================================================
// Helper Functions
// =============================================================================

vec3 computeLight(
    vec3 position,
    vec3 normal,
    vec4 lightDiffuse,
    vec4 lightSpecular,
    vec4 lightAmbient,
    vec4 lightPosition,
    vec4 lightDirection,
    vec4 lightAttenuation,
    vec4 lightSpotParams,
    float materialPower,
    bool localViewer,
    out vec3 specular
) {
    float lightType = lightPosition.w;
    float range = lightDirection.w;
    vec3 lightPos = lightPosition.xyz;
    vec3 lightDir = lightDirection.xyz;

    vec3 L;
    float attenuation = 1.0;

    if (lightType == LIGHT_DIRECTIONAL) {
        L = -lightDir;
    } else {
        vec3 lightVec = lightPos - position;
        float dist = length(lightVec);
        L = lightVec / max(dist, 0.0001);

        // Distance attenuation
        float atten0 = lightAttenuation.x;
        float atten1 = lightAttenuation.y;
        float atten2 = lightAttenuation.z;
        attenuation = 1.0 / (atten0 + atten1 * dist + atten2 * dist * dist);

        // Range check
        attenuation = dist > range ? 0.0 : attenuation;

        // Spotlight attenuation
        if (lightType == LIGHT_SPOT) {
            float rho = dot(-L, lightDir);
            float theta = lightSpotParams.x;
            float phi = lightSpotParams.y;
            float falloff = lightAttenuation.w;

            float spotFactor = clamp((rho - phi) / (theta - phi), 0.0, 1.0);
            spotFactor = pow(spotFactor, falloff);
            attenuation *= spotFactor;
        }
    }

    // Diffuse
    float NdotL = max(dot(normal, L), 0.0);
    vec3 diffuse = lightDiffuse.rgb * NdotL * attenuation;

    // Specular
    specular = vec3_splat(0.0);
    if (NdotL > 0.0 && materialPower > 0.0) {
        vec3 V = localViewer ? normalize(position) : vec3(0.0, 0.0, 1.0);
        vec3 H = normalize(L + V);
        float NdotH = max(dot(normal, H), 0.0);
        specular = lightSpecular.rgb * pow(NdotH, materialPower) * attenuation;
    }

    // Ambient
    vec3 ambient = lightAmbient.rgb * attenuation;

    return diffuse + ambient;
}

float computeFog(vec3 position, float fogMode, float fogStart, float fogEnd, float fogDensity) {
    float dist = length(position);
    float fogFactor = 1.0;

    if (fogMode == FOG_LINEAR) {
        fogFactor = (fogEnd - dist) / (fogEnd - fogStart);
    } else if (fogMode == FOG_EXP) {
        fogFactor = exp(-fogDensity * dist);
    } else if (fogMode == FOG_EXP2) {
        fogFactor = exp(-fogDensity * fogDensity * dist * dist);
    }

    return clamp(fogFactor, 0.0, 1.0);
}

// =============================================================================
// Main
// =============================================================================

void main() {
    // Transform position
    vec4 worldPos = mul(u_worldView, vec4(a_position, 1.0));
    gl_Position = mul(u_worldViewProj, vec4(a_position, 1.0));

    v_viewPos = worldPos.xyz;

    // Transform normal
    vec3 normal = mul(u_normalMatrix, vec4(a_normal, 0.0)).xyz;
    if (u_flags.z > 0.5) {  // normalizeNormals
        normal = normalize(normal);
    }
    v_normal = normal;

    // Lighting calculation
    bool useLighting = u_flags.x > 0.5;
    bool specularEnabled = u_flags.y > 0.5;
    bool localViewer = u_flags.w > 0.5;

    if (useLighting) {
        vec3 diffuseAccum = vec3_splat(0.0);
        vec3 specularAccum = vec3_splat(0.0);
        vec3 spec;

        float power = u_materialPower.x;

        // Light 0
        diffuseAccum += computeLight(
            worldPos.xyz, normal,
            u_light0Diffuse, u_light0Specular, u_light0Ambient,
            u_light0Position, u_light0Direction, u_light0Attenuation, u_light0SpotParams,
            power, localViewer, spec
        );
        specularAccum += spec;

        // Light 1
        diffuseAccum += computeLight(
            worldPos.xyz, normal,
            u_light1Diffuse, u_light1Specular, u_light1Ambient,
            u_light1Position, u_light1Direction, u_light1Attenuation, u_light1SpotParams,
            power, localViewer, spec
        );
        specularAccum += spec;

        // Final color
        vec3 ambient = u_materialAmbient.rgb * u_globalAmbient.rgb;
        vec3 emissive = u_materialEmissive.rgb;

        v_color0.rgb = emissive + ambient + u_materialDiffuse.rgb * diffuseAccum;
        v_color0.a = u_materialDiffuse.a;
        v_color0 = clamp(v_color0, 0.0, 1.0);

        if (specularEnabled) {
            v_color1.rgb = u_materialSpecular.rgb * specularAccum;
            v_color1.a = 1.0;
            v_color1 = clamp(v_color1, 0.0, 1.0);
        } else {
            v_color1 = a_color1;
        }
    } else {
        v_color0 = a_color0;
        v_color1 = a_color1;
    }

    // Texture coordinates
    v_texcoord0 = mul(u_texMatrix0, vec4(a_texcoord0, 0.0, 1.0));
    v_texcoord1 = mul(u_texMatrix1, vec4(a_texcoord1, 0.0, 1.0));
    v_texcoord2 = mul(u_texMatrix2, vec4(a_texcoord2, 0.0, 1.0));
    v_texcoord3 = mul(u_texMatrix3, vec4(a_texcoord3, 0.0, 1.0));

    // Fog
    float fogMode = u_fogParams.w;
    if (fogMode > 0.0) {
        v_fog = computeFog(worldPos.xyz, fogMode, u_fogParams.x, u_fogParams.y, u_fogParams.z);
    } else {
        v_fog = 1.0;
    }
}
