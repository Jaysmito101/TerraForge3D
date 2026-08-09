float evaluate(inout MaskContext context)
{
	float curvature = CurvatureValue(context.coordinate);
	return ThresholdMask(RidgeValleyStrength(u_Feature != 0 ? -curvature : curvature));
}
