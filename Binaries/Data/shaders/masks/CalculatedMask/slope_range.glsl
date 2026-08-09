float evaluate(inout MaskContext context)
{
	return RangeMask(SlopeDegrees(context.coordinate));
}
