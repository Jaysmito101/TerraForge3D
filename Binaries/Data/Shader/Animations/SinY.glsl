#ifndef SIN_Y_GLSL

vec3 sin_y(vec3 pos)
{
	pos.y += sin(pos.x + pos.z) * 0.5f;
	return pos;
}

#endif // SIN_Y_GLSL