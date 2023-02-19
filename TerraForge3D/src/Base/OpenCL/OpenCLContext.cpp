#include "Base/OpenCL/OpenCLContext.h"
#include <iostream>
#include <cassert>

OpenCLBuffer::OpenCLBuffer(OpenCLContext* context, size_t size, int type)
{
	assert(context);
	assert(size > 0);
	assert(type == CL_MEM_READ_ONLY || type == CL_MEM_WRITE_ONLY || type == CL_MEM_READ_WRITE);
	m_Context = context;
	m_Size = size;
	m_Type = type;
	m_Handle = cl::Buffer(m_Context->m_Handle, m_Type, m_Size);
}

OpenCLBuffer::~OpenCLBuffer()
{
}

bool OpenCLBuffer::Resize(size_t newSize)
{
	if (newSize == m_Size) return true;
	if (newSize <= 0) return false;
	m_Size = newSize;
	m_Handle = cl::Buffer(m_Context->m_Handle, m_Type, m_Size);
	return true;
}

OpenCLContext::OpenCLContext(OpenCLDevice device)
{
	m_Device = device;
	m_Handle = cl::Context(device.m_Handle);
	m_CommandQueue = cl::CommandQueue(m_Handle, m_Device.m_Handle);
}

OpenCLContext::~OpenCLContext()
{
}

void OpenCLContext::Clear()
{
	m_Sources.clear();
	m_Kernels.clear();
	m_Program = cl::Program();
}

void OpenCLContext::AddSource(std::string source)
{
	m_Sources.push_back({ source.c_str(), source.size() });
}

void OpenCLContext::BuildProgram(std::string options)
{
	m_Program = cl::Program(m_Handle, m_Sources);
	if (m_Program.build({ m_Device.m_Handle }, options.c_str()) != CL_SUCCESS)
		std::cout << "Error building: \n" << m_Program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(m_Device.m_Handle) << std::endl;
}

void OpenCLContext::AddKernel(std::string name)
{
	m_Kernels[name] = cl::Kernel(m_Program, name.c_str());
}

void OpenCLContext::ExecuteKernel(std::string name, cl::NDRange local, cl::NDRange global)
{
	auto res = m_CommandQueue.enqueueNDRangeKernel(m_Kernels[name], cl::NullRange, global, local);
	m_CommandQueue.finish();
}

void OpenCLContext::SetKernelArg(std::string name, int arg, std::string bufferName)
{
	auto res = m_Kernels[name].setArg(arg, m_Buffers[bufferName].m_Handle);
}

void OpenCLContext::ReadBuffer(std::string name, bool blocking, size_t offset, size_t size, void* data)
{
	if (m_Buffers.find(name) == m_Buffers.end()) return;
	m_CommandQueue.enqueueReadBuffer(m_Buffers[name].m_Handle, blocking ? CL_TRUE : CL_FALSE, offset, size, data);
}

void OpenCLContext::WriteBuffer(std::string name, bool blocking, size_t offset, size_t size, void* data)
{
	if (m_Buffers.find(name) == m_Buffers.end()) return;
	m_CommandQueue.enqueueWriteBuffer(m_Buffers[name].m_Handle, blocking ? CL_TRUE : CL_FALSE, offset, size, data);
}

void OpenCLContext::CreateBuffer(std::string name, int type, size_t size)
{
	if (m_Buffers.find(name) != m_Buffers.end())
		if (m_Buffers[name].m_Size == size && m_Buffers[name].m_Type == type)
			return;
	m_Buffers[name] = OpenCLBuffer(this, size, type);
}
