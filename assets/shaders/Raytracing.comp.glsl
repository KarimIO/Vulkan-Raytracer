#version 450

layout (local_size_x = 16, local_size_y = 16) in;
layout (binding = 0, rgba8) uniform writeonly image2D resultImage;

layout(binding = 1) uniform UniformBufferObject {
	mat4 cameraToWorld;
	mat4 cameraInverseProj;
	float time;
	int maxRayBounceCount;
	int numRaysPerPixel;

	vec3 sunLightDirection;
	vec3 skyColor;
	vec3 horizonColor;
	vec3 groundColor;
	float sunFocus;
	float sunIntensity;
} ubo;

const int numSpheres = 4;

struct Material {
	vec3 albedo;
	vec3 emission;
	float metallic;
	float roughness;
};

struct Sphere {
	vec3 center;
	float radius;
	Material material;
};

struct Ray {
	vec3 origin;
	vec3 dir;
};

struct HitInfo {
	bool isHit;
	float hitDistance;
	vec3 position;
	vec3 normal;
	Material material;
};

vec3 SRGBToLinear(vec3 inColor) {
	vec3 outColor;
	outColor.r = inColor.r <= 0.04045 ? (inColor.r / 12.92) : pow((inColor.r + 0.055) / 1.055, 2.4);
	outColor.g = inColor.g <= 0.04045 ? (inColor.g / 12.92) : pow((inColor.g + 0.055) / 1.055, 2.4);
	outColor.b = inColor.b <= 0.04045 ? (inColor.b / 12.92) : pow((inColor.b + 0.055) / 1.055, 2.4);
	return outColor;
}

vec3 LinearToSRGB(vec3 inColor) {
	vec3 outColor;
	outColor.r = inColor.r <= 0.0031308 ? (inColor.r * 12.92) : 1.055 * pow(inColor.r, 1.0/2.4) - 0.055;
	outColor.g = inColor.g <= 0.0031308 ? (inColor.g * 12.92) : 1.055 * pow(inColor.g, 1.0/2.4) - 0.055;
	outColor.b = inColor.b <= 0.0031308 ? (inColor.b * 12.92) : 1.055 * pow(inColor.b, 1.0/2.4) - 0.055;
	return outColor;
}

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
		return (-b - sqrt(discriminant) ) / (2.0*a);
	}
}

HitInfo IntersectSphere(Sphere sphere, Ray ray) {
	HitInfo hitInfo;

	float hitDistanceAlongRay = HitSphere(sphere.center, sphere.radius, ray.origin, ray.dir);
	if (hitDistanceAlongRay > 0.0) {
		vec3 position = ray.origin + hitDistanceAlongRay * ray.dir;
		vec3 normal = normalize(position - sphere.center);

		hitInfo.isHit = true;
		hitInfo.hitDistance = hitDistanceAlongRay;
		hitInfo.position = position;
		hitInfo.normal = normal;
		return hitInfo;
	}
	
	hitInfo.isHit = false;
	hitInfo.hitDistance = -1;
	return hitInfo;
}

vec3 GetSkyColor(vec3 dir) {
	float skyGradientTransition = pow(smoothstep(0.0f, -0.4f, dir.y), 0.35f);
	vec3 skyGradient = mix(ubo.horizonColor, ubo.skyColor, skyGradientTransition);
	float groundToSkyTransition = smoothstep(-0.01f, 0.0f, dir.y);

	float sun = pow(max(0, dot(normalize(dir), -ubo.sunLightDirection)), ubo.sunFocus) * ubo.sunIntensity;
	float sunMask = groundToSkyTransition <= 0 ? 1.0f : 0.0f;
	float sunFactor = sun * sunMask;

	vec3 finalSkyColor = mix(skyGradient, ubo.groundColor, groundToSkyTransition) + sunFactor;
	return SRGBToLinear(finalSkyColor);
}

