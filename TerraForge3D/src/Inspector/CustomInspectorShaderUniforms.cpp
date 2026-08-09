#include "Inspector/CustomInspector.h"
#include "Inspector/CustomInspectorTypes.h"
#include "Inspector/CustomInspectorValue.h"
#include "Utils/Utils.h"

#include <map>
#include <utility>

namespace tf3d::inspector
{

    namespace
    {
        struct ShaderUniformDeclaration {
            std::string type;
            size_t arraySize = 0;
        };

        using ShaderUniformDeclarationMap = std::map<std::string, ShaderUniformDeclaration>;

        bool RegisterUniform(ShaderUniformDeclarationMap &uniforms,
                             const std::string &name,
                             ShaderUniformDeclaration declaration,
                             std::string *error)
        {
            const auto existing = uniforms.find(name);
            if (existing == uniforms.end()) {
                uniforms.emplace(name, std::move(declaration));
                return true;
            }

            if (existing->second.type == declaration.type && existing->second.arraySize == declaration.arraySize)
                return true;

            if (error) {
                *error = "uniform '" + name + "' has conflicting declarations ('" + existing->second.type +
                         (existing->second.arraySize > 0 ? "[]" : "") + "' and '" + declaration.type +
                         (declaration.arraySize > 0 ? "[]" : "") + "')";
            }
            return false;
        }

        bool AddShaderUniformValue(const CustomInspectorValue &value,
                                   std::string_view owner,
                                   ShaderUniformDeclarationMap &uniforms,
                                   std::string *error)
        {
            auto fail = [&](const std::string &message) {
                if (error)
                    *error = std::string(owner) + ": " + message;
                return false;
            };

            const std::string uniformName = value.IsShaderUniformConfigured()
                                                ? value.GetShaderUniformName()
                                                : "u_" + value.GetName();
            if (uniformName.empty())
                return true;
            if (!utils::IsValidShaderSymbol(uniformName))
                return fail("value '" + value.GetName() + "' resolves to invalid shader uniform '" + uniformName + "'");

            ShaderUniformDeclaration declaration;
            switch (value.GetType()) {
                case CustomInspectorValueType::Int:
                case CustomInspectorValueType::Bool:
                    declaration.type = "int";
                    break;
                case CustomInspectorValueType::Float:
                    declaration.type = "float";
                    break;
                case CustomInspectorValueType::Vector2:
                    declaration.type = "vec2";
                    break;
                case CustomInspectorValueType::Vector3:
                    declaration.type = "vec3";
                    break;
                case CustomInspectorValueType::Vector4:
                    declaration.type = "vec4";
                    break;
                case CustomInspectorValueType::FloatArray: {
                    const auto values = value.Get<std::vector<float>>();
                    if (values.empty())
                        return fail("value '" + value.GetName() + "' has an empty FloatArray");
                    declaration.type      = "float";
                    declaration.arraySize = values.size();
                    break;
                }
                case CustomInspectorValueType::Path:
                    declaration.type      = "vec2";
                    declaration.arraySize = CustomInspectorMaxPathPoints;
                    break;
                case CustomInspectorValueType::String:
                case CustomInspectorValueType::Texture:
                case CustomInspectorValueType::Curve:
                case CustomInspectorValueType::Unknown:
                case CustomInspectorValueType::Count:
                default:
                    return fail("value '" + value.GetName() + "' uses unsupported shader-bound type");
            }

            if (!RegisterUniform(uniforms, uniformName, declaration, error))
                return false;

            if (value.GetType() == CustomInspectorValueType::Path) {
                const std::string countName = uniformName + "Count";
                if (!utils::IsValidShaderSymbol(countName))
                    return fail("path value '" + value.GetName() + "' resolves to invalid count uniform '" + countName + "'");
                if (!RegisterUniform(uniforms, countName, {"int", 0}, error))
                    return false;
            }
            return true;
        }

        std::string EmitShaderUniformDeclarations(const ShaderUniformDeclarationMap &uniforms)
        {
            std::string declarations;
            for (const auto &[name, declaration] : uniforms) {
                declarations += "uniform " + declaration.type + " " + name;
                if (declaration.arraySize > 0)
                    declarations += "[" + std::to_string(declaration.arraySize) + "]";
                declarations += ";\n";
            }
            return declarations;
        }
    } // namespace

    std::optional<std::string> CustomInspector::GetShaderUniformDeclarations(std::string *error) const
    {
        ShaderUniformDeclarationMap uniforms;
        std::string uniformError;
        for (const auto &[path, value] : m_Values) {
            if (!AddShaderUniformValue(value, path, uniforms, &uniformError)) {
                if (error)
                    *error = uniformError;
                return std::nullopt;
            }
        }

        if (error)
            error->clear();
        return EmitShaderUniformDeclarations(uniforms);
    }

} // namespace tf3d::inspector
