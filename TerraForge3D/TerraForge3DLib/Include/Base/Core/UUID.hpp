#pragma once

#include <vector>
#include <string>

namespace TerraForge3D
{
	/*
	*  This is one of the core classes of TerraForge3DLib
	*  This represents a 128 bit UUID generated using Native OS API
	*/
	class UUID
	{
	public:
		UUID();
		~UUID();

		std::string ToString(bool includeSeparators = false) const;

	public:
		std::vector<uint8_t> GetRawData() const;

		// This generates an UUID
		static UUID Generate();

	private:
		void SetData(uint8_t* data);

	private:
		uint8_t data[16];
	};

	/*
	* This is just to provide a STL like interface
	*/
	inline std::string to_string(UUID uuid)
	{
		return uuid.ToString();
	}

}


// For using UUID with cout
inline std::ostream& operator<<(std::ostream& os, const TerraForge3D::UUID& uuid)
{
	os << uuid.ToString();
	return os;
}

// The following piece code is for making sure that UUID can be easily logged

// This ignores all warnings raised inside External headers
#pragma warning(push, 0)
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#pragma warning(pop)

template<typename OStream>
inline OStream& operator<<(OStream& os, const TerraForge3D::UUID& uuid)
{
	return os << uuid.ToString();
}

// The folowing piece of code is for UUID to work with std::unordered_map as key
namespace std {
	template <>
	struct hash<TerraForge3D::UUID>
	{
		std::size_t operator()(const TerraForge3D::UUID& k) const
		{
			using std::size_t;
			using std::hash;
			using std::string;

			// Compute individual hash values for first,
			// second and third and combine them using XOR
			// and bit shifting:
			
			return hash<std::string>()(k.ToString());
		}
	};
}