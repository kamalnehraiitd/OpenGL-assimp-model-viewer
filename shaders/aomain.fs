#version 410 core

out float FragColor;
in vec2 uv;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D noiseTex;

uniform vec3  hemisphere[64];
uniform int   sampleCount;  
uniform float occlusionRadius;
uniform float depthBias;
uniform vec2  framebufferSize; 
uniform mat4  proj;

void main() {
    vec3 origin = texture(gPosition, uv).xyz;
    vec3 N      = normalize(texture(gNormal, uv).rgb);


    vec2 noiseUV   = uv * (framebufferSize / 4.0);
    vec3 randDir   = normalize(texture(noiseTex, noiseUV).xyz);

    
    vec3 T = normalize(randDir - N * dot(randDir, N));
    vec3 B = cross(N, T);
    mat3 orientBasis = mat3(T, B, N);

    float totalOcclusion = 0.0;

    for (int i = 0; i < sampleCount; ++i) {
      
        vec3 samplePt = origin + orientBasis * hemisphere[i] * occlusionRadius;

       
        vec4 clipPos = proj * vec4(samplePt, 1.0);
        vec2 sampleUV = (clipPos.xy / clipPos.w) * 0.5 + 0.5;

       
        float storedZ = texture(gPosition, sampleUV).z;

       
        float gap       = abs(origin.z - storedZ);
        float falloff   = 1.0 - clamp(gap / occlusionRadius, 0.0, 1.0);
        float occluded  = (storedZ >= samplePt.z + depthBias) ? 1.0 : 0.0;

        totalOcclusion += occluded * falloff;
    }

   
    FragColor = 1.0 - (totalOcclusion / float(sampleCount));
}
