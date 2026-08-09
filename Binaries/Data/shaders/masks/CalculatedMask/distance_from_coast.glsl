float evaluate(inout MaskContext context)
{
	return RangeMask(abs(TerrainValue(context.coordinate) - u_SeaLevel));
}
