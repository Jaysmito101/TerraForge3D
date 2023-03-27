#pragma once

#include <string>
#include "Base/Base.h"
#include "Exporters/Serializer.h"

enum CustomInspectorValueType
{
	CustomInspectorValueType_Unknown = 0,
	CustomInspectorValueType_Int,
	CustomInspectorValueType_Float,
	CustomInspectorValueType_Bool,
	CustomInspectorValueType_String,
	CustomInspectorValueType_Vector2,
	CustomInspectorValueType_Vector3,
	CustomInspectorValueType_Vector4,
	CustomInspectorValueType_Texture,
	CustomInspectorValueType_Count
};

class CustomInspectorValue
{
public:
	CustomInspectorValue(CustomInspectorValueType type = CustomInspectorValueType_Unknown) :m_Type(type) {}
	~CustomInspectorValue() = default;

	inline CustomInspectorValueType GetType() const { return m_Type; }
	inline std::string GetTypeString() const { return CustomInspectorValueTypeToString(m_Type); }
	inline std::string GetName() const { return m_Name; }

	inline int32_t GetInt() const
	{
		switch (m_Type)
		{
		case CustomInspectorValueType_Int:     return m_IntValue;
		case CustomInspectorValueType_Float:   return static_cast<int32_t>(m_FloatValue);
		case CustomInspectorValueType_Bool:	   return static_cast<int32_t>(m_BoolValue);
		case CustomInspectorValueType_String:  return std::atoi(m_StringValue.c_str());
		case CustomInspectorValueType_Vector2:
		case CustomInspectorValueType_Vector3:
		case CustomInspectorValueType_Vector4: return static_cast<int32_t>(m_VectorValue[0]);
		case CustomInspectorValueType_Unknown:
		case CustomInspectorValueType_Texture:
		default: return 0;
		}
		return 0;
	}

	inline float GetFloat() const
	{
		switch (m_Type)
		{
		case CustomInspectorValueType_Int:     return static_cast<float>(m_IntValue);
		case CustomInspectorValueType_Float:   return (m_FloatValue);
		case CustomInspectorValueType_Bool:	   return static_cast<float>(m_BoolValue);
		case CustomInspectorValueType_String:  return std::atof(m_StringValue.c_str());
		case CustomInspectorValueType_Vector2:
		case CustomInspectorValueType_Vector3:
		case CustomInspectorValueType_Vector4: return (m_VectorValue[0]);
		case CustomInspectorValueType_Unknown:
		case CustomInspectorValueType_Texture:
		default: return 0.0f;
		}
		return 0.0f;
	}

	inline bool GetBool() const
	{
		switch (m_Type)
		{
		case CustomInspectorValueType_Int:     return static_cast<bool>(m_IntValue);
		case CustomInspectorValueType_Float:   return static_cast<bool>(m_FloatValue);
		case CustomInspectorValueType_Bool:	   return (m_BoolValue);
		case CustomInspectorValueType_String:  return m_StringValue == "true";
		case CustomInspectorValueType_Vector2:
		case CustomInspectorValueType_Vector3:
		case CustomInspectorValueType_Vector4: return static_cast<bool>(m_VectorValue[0]);
		case CustomInspectorValueType_Unknown:
		case CustomInspectorValueType_Texture:
		default: return false;
		}
		return false;
	}

	inline std::string GetString() const
	{
		switch (m_Type)
		{
		case CustomInspectorValueType_Int:     return std::to_string(m_IntValue);
		case CustomInspectorValueType_Float:   return std::to_string(m_FloatValue);
		case CustomInspectorValueType_Bool:	   return m_BoolValue ? "true" : "false";
		case CustomInspectorValueType_String:  return m_StringValue;
		case CustomInspectorValueType_Vector2: return std::to_string(m_VectorValue[0]) + " " + std::to_string(m_VectorValue[1]);
		case CustomInspectorValueType_Vector3: return std::to_string(m_VectorValue[0]) + " " + std::to_string(m_VectorValue[1]) + " " + std::to_string(m_VectorValue[2]);
		case CustomInspectorValueType_Vector4: return std::to_string(m_VectorValue[0]) + " " + std::to_string(m_VectorValue[1]) + " " + std::to_string(m_VectorValue[2]) + " " + std::to_string(m_VectorValue[3]);
		case CustomInspectorValueType_Unknown:
		case CustomInspectorValueType_Texture:
		default: return "";
		}
		return "";
	}

	inline glm::vec2 GetVector2()
	{
		switch (m_Type)
		{
		case CustomInspectorValueType_Int:     return glm::vec2(static_cast<float>(m_IntValue));
		case CustomInspectorValueType_Float:   return glm::vec2(static_cast<float>(m_FloatValue));
		case CustomInspectorValueType_Bool:	   return glm::vec2(static_cast<float>(m_BoolValue));
		case CustomInspectorValueType_Vector2:
		case CustomInspectorValueType_Vector3:
		case CustomInspectorValueType_Vector4: return glm::vec2(m_VectorValue[0], m_VectorValue[1]);
		case CustomInspectorValueType_Unknown:
		case CustomInspectorValueType_String:
		case CustomInspectorValueType_Texture:
		default: return glm::vec2(0.0f);
		}
		return glm::vec2(0.0f);
	}

