#include "OpenCLBuffer.h"


OpenCLBuffer::OpenCLBuffer(cl_context cont, cl_command_queue queue, cl_int t, unsigned int size)
{
	context = cont;
	commandQueue = queue;
	maxSize = size;
	type = t;
	obj = clCreateBuffer(context, t, size, NULL, &ret);
}

OpenCLBuffer::~OpenCLBuffer()
{
	ret = clReleaseMemObject(obj);
}

void OpenCLBuffer::SetData(int size, void* data, int offset)
{
	currSize = size;
	ret = clEnqueueWriteBuffer(commandQueue, obj, CL_TRUE, offset, size, data, 0, NULL,  NULL);
}

void OpenCLBuffer::GetData(void* data, int size)
{
	if (size == -1)
		size = currSize;
	ret = clEnqueueReadBuffer(commandQueue, obj, CL_TRUE, 0, size, data, 0, NULL, NULL);
}
