#pragma once

#include "Base/OpenCL/OpenCLPlatform.h"

class OpenCLContext;

class OpenCLBuffer
{
public:
	OpenCLBuffer() = default;
	OpenCLBuffer(OpenCLContext* context, size_t size, int type);
	~OpenCLBuffer();
	bool Resize(size_t newSize);

	bool IsValid() const { return m_Handle(); }
	size_t GetSize() const { return m_Size; }
	int GetType() const { return m_Type; }

	friend class OpenCLContext;
private:
	OpenCLContext* m_Context;
	cl::Buffer m_Handle;
	size_t m_Size;
	int m_Type;
};

class OpenCLContext
{
public:
	OpenCLContext(OpenCLDevice device);
	~OpenCLContext();

	void Clear();
	void AddSource(std::string source);
	void BuildProgram(std::string options);
	void AddKernel(std::string name);
	void ExecuteKernel(std::string name, cl::NDRange local, cl::NDRange global);
	void SetKernelArg(std::string name, int arg, std::string bufferName);
	void ReadBuffer(std::string name, bool blocking, size_t offset, size_t size, void* data);
	void WriteBuffer(std::string name, bool blocking, size_t offset, size_t size, void* data);
	void CreateBuffer(std::string name, int type, size_t size);
	
	inline bool IsReady() const { return m_Kernels.size() > 0; }
	inline const OpenCLDevice& GetDevice() const { return m_Device; }
	inline OpenCLDevice& GetDevice() { return m_Device; }
	inline int GetSourceCount() const { return (int)m_Sources.size(); }
	inline bool HasKernel(std::string name) const { return m_Kernels.find(name) != m_Kernels.end(); }
	inline bool HasBuffer(std::string name) const { return m_Buffers.find(name) != m_Buffers.end(); }
	inline const OpenCLBuffer* GetBuffer(std::string name) const { return &m_Buffers.at(name); }
	inline OpenCLBuffer* GetBuffer(std::string name) { return &m_Buffers.at(name); }

	friend class OpenCLBuffer;
private:
	cl::Context m_Handle;
	cl::Program m_Program;
	cl::Program::Sources m_Sources;
	cl::CommandQueue m_CommandQueue;
	OpenCLDevice m_Device;
	std::unordered_map<std::string, cl::Kernel> m_Kernels;
	std::unordered_map<std::string, OpenCLBuffer> m_Buffers;

};