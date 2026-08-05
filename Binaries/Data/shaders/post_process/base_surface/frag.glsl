#version 430 core

layout(location = 0) out vec4 FragColor;

uniform mat4 u_InverseProjectionView;
uniform mat4 u_ProjectionView;
uniform vec3 u_CameraPosition;
uniform vec2 u_ViewportResolution;
uniform vec3 u_Color;

void main()
{
	vec2 ndc = (gl_FragCoord.xy / u_ViewportResolution) * 2.0 - 1.0;
	vec4 nearPoint = u_InverseProjectionView * vec4(ndc, -1.0, 1.0);
	vec4 farPoint = u_InverseProjectionView * vec4(ndc, 1.0, 1.0);
	nearPoint /= nearPoint.w;
	farPoint /= farPoint.w;

	vec3 rayOrigin = u_CameraPosition;
	vec3 rayDirection = normalize(farPoint.xyz - nearPoint.xyz);
	if (abs(rayDirection.y) < 0.000001) discard;

	float distanceToSurface = -rayOrigin.y / rayDirection.y;
	if (distanceToSurface <= 0.0) discard;

	vec3 surfacePosition = rayOrigin + rayDirection * distanceToSurface;
	vec4 projectedPosition = u_ProjectionView * vec4(surfacePosition, 1.0);
	float depth = projectedPosition.z / projectedPosition.w * 0.5 + 0.5;
	if (depth < 0.0 || depth > 1.0) discard;

	gl_FragDepth = depth;
	FragColor = vec4(u_Color, 1.0);
}
