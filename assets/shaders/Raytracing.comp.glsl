#version 450

layout (local_size_x = 16, local_size_y = 16) in;
layout (binding = 0, rgba8) uniform writeonly image2D resultImage;

vec3 sphereCenter = vec3(0,0,2);
float sphereRadius = 0.5f;
vec3 sphereColor = vec3(0.8,0.05,0.06);

vec3 skyColor = vec3(0.5, 0.7, 1.0);
vec3 groundColor = vec3(1, 1, 1);

vec3 lightPos = vec3(2, 2, 0);

float HitSphere(vec3 center, float radius, vec3 origin, vec3 dir) {
    vec3 oc = origin - center;
    float a = dot(dir, dir);
    float b = 2.0 * dot(oc, dir);
    float c = dot(oc, oc) - radius*radius;
    float discriminant = b*b - 4*a*c;
    
    if (discriminant < 0) {
        return -1.0;
    }
    else {
        return (b - sqrt(discriminant) ) / (2.0*a);
    }
}

const float pi = 3.14159f;

// Schlick
vec3 Light_F(in vec3 f0, in float f90, in float VH) {
	return f0 + (f90-f0) * pow(1-VH, 5.0f);
}

// GGX
float Light_D(in float alphaSqr, in float NH) {
	float denom = NH * NH * (alphaSqr - 1) + 1;

	return alphaSqr / (pi * denom * denom);
}

float Light_V( in float NL, in float NV, in float alphaSqr ) {
	float Lambda_GGXV = NL * sqrt (( - NV * alphaSqr + NV ) * NV + alphaSqr );
	float Lambda_GGXL = NV * sqrt (( - NL * alphaSqr + NL ) * NL + alphaSqr );

	return 0.25f / ( Lambda_GGXV + Lambda_GGXL );
}

float Diff_Disney(float NdotV, float NdotL, float LdotH, float linearRoughness) {
	float energyBias = 0 * (1-linearRoughness) + 0.5*linearRoughness;
	float energyFactor = 1.0 * (1-linearRoughness) + linearRoughness / 1.51;
	float fd90 = energyBias + 2.0 * LdotH * LdotH * linearRoughness;
	vec3 f0 = vec3(1.0f , 1.0f , 1.0f);
	float lightScatter = Light_F( f0 , fd90 , NdotL ) .r;
	float viewScatter = Light_F(f0 , fd90 , NdotV ).r;

	return lightScatter * viewScatter * energyFactor;
}

vec3 BSDF(float NV, float NL, float LH, float NH, float alpha, vec3 SpecularColor) {
	vec3 f0 = 0.32*SpecularColor*SpecularColor;
	float f90 = clamp(50 * dot(f0, vec3(0.33)), 0, 1);
	
	vec3 F = Light_F(f0, f90, LH);
	float D = Light_D(alpha, NH);
	float Vis = Light_V(NL, NV, alpha);

	return (F * D * Vis);
}

vec3 LightPointCalc(
	in vec3 albedo,
	in vec3 position,
	in vec3 specularTexture,
	in float roughness,
	in vec3 normal,
	in vec3 lightPos,
	in float lightRadius,
	in vec3 lightColor,
	in vec3 eyePos
) {
	vec3 lightDir	= position - lightPos;
	float lightDistance	= length(lightDir);

	vec3 eyeDir		= normalize(eyePos - position);
	vec3 eyeReflect = reflect(-eyeDir, normal);
	
	lightDir		= -normalize(lightDir);

	float alpha = roughness * roughness;
	float alphaSqr = alpha * alpha;

	float NL = clamp(dot(normal, lightDir), 0, 1);
	
	float distSqr = lightDistance * lightDistance;
	float lightRadiusSqr = lightRadius * lightRadius;
	float attenuationFactor = distSqr / lightRadiusSqr;
	float attenuation = clamp(1 - attenuationFactor * attenuationFactor, 0, 1); 
	attenuation = attenuation * attenuation / (distSqr + 0.001);

	vec3 H = normalize(eyeDir + lightDir);
	
	float NV = abs(dot(normal, eyeDir)) + 0.00001;
	float NH = clamp(dot(normal, H), 0, 1);
	float LH = clamp(dot(lightDir, H), 0, 1);
	float VH = clamp(dot(eyeDir, H), 0, 1);

	vec3 specular = BSDF(NV, NL, LH, NH, alpha, specularTexture.rgb);
	vec3 diffuse = albedo.rgb * vec3(Diff_Disney(NV, NL, LH, roughness)) / pi;

	vec3 lightModifier = lightColor.xyz * attenuation;
	vec3 BSDFValue = diffuse + specular;
	return vec3(NL * BSDFValue * lightModifier);
}

vec3 RenderRay(vec3 origin, vec3 dir) {
    float hitDistanceAlongRay = HitSphere(sphereCenter, sphereRadius, origin, dir);
    if (hitDistanceAlongRay > 0.0) {
        vec3 hitPos = (origin - sphereCenter) + -hitDistanceAlongRay * dir;
        vec3 normal = normalize(hitPos);

		vec3 albedo = sphereColor;
		vec3 specularTexture = vec3(1);
		float roughness = 0;
		float lightRadius = 10;
		vec3 lightColor = vec3(1.0, 0.8, 0.6) * 20;
        
		return LightPointCalc(
			albedo,
			hitPos,
			specularTexture,
			roughness,
			normal,
			lightPos,
			lightRadius,
			lightColor,
			origin
		);
    }

    float t = dir.y;
    return mix(skyColor, groundColor, t);
}

void main() {
	ivec2 dim = imageSize(resultImage);
	vec2 uv = vec2(gl_GlobalInvocationID.xy) / dim;

    float aspect_ratio = float(dim.x) / float(dim.y);
    
    float viewport_height = 2.0;
    float viewport_width = aspect_ratio * viewport_height;
    float focal_length = 1.0;
    
	vec3 origin = vec3(0, 0, 0);
    vec3 horizontal = vec3(viewport_width, 0, 0);
    vec3 vertical = vec3(0, viewport_height, 0);
    vec3 lower_left_corner = origin - horizontal/2 - vertical/2 - vec3(0, 0, focal_length);

    vec3 fwd = vec3(0, 0, 1);
	vec3 dir = lower_left_corner + uv.x * horizontal + uv.y * vertical - origin;
	vec4 pixelColor = vec4(RenderRay(origin, dir), 1);
	imageStore(resultImage, ivec2(gl_GlobalInvocationID.xy), pixelColor);
}
