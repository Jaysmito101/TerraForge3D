#version 430 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

const float PI = 3.141592f;
const float TwoPI = 2.0f * PI;

uniform sampler2D u_InputTexture;

layout(binding = 0, rgba32f) uniform imageCube outputTexture;

vec3 getSamplingVector()
{
    vec2 st = gl_GlobalInvocationID.xy / vec2(imageSize(outputTexture));
    vec2 uv = 2.0f * vec2(st.x, 1.0f - st.y) - vec2(1.0f);
    vec3 ret = vec3(0.0f);
    if(gl_GlobalInvocationID.z == 0)      ret = vec3(1.0f,  uv.y, -uv.x);
    else if(gl_GlobalInvocationID.z == 1) ret = vec3(-1.0f, uv.y,  uv.x);
    else if(gl_GlobalInvocationID.z == 2) ret = vec3(uv.x, 1.0f, -uv.y);
    else if(gl_GlobalInvocationID.z == 3) ret = vec3(uv.x, -1.0f, uv.y);
    else if(gl_GlobalInvocationID.z == 4) ret = vec3(uv.x, uv.y, 1.0f);
    else if(gl_GlobalInvocationID.z == 5) ret = vec3(-uv.x, uv.y, -1.0f);
    return normalize(ret);
}


void main(void)
{
	vec3 v = getSamplingVector();
	float phi   = atan(v.z, v.x);
	float theta = acos(v.y);
	vec4 color = texture(u_InputTexture, vec2(phi/TwoPI, theta/PI));
    imageStore(outputTexture, ivec3(gl_GlobalInvocationID), color);
}
