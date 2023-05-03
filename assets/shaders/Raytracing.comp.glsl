#version 450

layout (local_size_x = 16, local_size_y = 16) in;
layout (binding = 0, rgba8) uniform writeonly image2D resultImage;

layout(binding = 1) uniform RenderUniformBufferObject {
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
	uint materialIndex;
};

layout(binding = 2) uniform MaterialUniformBufferObject {
	Material materials[32];
} materialsUbo;

layout(binding = 3) uniform SphereUniformBufferObject {
	uint numSpheres;
	Sphere spheres[64];
} spheresUbo;

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

struct Vertex {
	vec3 position;
	vec3 normal;
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

bool HitTriangle(Ray ray, Vertex vertexA, Vertex vertexB, Vertex vertexC, out float hitDistance, out vec3 normal, out bool isInside)
{
	const float EPSILON = 0.0000001;
	vec3 edgeAB = vertexB.position - vertexA.position;
	vec3 edgeAC = vertexC.position - vertexA.position;
	vec3 normalVector = cross(edgeAB, edgeAC);
	vec3 originToA = ray.origin - vertexA.position;
	vec3 dao = cross(originToA, ray.dir);

	float determinant = -dot(ray.dir, normalVector);
	float invDeterminant = 1.0f / determinant;

	float u = invDeterminant *  dot(edgeAC, dao);
	float v = invDeterminant * -dot(edgeAB, dao);
	float w = 1 - u - v;

	hitDistance = invDeterminant * dot(originToA, normalVector);

	if (determinant >= EPSILON && hitDistance >= EPSILON && u >= 0.0f && v >= 0.0f && w >= 0.0f) {
		normal = normalize(vertexA.normal * w + vertexB.normal * u + vertexC.normal * v);
		isInside = dot(normal, normalVector) > 0.0f;
		normal = isInside ? -normal : normal;
		return true;
	}
	else {
		return false;
	}
}

HitInfo IntersectTriangle(Vertex vertex0, Vertex vertex1, Vertex vertex2, Ray ray) {
	HitInfo hitInfo;

	bool isInside;
	float hitDistanceAlongRay;
	vec3 normal;
	bool hasHit = HitTriangle(ray, vertex0, vertex1, vertex2, hitDistanceAlongRay, normal, isInside);
	if (hasHit) {
		vec3 position = ray.origin + hitDistanceAlongRay * ray.dir;

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

HitInfo CalculateRayHit(Ray ray) {
	HitInfo closestHit;
	closestHit.isHit = false;
	const float positiveInfinity = uintBitsToFloat(0x7F800000);
	closestHit.hitDistance = 100000000.0f;

	for (int i = 0; i < spheresUbo.numSpheres; ++i) {
		Sphere sphere = spheresUbo.spheres[i];

		HitInfo hitInfo = IntersectSphere(sphere, ray);
		if (hitInfo.isHit && closestHit.hitDistance > hitInfo.hitDistance) {
			closestHit = hitInfo;
			closestHit.material = materialsUbo.materials[sphere.materialIndex];
		}
	}

	{
		vec3 v0 = vec3(0,0,0);
		vec3 v1 = vec3(8,0,0);
		vec3 v2 = vec3(8,0,8);
		
		vec3 n0 = vec3(0,1,0);
		vec3 n1 = vec3(0,1,0);
		vec3 n2 = vec3(0,1,0);

		Vertex vert0;
		vert0.position = v0;
		vert0.normal = n0;
		
		Vertex vert1;
		vert1.position = v1;
		vert1.normal = n1;
		
		Vertex vert2;
		vert2.position = v2;
		vert2.normal = n2;
		
		HitInfo hitInfo = IntersectTriangle(vert0, vert1, vert2, ray);
		if (hitInfo.isHit && closestHit.hitDistance > hitInfo.hitDistance) {
			closestHit = hitInfo;
			closestHit.material = materialsUbo.materials[6];
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

vec3 Raytrace(Ray originalRay, inout uint seed) {
	vec3 incomingLight = vec3(0);
	vec3 rayColor = vec3(1);
	
	Ray ray = originalRay;

	for (int i = 0; i <= ubo.maxRayBounceCount; ++i) {
		HitInfo hitInfo = CalculateRayHit(ray);
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

vec3 MultiRaytrace(vec2 uv, ivec2 dim, inout uint seed) {
	vec3 totalLight = vec3(0.0f);

	uv = uv * 2 - 1;

	vec3 camOrigin = (ubo.cameraToWorld * vec4(0,0,0,1)).xyz;
	vec3 camDirection = (ubo.cameraInverseProj * vec4(uv,0,1)).xyz;
	camDirection = (ubo.cameraToWorld * vec4(camDirection, 0)).xyz;

	vec3 camRight = vec3(ubo.cameraToWorld[0][0], ubo.cameraToWorld[1][0], ubo.cameraToWorld[2][0]);
	vec3 camUp = vec3(ubo.cameraToWorld[0][1], ubo.cameraToWorld[1][1], ubo.cameraToWorld[2][1]);

	for (int i = 0; i < ubo.numRaysPerPixel; ++i) {
		Ray ray = CreateCameraRay(camOrigin, camDirection, camRight, camUp, uv, dim, seed);
		totalLight += Raytrace(ray, seed);
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
	ivec2 dim = imageSize(resultImage);
	vec2 uv = vec2(gl_GlobalInvocationID.xy) / dim;

	uint randomSeed = gl_GlobalInvocationID.x * dim.x + gl_GlobalInvocationID.y;
	vec3 pixelColor = MultiRaytrace(uv, dim, randomSeed);

	const float exposure = 1.0f;
	pixelColor *= exposure;
 
	// convert unbounded HDR color range to SDR color range
	// pixelColor = ACESFilmicTonemapping(pixelColor);
 
	// convert from linear to sRGB for display
	// pixelColor = LinearToSRGB(pixelColor);

	imageStore(resultImage, ivec2(gl_GlobalInvocationID.xy), vec4(pixelColor, 1));
}
