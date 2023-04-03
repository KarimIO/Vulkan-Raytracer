
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
