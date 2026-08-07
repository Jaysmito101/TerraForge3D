#pragma once

#include <nlohmann/json.hpp>

#include "Generators/NoiseAlgorithmCatalog.h"

namespace tf3d::generators
{

    inline bool ApplyNoiseAlgorithmMetadata(nlohmann::json &config, const NoiseAlgorithmCatalog &catalog)
    {
        if (!catalog.IsValid())
            return false;

        const auto applyParams = [&](nlohmann::json &params) {
            if (!params.is_array())
                return true;
            for (auto &parameter : params) {
                if (!parameter.is_object())
                    continue;
                if (parameter.value("Name", "") == "NoiseAlgorithm") {
                    parameter["Options"] = catalog.Labels();
                    if (parameter.contains("Default") && parameter["Default"].is_string()) {
                        int defaultValue = 0;
                        if (!catalog.TryGetValue(parameter["Default"].get<std::string>(), defaultValue))
                            return false;
                        parameter["Default"] = defaultValue;
                    }
                    const std::string description = parameter.value("Description", "");
                    parameter["Tooltip"]          = description.empty()
                                                        ? catalog.Tooltip()
                                                        : description + "\n\n" + catalog.Tooltip();
                }

                if (parameter.value("Conditional", "") != "NoiseAlgorithm" ||
                    !parameter.contains("ConditionalValues") || !parameter["ConditionalValues"].is_array())
                    continue;

                nlohmann::json resolvedValues = nlohmann::json::array();
                for (const auto &conditionalValue : parameter["ConditionalValues"]) {
                    if (conditionalValue.is_number_integer())
                        resolvedValues.push_back(conditionalValue.get<int32_t>());
                    else if (conditionalValue.is_string()) {
                        int algorithmValue = 0;
                        if (!catalog.TryGetValue(conditionalValue.get<std::string>(), algorithmValue))
                            return false;
                        resolvedValues.push_back(algorithmValue);
                    }
                }
                parameter["ConditionalValues"] = resolvedValues;
            }
            return true;
        };

        if (config.contains("Params") && !applyParams(config["Params"]))
            return false;
        if (config.contains("Sections") && config["Sections"].is_array()) {
            for (auto &section : config["Sections"]) {
                if (section.is_object() && section.contains("Params") && !applyParams(section["Params"]))
                    return false;
            }
        }
        return true;
    }

} // namespace tf3d::generators
