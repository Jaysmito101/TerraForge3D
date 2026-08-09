float evaluate(inout MaskContext context)
{
	return RangeMask(CurvatureMagnitude(CurvatureValue(context.coordinate)));
}