HitInfo CalculateRayHit(Sphere[numSpheres] spheres, Ray ray) {
	HitInfo closestHit;
	closestHit.isHit = false;
	const float positiveInfinity = uintBitsToFloat(0x7F800000);
	closestHit.hitDistance = 100000000.0f;

	for (int i = 0; i < numSpheres; ++i) {
		Sphere sphere = spheres[i];

		HitInfo hitInfo = IntersectSphere(sphere, ray);
		if (hitInfo.isHit && closestHit.hitDistance > hitInfo.hitDistance) {
			closestHit = hitInfo;
			closestHit.material = sphere.material;
		}
	}

	return closestHit;
}

float GetSeededRandom(inout uint seed) {
	seed = seed * 747796405 + 2891336453;
	uint result = ((seed >> ((seed >> 28) + 4)) ^ seed) * 277803737;
	result = (result >> 22) ^ result;
	return result / 4294967295.0;
}

float GetNormalizedSeededRandom(inout uint seed) {
	float theta = 2 * 3.1415926 * GetSeededRandom(seed);
	float rho = sqrt(-2 * log(GetSeededRandom(seed)));
	
	return rho * cos(theta);
}

vec3 GetSeededRandomDir(inout uint seed) {
	float x = GetNormalizedSeededRandom(seed) * 2 - 1;
	float y = GetNormalizedSeededRandom(seed) * 2 - 1;
	float z = GetNormalizedSeededRandom(seed) * 2 - 1;

	return normalize(vec3(x, y, z));
}

vec3 GetSeededRandomHemisphereDir(vec3 normal, inout uint seed) {
	vec3 randomDir = GetSeededRandomDir(seed);

	return randomDir * sign(dot(normal, randomDir));
}

vec3 GetRayBounceDir(vec3 normal, vec3 rayDir, float roughness, inout uint seed) {
	vec3 diffuseDir = normalize(normal + GetSeededRandomHemisphereDir(normal, seed));
	vec3 specularDir = reflect(rayDir, normal);

	return mix(specularDir, diffuseDir, roughness);
}

vec3 BrdfFresnel(float cosTheta, vec3 f0) {
	return f0 + (1.0 - f0) * pow(1.0 - cosTheta, 5.0);
}

vec3 FSpecularCookTorrance(vec3 wi, vec3 wo, vec3 normal, vec3 albedoColor, float metalness) {
	vec3 H = normalize(wi + wo);
	float wiDotN = max(dot(wi, normal), 0.0f);
	float woDotN = max(dot(wo, normal), 0.0f);
	float cosTheta = max(dot(H, wo), 0.0f);
	vec3 f0 = vec3(1); //mix(vec3(0.04), albedoColor, metalness);
	vec3 fresnel = BrdfFresnel(cosTheta, f0);
	return fresnel; // / (4 * woDotN * wiDotN);
}

// Cook-Torrance BRDF
// Represents fr in Lo(p,wo)=Integral(fr(p,Wi,Wo) * Li(p,Wi) * dot(n, Wi) * dWi);
vec3 Brdf(vec3 wi, vec3 wo, vec3 normal, vec3 albedoColor, float metalness) {
	float kDiffuse = 1.0f;
	const float PI = 3.14159f;
	vec3 fLambert = albedoColor / PI;
	vec3 fSpecular = FSpecularCookTorrance(wi, wo, normal, albedoColor, metalness);

	// But kSpecular is considered in fSpecular so we don't need kSpecular here.
	return fSpecular; // kDiffuse * fLambert + 
}

