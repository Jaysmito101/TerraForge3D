#pragma once

#define CL_HPP_TARGET_OPENCL_VERSION 200
#include "CL/opencl.hpp"

#include <string>
#include <functional>
#include <unordered_map>

struct OpenCLBuffer
{
	cl::Buffer buffer;
	size_t size;
};

class ComputeKernel
{
public:
	ComputeKernel();

	~ComputeKernel();

	ComputeKernel(std::function<void(std::string)> errFunc, std::function<void(std::string)> statusFunc);

	void AddSoruce(std::string source);

	void BuildProgram(std::string options);

	void AddKernel(std::string name);

	void Clear();

	void ExecuteKernel(std::string name, cl::NDRange local, cl::NDRange global);

	void SetKernelArg(std::string name, int arg, std::string bufferName);

	void ReadBuffer(std::string name, bool blocking, size_t size, void* data);

	void WriteBuffer(std::string name, bool blocking, size_t size, void* data);

	void CreateBuffer(std::string name, int type, size_t size);

	std::function<void(std::string)> onError;
	std::function<void(std::string)> onStatus;
	cl::Platform platform;
	cl::Device device;
	cl::Context context;
	cl::Program::Sources sources;
	cl::Program program;
	cl::CommandQueue queue;

	std::unordered_map<std::string, OpenCLBuffer> buffers;
	std::unordered_map<std::string, cl::Kernel> kernels;


};
