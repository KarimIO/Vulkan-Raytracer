#version 450

const vec2 madd=vec2(0.5,0.5);

layout(location = 0) in vec3 inPosition;
layout(location = 0) out vec2 fragTexCoord;

void main() {
	fragTexCoord = inPosition.xy * madd + madd; // scale vertex attribute to [0-1] range
	gl_Position = vec4(inPosition.xy, 0.0, 1.0);
}
