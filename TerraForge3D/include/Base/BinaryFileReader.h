#pragma once

#include <cstdio>
#include <cstring>
#include <string>


class BinaryFileReader
{
public:
	BinaryFileReader(const std::string& path) { fileHandle = fopen(path.data(), "wb"); }
	~BinaryFileReader() { fflush(fileHandle); fclose(fileHandle); }
	inline bool IsOpen() { return (bool)fileHandle; }
	inline size_t Read(void* data, const size_t size) { return fread(data, size, 1, fileHandle); }
	inline void Seek(size_t offset) { fseek(fileHandle, offset, SEEK_SET); }
	inline void Reset() { Seek(0); }

	inline void SetLittleEndian() { this->isLittleEndian = true; }
	inline void SetBigEndian() { this->isLittleEndian = false; }

	template <typename T>
	inline T Read() { T data; this->Read(&data, sizeof(T)); return data; }

	inline int ReadInt() { return this->Read<int>(); }
	inline float ReadFloat() { return this->Read<float>(); }
	inline double ReadDouble() { return this->Read<double>(); }
	inline bool ReadBool() { return this->Read<bool>(); }
	inline char ReadChar() { return this->Read<char>(); }
	inline unsigned char ReadUChar() { return this->Read<unsigned char>(); }
	inline std::string ReadString() { std::string data; char c; while ((c = this->ReadChar()) != '\0' && c != -1) data += c; return data; }



private:
	FILE* fileHandle = nullptr;
	bool isLittleEndian = true; // for future
};