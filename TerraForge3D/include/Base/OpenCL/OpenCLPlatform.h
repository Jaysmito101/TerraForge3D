#pragma once
#define CL_HPP_MINIMUM_OPENCL_VERSION 120
#define CL_HPP_TARGET_OPENCL_VERSION 120
#include "CL/opencl.hpp"

#include <string>
#include <functional>
#include <unordered_map>
#include <vector>

class OpenCLDevice;

class OpenCLPlatform
{
public:
	OpenCLPlatform();
	~OpenCLPlatform();

	std::vector<OpenCLDevice> GetDevices();

	static std::vector<OpenCLPlatform> GetPlatforms();

	inline const std::string& GetName() const { return m_Name; }
	inline const std::string& GetExtensions() const { return m_Extensions; }
	inline const std::string& GetVendor() const { return m_Vendor; }
	inline const std::string& GetVersion() const { return m_Version; }
	inline const std::string& GetProfile() const { return m_Profile; }
	inline std::string& GetName() { return m_Name; }
	inline std::string& GetExtensions() { return m_Extensions; }
	inline std::string& GetVendor() { return m_Vendor; }
	inline std::string& GetVersion() { return m_Version; }
	inline std::string& GetProfile() { return m_Profile; }

private:
	cl::Platform m_Handle;
	std::vector<OpenCLDevice> m_Devices;
	std::string m_Name;
	std::string m_Extensions;
	std::string m_Vendor;
	std::string m_Version;
	std::string m_Profile;
};

class OpenCLDevice
{
public:
	OpenCLDevice() = default;
	OpenCLDevice(cl::Device device, OpenCLPlatform& platform);
	~OpenCLDevice();

	inline const std::string& GetName() const { return m_Name; }
	inline const std::string& GetTypeString() const { return m_TypeString; }
	inline const std::string& GetVendor() const { return m_Vendor; }
	inline const std::string& GetVersion() const { return m_Version; }
	inline const std::string& GetExtensions() const { return m_Extensions; }
	inline const cl_device_type& GetType() const { return m_Type; }
	inline const uint64_t& GetGlobalMemorySize() const { return m_GlobalMemorySize; }
	inline const uint64_t& GetLocalMemorySize() const { return m_LocalMemorySize; }
	inline const OpenCLPlatform& GetPlatform() const { return m_Platform; }
	inline std::string& GetName() { return m_Name; }
	inline std::string& GetTypeString() { return m_TypeString; }
	inline std::string& GetVendor() { return m_Vendor; }
	inline std::string& GetVersion() { return m_Version; }
	inline std::string& GetExtensions() { return m_Extensions; }
	inline cl_device_type& GetType() { return m_Type; }
	inline uint64_t& GetGlobalMemorySize() { return m_GlobalMemorySize; }
	inline uint64_t& GetLocalMemorySize() { return m_LocalMemorySize; }
	inline OpenCLPlatform& GetPlatform() { return m_Platform; }
	inline bool IsReady() const { return m_IsReady; }

	friend class OpenCLContext;
private:
	bool m_IsReady = false;
	cl::Device m_Handle;
	cl_device_type m_Type;
	std::string m_Name;
	std::string m_TypeString;
	std::string m_Vendor;
	std::string m_Version;
	std::string m_Extensions;
	uint64_t m_GlobalMemorySize;
	uint64_t m_LocalMemorySize;
	OpenCLPlatform m_Platform;
};