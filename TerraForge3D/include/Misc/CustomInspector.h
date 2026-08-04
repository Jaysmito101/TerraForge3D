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
	CustomInspectorValueType_Path,
	CustomInspectorValueType_Count
};

inline constexpr size_t CustomInspectorMaxPathPoints = 16;

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
		case CustomInspectorValueType_Bool:	   return m_BoolValue ? 1 : 0;
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
		case CustomInspectorValueType_Bool:	   return m_BoolValue ? 1.0f : 0.0f;
		case CustomInspectorValueType_String:  return static_cast<float>(std::atof(m_StringValue.c_str()));
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
		case CustomInspectorValueType_Bool:	   return m_BoolValue;
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

	inline glm::vec2 GetVector2() const
	{
		switch (m_Type)
		{
		case CustomInspectorValueType_Int:     return glm::vec2(static_cast<float>(m_IntValue));
		case CustomInspectorValueType_Float:   return glm::vec2(static_cast<float>(m_FloatValue));
		case CustomInspectorValueType_Bool:	   return glm::vec2(m_BoolValue ? 1.0f : 0.0f);
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

	inline glm::vec3 GetVector3() const
	{
		switch (m_Type)
		{
		case CustomInspectorValueType_Int:     return glm::vec3(static_cast<float>(m_IntValue));
		case CustomInspectorValueType_Float:   return glm::vec3(static_cast<float>(m_FloatValue));
		case CustomInspectorValueType_Bool:	   return glm::vec3(m_BoolValue ? 1.0f : 0.0f);
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

	inline glm::vec4 GetVector4() const
	{
		switch (m_Type)
		{
		case CustomInspectorValueType_Int:     return glm::vec4(static_cast<float>(m_IntValue));
		case CustomInspectorValueType_Float:   return glm::vec4(static_cast<float>(m_FloatValue));
		case CustomInspectorValueType_Bool:	   return glm::vec4(m_BoolValue ? 1.0f : 0.0f);
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

	inline const std::array<glm::vec2, CustomInspectorMaxPathPoints>& GetPathPoints() const { return m_PathPoints; }
	inline int GetPathPointCount() const { return m_PathPointCount; }
	inline void SetInt(int value)
	{
		if (m_Type == CustomInspectorValueType_Int) m_IntValue = value;
	}
	inline void SetVector2(glm::vec2 value)
	{
		if (m_Type != CustomInspectorValueType_Vector2) return;
		m_VectorValue[0] = value.x;
		m_VectorValue[1] = value.y;
	}

	inline void ResetValue()
	{
		m_IntValue = m_DefaultIntValue;
		m_FloatValue = m_DefaultFloatValue;
		m_BoolValue = m_DefaultBoolValue;
		m_StringValue = m_DefaultStringValue;
		m_TextureValue = m_DefaultTextureValue;
		m_VectorValue[0] = m_DefaultVectorValue[0];
		m_VectorValue[1] = m_DefaultVectorValue[1];
		m_VectorValue[2] = m_DefaultVectorValue[2];
		m_VectorValue[3] = m_DefaultVectorValue[3];
		m_PathPoints = m_DefaultPathPoints;
		m_PathPointCount = m_DefaultPathPointCount;
	}

	SerializerNode Save() const;
	void Load(const SerializerNode& node);

	static std::string CustomInspectorValueTypeToString(CustomInspectorValueType type);
	static CustomInspectorValueType CustomInspectorValueTypeFromString(const std::string& type);

	friend class CustomInspector;
private:
	std::string m_Name = "";
	CustomInspectorValueType m_Type = CustomInspectorValueType_Unknown;
	int32_t m_IntValue = 0, m_DefaultIntValue = 0;
	float m_FloatValue = 0.0f, m_DefaultFloatValue = 0.0f;
	bool m_BoolValue = false, m_DefaultBoolValue = false;
	std::string m_StringValue = "", m_DefaultStringValue = "";
	std::shared_ptr<Texture2D> m_TextureValue = nullptr, m_DefaultTextureValue = nullptr;
	float m_VectorValue[4] = { 0.0f, 0.0f, 0.0f, 0.0f }, m_DefaultVectorValue[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	std::array<glm::vec2, CustomInspectorMaxPathPoints> m_PathPoints{};
	std::array<glm::vec2, CustomInspectorMaxPathPoints> m_DefaultPathPoints{};
	int32_t m_PathPointCount = 2, m_DefaultPathPointCount = 2;
};

enum CustomInspectorWidgetType
{
	CustomInspectorWidgetType_Unknown = 0,
	CustomInspectorWidgetType_Slider,
	CustomInspectorWidgetType_Drag,
	CustomInspectorWidgetType_Color,
	CustomInspectorWidgetType_Texture,
	CustomInspectorWidgetType_Path,
	CustomInspectorWidgetType_Button,
	CustomInspectorWidgetType_Checkbox,
	CustomInspectorWidgetType_Input,
	CustomInspectorWidgetType_Seed,
	CustomInspectorWidgetType_Dropdown,
	CustomInspectorWidgetType_Seperator,
	CustomInspectorWidgetType_NewLine,
	CustomInspectorWidgetType_Text
};

class CustomInspectorWidget
{
public:
	CustomInspectorWidget(CustomInspectorWidgetType type = CustomInspectorWidgetType_Unknown);
	~CustomInspectorWidget();

	inline CustomInspectorWidgetType GetType() const { return m_Type; }
	inline std::string GetTypeName() const { return CustomInspectorWidgetTypeToString(m_Type); }
	inline std::string GetLabel() const { return m_Label; }
	inline std::string GetVariableName() const { return m_VariableName; }
	inline void SetLabel(const std::string& label) { m_Label = label; }
	inline void SetVariableName(const std::string& variableName) { m_VariableName = variableName; }
	inline void SetActionName(const std::string& actionName) { m_VariableName = actionName; }
	inline void SetTooltip(const std::string& tooltip) { m_Tooltip = tooltip; }
	inline void SetConstraints(float a = 0.0f, float b = 0.0f, float c = 0.0f, float d = 0.0f) { m_Constratins[0] = a; m_Constratins[1] = b; m_Constratins[2] = c; m_Constratins[3] = d; }
	inline void SetFontName(const std::string& fontName) { m_FontName = fontName; }
	inline void SetDropdownOptions(const std::vector<std::string>& options) { m_DropdownOptions = options; }
	inline void SetSpeed(float speed) { m_FSpeed = speed; m_ISpeed = static_cast<int32_t>(speed); }
	inline void SetRenderOnCondition(const std::string condtionName, int32_t conditionValue) { m_UseRenderOnCondition = true; m_RenderOnConditionName = condtionName; m_RenderOnConditionValue = conditionValue; }
	inline void ClearCondition() { m_UseRenderOnCondition = false; m_RenderOnConditionName = ""; m_RenderOnConditionValue = 0; }

	SerializerNode Save() const;
	void Load(SerializerNode node);

	static std::string CustomInspectorWidgetTypeToString(CustomInspectorWidgetType type);
	static CustomInspectorWidgetType CustomInspectorWidgetTypeFromString(const std::string& type);

	friend class CustomInspector;
private:
	std::string m_Label = "";
	std::string m_VariableName = "";
	std::string m_FontName = "";
	CustomInspectorWidgetType m_Type = CustomInspectorWidgetType_Unknown;
	float m_Constratins[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	float m_FSpeed = 1.0f;
	int32_t m_ISpeed = 1;
	std::string m_Tooltip = "";
	std::vector<int32_t> m_SeedHistory;
	std::vector<std::string> m_DropdownOptions;
	std::string m_ID = "";
	bool m_UseRenderOnCondition = false;
	std::string m_RenderOnConditionName = "";
	int32_t m_RenderOnConditionValue = 0;
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
	CustomInspectorValue& AddPathVariable(const std::string& name,
		const std::array<glm::vec2, CustomInspectorMaxPathPoints>& defaultPoints = {}, int defaultPointCount = 2);
	CustomInspectorValue& AddVairableFromConfig(const nlohmann::json& config);

	bool HasWidget(const std::string& name);
	CustomInspectorWidget& GetWidget(const std::string& name);
	void RemoveWidget(const std::string& name);
	CustomInspectorWidget& AddWidget(const std::string& name, const CustomInspectorWidget& widget);
	CustomInspectorWidget& AddSliderWidget(const std::string& label, const std::string& variableName, float min = 0.0f, float max = 0.0f);
	CustomInspectorWidget& AddDragWidget(const std::string& label, const std::string& variableName, float min = 0.0f, float max = 0.0f, float speed = 1.0f);
	CustomInspectorWidget& AddColorWidget(const std::string& label, const std::string& variableName);
	CustomInspectorWidget& AddTextureWidget(const std::string& label, const std::string& variableName, float width = 100.0f, float height = 100.0f);
	CustomInspectorWidget& AddPathWidget(const std::string& label, const std::string& variableName);
	CustomInspectorWidget& AddButtonWidget(const std::string& label, const std::string& actionName);
	CustomInspectorWidget& AddCheckboxWidget(const std::string& label, const std::string& variableName);
	CustomInspectorWidget& AddInputWidget(const std::string& label, const std::string& variableName);
	CustomInspectorWidget& AddSeedWidget(const std::string& label, const std::string& variableName);
	CustomInspectorWidget& AddDropdownWidget(const std::string& label, const std::string& variableName, const std::vector<std::string>& options);
	CustomInspectorWidget& AddTextWidget(const std::string& label, const std::string& font = "");
	CustomInspectorWidget& AddSeperatorWidget();
	CustomInspectorWidget& AddNewLineWidget();
	CustomInspectorWidget& AddWidgetFromString(const std::string& label, const std::string& type, const std::string& variableName);

	
	CustomInspectorWidget& SetWidgetDropdownOptions(const std::string& label, const std::vector<std::string>& options);
	CustomInspectorWidget& SetWidgetConstraints(const std::string& label, float a = 0.0f, float b = 0.0f, float c = 0.0f, float d = 0.0f);
	CustomInspectorWidget& SetWidgetSpeed(const std::string& label, float speed = 1.0f);
	CustomInspectorWidget& SetWidgetTooltip(const std::string& label, const std::string& value = "Default Tooltip");
	CustomInspectorWidget& SetWidgetFont(const std::string& label, const std::string& value = "");

	SerializerNode SaveData() const;
	void LoadData(SerializerNode node);
	SerializerNode Save() const;
	void Load(SerializerNode node);
	bool LoadConfig(const nlohmann::json& config);

	bool Render();
	inline const std::string& GetDescription() const { return m_Description; }
	inline const std::string& GetLastChangedVariable() const { return m_LastChangedVariable; }
	inline const std::string& GetLastAction() const { return m_LastAction; }
	inline void SetShowResetButton(bool show) { m_ShowResetButton = show; }

	inline void Clear()
	{
		m_Values.clear();
		m_Widgets.clear();
		m_WidgetsOrder.clear();
		m_Description.clear();
		m_LastChangedVariable.clear();
		m_LastAction.clear();
	}
	inline const std::unordered_map<std::string, CustomInspectorValue>& GetValues() const { return m_Values; }
	inline const std::unordered_map<std::string, CustomInspectorWidget>& GetWidgets() const { return m_Widgets; }
	inline const std::vector<std::string>& GetWidgetsOrder() const { return m_WidgetsOrder; }

private:

	bool RenderSlider(const CustomInspectorWidget& widget);
	bool RenderDrag(const CustomInspectorWidget& widget);
	bool RenderColor(const CustomInspectorWidget& widget);
	bool RenderTexture(const CustomInspectorWidget& widget);
	bool RenderButton(const CustomInspectorWidget& widget);
	bool RenderCheckbox(const CustomInspectorWidget& widget);
	bool RenderInput(const CustomInspectorWidget& widget);
	bool RenderSeed(CustomInspectorWidget& widget);
	bool RenderDropdown(const CustomInspectorWidget& widget);
	bool RenderPath(const CustomInspectorWidget& widget);


private:
	std::unordered_map<std::string, CustomInspectorValue> m_Values;
	std::unordered_map<std::string, CustomInspectorWidget> m_Widgets;
	std::vector<std::string> m_WidgetsOrder;
	std::string m_ID = "";
	std::string m_Description;
	std::string m_LastChangedVariable;
	std::string m_LastAction;
	bool m_ShowResetButton = true;
};
