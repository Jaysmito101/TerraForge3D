float evaluate(inout MaskContext context)
{
	return SlopeRampMask(SlopeDegrees(context.coordinate));
}
