#version 410 core

#define MAX_LIGHTS 16
#define NUM_CASCADES 2
#define MAX_SHADOW_LIGHTS 4
#define TOTAL_SHADOW_MAPS (MAX_SHADOW_LIGHTS * NUM_CASCADES)
const float PI = 3.14159265359;
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in mat3 TBN;
in vec4 ClipPos;
uniform bool PCF;
uniform int  blend;
layout(location = 0) out vec4 out0;
layout(location = 1) out vec4 out1;
uniform bool oitPass;
uniform sampler2DShadow shadowMaps[TOTAL_SHADOW_MAPS];
struct ShadowInfo {
    mat4  viewToLightClip[NUM_CASCADES];
    float splits[NUM_CASCADES];
    bool  hasShadow;
};
uniform ShadowInfo shadowData[MAX_SHADOW_LIGHTS];
uniform int numShadowLights;
uniform int shadowLightIndex[MAX_SHADOW_LIGHTS];
uniform vec3 shadowLightDirs[MAX_SHADOW_LIGHTS];
float sampleCascade(int shadowIdx, int cascade, vec3 fragPos, float bias) {
    int mapIdx = shadowIdx * NUM_CASCADES + cascade;
    vec4 lc = shadowData[shadowIdx].viewToLightClip[cascade] * vec4(fragPos, 1.0);
    vec3 pc = lc.xyz / lc.w * 0.5 + 0.5;
    if (pc.x < 0.01 || pc.x > 0.99 ||
        pc.y < 0.01 || pc.y > 0.99 ||
        pc.z > 1.0  || pc.z < 0.0)
        return -1.0;
    if (PCF) {
        float shadow = 0.0;
        vec2 texelSize = vec2(1.0 / 2048.0);
        for (int x = -1; x <= 1; x++)
            for (int y = -1; y <= 1; y++)
                shadow += texture(shadowMaps[mapIdx],
                            vec3(pc.xy + vec2(x, y) * texelSize, pc.z - bias));
        return shadow / 9.0;
    } else {
        return texture(shadowMaps[mapIdx], vec3(pc.xy, pc.z - bias));
    }
}

float calcShadow(int shadowIdx, vec3 fragPos, vec3 N) {
    if (!shadowData[shadowIdx].hasShadow) return 1.0;
    vec3 L = normalize(-shadowLightDirs[shadowIdx]);
    float bias = max(0.05 * (1.0 - dot(N, L)), 0.005);
    float s0 = sampleCascade(shadowIdx, 0, fragPos, bias);
    float s1 = sampleCascade(shadowIdx, 1, fragPos, bias);
    if (s0 >= 0.0 && s1 >= 0.0) {
        if (blend == 0) return s0;
        if (blend == 1) return min(s0, s1);
        return (s0 + s1) * 0.5;
    }
    if (s0 >= 0.0) return s0;
    if (s1 >= 0.0) return s1;
    return 1.0;
}

float getShadowForLight(int sceneLightIdx, vec3 fragPos, vec3 N) {
    for (int s = 0; s < numShadowLights; s++) {
        if (shadowLightIndex[s] == sceneLightIdx)
            return calcShadow(s, fragPos, N);
    }
    return 1.0;
}
struct Material {
    sampler2D diffuseMap;
    sampler2D specularMap;
    sampler2D ambientMap;
    sampler2D normalMap;
    vec3  Kd;
    vec3  Ka;
    vec3  Ks;
    float Ns;
    float Ni;
    float d;
    float roughness;
};
uniform Material material;

uniform bool hasDiffuseMap;
uniform bool hasSpecularMap;
uniform bool hasAmbientMap;
uniform bool hasNormalMap;
uniform bool iscooktorrence;
uniform float cookambientStrength;
uniform float blinambientStrength;
uniform bool flipNormalY;
uniform bool debugcascades;
struct DirLight {
    vec3 direction;
    vec3 color;
};
uniform int numLights_camera;
uniform int numLights_world;
uniform DirLight camera_lights[MAX_LIGHTS];
uniform DirLight world_lights[MAX_LIGHTS];
vec3 getDiffuse() {
    if (hasDiffuseMap) return material.Kd * texture(material.diffuseMap, TexCoords).rgb;
    return material.Kd;
}
vec3 getSpecular() {
    if (hasSpecularMap) return material.Ks * texture(material.specularMap, TexCoords).rgb;
    return material.Ks;
}
vec3 getAmbient() {
    if (hasAmbientMap) return material.Ka * texture(material.ambientMap, TexCoords).rgb;
    return material.Ka;
}
vec3 getNormal() {
    if (hasNormalMap) {
        vec3 n = texture(material.normalMap, TexCoords).rgb;
        n = n * 2.0 - 1.0;
        if (flipNormalY) n.y = -n.y;
        return normalize(TBN * n);
    }
    return normalize(Normal);
}
vec3 CalcBlinnPhong(DirLight light, vec3 N, vec3 V, vec3 diffColor, vec3 specColor, float shadow) {
    vec3 L = normalize(-light.direction);
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = diff * diffColor * light.color;
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), material.Ns);
    vec3 specular = spec * specColor * light.color;
    return shadow * (diffuse + specular);
}

