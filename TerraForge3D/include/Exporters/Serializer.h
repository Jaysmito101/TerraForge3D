#pragma once

#include <memory>

#include "Base/Base.h"

class SerializerNodeInternal;
std::shared_ptr<SerializerNodeInternal> CreateSerializerNode();

class SerializerNodeInternal
{
private:
	template <typename T>
	inline SerializerNodeInternal* SetSimple(const std::string& key, const T& value, const std::string& valueTypeName)
	{
		nlohmann::json data;
		data["Type"] = valueTypeName;
		data["Value"] = value;
		m_Value[key] = data;
		return this;
	}

	template <typename T>
	inline T GetSimple(const std::string& key, const std::string& valueTypeName, const T& defaultValue)
	{
		// if(!HasKey(key)) return defaultValue;
		if (GetKeyType(key) != valueTypeName) return defaultValue;
		try
		{
			return m_Value[key]["Value"].get<T>();
		}
		catch (...)
		{
			// std::cout << "Error: Could not get value for key: " << key << std::endl;
		}
		return defaultValue;
	}

	inline SerializerNodeInternal* CreateArray(const std::string& key, const std::string& valueTypeName)
	{
		if (valueTypeName == "NodeArray")
		{
			m_Arrays[key] = std::vector<std::shared_ptr<SerializerNodeInternal>>();
			return this;
		}
		nlohmann::json data;
		data["Type"] = valueTypeName;
		data["Value"] = nullptr;
		m_Value[key] = data;
		return this;
	}

	template <typename T>
	inline SerializerNodeInternal* PushToArray(const std::string& key, const T& value, const std::string& valueTypeName)
	{
		// if (!HasKey(key)) CreateArray(key, valueTypeName);
		if (GetKeyType(key) != valueTypeName) CreateArray(key, valueTypeName);
		m_Value[key]["Value"].push_back(value);
		return this;
	}


public:
	SerializerNodeInternal() = default;
	~SerializerNodeInternal() = default;

	inline SerializerNodeInternal* SetInteger(const std::string& key, int value) { return SetSimple(key, value, "Integer"); }
	inline SerializerNodeInternal* SetFloat(const std::string& key, float value) { return SetSimple(key, value, "Float"); }
	inline SerializerNodeInternal* SetString(const std::string& key, const std::string& value) { return SetSimple(key, value, "String"); }
	inline SerializerNodeInternal* SetChildNode(const std::string& key, std::shared_ptr<SerializerNodeInternal> value) { m_Children[key] = value; return this; }
	inline SerializerNodeInternal* SetFile(const std::string& key, const std::string& path) { m_Files.push_back({ path, false }); return SetSimple(key, path, "File"); }

	inline SerializerNodeInternal* SetIntegerArray(const std::string& key, std::vector<int> value) { return SetSimple(key, value, "IntegerArray"); }
	inline SerializerNodeInternal* SetFloatArray(const std::string& key, std::vector<float> value) { return SetSimple(key, value, "FloatArray"); }
	inline SerializerNodeInternal* SetStringArray(const std::string& key, const std::vector<std::string>& value) { return SetSimple(key, value, "StringArray"); }
	inline SerializerNodeInternal* SetNodeArray(const std::string& key, std::vector<std::shared_ptr<SerializerNodeInternal>> value) { m_Arrays[key] = value; return this; }

	inline SerializerNodeInternal* CreateIntegerArray(const std::string& key) { return CreateArray(key, "IntegerArray"); }
	inline SerializerNodeInternal* CreateFloatArray(const std::string& key) { return CreateArray(key, "FloatArray"); }
	inline SerializerNodeInternal* CreateStringArray(const std::string& key) { return CreateArray(key, "StringArray"); }
	inline SerializerNodeInternal* CreateNodeArray(const std::string& key) { return CreateArray(key, "NodeArray"); }

	inline SerializerNodeInternal* PushToIntegerArray(const std::string& key, int value) { return PushToArray(key, value, "IntegerArray"); }
	inline SerializerNodeInternal* PushToFloatArray(const std::string& key, float value) { return PushToArray(key, value, "FloatArray"); }
	inline SerializerNodeInternal* PushToStringArray(const std::string& key, const std::string& value) { return PushToArray(key, value, "StringArray"); }
	inline SerializerNodeInternal* PushToNodeArray(const std::string& key, std::shared_ptr<SerializerNodeInternal> value) { if (GetKeyType(key) != "NodeArray") CreateArray(key, "NodeArray"); m_Arrays[key].push_back(value); return this; }

	inline int GetInteger(const std::string& key, int defaultValue = 0) { return GetSimple<int>(key, "Integer", defaultValue); }
	inline float GetFloat(const std::string& key, float defaultValue = 0.0f) { return GetSimple<float>(key, "Float", defaultValue); }
	inline std::string GetString(const std::string& key, const std::string& defaultValue = "") { return GetSimple<std::string>(key, "String", defaultValue); }
	inline std::shared_ptr<SerializerNodeInternal> GetChildNode(const std::string& key, std::shared_ptr<SerializerNodeInternal> defaultValue = nullptr) { const auto& it = m_Children.find(key); if (it != m_Children.end()) return it->second; return defaultValue; }
	inline std::string GetFile(const std::string& key, const std::string& defaultValue = "") { return GetSimple<std::string>(key, "File", defaultValue); }