vec3 Raytrace(Sphere[numSpheres] spheres, Ray originalRay, inout uint seed) {
	vec3 incomingLight = vec3(0);
	vec3 rayColor = vec3(1);
	
	Ray ray = originalRay;

	for (int i = 0; i <= ubo.maxRayBounceCount; ++i) {
		HitInfo hitInfo = CalculateRayHit(spheres, ray);
		if (hitInfo.isHit) {
			Material material = hitInfo.material;

			vec3 wo = ray.dir;

			ray.dir = GetRayBounceDir(hitInfo.normal, ray.dir, material.roughness, seed);
			ray.origin = hitInfo.position + ray.dir * 0.001;

			vec3 wi = ray.dir;
			float nDotWi = dot(hitInfo.normal, wi);
			float dW = 1.0f;
			// vec3 reflectanceEquation = Brdf(wi, wo, hitInfo.normal, hitInfo.material.albedo, hitInfo.material.metallic); // * incomingLight * nDotWi * dW;

			incomingLight += material.emission * rayColor;
			rayColor *= hitInfo.material.albedo;
		}
		else {
			incomingLight += GetSkyColor(ray.dir) * rayColor;
			break;
		}
	}

	return incomingLight;
}

vec3 MultiRaytrace(Sphere[numSpheres] spheres, Ray ray, inout uint seed) {
	vec3 totalLight = vec3(0.0f);

	for (int i = 0; i < ubo.numRaysPerPixel; ++i) {
		totalLight += Raytrace(spheres, ray, seed);
	}

	return totalLight / ubo.numRaysPerPixel;
}

Ray CreateCameraRay(vec2 uv) {
	uv = uv * 2 - 1;

	vec3 origin = (ubo.cameraToWorld * vec4(0,0,0,1)).xyz;
	vec3 direction = (ubo.cameraInverseProj * vec4(uv,0,1)).xyz;
	direction = (ubo.cameraToWorld * vec4(direction,0)).xyz;
	direction = normalize(direction);

	Ray ray;
	ray.origin = origin;
	ray.dir = direction;

	return ray;
}

vec3 ACESFilmicTonemapping(vec3 color) {
	float a = 2.51f;
	float b = 0.03f;
	float c = 2.43f;
	float d = 0.59f;
	float e = 0.14f;

	return clamp((color*(a*color+b))/(color*(c*color+d)+e), 0, 1);
}

void main() {
	Sphere spheres[numSpheres];

	spheres[0].center = vec3(-0.9, 0.2, 0);
	spheres[0].radius = 0.4;
	spheres[0].material.albedo = vec3(0.05,0.06,0.8);
	spheres[0].material.emission = vec3(0);
	spheres[0].material.roughness = 0.95;
	
	spheres[1].center = vec3(0, 5.5, 0);
	spheres[1].radius = 5;
	spheres[1].material.albedo = vec3(0.99,0.95,0.90);
	spheres[1].material.emission = vec3(0);
	spheres[1].material.roughness = 0.9;
	
	spheres[2].center = vec3(0, sin(ubo.time) * 0.5, 0);
	spheres[2].radius = 0.5;
	spheres[2].material.albedo = vec3(1.0, 1.0, 1.0);
	spheres[2].material.emission = vec3(0);
	spheres[2].material.roughness = 0.05;
	
	spheres[3].center = vec3(1.2, 0, 0);
	spheres[3].radius = 0.5;
	spheres[3].material.emission = vec3(5.5);
	spheres[3].material.roughness = 0.0;
	
	ivec2 dim = imageSize(resultImage);
	vec2 uv = vec2(gl_GlobalInvocationID.xy) / dim;

	Ray ray = CreateCameraRay(uv);

	uint randomSeed = gl_GlobalInvocationID.x * dim.x + gl_GlobalInvocationID.y;
	vec3 randomDir = GetSeededRandomDir(randomSeed);
	vec3 pixelColor = MultiRaytrace(spheres, ray, randomSeed);

	const float exposure = 0.8f;
	pixelColor *= exposure;
 
	// convert unbounded HDR color range to SDR color range
	pixelColor = ACESFilmicTonemapping(pixelColor);
 
	// convert from linear to sRGB for display
	pixelColor = LinearToSRGB(pixelColor);

	imageStore(resultImage, ivec2(gl_GlobalInvocationID.xy), vec4(pixelColor, 1));
}
