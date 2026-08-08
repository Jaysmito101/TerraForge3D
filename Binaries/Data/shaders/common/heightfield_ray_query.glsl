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

	vec2 directionUv = vec2(rayDirection.x, -rayDirection.z) / terrainWorldSize;
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


bool HeightfieldWorldDomainRange(
	vec2 uv,
	vec2 directionUv,
	out float entryDistance,
	out float exitDistance)
{
	entryDistance = 0.0;
	exitDistance = HEIGHTFIELD_QUERY_INFINITE_DISTANCE;

	for (int axis = 0; axis < 2; ++axis)
	{
		float origin = uv[axis];
		float direction = directionUv[axis];
		if (abs(direction) <= HEIGHTFIELD_QUERY_EPSILON)
		{
			if (origin < 0.0 || origin > 1.0) return false;
			continue;
		}

		float axisEntry = (0.0 - origin) / direction;
		float axisExit = (1.0 - origin) / direction;
		if (axisEntry > axisExit)
		{
			float swapValue = axisEntry;
			axisEntry = axisExit;
			axisExit = swapValue;
		}
		entryDistance = max(entryDistance, axisEntry);
		exitDistance = min(exitDistance, axisExit);
	}

	if (exitDistance < max(entryDistance, 0.0)) return false;
	entryDistance = max(entryDistance, 0.0);
	return true;
}

void HeightfieldWriteWorldRayHit(
	sampler2D heightPyramid,
	vec2 rayUv,
	float rayDistance,
	out vec2 hitUv,
	out float hitHeight,
	out float hitDistance)
{
	hitUv = clamp(rayUv, vec2(0.0), vec2(1.0));
	hitHeight = textureLod(heightPyramid, hitUv, 0.0).r;
	hitDistance = rayDistance;
}

bool HeightfieldTraceWorldPyramid(
	sampler2D heightPyramid,
	vec2 startUv,
	float startHeight,
	vec3 rayDirection,
	vec2 terrainWorldSize,
	float startDistance,
	float maximumDistance,
	float terrainHeightOffset,
	float heightBias,
	int pyramidLevels,
	out vec2 hitUv,
	out float hitHeight,
	out float hitDistance)
{
	hitUv = vec2(0.0);
	hitHeight = 0.0;
	hitDistance = 0.0;
	if (pyramidLevels <= 0 || any(lessThanEqual(terrainWorldSize, vec2(HEIGHTFIELD_QUERY_EPSILON))))
		return false;
	if (maximumDistance < startDistance) return false;

	vec2 directionUv = vec2(rayDirection.x, -rayDirection.z) / terrainWorldSize;
	if (dot(directionUv, directionUv) <= HEIGHTFIELD_QUERY_EPSILON * HEIGHTFIELD_QUERY_EPSILON &&
		abs(rayDirection.y) <= HEIGHTFIELD_QUERY_EPSILON)
		return false;

	vec2 rayUv = clamp(startUv, vec2(0.0), vec2(1.0));
	float rayDistance = max(startDistance, 0.0);
	float epsilon = max(heightBias, HEIGHTFIELD_QUERY_EPSILON);

	for (int step = 0; step < HEIGHTFIELD_QUERY_MAX_STEPS; ++step)
	{
		if (rayDistance > maximumDistance + epsilon) return false;

		float rayHeight = startHeight + rayDistance * rayDirection.y;
		bool advanced = false;
		for (int level = pyramidLevels - 1; level >= 0; --level)
		{
			ivec2 levelSize = textureSize(heightPyramid, level);
			vec2 bounds = textureLod(heightPyramid, rayUv, float(level)).rg + terrainHeightOffset;
			if (!(bounds.x <= bounds.y)) return false;

			if (rayHeight < bounds.x - epsilon)
			{
				if (rayDirection.y > HEIGHTFIELD_QUERY_EPSILON)
				{
					float distanceToMinimum = (bounds.x - epsilon - rayHeight) / rayDirection.y;
					float distanceToCellExit = HeightfieldCellExitDistance(rayUv, directionUv, levelSize);
					float maximumAdvance = min(distanceToCellExit, maximumDistance - rayDistance);
					if (distanceToMinimum >= 0.0 && distanceToMinimum <= maximumAdvance + epsilon)
					{
						float advanceDistance = max(distanceToMinimum, HEIGHTFIELD_QUERY_EPSILON);
						if (advanceDistance <= maximumAdvance + epsilon)
						{
							rayDistance += advanceDistance;
							rayUv += directionUv * advanceDistance;
							advanced = true;
							break;
						}
					}
				}
			}
			else if (rayHeight > bounds.y + epsilon)
			{
				if (rayDirection.y < -HEIGHTFIELD_QUERY_EPSILON)
				{
					float distanceToMaximum = (rayHeight - bounds.y - epsilon) / -rayDirection.y;
					float distanceToCellExit = HeightfieldCellExitDistance(rayUv, directionUv, levelSize);
					float maximumAdvance = min(distanceToCellExit, maximumDistance - rayDistance);
					if (distanceToMaximum >= 0.0 && distanceToMaximum <= maximumAdvance + epsilon)
					{
						float advanceDistance = max(distanceToMaximum, HEIGHTFIELD_QUERY_EPSILON);
						if (advanceDistance <= maximumAdvance + epsilon)
						{
							rayDistance += advanceDistance;
							rayUv += directionUv * advanceDistance;
							advanced = true;
							break;
						}
					}
				}
			}
			else if (level == 0)
			{
				HeightfieldWriteWorldRayHit(heightPyramid, rayUv, rayDistance,
					hitUv, hitHeight, hitDistance);
				return true;
			}
		}

		if (!advanced)
		{
			float fallbackDistance = HeightfieldCellExitDistance(
				rayUv, directionUv, textureSize(heightPyramid, 0));
			float remainingDistance = maximumDistance - rayDistance;
			if (fallbackDistance >= HEIGHTFIELD_QUERY_INFINITE_DISTANCE ||
				remainingDistance < HEIGHTFIELD_QUERY_EPSILON)
				return false;
			float advanceDistance = min(max(fallbackDistance, HEIGHTFIELD_QUERY_EPSILON), remainingDistance);
			if (advanceDistance < HEIGHTFIELD_QUERY_EPSILON) return false;
			rayDistance += advanceDistance;
			rayUv += directionUv * advanceDistance;
		}
	}

	return false;
}

