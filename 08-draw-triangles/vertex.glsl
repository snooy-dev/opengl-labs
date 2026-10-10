#version 330 core

layout (location = 0) in vec3 vPos;

uniform vec3 vSca;
uniform vec3 vLoc;

void main()
{
    gl_Position = vec4(vPos * vSca + vLoc, 1.0);
}