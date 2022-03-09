#include "Base/OpenCL/ComputeKernel.h"

#include <iostream>

static void OnError(std::string message)
{
	std::cout << "OpenCL Error : " << message << std::endl;
}

static void OnStatus(std::string message)
{
	std::cout << "OpenCL Status : " << message << std::endl;
}

ComputeKernel::ComputeKernel()
	:ComputeKernel(OnError, OnStatus)
{
}

ComputeKernel::~ComputeKernel()
{
}

ComputeKernel::ComputeKernel(std::function<void(std::string)> errFunc, std::function<void(std::string)> statusFunc)
{
	onError = errFunc;
	onStatus = statusFunc;
	std::vector<cl::Platform> platforms;
	cl::Platform::get(&platforms);

	if (platforms.size() == 0)
	{
		onError("No Platform found supporting OpenCL!");
		return;
	}

	platform = platforms[0];
	onStatus("Using OpenCL Platform : " + platform.getInfo<CL_PLATFORM_NAME>() );
	std::vector<cl::Device> devices;
	platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);

	if (devices.size() == 0)
	{
		onError("No Device found supporting OpenCL!");
		return;
	}

	bool tmp = false;

	for (auto &d : devices)
	{
		tmp = (d.getInfo<CL_DEVICE_TYPE>() == CL_DEVICE_TYPE_GPU);

		if (tmp)
		{
			device = d;
			onStatus("Using GPU Device : " + device.getInfo<CL_DEVICE_NAME>());
			break;
		}
	}

	if (!tmp)
	{
		device = devices[0];
		onStatus("Using CPU Device : " + device.getInfo<CL_DEVICE_NAME>());
	}

	context = cl::Context({device});
	queue = cl::CommandQueue(context, device);
}

void ComputeKernel::AddSoruce(std::string source)
{
	sources.push_back({source.c_str(), source.size()});
}

void ComputeKernel::BuildProgram(std::string options)
{
	program = cl::Program(context, sources);

	if (program.build({ device }, options.c_str()) != CL_SUCCESS)
	{
		onStatus("Error Building : " + program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device));
		return;
	}
}

void ComputeKernel::AddKernel(std::string name)
{
	kernels[name] = cl::Kernel(program, name.c_str());
}

void ComputeKernel::Clear()
{
	sources.clear();
	kernels.clear();
}

void ComputeKernel::ExecuteKernel(std::string name, cl::NDRange local, cl::NDRange global)
{
	queue.enqueueNDRangeKernel(kernels[name], cl::NullRange, global, local);
	queue.finish();
}

void ComputeKernel::CreateBuffer(std::string name, int type, size_t size)
{
	OpenCLBuffer buffer;
	buffer.size = size;
	buffer.buffer = cl::Buffer(context, type, size);
	buffers[name] = buffer;
}

void ComputeKernel::SetKernelArg(std::string name, int arg, std::string buffer)
{
	kernels[name].setArg(arg, buffers[buffer].buffer);
}

void ComputeKernel::ReadBuffer(std::string buffer, bool blocking, size_t size, void *data)
{
	queue.enqueueReadBuffer(buffers[buffer].buffer, blocking ? CL_TRUE : CL_FALSE, 0, size, data);
}

void ComputeKernel::WriteBuffer(std::string buffer, bool blocking, size_t size, void *data)
{
	queue.enqueueWriteBuffer(buffers[buffer].buffer, blocking ? CL_TRUE : CL_FALSE, 0, size, data);
}
