#version 330 core

layout (location = 0) in vec3 vPos;

uniform vec3 vSca;
uniform vec3 vLoc;
uniform float vRot;

void main()
{
    vec3 pos = vPos * vSca;
    float c = cos(vRot);
    float s = sin(vRot);
    pos.xy = mat2(c, s, -s, c) * pos.xy;
    gl_Position = vec4(pos + vLoc, 1.0);
}