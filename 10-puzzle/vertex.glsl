#version 330 core

layout (location = 0) in vec2 aPosition;

uniform vec2 uPosition;
uniform vec2 uScale;
uniform float uRotation;

void main()
{
    vec2 position = aPosition * uScale;
    float c = cos(uRotation);
    float s = sin(uRotation);
    position = mat2(c, s, -s, c) * position;
    gl_Position = vec4(position + uPosition, 0.0, 1.0);
}
