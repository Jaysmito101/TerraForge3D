#include "Inspector/CustomInspectorWidget.h"
#include "Utils/Utils.h"

namespace tf3d::inspector
{

    CustomInspectorWidget::CustomInspectorWidget(CustomInspectorWidgetType type)
        : m_Type(type), m_ID(GenerateId(16))
    {
    }

    CustomInspectorWidget::~CustomInspectorWidget() = default;

    std::string CustomInspectorWidget::CustomInspectorWidgetTypeToString(CustomInspectorWidgetType type)
    {
        switch (type) {
            case CustomInspectorWidgetType::Slider:
                return "Slider";
            case CustomInspectorWidgetType::Drag:
                return "Drag";
            case CustomInspectorWidgetType::Color:
                return "Color";
            case CustomInspectorWidgetType::Texture:
                return "Texture";
            case CustomInspectorWidgetType::Path:
                return "Path";
            case CustomInspectorWidgetType::Curve:
                return "Curve";
            case CustomInspectorWidgetType::Octaves:
                return "Octaves";
            case CustomInspectorWidgetType::Button:
                return "Button";
            case CustomInspectorWidgetType::Checkbox:
                return "Checkbox";
            case CustomInspectorWidgetType::Input:
                return "Input";
            case CustomInspectorWidgetType::Seed:
                return "Seed";
            case CustomInspectorWidgetType::Dropdown:
                return "Dropdown";
            case CustomInspectorWidgetType::Separator:
                return "Separator";
            case CustomInspectorWidgetType::NewLine:
                return "NewLine";
            case CustomInspectorWidgetType::Text:
                return "Text";
            case CustomInspectorWidgetType::Unknown:
            default:
                return "Unknown";
        }
    }

    CustomInspectorWidgetType CustomInspectorWidget::CustomInspectorWidgetTypeFromString(const std::string &type)
    {
        if (type == "Slider")
            return CustomInspectorWidgetType::Slider;
        if (type == "Drag")
            return CustomInspectorWidgetType::Drag;
        if (type == "Color")
            return CustomInspectorWidgetType::Color;
        if (type == "Texture")
            return CustomInspectorWidgetType::Texture;
        if (type == "Path")
            return CustomInspectorWidgetType::Path;
        if (type == "Curve")
            return CustomInspectorWidgetType::Curve;
        if (type == "Octaves")
            return CustomInspectorWidgetType::Octaves;
        if (type == "Button")
            return CustomInspectorWidgetType::Button;
        if (type == "Checkbox")
            return CustomInspectorWidgetType::Checkbox;
        if (type == "Input")
            return CustomInspectorWidgetType::Input;
        if (type == "Seed")
            return CustomInspectorWidgetType::Seed;
        if (type == "Dropdown")
            return CustomInspectorWidgetType::Dropdown;
        if (type == "Separator")
            return CustomInspectorWidgetType::Separator;
        if (type == "NewLine")
            return CustomInspectorWidgetType::NewLine;
        if (type == "Text")
            return CustomInspectorWidgetType::Text;
        return CustomInspectorWidgetType::Unknown;
    }

} // namespace tf3d::inspector