	inline std::vector<int> GetIntegerArray(const std::string& key, const std::vector<int>& defaultValue = std::vector<int>()) { return GetSimple<std::vector<int>>(key, "IntegerArray", defaultValue); }
	inline std::vector<float> GetFloatArray(const std::string& key, const std::vector<float>& defaultValue = std::vector<float>()) { return GetSimple<std::vector<float>>(key, "FloatArray", defaultValue); }
	inline std::vector<std::string> GetStringArray(const std::string& key, const std::vector<std::string>& defaultValue = std::vector<std::string>()) { return GetSimple<std::vector<std::string>>(key, "StringArray", defaultValue); }
	inline std::vector<std::shared_ptr<SerializerNodeInternal>> GetNodeArray(const std::string& key, const std::vector<std::shared_ptr<SerializerNodeInternal>>& defaultValue = std::vector<std::shared_ptr<SerializerNodeInternal>>()) { const auto& it = m_Arrays.find(key); if (it != m_Arrays.end()) return it->second; return defaultValue; }

	inline int GetArraySize(const std::string& key)
	{
		const auto& it0 = m_Value.find(key);
		if (it0 != m_Value.end()) return (int)it0.value()["Value"].size();
		const auto& it2 = m_Arrays.find(key);
		if (it2 != m_Arrays.end()) return (int)it2->second.size();
		return -1;
	}

	inline bool HasKey(const std::string& key) const { return (m_Value.find(key) != m_Value.end()) || (m_Children.find(key) != m_Children.end()); }

	inline std::string GetKeyType(const std::string& key) const
	{
		const auto& it0 = m_Value.find(key);
		if (it0 != m_Value.end()) return it0.value()["Type"];
		const auto& it2 = m_Children.find(key);
		if (it2 != m_Children.end()) return "Node";
		const auto& it3 = m_Arrays.find(key);
		if (it3 != m_Arrays.end()) return "NodeArray";
		return "Unknown";
	}

	inline std::vector<std::string> GetKeys() const
	{
		std::vector<std::string> keys;
		for (const auto& it : m_Value) keys.push_back(it.get<std::string>());
		for (const auto& it : m_Children) keys.push_back(it.first);
		for (const auto& it : m_Arrays) keys.push_back(it.first);
		return keys;
	}

	inline std::shared_ptr<SerializerNodeInternal> Clone() const
	{
		auto node = std::make_shared<SerializerNodeInternal>();
		node->m_Value = m_Value;
		for (const auto& it : m_Children)
		{
			node->m_Children[it.first] = it.second->Clone();
		}
		for (const auto& it : m_Arrays)
		{
			for (const auto& it2 : it.second)
			{
				node->m_Arrays[it.first].push_back(it2->Clone());
			}
		}
		return node;
	}

	/*
	// TODO
	inline bool ResolveFilesToHandler(const TF3DFile& file);
	inline bool ResolveFilesFromHandler(const TF3DFile& file);
	*/

	inline nlohmann::json ToJson() const
	{
		nlohmann::json data;
		data["Value"] = m_Value;
		data["Children"] = nlohmann::json::object();
		data["Arrays"] = nlohmann::json::object();
		for (const auto& it : m_Children)
		{
			data["Children"][it.first] = it.second->ToJson();
		}
		for (const auto& it : m_Arrays)
		{
			for (const auto& it2 : it.second)
			{
				data["Arrays"][it.first].push_back(it2->ToJson());
			}
		}
		return data;
	}

	inline void LoadJson(nlohmann::json data)
	{
		Clear();
		try
		{
			const auto& it0 = data.find("Value");
			if (it0 != data.end()) m_Value = it0.value();
		}
		catch (...)
		{
			std::cout << "Error loading Serializer Value" << std::endl;
		}
		try
		{
			const auto& it1 = data.find("Children");
			if (it1 != data.end())
			{
				for (const auto& it2 : it1.value().items())
				{
					m_Children[it2.key()] = std::make_shared<SerializerNodeInternal>();
					m_Children[it2.key()]->LoadJson(it2.value());
				}
			}
		}
		catch (...)
		{
			std::cout << "Error loading Serializer Children" << std::endl;
		}
		try
		{
			const auto& it3 = data.find("Arrays");
			if (it3 != data.end())
			{
				for (const auto& it4 : it3.value().items())
				{
					for (const auto& it5 : it4.value())
					{
						auto node = std::make_shared<SerializerNodeInternal>();
						node->LoadJson(it5);
						m_Arrays[it4.key()].push_back(node);
					}
				}
			}
		}
		catch (...)
		{
			std::cout << "Error loading Serializer Arrays" << std::endl;
		}
	}

	inline void Clear()
	{
		m_Value.clear();
		m_Children.clear();
		m_Arrays.clear();
	}

private:
	nlohmann::json m_Value;
	std::unordered_map<std::string, std::vector<std::shared_ptr<SerializerNodeInternal>>> m_Arrays;
	std::unordered_map<std::string, std::shared_ptr<SerializerNodeInternal>> m_Children;
	std::vector<std::pair<std::string, bool>> m_Files;
};

using SerializerNode = std::shared_ptr<SerializerNodeInternal>;

inline static SerializerNode CreateSerializerNode()
{
	return std::make_shared<SerializerNodeInternal>();
}

inline static SerializerNode CreateSerializerNodeFromJson(nlohmann::json data)
{
	SerializerNode node = CreateSerializerNode();
	node->LoadJson(data);
	return node;
}