	inline glm::vec3 GetVector3()
	{
		switch (m_Type)
		{
		case CustomInspectorValueType_Int:     return glm::vec3(static_cast<float>(m_IntValue));
		case CustomInspectorValueType_Float:   return glm::vec3(static_cast<float>(m_FloatValue));
		case CustomInspectorValueType_Bool:	   return glm::vec3(static_cast<float>(m_BoolValue));
		case CustomInspectorValueType_Vector2: return glm::vec3(m_VectorValue[0], m_VectorValue[1], 0.0f);
		case CustomInspectorValueType_Vector3:
		case CustomInspectorValueType_Vector4: return glm::vec3(m_VectorValue[0], m_VectorValue[1], m_VectorValue[2]);
		case CustomInspectorValueType_Unknown:
		case CustomInspectorValueType_Texture:
		case CustomInspectorValueType_String:
		default: return glm::vec3(0.0f);
		}
		return glm::vec3(0.0f);
	}

	inline glm::vec4 GetVector4()
	{
		switch (m_Type)
		{
		case CustomInspectorValueType_Int:     return glm::vec4(static_cast<float>(m_IntValue));
		case CustomInspectorValueType_Float:   return glm::vec4(static_cast<float>(m_FloatValue));
		case CustomInspectorValueType_Bool:	   return glm::vec4(static_cast<float>(m_BoolValue));
		case CustomInspectorValueType_Vector2: return glm::vec4(m_VectorValue[0], m_VectorValue[1], 0.0f, 0.0f);
		case CustomInspectorValueType_Vector3: return glm::vec4(m_VectorValue[0], m_VectorValue[1], m_VectorValue[2], 0.0f);
		case CustomInspectorValueType_Vector4: return glm::vec4(m_VectorValue[0], m_VectorValue[1], m_VectorValue[2], m_VectorValue[3]);
		case CustomInspectorValueType_Unknown:
		case CustomInspectorValueType_Texture:
		case CustomInspectorValueType_String:
		default: return glm::vec4(0.0f);
		}
		return glm::vec4(0.0f);
	}

	inline std::shared_ptr<Texture2D> GetTexture() const
	{
		switch (m_Type)
		{
		case CustomInspectorValueType_Texture: return m_TextureValue;
		case CustomInspectorValueType_Int:
		case CustomInspectorValueType_Float:
		case CustomInspectorValueType_Bool:
		case CustomInspectorValueType_String:
		case CustomInspectorValueType_Vector2:
		case CustomInspectorValueType_Vector3:
		case CustomInspectorValueType_Vector4:
		case CustomInspectorValueType_Unknown:
		default: return nullptr;
		}
		return nullptr;
	}

	SerializerNode Save() const;
	void Load(const SerializerNode& node);

	static std::string CustomInspectorValueTypeToString(CustomInspectorValueType type);
	static CustomInspectorValueType CustomInspectorValueTypeFromString(const std::string& type);

	friend class CustomInspector;
private:
	std::string m_Name = "";
	CustomInspectorValueType m_Type = CustomInspectorValueType_Unknown;
	int m_IntValue = 0, m_DefaultIntValue = 0;
	float m_FloatValue = 0.0f, m_DefaultFloatValue = 0.0f;
	bool m_BoolValue = false, m_DefaultBoolValue = false;
	std::string m_StringValue = "", m_DefaultStringValue = "";
	std::shared_ptr<Texture2D> m_TextureValue = nullptr, m_DefaultTextureValue = nullptr;
	float m_VectorValue[4] = { 0.0f, 0.0f, 0.0f, 0.0f }, m_DefaultVectorValue[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
};

enum CustomInspectorWidgetType
{
	CustomInspectorWidgetType_Unknown = 0,
	CustomInspectorWidgetType_Slider,
	CustomInspectorWidgetType_Drag,
	CustomInspectorWidgetType_Color,
	CustomInspectorWidgetType_Texture,
	CustomInspectorWidgetType_Button,
	CustomInspectorWidgetType_Checkbox,
	CustomInspectorWidgetType_Input,
	CustomInspectorWidgetType_Seed,
	CustomInspectorWidgetType_Dropdown,
	CustomInspectorWidgetType_Seperator,
	CustomInspectorWidgetType_NewLine
};

class CustomInspectorWidget
{
public:
	CustomInspectorWidget(CustomInspectorWidgetType type = CustomInspectorWidgetType_Unknown) : m_Type(type) {}
	~CustomInspectorWidget() = default;

