float evaluate(inout MaskContext context)
{
	float slope = SlopeDegrees(context.coordinate);
	return RangeMask(1.0 - clamp(slope / 90.0, 0.0, 1.0));
}
