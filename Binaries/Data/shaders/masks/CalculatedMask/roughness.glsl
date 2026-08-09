float evaluate(inout MaskContext context)
{
	return RangeMask(RoughnessValue(context.coordinate));
}