float DistributionBeckmann(vec3 N, vec3 H, float roughness) {
    float NdotH = max(dot(N, H), 0.0001);
    float cos2 = NdotH * NdotH;
    float cos4 = cos2 * cos2;
    float tan2 = (1.0 - cos2) / cos2;
    float m2 = roughness * roughness;
    return exp(-tan2 / m2) / (PI * m2 * cos4);
}
vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}
float GeometryCookTorrance(vec3 N, vec3 V, vec3 L, vec3 H) {
    float NdotH = max(dot(N, H), 0.0001);
    float NdotV = max(dot(N, V), 0.0001);
    float NdotL = max(dot(N, L), 0.0001);
    float VdotH = max(dot(V, H), 0.0001);
    float g1 = (2.0 * NdotH * NdotV) / VdotH;
    float g2 = (2.0 * NdotH * NdotL) / VdotH;
    return min(1.0, min(g1, g2));
}
vec3 CalcCookTorrance(DirLight light, vec3 N, vec3 V, vec3 albedo, vec3 F0, float rough, float shadow) {
    vec3 L = normalize(-light.direction);
    vec3 H = normalize(V + L);
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0001);
    float D = DistributionBeckmann(N, H, rough);
    float G = GeometryCookTorrance(N, V, L, H);
    vec3  F = FresnelSchlick(max(dot(H, V), 0.0), F0);
    vec3  num = D * G * F;
    float denom = 4.0 * NdotV * NdotL + 0.0001;
    vec3  spec = num / denom;
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    return shadow * (kD * albedo / PI + spec) * light.color * NdotL;
}
uniform bool       enableAO;
uniform float      f_factor;
uniform sampler2D  AOtexture;
uniform bool       debugAO;
uniform bool       scaleEntireScene;

float getOcclusion() {
    if (!enableAO) return 0.0;
    vec3 ndc = ClipPos.xyz / ClipPos.w;
    vec2 screenUV = ndc.xy * 0.5 + 0.5;
    return 1.0 - texture(AOtexture, screenUV).r;
}

void main() {
    vec3 N = getNormal();
    vec3 V = normalize(-FragPos);

    float a = getOcclusion();
    float aoMul = max(1.0 - a * f_factor, 0.0);

    vec3 totalLight;
    float alpha;

    if (!iscooktorrence) {
        vec3 diffColor = getDiffuse();
        vec3 specColor = getSpecular();
        vec3 ambient   = getAmbient() * blinambientStrength;

        vec3 result = vec3(0.0);
        for (int i = 0; i < numLights_camera; i++) {
            float sh = getShadowForLight(i, FragPos, N);
            result += CalcBlinnPhong(camera_lights[i], N, V, diffColor, specColor, sh);
        }
        for (int i = 0; i < numLights_world; i++) {
            float sh = getShadowForLight(numLights_camera + i, FragPos, N);
            result += CalcBlinnPhong(world_lights[i], N, V, diffColor, specColor, sh);
        }

        if (scaleEntireScene)
            totalLight = (ambient + result) * aoMul;
        else
            totalLight = ambient * aoMul + result;

        alpha = material.d;

    } else {
        vec3 albedo  = getDiffuse();
        vec3 ambient = getAmbient() * cookambientStrength * albedo;
        float rough  = clamp(material.roughness, 0.01, 1.0);
        float r  = (material.Ni - 1.0) / (material.Ni + 1.0);
        vec3  F0 = vec3(r * r);

        vec3 Lo = vec3(0.0);
        for (int i = 0; i < numLights_camera; i++) {
            float sh = getShadowForLight(i, FragPos, N);
            Lo += CalcCookTorrance(camera_lights[i], N, V, albedo, F0, rough, sh);
        }
        for (int i = 0; i < numLights_world; i++) {
            float sh = getShadowForLight(numLights_camera + i, FragPos, N);
            Lo += CalcCookTorrance(world_lights[i], N, V, albedo, F0, rough, sh);
        }

        if (scaleEntireScene)
            totalLight = (ambient + Lo) * aoMul;
        else
            totalLight = ambient * aoMul + Lo;

        alpha = material.d;
    }
    if (oitPass) {
    vec4 color = vec4(totalLight, alpha);
        float a = color.a;
        float z = gl_FragCoord.z;

        float colorFactor = max(min(1.0, max(max(color.r, color.g), color.b) * a), a);
        float depthFactor = clamp(0.03 / (1e-5 + pow(z / 200.0, 4.0)), 1e-2, 3e3);
        float w = colorFactor * depthFactor;

        out0 = vec4(color.rgb * a, a) * w;
        out1 = vec4(a, 0.0, 0.0, 0.0);
} else {
        out0 = vec4(totalLight, alpha);
        out1 = vec4(0.0);
        if (debugcascades && numShadowLights > 0) {
            vec3 cascadeColors[2] = vec3[2](
                vec3(1.0, 0.0, 0.0),
                vec3(0.0, 0.3, 1.0)
            );
            float s0 = sampleCascade(0, 0, FragPos, 0.005);
            float s1 = sampleCascade(0, 1, FragPos, 0.005);
            vec3 tint;
            if (s0 >= 0.0 && s1 >= 0.0)      tint = vec3(0.0, 1.0, 0.0);
            else if (s0 >= 0.0)               tint = cascadeColors[0];
            else if (s1 >= 0.0)               tint = cascadeColors[1];
            else                               tint = vec3(1.0, 1.0, 0.0);
            out0 = vec4(mix(out0.rgb, tint, 0.3), out0.a);
        }
        if (debugAO) {
            out0 = vec4(vec3(aoMul), 1.0);
        }
    }
}