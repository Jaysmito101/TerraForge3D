#pragma once

#include <cstddef>

namespace tf3d::inspector
{

    enum class CustomInspectorValueType {
        Unknown = 0,
        Int,
        Float,
        Bool,
        String,
        Vector2,
        Vector3,
        Vector4,
        FloatArray,
        Texture,
        Path,
        Curve,
        Count
    };

    inline constexpr size_t CustomInspectorMaxPathPoints  = 16;
    inline constexpr size_t CustomInspectorMaxCurvePoints = 16;

    enum class CustomInspectorWidgetType {
        Unknown = 0,
        Slider,
        Drag,
        Color,
        Texture,
        Path,
        Curve,
        Octaves,
        Button,
        Checkbox,
        Input,
        Seed,
        Dropdown,
        Separator,
        NewLine,
        Text
    };

} // namespace tf3d::inspector
