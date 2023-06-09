#version 450

layout(location = 0) in vec2 inPosition;

layout(location = 0) out vec2 fragTexCoord;

void main() {
    gl_Position = vec4((inPosition - vec2(0.5f)) * vec2(2.0f), 0.0, 1.0);
    fragTexCoord = vec2(inPosition);
}
