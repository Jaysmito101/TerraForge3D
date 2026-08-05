const float HEIGHTFIELD_QUERY_EPSILON = 0.000001;
const float HEIGHTFIELD_QUERY_INFINITE_DISTANCE = 1.0e30;
const int HEIGHTFIELD_QUERY_MAX_STEPS = 1024;

float HeightfieldCellExitDistance(vec2 uv, vec2 directionUv, ivec2 levelSize)
{
	vec2 cellCoordinate = floor(clamp(uv, vec2(0.0), vec2(0.999999)) * vec2(levelSize));
	vec2 cellMinimum = cellCoordinate / vec2(levelSize);
	vec2 cellMaximum = (cellCoordinate + vec2(1.0)) / vec2(levelSize);
	float distanceToExit = HEIGHTFIELD_QUERY_INFINITE_DISTANCE;

	if (directionUv.x > HEIGHTFIELD_QUERY_EPSILON) {
		distanceToExit = min(distanceToExit, (cellMaximum.x - uv.x) / directionUv.x);
	} else if (directionUv.x < -HEIGHTFIELD_QUERY_EPSILON) {
		distanceToExit = min(distanceToExit, (cellMinimum.x - uv.x) / directionUv.x);
	}

	if (directionUv.y > HEIGHTFIELD_QUERY_EPSILON) {
		distanceToExit = min(distanceToExit, (cellMaximum.y - uv.y) / directionUv.y);
	} else if (directionUv.y < -HEIGHTFIELD_QUERY_EPSILON) {
		distanceToExit = min(distanceToExit, (cellMinimum.y - uv.y) / directionUv.y);
	}

	return max(distanceToExit, 0.0);
}

bool HeightfieldRayOccluded(
	sampler2D heightPyramid,
	vec2 startUv,
	float startHeight,
	vec3 rayDirection,
	vec2 terrainWorldSize,
	float heightBias,
	int pyramidLevels)
{
	if (rayDirection.y <= HEIGHTFIELD_QUERY_EPSILON) return false;

	vec2 directionUv = rayDirection.xz / terrainWorldSize;
	if (dot(directionUv, directionUv) <= HEIGHTFIELD_QUERY_EPSILON * HEIGHTFIELD_QUERY_EPSILON)
		return false;

	vec2 rayUv = startUv;
	float rayDistance = 0.0;
	float maximumDistance = length(terrainWorldSize) / max(length(rayDirection.xz), HEIGHTFIELD_QUERY_EPSILON);

	for (int step = 0; step < HEIGHTFIELD_QUERY_MAX_STEPS; ++step)
	{
		if (any(lessThan(rayUv, vec2(0.0))) || any(greaterThan(rayUv, vec2(1.0)))) return false;
		if (rayDistance > maximumDistance) return false;

		float rayHeight = startHeight + rayDistance * rayDirection.y;
		bool advanced = false;
		for (int level = pyramidLevels - 1; level >= 0; --level)
		{
			ivec2 levelSize = textureSize(heightPyramid, level);
			vec2 bounds = textureLod(heightPyramid, rayUv, float(level)).rg;

			if (rayHeight < bounds.x - heightBias) return true;
			if (rayHeight > bounds.y + heightBias)
			{
				float distanceToCellExit = HeightfieldCellExitDistance(rayUv, directionUv, levelSize);
				if (distanceToCellExit > HEIGHTFIELD_QUERY_EPSILON &&
					distanceToCellExit < HEIGHTFIELD_QUERY_INFINITE_DISTANCE)
				{
					rayDistance += distanceToCellExit;
					rayUv += directionUv * distanceToCellExit;
					advanced = true;
					break;
				}
			}

			if (level == 0 && rayHeight <= bounds.y + HEIGHTFIELD_QUERY_EPSILON) return true;
		}

		if (!advanced)
		{
			float fallbackDistance = HeightfieldCellExitDistance(rayUv, directionUv, textureSize(heightPyramid, 0));
			if (fallbackDistance >= HEIGHTFIELD_QUERY_INFINITE_DISTANCE) {
				return false;
			}
			rayDistance += max(fallbackDistance, HEIGHTFIELD_QUERY_EPSILON);
			rayUv += directionUv * max(fallbackDistance, HEIGHTFIELD_QUERY_EPSILON);
		}
	}

	return false;
}
