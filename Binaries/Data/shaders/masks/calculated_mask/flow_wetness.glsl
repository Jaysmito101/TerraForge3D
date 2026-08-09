float evaluate(inout MaskContext context)
{
	return RangeMask(LocalWetness(context.coordinate));
}