bool HeightfieldWorldRayIntersect(
	sampler2D heightPyramid,
	vec3 rayOrigin,
	vec3 rayDirection,
	vec2 terrainMinimumXZ,
	vec2 terrainWorldSize,
	float terrainHeightOffset,
	float heightBias,
	int pyramidLevels,
	out vec2 hitUv,
	out float hitHeight,
	out float hitDistance)
{
	hitUv = vec2(0.0);
	hitHeight = 0.0;
	hitDistance = 0.0;
	if (pyramidLevels <= 0 || any(lessThanEqual(terrainWorldSize, vec2(HEIGHTFIELD_QUERY_EPSILON))))
		return false;

	float directionLength = length(rayDirection);
	if (directionLength <= HEIGHTFIELD_QUERY_EPSILON) return false;
	rayDirection /= directionLength;

	vec2 terrainMaximumXZ = terrainMinimumXZ + terrainWorldSize;
	vec2 startUv = vec2(
		(rayOrigin.x - terrainMinimumXZ.x) / terrainWorldSize.x,
		(terrainMaximumXZ.y - rayOrigin.z) / terrainWorldSize.y);
	vec2 directionUv = vec2(
		rayDirection.x / terrainWorldSize.x,
		-rayDirection.z / terrainWorldSize.y);

	float entryDistance;
	float exitDistance;
	if (!HeightfieldWorldDomainRange(startUv, directionUv, entryDistance, exitDistance)) return false;
	vec2 domainEntryUv = startUv + directionUv * entryDistance;

	return HeightfieldTraceWorldPyramid(
		heightPyramid, domainEntryUv, rayOrigin.y, rayDirection, terrainWorldSize,
		entryDistance, exitDistance, terrainHeightOffset, heightBias,
		pyramidLevels, hitUv, hitHeight, hitDistance);
}


float HeightfieldDomainEntryDistance(vec2 uv, vec2 directionUv)
{
	float entryDistance = 0.0;
	float exitDistance = HEIGHTFIELD_QUERY_INFINITE_DISTANCE;

	if (abs(directionUv.x) <= HEIGHTFIELD_QUERY_EPSILON)
	{
		if (uv.x < 0.0 || uv.x > 1.0) return HEIGHTFIELD_QUERY_INFINITE_DISTANCE;
	}
	else
	{
		float axisEntry = (0.0 - uv.x) / directionUv.x;
		float axisExit = (1.0 - uv.x) / directionUv.x;
		if (axisEntry > axisExit)
		{
			float swapValue = axisEntry;
			axisEntry = axisExit;
			axisExit = swapValue;
		}
		entryDistance = max(entryDistance, axisEntry);
		exitDistance = min(exitDistance, axisExit);
	}

	if (abs(directionUv.y) <= HEIGHTFIELD_QUERY_EPSILON)
	{
		if (uv.y < 0.0 || uv.y > 1.0) return HEIGHTFIELD_QUERY_INFINITE_DISTANCE;
	}
	else
	{
		float axisEntry = (0.0 - uv.y) / directionUv.y;
		float axisExit = (1.0 - uv.y) / directionUv.y;
		if (axisEntry > axisExit)
		{
			float swapValue = axisEntry;
			axisEntry = axisExit;
			axisExit = swapValue;
		}
		entryDistance = max(entryDistance, axisEntry);
		exitDistance = min(exitDistance, axisExit);
	}

	if (exitDistance < max(entryDistance, 0.0)) return HEIGHTFIELD_QUERY_INFINITE_DISTANCE;
	return max(entryDistance, 0.0);
}

