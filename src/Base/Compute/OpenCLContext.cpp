#include "OpenCLContext.h"

static void _stdcall OpenCLErrorFunc(const char* errinfo, const void* private_info, size_t cb, void* user_data){
	std::cout << "OpenCL (" << user_data << ") Error : \n" << errinfo << "\n";
}

OpenCLContext::OpenCLContext(std::string n)
{
	ret = clGetPlatformIDs(1, &plarformId, &numPlatforms);
	ret = clGetDeviceIDs(plarformId, CL_DEVICE_TYPE_DEFAULT, 1, &deviceId, &numDevices);
	context = clCreateContext(NULL, 1, &deviceId, OpenCLErrorFunc, name, &ret);
	commandQueue = clCreateCommandQueue(context, deviceId, 0, &ret);

	memcpy_s(name, 1024, n.data(), std::min(1024, (int)n.size()));
}

OpenCLContext::~OpenCLContext()
{
	for (std::pair<std::string, char*> data : sources) {
		if (data.second)
			delete data.second;
	}

	ret = clFlush(commandQueue);
	ret = clReleaseCommandQueue(commandQueue);
	ret = clReleaseContext(context);
}

OpenCLBuffer* OpenCLContext::CreateBuffer(void* data, int size, int type)
{
	OpenCLBuffer* buffer = new OpenCLBuffer(context, commandQueue, type, size);
	buffer->SetData(size, data);
	return buffer;
}

OpenCLBuffer* OpenCLContext::CreateBuffer(int size, int type)
{
	OpenCLBuffer* buffer = new OpenCLBuffer(context, commandQueue, type, size);
	return buffer;
}

void OpenCLContext::AddProgram(std::string name, std::string source)
{
	char* sc = new char[source.size()];
	memcpy_s(sc, source.size(), source.data(), source.size());
	sources[name] = sc;
	int sourceSize = source.size();
	programs[name] = clCreateProgramWithSource(context, 1, (const char**)&sc, (const size_t*)&sourceSize, &ret);
	ret = clBuildProgram(programs[name], 1, &deviceId, NULL, NULL, NULL);
}

void OpenCLContext::MakeKernel(std::string programName, std::string kernelName)
{
	kernels[kernelName] = clCreateKernel(programs[programName], kernelName.c_str(), &ret);
}

void OpenCLContext::SetKernelArg(std::string kernelName, int num, int size, void* arg)
{
	ret = clSetKernelArg(kernels[kernelName], num, size, arg);
}

void OpenCLContext::ReleaseKernerl(std::string kernelName)
{
	ret = clFlush(commandQueue);
	ret = clReleaseKernel(kernels[kernelName]);
}

void OpenCLContext::ReleaseProgram(std::string programName)
{
	ret = clFlush(commandQueue);
	ret = clReleaseProgram(programs[programName]);
}

void OpenCLContext::Dispatch(std::string kernelName, int globalItemSize, int localItemSize)
{
	ret = clEnqueueNDRangeKernel(commandQueue, kernels[kernelName], 1, NULL, (const size_t*)&globalItemSize, (const size_t*)&localItemSize, 0, NULL, NULL);
}
