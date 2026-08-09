float evaluate(inout MaskContext context)
{
	return RangeMask(AmbientCavity(context.coordinate));
}
