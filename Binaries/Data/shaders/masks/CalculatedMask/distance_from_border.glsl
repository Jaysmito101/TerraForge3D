float evaluate(inout MaskContext context)
{
	float borderDistance = min(min(float(context.coordinate.x), float(context.coordinate.y)),
		min(float(u_Resolution - 1 - context.coordinate.x), float(u_Resolution - 1 - context.coordinate.y)));
	return RangeMask(borderDistance / max(float(u_Resolution) * 0.5, 1.0));
}
