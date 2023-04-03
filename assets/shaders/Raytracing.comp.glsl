#version 450

layout (local_size_x = 16, local_size_y = 16) in;
layout (binding = 0, rgba8) uniform writeonly image2D resultImage;

vec3 skyColor = vec3(0.5, 0.7, 1.0);
vec3 horizonColor = vec3(1, 1, 1);
vec3 groundColor = vec3(0.5, 0.5, 0.5);

struct Light {
	vec3 position;
	vec3 color;
	float strength;
};

struct Material {
	vec3 albedo;
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
	float skyGradientTransition = pow(smoothstep(0.0f, -0.4f, dir.y), 0.35f);
	vec3 skyGradient = mix(horizonColor, skyColor, skyGradientTransition);
	float groundToSkyTransition = smoothstep(-0.01f, 0.0f, dir.y);
	return mix(skyGradient, groundColor, groundToSkyTransition);
}

HitInfo CalculateRayHit(Sphere[3] spheres, Ray ray) {
	HitInfo closestHit;
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

vec3 RenderRay(Sphere[3] spheres, Light light, Ray ray) {
	HitInfo hitInfo = CalculateRayHit(spheres, ray);
	if (hitInfo.isHit) {
		vec3 albedo = hitInfo.material.albedo;
		vec3 specularTexture = vec3(1);
		float roughness = hitInfo.material.roughness;
		vec3 lightColor = light.color * light.strength;
		
		vec3 lightDir	= light.position - hitInfo.position;
		float lightDistance	= length(lightDir);
		lightDir = normalize(lightDir);
		float NL = clamp(dot(hitInfo.normal, lightDir), 0, 1);
		
		return albedo * lightDistance * NL;
	}

	return GetSkyColor(ray.dir);
}

void main() {
	Sphere spheres[3];

	spheres[0].center = vec3(-1, 0, -2);
	spheres[0].radius = 0.2;
	spheres[0].material.albedo = vec3(0.05,0.06,0.8);
	spheres[0].material.roughness = 1;
	
	spheres[1].center = vec3(0, 0, -10);
	spheres[1].radius = 8;
	spheres[1].material.albedo = vec3(0.05,0.8,0.06);
	spheres[1].material.roughness = 0.5;
	
	spheres[2].center = vec3(1, 0, -2);
	spheres[2].radius = 0.5;
	spheres[2].material.albedo = vec3(0.8,0.05,0.06);
	spheres[2].material.roughness = 0.1;
	
	Light light1;
	light1.position = vec3(2, -2, 0);
	light1.color = vec3(1, 1, 1);
	light1.strength = 10;

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

	Ray ray;
	ray.origin = origin;
	ray.dir = dir;

	vec4 pixelColor = vec4(RenderRay(spheres, light1, ray), 1);
	imageStore(resultImage, ivec2(gl_GlobalInvocationID.xy), pixelColor);
}