bool HeightfieldRayOccludedFromPlane(
	sampler2D heightPyramid,
	vec2 startUv,
	float startHeight,
	vec3 rayDirection,
	vec2 terrainWorldSize,
	float terrainHeightOffset,
	float heightBias,
	float maximumDistance,
	int pyramidLevels)
{
	if (rayDirection.y <= HEIGHTFIELD_QUERY_EPSILON) return false;

	vec2 directionUv = vec2(rayDirection.x, -rayDirection.z) / terrainWorldSize;
	if (dot(directionUv, directionUv) <= HEIGHTFIELD_QUERY_EPSILON * HEIGHTFIELD_QUERY_EPSILON)
		return false;

	vec2 rayUv = startUv;
	float rayDistance = 0.0;
	for (int step = 0; step < HEIGHTFIELD_QUERY_MAX_STEPS; ++step)
	{
		if (rayDistance > maximumDistance) return false;
		if (any(lessThan(rayUv, vec2(0.0))) || any(greaterThan(rayUv, vec2(1.0))))
		{
			float entryDistance = HeightfieldDomainEntryDistance(rayUv, directionUv);
			if (entryDistance >= HEIGHTFIELD_QUERY_INFINITE_DISTANCE) return false;
			float advanceDistance = max(entryDistance, HEIGHTFIELD_QUERY_EPSILON);
			rayDistance += advanceDistance;
			rayUv += directionUv * advanceDistance;
			continue;
		}

		float rayHeight = startHeight + rayDistance * rayDirection.y;
		bool advanced = false;
		for (int level = pyramidLevels - 1; level >= 0; --level)
		{
			ivec2 levelSize = textureSize(heightPyramid, level);
			vec2 bounds = textureLod(heightPyramid, rayUv, float(level)).rg + terrainHeightOffset;

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
			if (fallbackDistance >= HEIGHTFIELD_QUERY_INFINITE_DISTANCE) return false;
			float advanceDistance = max(fallbackDistance, HEIGHTFIELD_QUERY_EPSILON);
			rayDistance += advanceDistance;
			rayUv += directionUv * advanceDistance;
		}
	}

	return false;
}

bool HeightfieldRayIntersect(
	sampler2D heightPyramid,
	vec2 startUv,
	float startHeight,
	vec3 rayDirection,
	vec2 terrainWorldSize,
	float heightBias,
	int pyramidLevels,
	out vec2 hitUv,
	out float hitHeight,
	out float hitDistance)
{
	hitUv = vec2(0.0);
	hitHeight = 0.0;
	hitDistance = 0.0;
	if (rayDirection.y <= HEIGHTFIELD_QUERY_EPSILON) return false;

	vec2 directionUv = vec2(rayDirection.x, -rayDirection.z) / terrainWorldSize;
	if (dot(directionUv, directionUv) <= HEIGHTFIELD_QUERY_EPSILON * HEIGHTFIELD_QUERY_EPSILON)
		return false;

	vec2 rayUv = startUv;
	float rayDistance = 0.0;
	float maximumDistance = length(terrainWorldSize) /
		max(length(rayDirection.xz), HEIGHTFIELD_QUERY_EPSILON);

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

			if (rayHeight < bounds.x - heightBias)
			{
				if (level > 0) continue;
				hitUv = clamp(rayUv, vec2(0.0), vec2(1.0));
				hitHeight = textureLod(heightPyramid, hitUv, 0.0).r;
				hitDistance = rayDistance;
				return true;
			}

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

			if (level == 0 && rayHeight <= bounds.y + HEIGHTFIELD_QUERY_EPSILON)
			{
				hitUv = clamp(rayUv, vec2(0.0), vec2(1.0));
				hitHeight = textureLod(heightPyramid, hitUv, 0.0).r;
				hitDistance = rayDistance;
				return true;
			}
		}

		if (!advanced)
		{
			float fallbackDistance = HeightfieldCellExitDistance(
				rayUv, directionUv, textureSize(heightPyramid, 0));
			if (fallbackDistance >= HEIGHTFIELD_QUERY_INFINITE_DISTANCE) return false;
			float advanceDistance = max(fallbackDistance, HEIGHTFIELD_QUERY_EPSILON);
			rayDistance += advanceDistance;
			rayUv += directionUv * advanceDistance;
		}
	}

	return false;
}
