#include "Base/OpenCL/OpenCLPlatform.h"

#include <iostream>

static std::string CLDeviceTypeToString(cl_device_type type)
{
	switch (type)
	{
	case CL_DEVICE_TYPE_CPU: return "CPU";
	case CL_DEVICE_TYPE_GPU: return "GPU";
	case CL_DEVICE_TYPE_ACCELERATOR: return "Accelerator";
	case CL_DEVICE_TYPE_DEFAULT: return "Default";
	case CL_DEVICE_TYPE_CUSTOM: return "Custom";
	default: return "Unknown";
	}
}

std::vector<OpenCLPlatform> OpenCLPlatform::GetPlatforms()
{
	cl::vector<cl::Platform> platforms;
	cl::Platform::get(&platforms);
	std::vector<OpenCLPlatform> outPlatforms;
	for (auto platform : platforms)
	{
		outPlatforms.emplace_back();
		outPlatforms.back().m_Name = platform.getInfo<CL_PLATFORM_NAME>();
		outPlatforms.back().m_Extensions = platform.getInfo<CL_PLATFORM_EXTENSIONS>();
		outPlatforms.back().m_Version = platform.getInfo<CL_PLATFORM_VERSION>();
		outPlatforms.back().m_Vendor = platform.getInfo<CL_PLATFORM_VENDOR>();
		outPlatforms.back().m_Profile = platform.getInfo<CL_PLATFORM_PROFILE>();
		outPlatforms.back().m_Handle = platform;
	}
	return outPlatforms;
}

OpenCLPlatform::~OpenCLPlatform()
{
}

OpenCLPlatform::OpenCLPlatform()
{
}

std::vector<OpenCLDevice> OpenCLPlatform::GetDevices()
{
	if (m_Devices.size() > 0) return m_Devices;
	cl::vector<cl::Device> devices;
	std::vector<OpenCLDevice> outDevices;
	m_Handle.getDevices(CL_DEVICE_TYPE_ALL, &devices);
	for (auto device : devices) outDevices.emplace_back(device, *this);
	m_Devices = outDevices;
	return m_Devices;
}

OpenCLDevice::~OpenCLDevice()
{
}

OpenCLDevice::OpenCLDevice(cl::Device device, OpenCLPlatform& platform)
{
	m_IsReady = true;
	m_Platform = platform;
	m_Handle = device;
	m_Name = device.getInfo<CL_DEVICE_NAME>();
	m_Type = device.getInfo<CL_DEVICE_TYPE>();
	m_TypeString = CLDeviceTypeToString(m_Type);
	m_Vendor = device.getInfo<CL_DEVICE_VENDOR>();
	m_Version = device.getInfo<CL_DEVICE_VERSION>();
	m_GlobalMemorySize = device.getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>();
	m_LocalMemorySize = device.getInfo<CL_DEVICE_LOCAL_MEM_SIZE>();
	m_Extensions = device.getInfo<CL_DEVICE_EXTENSIONS>();
}
