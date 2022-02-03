// Local Group Size is LOCAL_GROUP_SIZE

__kernel void erode(__global float *inputMap, __global float *outputMap, __global int *resolution)
{
	int i, j, res;

	i = get_global_id(0);
	j = get_global_id(1);
	res = resolution[0];

	outputMap[i * res + j] = inputMap[i * res + j] + sin(0.05 * j) * 0.2;
}