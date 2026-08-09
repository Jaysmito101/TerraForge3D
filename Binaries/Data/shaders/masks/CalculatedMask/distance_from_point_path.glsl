float evaluate(inout MaskContext context)
{
	float distanceValue = u_UsePath != 0
		? PointPathDistance(context.uv)
		: distance(context.uv, u_Center);
	return RangeMask(distanceValue);
}
