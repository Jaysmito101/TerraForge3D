#pragma once

#include <string>
#include <unordered_map>
#include <iostream>


#include <CL/cl.h>
#include "OpenCLBuffer.h"



class OpenCLContext {
public:

	OpenCLContext(std::string name = "OpemCL Kernel");
	~OpenCLContext();


	OpenCLBuffer* CreateBuffer(void *data, int size, int bufferType = CL_MEM_WRITE_ONLY);
	OpenCLBuffer* CreateBuffer(int size, int type);

	void AddProgram(std::string name, std::string source);
	void MakeKernel(std::string programName, std::string kernelName);
	void SetKernelArg(std::string kernelName, int index, int size, void* arg);
	void ReleaseKernerl(std::string kernelName);
	void ReleaseProgram(std::string programName);
	void Dispatch(std::string kernelName, int globalItemSize, int localItemSize);

	std::unordered_map<std::string, char*> sources;
	std::unordered_map<std::string, std::string> entryPoints;


	std::unordered_map<std::string, cl_program> programs;
	std::unordered_map<std::string, cl_kernel> kernels;
	

	cl_platform_id plarformId;
	cl_device_id deviceId;
	cl_context context;
	cl_uint numDevices;
	cl_uint numPlatforms;
	cl_command_queue commandQueue;	
	cl_int ret;
	char name[1024];
};