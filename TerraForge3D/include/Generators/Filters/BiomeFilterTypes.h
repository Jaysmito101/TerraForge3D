#pragma once

#include "Base/Base.h"

#include <string_view>

namespace tf3d::generators
{

    enum class BiomeFilterImplementation {
        Unknown,
        PhaseChain,
    };

    enum class BiomeFilterMergeMode {
        Override,
        Add,
        Subtract,
        Multiply,
        Blend,
    };

    inline constexpr BiomeFilterImplementation BiomeFilterImplementationFromString(std::string_view value) noexcept
    {
        if (value == "PhaseChain") {
            return BiomeFilterImplementation::PhaseChain;
        }
        return BiomeFilterImplementation::Unknown;
    }

    inline constexpr BiomeFilterMergeMode BiomeFilterMergeModeFromString(std::string_view value) noexcept
    {
        if (value == "Override") {
            return BiomeFilterMergeMode::Override;
        }
        if (value == "Add") {
            return BiomeFilterMergeMode::Add;
        }
        if (value == "Subtract") {
            return BiomeFilterMergeMode::Subtract;
        }
        if (value == "Multiply") {
            return BiomeFilterMergeMode::Multiply;
        }
        return BiomeFilterMergeMode::Blend;
    }

} // namespace tf3d::generators
