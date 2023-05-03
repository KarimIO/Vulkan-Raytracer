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
	float dofStrength;
	float blurStrength;
} ubo;

const int NUM_SPHERES = 16;

struct Material {
	vec3 albedo;
	vec3 specular;
	float specularChance;
	float ior;
	vec3 emission;
	float surfaceRoughness;
	float refractionRoughness;
	vec3 transmissionColor;
	float refractionChance;
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
	bool isInside;
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

bool HitSphere(vec3 center, float radius, vec3 origin, vec3 dir, out float distance, out bool isInside) {
	vec3 oc = origin - center;
	float b = dot(oc, dir);
	float c = dot(oc, oc) - radius*radius;
	
	if(c > 0.0 && b > 0.0) {
		return false;
	}
	
	float discriminant = b*b - c;
	
	if (discriminant < 0) {
		return false;
	}
	
	isInside = false;
	distance = -b - sqrt(discriminant);

	if (distance < 0.0f) {
		distance = -b + sqrt(discriminant);
		isInside = true;
	}

	if (distance > 0.0f) {
		return true;
	}
	
	// In this case, the entire sphere is behind the camera
	return false;
}

HitInfo IntersectSphere(Sphere sphere, Ray ray) {
	HitInfo hitInfo;

	bool isInside;
	float hitDistanceAlongRay;
	bool hasHit = HitSphere(sphere.center, sphere.radius, ray.origin, ray.dir, hitDistanceAlongRay, isInside);
	if (hasHit) {
		vec3 position = ray.origin + hitDistanceAlongRay * ray.dir;
		vec3 normal = normalize(position - sphere.center) * (isInside ? -1.0f : 1.0f);

		hitInfo.isHit = true;
		hitInfo.hitDistance = hitDistanceAlongRay;
		hitInfo.position = position;
		hitInfo.normal = normal;
		hitInfo.isInside = isInside;
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
	return finalSkyColor; //SRGBToLinear(finalSkyColor);
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

vec2 GetSeededRandomInCircle(inout uint seed) {
	float x = GetNormalizedSeededRandom(seed) * 2 - 1;
	float y = GetNormalizedSeededRandom(seed) * 2 - 1;

	return normalize(vec2(x, y));
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

vec3 GetRayBounceDir(vec3 normal, vec3 incidentRayDir, float surfaceRoughness, float refractionRoughness, float doSpecular, float doRefraction, bool isInside, float ior, inout uint seed) {
	vec3 diffuseDir = normalize(normal + GetSeededRandomDir(seed));
	vec3 specularDir = reflect(incidentRayDir, normal);

	vec3 roughRefractionDir = normalize(-normal + GetSeededRandomDir(seed));
	float refractCoefficient = isInside ? ior : 1.0f / ior;
	vec3 pureRefractionDir = refract(incidentRayDir, normal, refractCoefficient);

	vec3 specularAfterSurfaceRoughness = normalize(mix(specularDir, diffuseDir, surfaceRoughness * surfaceRoughness));
	vec3 refractionAfterSurfaceRoughness = normalize(mix(pureRefractionDir, roughRefractionDir, refractionRoughness * refractionRoughness));
	
	vec3 rayDir = mix(diffuseDir, specularAfterSurfaceRoughness, doSpecular);
	return mix(rayDir, refractionAfterSurfaceRoughness, doRefraction);
}

float FresnelReflectAmount(float ior1, float ior2, vec3 normal, vec3 incident, float f0, float f90) {
	// Schlick aproximation
	float r0 = (ior1 - ior2) / (ior1 + ior2);
	r0 *= r0;
	float cosX = -dot(normal, incident);
	if (ior1 > ior2) {
		float n = ior1 / ior2;
		float sinT2 = n * n * (1.0 - cosX * cosX);

		// Total internal reflection
		if (sinT2 > 1.0) {
			return f90;
		}

		cosX = sqrt(1.0 - sinT2);
	}
	
	float x = 1.0 - cosX;
	float ret = r0 + (1.0 - r0) * x*x*x*x*x;
 
	// adjust reflect multiplier for object reflectivity
	return mix(f0, f90, ret);
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
			
			if (hitInfo.isInside) {
				rayColor *= exp(-material.transmissionColor * hitInfo.hitDistance);
			}

			float specularChance = material.specularChance;
			float refractionChance = material.refractionChance;
			float f90 = 1;
			if (specularChance > 0.0f) {
				float iorStart = hitInfo.isInside ? material.ior : 1.0f;
				float iorEnd = hitInfo.isInside ? 1.0f : material.ior;
				specularChance = FresnelReflectAmount(
					iorStart,
					iorEnd,
					wo,
					hitInfo.normal,
					specularChance,
					f90
				);
				
				float chanceMultiplier = (1.0f - specularChance) / (1.0f - material.specularChance);
				refractionChance *= chanceMultiplier;
			}
			
			float raySelectionRandom = GetSeededRandom(seed);
			float doRefraction = 0.0f;
			float doSpecular = 0.0f;
			float rayProbability = 1.0f;
			if (specularChance > 0.0f && raySelectionRandom < specularChance) {
				doSpecular = 1.0f;
				rayProbability = specularChance;
			}
			else if (refractionChance > 0.0f && raySelectionRandom < specularChance + refractionChance) {
				doRefraction = 1.0f;
				rayProbability = refractionChance;
			}
			else {
				rayProbability = 1.0f - (specularChance + refractionChance);
			}

			// Don't divide by zero
			rayProbability = max(rayProbability, 0.001f);

			if (doRefraction == 1.0f) {
				ray.origin = hitInfo.position - hitInfo.normal * 0.001;
			}
			else {
				ray.origin = hitInfo.position + hitInfo.normal * 0.001;
			}

			ray.dir = GetRayBounceDir(hitInfo.normal, ray.dir, material.surfaceRoughness, material.refractionRoughness, doSpecular, doRefraction, hitInfo.isInside, material.ior, seed);
			
			incomingLight += material.emission * rayColor;
			if (doRefraction == 0.0f) {
				rayColor *= mix(material.albedo, material.specular, doSpecular);
			}

			rayColor /= rayProbability;

			// Russian Roulette to terminate early
			float probabilityToEnd = max(rayColor.r, max(rayColor.g, rayColor.b));
			if (GetSeededRandom(seed) > probabilityToEnd) {
				break;
			}
 
			// Add the energy we 'lose' by randomly terminating paths
			rayColor *= 1.0f / probabilityToEnd;
		}
		else {
			incomingLight += GetSkyColor(ray.dir) * rayColor;
			break;
		}
	}

	return incomingLight;
}

Ray CreateCameraRay(
	vec3 camOrigin,
	vec3 camDirection,
	vec3 camRight,
	vec3 camUp,
	vec2 uv,
	ivec2 dim,
	inout uint seed
) {	
	vec2 originNoise = GetSeededRandomInCircle(seed) * ubo.dofStrength / dim.x;
	vec2 targetNoise = GetSeededRandomInCircle(seed) * ubo.blurStrength / dim.x;

	vec3 originNoiseOffset = camRight * originNoise.x + camUp * originNoise.y;
	vec3 targetNoiseOffset = camRight * targetNoise.x + camUp * targetNoise.y;

	Ray ray;
	ray.origin = camOrigin + originNoiseOffset;

	vec3 camTarget = ray.origin + camDirection + targetNoiseOffset;
	ray.dir = normalize(camTarget - ray.origin);

	return ray;
}

vec3 MultiRaytrace(Sphere[NUM_SPHERES] spheres, vec2 uv, ivec2 dim, inout uint seed) {
	vec3 totalLight = vec3(0.0f);

	uv = uv * 2 - 1;

	vec3 camOrigin = (ubo.cameraToWorld * vec4(0,0,0,1)).xyz;
	vec3 camDirection = (ubo.cameraInverseProj * vec4(uv,0,1)).xyz;
	camDirection = (ubo.cameraToWorld * vec4(camDirection, 0)).xyz;

	vec3 camRight = vec3(ubo.cameraToWorld[0][0], ubo.cameraToWorld[1][0], ubo.cameraToWorld[2][0]);
	vec3 camUp = vec3(ubo.cameraToWorld[0][1], ubo.cameraToWorld[1][1], ubo.cameraToWorld[2][1]);

	for (int i = 0; i < ubo.numRaysPerPixel; ++i) {
		Ray ray = CreateCameraRay(camOrigin, camDirection, camRight, camUp, uv, dim, seed);
		totalLight += Raytrace(spheres, ray, seed);
	}

	return totalLight / ubo.numRaysPerPixel;
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
	spheres[0].material.albedo = vec3(0);
	spheres[0].material.specular = vec3(0.04);
	spheres[0].material.specularChance = 0.3;
	spheres[0].material.ior = 2.950;
	spheres[0].material.emission = vec3(4);
	spheres[0].material.surfaceRoughness = 0.9;
	spheres[0].material.refractionRoughness = 0;
	spheres[0].material.transmissionColor = vec3(1);
	spheres[0].material.refractionChance = 0;

	spheres[1].center = vec3(-2, 0.2, 0);
	spheres[1].radius = 0.4;
	spheres[1].material.albedo = vec3(1.0, 0.0, 0.0);
	spheres[1].material.specular = vec3(0.04);
	spheres[1].material.specularChance = 0.04;
	spheres[1].material.ior = 1.5;
	spheres[1].material.emission = vec3(0);
	spheres[1].material.surfaceRoughness = 0.001;
	spheres[1].material.refractionRoughness = 0;
	spheres[1].material.transmissionColor = vec3(1);
	spheres[1].material.refractionChance = 0;
	
	spheres[2].center = vec3(-1, 0.2, 0);
	spheres[2].radius = 0.4;
	spheres[2].material.albedo = vec3(1.0, 0.0, 0.0);
	spheres[2].material.specular = vec3(0.04);
	spheres[2].material.specularChance = 0.04;
	spheres[2].material.ior = 1.5;
	spheres[2].material.emission = vec3(0);
	spheres[2].material.surfaceRoughness = 0.25;
	spheres[2].material.refractionRoughness = 0;
	spheres[2].material.transmissionColor = vec3(1);
	spheres[2].material.refractionChance = 0;
	
	spheres[3].center = vec3(0, 0.2, 0);
	spheres[3].radius = 0.4;
	spheres[3].material.albedo = vec3(1.0, 0.0, 0.0);
	spheres[3].material.specular = vec3(0.04);
	spheres[3].material.specularChance = 0.04;
	spheres[3].material.ior = 1.5;
	spheres[3].material.emission = vec3(0);
	spheres[3].material.surfaceRoughness = 0.5;
	spheres[3].material.refractionRoughness = 0;
	spheres[3].material.transmissionColor = vec3(1);
	spheres[3].material.refractionChance = 0;
	
	spheres[4].center = vec3(1, 0.2, 0);
	spheres[4].radius = 0.4;
	spheres[4].material.albedo = vec3(1.0, 0.0, 0.0);
	spheres[4].material.specular = vec3(0.04);
	spheres[4].material.specularChance = 0.04;
	spheres[4].material.ior = 1.5;
	spheres[4].material.emission = vec3(0);
	spheres[4].material.surfaceRoughness = 0.75;
	spheres[4].material.refractionRoughness = 0;
	spheres[4].material.transmissionColor = vec3(1);
	spheres[4].material.refractionChance = 0;
	
	spheres[5].center = vec3(2, 0.2, 0);
	spheres[5].radius = 0.4;
	spheres[5].material.albedo = vec3(1.0, 0.0, 0.0);
	spheres[5].material.specular = vec3(0.04);
	spheres[5].material.specularChance = 0.04;
	spheres[5].material.ior = 1.5;
	spheres[5].material.emission = vec3(0);
	spheres[5].material.surfaceRoughness = 1;
	spheres[5].material.refractionRoughness = 0;
	spheres[5].material.transmissionColor = vec3(1);
	spheres[5].material.refractionChance = 0;

	spheres[6].center = vec3(-2, -1, 0);
	spheres[6].radius = 0.4;
	spheres[6].material.albedo = vec3(0.944, 0.776, 0.373);
	spheres[6].material.specular = vec3(0.944, 0.776, 0.373);
	spheres[6].material.specularChance = 0.9;
	spheres[6].material.ior = 2.950;
	spheres[6].material.emission = vec3(0);
	spheres[6].material.surfaceRoughness = 0.001;
	spheres[6].material.refractionRoughness = 0;
	spheres[6].material.transmissionColor = vec3(1);
	spheres[6].material.refractionChance = 0;
	
	spheres[7].center = vec3(-1, -1, 0);
	spheres[7].radius = 0.4;
	spheres[7].material.albedo = vec3(0.944, 0.776, 0.373);
	spheres[7].material.specular = vec3(0.944, 0.776, 0.373);
	spheres[7].material.specularChance = 0.9;
	spheres[7].material.ior = 2.950;
	spheres[7].material.emission = vec3(0);
	spheres[7].material.surfaceRoughness = 0.25;
	spheres[7].material.refractionRoughness = 0;
	spheres[7].material.transmissionColor = vec3(1);
	spheres[7].material.refractionChance = 0;
	
	spheres[8].center = vec3(0, -1, 0);
	spheres[8].radius = 0.4;
	spheres[8].material.albedo = vec3(0.944, 0.776, 0.373);
	spheres[8].material.specular = vec3(0.944, 0.776, 0.373);
	spheres[8].material.specularChance = 0.9;
	spheres[8].material.ior = 2.950;
	spheres[8].material.emission = vec3(0);
	spheres[8].material.surfaceRoughness = 0.5;
	spheres[8].material.refractionRoughness = 0;
	spheres[8].material.transmissionColor = vec3(1);
	spheres[8].material.refractionChance = 0;
	
	spheres[9].center = vec3(1, -1, 0);
	spheres[9].radius = 0.4;
	spheres[9].material.albedo = vec3(0.944, 0.776, 0.373);
	spheres[9].material.specular = vec3(0.944, 0.776, 0.373);
	spheres[9].material.specularChance = 0.9;
	spheres[9].material.ior = 2.950;
	spheres[9].material.emission = vec3(0);
	spheres[9].material.surfaceRoughness = 0.75;
	spheres[9].material.refractionRoughness = 0;
	spheres[9].material.transmissionColor = vec3(1);
	spheres[9].material.refractionChance = 0;
	
	spheres[10].center = vec3(2, -1, 0);
	spheres[10].radius = 0.4;
	spheres[10].material.albedo = vec3(0.944, 0.776, 0.373);
	spheres[10].material.specular = vec3(0.944, 0.776, 0.373);
	spheres[10].material.specularChance = 0.9;
	spheres[10].material.ior = 2.950;
	spheres[10].material.emission = vec3(0);
	spheres[10].material.surfaceRoughness = 1;
	spheres[10].material.refractionRoughness = 0;
	spheres[10].material.transmissionColor = vec3(1);
	spheres[10].material.refractionChance = 0;
	
	spheres[11].center = vec3(-2, -2.2, 0);
	spheres[11].radius = 0.4;
	spheres[11].material.albedo = vec3(1);
	spheres[11].material.specular = vec3(1);
	spheres[11].material.specularChance = 0.0f;
	spheres[11].material.ior = 1.52;
	spheres[11].material.emission = vec3(0);
	spheres[11].material.surfaceRoughness = 0.0f;
	spheres[11].material.refractionRoughness = 0.0f;
	spheres[11].material.transmissionColor = vec3(1);
	spheres[11].material.refractionChance = 1.0f;
	
	spheres[12].center = vec3(-1, -2.2, 0);
	spheres[12].radius = 0.4;
	spheres[12].material.albedo = vec3(1);
	spheres[12].material.specular = vec3(1);
	spheres[12].material.specularChance = 0.0f;
	spheres[12].material.ior = 1.52;
	spheres[12].material.emission = vec3(0);
	spheres[12].material.surfaceRoughness = 0.0f;
	spheres[12].material.refractionRoughness = 0.3f;
	spheres[12].material.transmissionColor = vec3(1);
	spheres[12].material.refractionChance = 1.0f;
	
	spheres[13].center = vec3(0, -2.2, 0);
	spheres[13].radius = 0.4;
	spheres[13].material.albedo = vec3(1);
	spheres[13].material.specular = vec3(1);
	spheres[13].material.specularChance = 0.0f;
	spheres[13].material.ior = 1.52;
	spheres[13].material.emission = vec3(0);
	spheres[13].material.surfaceRoughness = 0.0f;
	spheres[13].material.refractionRoughness = 0.5f;
	spheres[13].material.transmissionColor = vec3(1);
	spheres[13].material.refractionChance = 1.0f;
	
	spheres[14].center = vec3(1, -2.2, 0);
	spheres[14].radius = 0.4;
	spheres[14].material.albedo = vec3(1);
	spheres[14].material.specular = vec3(1);
	spheres[14].material.specularChance = 0.0f;
	spheres[14].material.ior = 1.52;
	spheres[14].material.emission = vec3(0);
	spheres[14].material.surfaceRoughness = 0.0f;
	spheres[14].material.refractionRoughness = 0.7f;
	spheres[14].material.transmissionColor = vec3(1);
	spheres[14].material.refractionChance = 1.0f;
	
	spheres[15].center = vec3(2, -2.2, 0);
	spheres[15].radius = 0.4;
	spheres[15].material.albedo = vec3(1);
	spheres[15].material.specular = vec3(1);
	spheres[15].material.specularChance = 0.0f;
	spheres[15].material.ior = 1.52;
	spheres[15].material.emission = vec3(0);
	spheres[15].material.surfaceRoughness = 0.0f;
	spheres[15].material.refractionRoughness = 0.9f;
	spheres[15].material.transmissionColor = vec3(1);
	spheres[15].material.refractionChance = 1.0f;
	
	ivec2 dim = imageSize(resultImage);
	vec2 uv = vec2(gl_GlobalInvocationID.xy) / dim;

	uint randomSeed = gl_GlobalInvocationID.x * dim.x + gl_GlobalInvocationID.y;
	vec3 pixelColor = MultiRaytrace(spheres, uv, dim, randomSeed);

	const float exposure = 1.0f;
	pixelColor *= exposure;
 
	// convert unbounded HDR color range to SDR color range
	// pixelColor = ACESFilmicTonemapping(pixelColor);
 
	// convert from linear to sRGB for display
	// pixelColor = LinearToSRGB(pixelColor);

	imageStore(resultImage, ivec2(gl_GlobalInvocationID.xy), vec4(pixelColor, 1));
}
