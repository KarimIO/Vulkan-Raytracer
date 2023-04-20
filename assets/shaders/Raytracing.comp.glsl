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

const int NUM_SPHERES = 11;

struct Material {
	vec3 albedo;
	vec3 emission;
	float metalness;
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

HitInfo CalculateRayHit(Sphere[NUM_SPHERES] spheres, Ray ray) {
	HitInfo closestHit;
	closestHit.isHit = false;
	const float positiveInfinity = uintBitsToFloat(0x7F800000);
	closestHit.hitDistance = 100000000.0f;

	for (int i = 0; i < NUM_SPHERES; ++i) {
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

vec3 GetRayBounceDir(vec3 normal, vec3 rayDir, float roughness, float doSpecular, inout uint seed) {
	vec3 diffuseDir = normalize(normal + GetSeededRandomHemisphereDir(normal, seed));
	vec3 specularDir = reflect(rayDir, normal);

	vec3 specularAfterRoughness = mix(specularDir, diffuseDir, roughness * roughness);
	return mix(diffuseDir, specularAfterRoughness, doSpecular);
}

float BrdfFresnel(float cosTheta, float f0, float f90) {
	return f0 + (f90 - f0) * pow(1.0 - cosTheta, 5.0);
}

vec3 Raytrace(Sphere[NUM_SPHERES] spheres, Ray originalRay, inout uint seed) {
	vec3 incomingLight = vec3(0);
	vec3 rayColor = vec3(1);
	
	Ray ray = originalRay;

	for (int i = 0; i <= ubo.maxRayBounceCount; ++i) {
		HitInfo hitInfo = CalculateRayHit(spheres, ray);
		if (hitInfo.isHit) {
			Material material = hitInfo.material;

			vec3 wo = ray.dir;
			
			float f0 = material.metalness;
			float f90 = 1;
			float cosTheta = -dot(hitInfo.normal, wo);
			float specularChance = BrdfFresnel(cosTheta, f0, f90);
			float doSpecular = (GetNormalizedSeededRandom(seed) < specularChance) ? 1.0f : 0.0f;
			vec3 specularColor = mix(vec3(0.04), material.albedo, doSpecular);
			ray.dir = GetRayBounceDir(hitInfo.normal, ray.dir, material.roughness, doSpecular, seed);
			ray.origin = hitInfo.position + ray.dir * 0.001;

			incomingLight += material.emission * rayColor;
			rayColor *= mix(material.albedo, specularColor, doSpecular);
		}
		else {
			incomingLight += GetSkyColor(ray.dir) * rayColor;
			break;
		}
	}

	return incomingLight;
}

vec3 MultiRaytrace(Sphere[NUM_SPHERES] spheres, Ray ray, inout uint seed) {
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
	Sphere spheres[NUM_SPHERES];
	
	spheres[0].center = vec3(0, 11, 0);
	spheres[0].radius = 10;
	spheres[0].material.albedo = vec3(0.99,0.95,0.90);
	spheres[0].material.emission = vec3(0);
	spheres[0].material.roughness = 0.9;

	spheres[1].center = vec3(-2, 0.2, 0);
	spheres[1].radius = 0.4;
	spheres[1].material.albedo = vec3(1.0);
	spheres[1].material.metalness = 0;
	spheres[1].material.emission = vec3(0);
	spheres[1].material.roughness = 0.001;
	
	spheres[2].center = vec3(-1, 0.2, 0);
	spheres[2].radius = 0.4;
	spheres[2].material.albedo = vec3(1.0);
	spheres[2].material.metalness = 0;
	spheres[2].material.emission = vec3(0);
	spheres[2].material.roughness = 0.25;
	
	spheres[3].center = vec3(0, 0.2, 0);
	spheres[3].radius = 0.4;
	spheres[3].material.albedo = vec3(1.0);
	spheres[3].material.metalness = 0;
	spheres[3].material.emission = vec3(0);
	spheres[3].material.roughness = 0.5;
	
	spheres[4].center = vec3(1, 0.2, 0);
	spheres[4].radius = 0.4;
	spheres[4].material.albedo = vec3(1.0);
	spheres[4].material.metalness = 0;
	spheres[4].material.emission = vec3(0);
	spheres[4].material.roughness = 0.75;
	
	spheres[5].center = vec3(2, 0.2, 0);
	spheres[5].radius = 0.4;
	spheres[5].material.albedo = vec3(1.0);
	spheres[5].material.metalness = 0;
	spheres[5].material.emission = vec3(0);
	spheres[5].material.roughness = 1;

	spheres[6].center = vec3(-2, -1, 0);
	spheres[6].radius = 0.4;
	spheres[6].material.albedo = vec3(1.0);
	spheres[6].material.metalness = 1;
	spheres[6].material.emission = vec3(0);
	spheres[6].material.roughness = 0.001;
	
	spheres[7].center = vec3(-1, -1, 0);
	spheres[7].radius = 0.4;
	spheres[7].material.albedo = vec3(1.0);
	spheres[7].material.metalness = 1;
	spheres[7].material.emission = vec3(0);
	spheres[7].material.roughness = 0.25;
	
	spheres[8].center = vec3(0, -1, 0);
	spheres[8].radius = 0.4;
	spheres[8].material.albedo = vec3(1.0);
	spheres[8].material.metalness = 1;
	spheres[8].material.emission = vec3(0);
	spheres[8].material.roughness = 0.5;
	
	spheres[9].center = vec3(1, -1, 0);
	spheres[9].radius = 0.4;
	spheres[9].material.albedo = vec3(1.0);
	spheres[9].material.metalness = 1;
	spheres[9].material.emission = vec3(0);
	spheres[9].material.roughness = 0.75;
	
	spheres[10].center = vec3(2, -1, 0);
	spheres[10].radius = 0.4;
	spheres[10].material.albedo = vec3(1.0);
	spheres[10].material.metalness = 1;
	spheres[10].material.emission = vec3(0);
	spheres[10].material.roughness = 1;
	
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
