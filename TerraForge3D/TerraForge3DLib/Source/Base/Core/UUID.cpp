#include "Base/Core/UUID.hpp"

#include <cstring>
#include <array>
#include <sstream>

#include <spdlog/fmt/fmt.h> // for fmt::format


#ifdef TF3D_WINDOWS

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <objbase.h>

#elif defined TF3D_LINUX

#include <uuid/uuid.h>

#elif defined TF3D_MACOSX

#include <CoreFoundation/CFUUID.h>

#endif

static void GenerateNativeUUID(uint8_t* data)
{
#ifdef TF3D_WINDOWS

    GUID newId;
    HRESULT hr = ::CoCreateGuid(&newId);

    if (FAILED(hr))
    {
        throw std::system_error(hr, std::system_category(), "CoCreateGuid failed");
    }

    std::array<uint8_t, 16> bytes =
    { {
          static_cast<unsigned char>((newId.Data1 >> 24) & 0xFF),
          static_cast<unsigned char>((newId.Data1 >> 16) & 0xFF),
          static_cast<unsigned char>((newId.Data1 >> 8) & 0xFF),
          static_cast<unsigned char>((newId.Data1) & 0xFF),

          (unsigned char)((newId.Data2 >> 8) & 0xFF),
          (unsigned char)((newId.Data2) & 0xFF),

          (unsigned char)((newId.Data3 >> 8) & 0xFF),
          (unsigned char)((newId.Data3) & 0xFF),

          newId.Data4[0],
          newId.Data4[1],
          newId.Data4[2],
          newId.Data4[3],
          newId.Data4[4],
          newId.Data4[5],
          newId.Data4[6],
          newId.Data4[7]
       } };

#elif defined TF3D_LINUX

    uuid_t id;
    uuid_generate(id);

    std::array<uint8_t, 16> bytes =
    { {
          id[0],
          id[1],
          id[2],
          id[3],
          id[4],
          id[5],
          id[6],
          id[7],
          id[8],
          id[9],
          id[10],
          id[11],
          id[12],
          id[13],
          id[14],
          id[15]
       } };

#elif defined TF3D_MACOSX
    auto newId = CFUUIDCreate(NULL);
    auto bytes0 = CFUUIDGetUUIDBytes(newId);
    CFRelease(newId);

    std::array<uint8_t, 16> bytes =
    { {
          bytes0.byte0,
          bytes0.byte1,
          bytes0.byte2,
          bytes0.byte3,
          bytes0.byte4,
          bytes0.byte5,
          bytes0.byte6,
          bytes0.byte7,
          bytes0.byte8,
          bytes0.byte9,
          bytes0.byte10,
          bytes0.byte11,
          bytes0.byte12,
          bytes0.byte13,
          bytes0.byte14,
          bytes0.byte15
       } };
#else
#error "Unknown Platform"
#endif
    memcpy(data, bytes.data(), sizeof(uint8_t) * 16);
}

namespace TerraForge3D
{
	UUID::UUID()
	{
		memset(data, 0, sizeof(data));
	}

	UUID::~UUID()
	{
	}

	std::string UUID::ToString(bool includeSeparators) const
	{
		std::stringstream result;
		result << fmt::format("{:X}", data[0]);
		result << fmt::format("{:X}", data[1]);
		result << fmt::format("{:X}", data[2]);
		result << fmt::format("{:X}", data[3]);
		if (includeSeparators)
			result << "-";
		result << fmt::format("{:X}", data[4]);
        result << fmt::format("{:X}", data[5]);
        result << fmt::format("{:X}", data[6]);
        result << fmt::format("{:X}", data[7]);
		if (includeSeparators)
			result << "-";
		result << fmt::format("{:X}", data[8]);
        result << fmt::format("{:X}", data[9]);
        result << fmt::format("{:X}", data[10]);
        result << fmt::format("{:X}", data[11]);
		if (includeSeparators)
			result << "-";
		result << fmt::format("{:X}", data[12]);
        result << fmt::format("{:X}", data[13]);
        result << fmt::format("{:X}", data[14]);
        result << fmt::format("{:X}", data[15]);
		return result.str();
	}

	void UUID::SetData(uint8_t* d)
	{
		memcpy(data, d, sizeof(data));
	}

    std::vector<uint8_t> UUID::GetRawData() const
    {
        std::vector<uint8_t> d(16);
        memcpy(d.data(), data, sizeof(uint8_t) * 16);
        return d;
    }


	UUID UUID::Generate()
	{
		UUID uuid;
		uint8_t data[16];
		GenerateNativeUUID(data);
		uuid.SetData(data);
		return uuid;
	}
}