float evaluate(inout MaskContext context)
{
	return RangeMask(TerrainValue(context.coordinate) - u_SeaLevel);
}
