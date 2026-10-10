#version 330 core

layout (location = 0) in vec2 aPosition;

uniform vec2 uPosition;
uniform vec2 uScale;

void main()
{
    gl_Position = vec4(aPosition * uScale + uPosition, 0.0, 1.0);
}
