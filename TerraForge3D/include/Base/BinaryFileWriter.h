#pragma once

#include <cstdio>
#include <string>


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

private:
	FILE* fileHandle = nullptr;
	bool isLittleEndian = true; // for future
};