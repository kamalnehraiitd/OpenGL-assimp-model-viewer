#version 410 core

#define MAX_LIGHTS 16
#define NUM_CASCADES 3
#define MAX_SHADOW_LIGHTS 3
#define TOTAL_SHADOW_MAPS (MAX_SHADOW_LIGHTS * NUM_CASCADES) 
const float PI = 3.14159265359;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in mat3 TBN;

out vec4 FragColor;


uniform sampler2DShadow shadowMaps[TOTAL_SHADOW_MAPS]; 

struct ShadowInfo {
    mat4  viewToLightClip[NUM_CASCADES];
    float splits[NUM_CASCADES];
    int   numCascades;
    bool  hasShadow;
};
uniform ShadowInfo shadowData[MAX_SHADOW_LIGHTS];
uniform int   numShadowLights;
uniform float shadowBias;

float calcShadow(int idx, vec3 fragPos) {
    if (!shadowData[idx].hasShadow) return 1.0;

    float depth = abs(fragPos.z);
    int numC = shadowData[idx].numCascades;
    int cascade = numC - 1;
    for (int i = 0; i < numC; i++) {
        if (depth < shadowData[idx].splits[i]) {
            cascade = i;
            break;
        }
    }

    vec4 lc = shadowData[idx].viewToLightClip[cascade] * vec4(fragPos, 1.0);
    vec3 pc = lc.xyz / lc.w * 0.5 + 0.5;

    if (pc.z > 1.0) return 1.0;

    
    int mapIdx = idx * NUM_CASCADES + cascade;

   
    float shadow = 0.0;
    vec2 texelSize = vec2(1.0 / 2048.0);
    for (int x = -1; x <= 1; x++)
        for (int y = -1; y <= 1; y++)
            shadow += texture(shadowMaps[mapIdx],
                        vec3(pc.xy + vec2(x, y) * texelSize, pc.z - shadowBias));
    return shadow / 9.0;
}


struct Material {
    sampler2D diffuseMap;
    sampler2D specularMap;
    sampler2D ambientMap;
    sampler2D normalMap;
    sampler2D opacityMap; 

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
    if (hasDiffuseMap)
        return material.Kd * texture(material.diffuseMap, TexCoords).rgb;
    return material.Kd;
}

vec3 getSpecular() {
    if (hasSpecularMap)
        return material.Ks * texture(material.specularMap, TexCoords).rgb;
    return material.Ks;
}

vec3 getAmbient() {
    if (hasAmbientMap)
        return material.Ka * texture(material.ambientMap, TexCoords).rgb;
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


vec3 CalcBlinnPhong(DirLight light, vec3 N, vec3 V, vec3 diffColor, vec3 specColor) {
    vec3 L = normalize(-light.direction);

    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = diff * diffColor * light.color;

    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), material.Ns);
    vec3 specular = spec * specColor * light.color;

    return diffuse + specular;
}


float DistributionBeckmann(vec3 N, vec3 H, float roughness) {
    float NdotH = max(dot(N, H), 0.0001);
    float cos2  = NdotH * NdotH;
    float cos4  = cos2 * cos2;
    float tan2  = (1.0 - cos2) / cos2;
    float m2    = roughness * roughness;
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

vec3 CalcCookTorrance(DirLight light, vec3 N, vec3 V, vec3 albedo, vec3 F0, float rough) {
    vec3 L = normalize(-light.direction);
    vec3 H = normalize(V + L);
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0001);

    float D = DistributionBeckmann(N, H, rough);
    float G = GeometryCookTorrance(N, V, L, H);
    vec3  F = FresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3  num   = D * G * F;
    float denom = 4.0 * NdotV * NdotL + 0.0001;
    vec3  spec  = num / denom;

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;

    return (kD * albedo / PI + spec) * light.color * NdotL;
}

/
void main() {
    vec3 N = getNormal();
    vec3 V = normalize(-FragPos);

    
    float shadow = 1.0;
    if (numShadowLights > 0)
        shadow = calcShadow(0, FragPos);

    if (!iscooktorrence) {
    
        vec3 diffColor = getDiffuse();
        vec3 specColor = getSpecular();
        vec3 ambient   = getAmbient() * blinambientStrength;

        vec3 result = vec3(0.0);
        for (int i = 0; i < numLights_camera; i++)
            result += CalcBlinnPhong(camera_lights[i], N, V, diffColor, specColor);
        for (int i = 0; i < numLights_world; i++)
            result += CalcBlinnPhong(world_lights[i], N, V, diffColor, specColor);

        FragColor = vec4(ambient + shadow * result, material.d);

    } else {
      
        vec3 albedo  = getDiffuse();
        vec3 ambient = getAmbient() * cookambientStrength * albedo;
        float rough  = clamp(material.roughness, 0.01, 1.0);

        float r  = (material.Ni - 1.0) / (material.Ni + 1.0);
        vec3  F0 = vec3(r * r);

        vec3 Lo = vec3(0.0);
        for (int i = 0; i < numLights_camera; i++)
            Lo += CalcCookTorrance(camera_lights[i], N, V, albedo, F0, rough);
        for (int i = 0; i < numLights_world; i++)
            Lo += CalcCookTorrance(world_lights[i], N, V, albedo, F0, rough);

        FragColor = vec4(ambient + shadow * Lo, material.d);
    }
    
}  