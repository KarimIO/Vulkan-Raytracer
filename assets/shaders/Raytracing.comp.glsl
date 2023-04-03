#version 450

layout (local_size_x = 16, local_size_y = 16) in;
layout (binding = 0, rgba8) uniform writeonly image2D resultImage;

layout(binding = 1) uniform UniformBufferObject {
	mat4 cameraMatrix;
	float time;
} ubo;

vec3 skyColor = vec3(0.11, 0.36, 0.57);
vec3 horizonColor = vec3(0.83, 0.82, 0.67);
vec3 groundColor = vec3(0.2, 0.2, 0.2);

struct Material {
	vec3 albedo;
	vec3 emission;
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
	vec3 sunLightDirection = normalize(vec3(0.2, 1, 1));
	float sunFocus = 200.0f;
	float sunIntensity = 10.0f;

	float skyGradientTransition = pow(smoothstep(0.0f, -0.4f, dir.y), 0.35f);
	vec3 skyGradient = mix(horizonColor, skyColor, skyGradientTransition);
	float groundToSkyTransition = smoothstep(-0.01f, 0.0f, dir.y);

	float sun = pow(max(0, dot(normalize(dir), -sunLightDirection)), sunFocus) * sunIntensity;
	float sunMask = groundToSkyTransition <= 0 ? 1.0f : 0.0f;
	float sunFactor = sun * sunMask;

	return mix(skyGradient, groundColor, groundToSkyTransition) + sunFactor;
}

HitInfo CalculateRayHit(Sphere[3] spheres, Ray ray) {
	HitInfo closestHit;
	closestHit.isHit = false;
	const float positiveInfinity = uintBitsToFloat(0x7F800000);
	closestHit.hitDistance = 100000000.0f;

	int numSpheres = 3;
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

vec3 Raytrace(Sphere[3] spheres, Ray originalRay, inout uint seed) {
	vec3 incomingLight = vec3(0);
	vec3 rayColor = vec3(1);
	
	Ray ray = originalRay;

	int maxBounceCount = 4;
	for (int i = 0; i <= maxBounceCount; ++i) {
		HitInfo hitInfo = CalculateRayHit(spheres, ray);
		if (hitInfo.isHit) {
			Material material = hitInfo.material;

			ray.dir = GetRayBounceDir(hitInfo.normal, ray.dir, material.roughness, seed);
			ray.origin = hitInfo.position + ray.dir * 0.001;

			incomingLight += material.emission * rayColor;
			rayColor *= material.albedo;
		}
		else {
			incomingLight += GetSkyColor(ray.dir) * rayColor;
			break;
		}
	}

	return incomingLight;
}

vec3 MultiRaytrace(Sphere[3] spheres, Ray ray, inout uint seed) {
	vec3 totalLight = vec3(0.0f);

	int numRaysPerPixel = 200;
	for (int i = 0; i < numRaysPerPixel; ++i) {
		totalLight += Raytrace(spheres, ray, seed);
	}

	return totalLight / numRaysPerPixel;
}

void main() {
	Sphere spheres[3];

	spheres[0].center = vec3(-1, 0.5, -3);
	spheres[0].radius = 0.4;
	spheres[0].material.albedo = vec3(0.05,0.06,0.8);
	spheres[0].material.emission = vec3(0);
	spheres[0].material.roughness = 1;
	
	spheres[1].center = vec3(0, 5.5, -3);
	spheres[1].radius = 5;
	spheres[1].material.albedo = vec3(0.8,0.05,0.06);
	spheres[1].material.emission = vec3(0);
	spheres[1].material.roughness = 0.05;
	
	spheres[2].center = vec3(1, 0, -3);
	spheres[2].radius = 0.5;
	spheres[2].material.emission = vec3(5.5);
	spheres[2].material.roughness = 0.0;
	
	ivec2 dim = imageSize(resultImage);
	vec2 uv = vec2(gl_GlobalInvocationID.xy) / dim;

	float aspect_ratio = float(dim.x) / float(dim.y);
	
	float viewport_height = 2.0;
	float viewport_width = aspect_ratio * viewport_height;
	float focal_length = 2.0;
	
	vec3 origin = vec3(0, 0, 0);
	vec3 horizontal = vec3(viewport_width, 0, 0);
	vec3 vertical = vec3(0, viewport_height, 0);
	vec3 lower_left_corner = origin - horizontal/2 - vertical/2 - vec3(0, 0, focal_length);

	vec3 fwd = vec3(0, 0, 1);
	vec3 dir = lower_left_corner + uv.x * horizontal + uv.y * vertical - origin;

	Ray ray;
	ray.origin = origin;
	ray.dir = normalize(dir);

	uint randomSeed = gl_GlobalInvocationID.x * dim.x + gl_GlobalInvocationID.y;
	vec3 randomDir = GetSeededRandomDir(randomSeed);
	vec4 pixelColor = vec4(MultiRaytrace(spheres, ray, randomSeed), 1);
	imageStore(resultImage, ivec2(gl_GlobalInvocationID.xy), pixelColor);
}
