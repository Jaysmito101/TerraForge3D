#pragma once

#include <cstdio>
#include <string>
#include <cstring>


class BinaryFileWriter
{
public:
	BinaryFileWriter(const std::string& path) { fileHandle = fopen(path.data(), "wb"); }
	~BinaryFileWriter() { fflush(fileHandle); fclose(fileHandle); }
	inline bool IsOpen() { return (bool)fileHandle; }
	inline size_t Write(const void* data, const size_t size){ return fwrite((const char*)data, size, 1, fileHandle); }
	inline void SetLittleEndian() { this->isLittleEndian = true; }
	inline void SetBigEndian() { this->isLittleEndian = false; }
	
	template <typename T>
	inline size_t Write(const T& data) { return this->Write(&data, sizeof(T)); }

	inline size_t Write(const char* data) { return this->Write(data, strlen(data)); }
	inline size_t WriteInt(int data) { return this->Write(data); }
	inline size_t WriteFloat(float data) { return this->Write(data); }
	inline size_t WriteDouble(double data) { return this->Write(data); }
	inline size_t WriteBool(bool data) { return this->Write(data); }
	inline size_t WriteChar(char data) { return this->Write(data); }
	inline size_t WriteUChar(unsigned char data) { return this->Write(data); }
	inline size_t Write(const std::string& data) { return this->Write(data.data(), data.size()); this->WriteChar(0); }

private:
	FILE* fileHandle = nullptr;
	bool isLittleEndian = true; // for future
};