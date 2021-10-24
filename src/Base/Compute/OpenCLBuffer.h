#pragma once

#include <CL/cl.h>

class OpenCLBuffer {
public:
	OpenCLBuffer(cl_context context, cl_command_queue queue, cl_int bufferType, unsigned int size);
	~OpenCLBuffer();

	void SetData(int size, void* data, int offset = 0);
	void GetData(void* data, int size = -1);

	void* GetNativeID() { return obj; }

	cl_mem obj;
	cl_command_queue commandQueue;
	cl_context context;
	cl_int ret;
	int type;
	int maxSize;
	int currSize;
}; 