	inline CustomInspectorWidgetType GetType() const { return m_Type; }
	inline std::string GetTypeName() const { return CustomInspectorWidgetTypeToString(m_Type); }
	inline std::string GetLabel() const { return m_Label; }
	inline std::string GetVariableName() const { return m_VariableName; }
	inline void SetLabel(const std::string& label) { m_Label = label; }
	inline void SetVariableName(const std::string& variableName) { m_VariableName = variableName; }
	inline void SetTooltip(const std::string& tooltip) { m_Tooltip = tooltip; }

	SerializerNode Save() const;
	void Load(SerializerNode node);

	static std::string CustomInspectorWidgetTypeToString(CustomInspectorWidgetType type);
	static CustomInspectorWidgetType CustomInspectorWidgetTypeFromString(const std::string& type);

	friend class CustomInspector;
private:
	std::string m_Label = "";
	std::string m_VariableName = "";
	CustomInspectorWidgetType m_Type = CustomInspectorWidgetType_Unknown;
	float m_Constratins[4] = { 0.0f, 1.0f, 0.0f, 0.0f };
	float m_FSpeed = 1.0f;
	int m_ISpeed = 1;
	std::string m_Tooltip = "";
	std::vector<int32_t> m_SeedHistory;
	std::vector<std::string> m_DropdownOptions;
};

class CustomInspector
{
public:
	CustomInspector();
	~CustomInspector();

	CustomInspectorValue& GetVariable(const std::string& name);
	bool HasVariable(const std::string& name);
	void RemoveVariable(const std::string& name);
	CustomInspectorValue& AddVariable(const std::string& name, const CustomInspectorValue& value);
	CustomInspectorValue& AddStringVariable(const std::string& name, const std::string& defaultValue);
	CustomInspectorValue& AddIntegerVariable(const std::string& name, int defaultValue = 0);
	CustomInspectorValue& AddFloatVariable(const std::string& name, float defaultValue = 0.0f);
	CustomInspectorValue& AddBoolVariable(const std::string& name, bool defaultValue = false);
	CustomInspectorValue& AddVector2Variable(const std::string& name, glm::vec2 defaultValue = glm::vec2(0.0f));
	CustomInspectorValue& AddVector3Variable(const std::string& name, glm::vec3 defaultValue = glm::vec3(0.0f));
	CustomInspectorValue& AddVector4Variable(const std::string& name, glm::vec4 defaultValue = glm::vec4(0.0f));
	CustomInspectorValue& AddTextureVariable(const std::string& name, std::shared_ptr<Texture2D> defaultValue = nullptr);

	bool HasWidget(const std::string& name);
	CustomInspectorWidget& GetWidget(const std::string& name);
	void RemoveWidget(const std::string& name);
	CustomInspectorWidget& AddWidget(const std::string& name, const CustomInspectorWidget& widget);
	CustomInspectorWidget& AddSliderWidget(const std::string& label, const std::string& variableName, float min = 0.0f, float max = 1.0f);
	CustomInspectorWidget& AddDragWidget(const std::string& label, const std::string& variableName, float min = 0.0f, float max = 1.0f, float speed = 1.0f);
	CustomInspectorWidget& AddColorWidget(const std::string& label, const std::string& variableName);
	CustomInspectorWidget& AddTextureWidget(const std::string& label, const std::string& variableName);
	CustomInspectorWidget& AddButtonWidget(const std::string& label, const std::string& actionName); // for future
	CustomInspectorWidget& AddCheckboxWidget(const std::string& label, const std::string& variableName);
	CustomInspectorWidget& AddInputWidget(const std::string& label, const std::string& variableName);
	CustomInspectorWidget& AddSeedWidget(const std::string& label, const std::string& variableName);
	CustomInspectorWidget& AddDropdownWidget(const std::string& label, const std::string& variableName, const std::vector<std::string>& options);
	CustomInspectorWidget& AddSeperatorWidget();
	CustomInspectorWidget& AddNewLineWidget();

	CustomInspectorWidget& SetWidgetConstraints(const std::string& label, float a = 0.0f, float b = 0.0f, float c = 0.0f, float d = 0.0f);
	CustomInspectorWidget& SetWidgetSpeed(const std::string& label, float speed = 1.0f);
	CustomInspectorWidget& SetWidgetTooltip(const std::string& label, const std::string& value = "Default Tooltip");

	SerializerNode SaveData() const;
	void LoadData(SerializerNode node);
	SerializerNode Save() const;
	void Load(SerializerNode node);

	void Render();


	inline const std::unordered_map<std::string, CustomInspectorValue>& GetValues() const { return m_Values; }
	inline const std::unordered_map<std::string, CustomInspectorWidget>& GetWidgets() const { return m_Widgets; }
	inline const std::vector<std::string>& GetWidgetsOrder() const { return m_WidgetsOrder; }

private:
	std::unordered_map<std::string, CustomInspectorValue> m_Values;
	std::unordered_map<std::string, CustomInspectorWidget> m_Widgets;
	std::vector<std::string> m_WidgetsOrder;
};