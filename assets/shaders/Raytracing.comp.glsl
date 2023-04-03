#version 450

layout (binding = 0, rgba8) uniform readonly image2D inputImage;
layout (binding = 1, rgba8) uniform writeonly image2D outputImage;

layout (local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

void main() 
{
	vec4 pixel = vec4(gl_GlobalInvocationID.xy, 0, 1); //imageLoad(inputImage, ivec2(gl_GlobalInvocationID.xy)).rgb;
	imageStore(outputImage, ivec2(gl_GlobalInvocationID.xy), pixel);
}
