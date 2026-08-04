#include "Generators/BiomeFilterStack.h"

#include "Data/ApplicationState.h"
#include "Data/ResourceManager.h"
#include "Utils/Utils.h"

#include <unordered_map>

BiomeFilterStack::BiomeFilterStack(ApplicationState* appState)
	: m_AppState(appState)
{
	m_Catalog = std::make_shared<BiomeFilterCatalog>(m_AppState);
	m_ResultA = std::make_shared<GeneratorData>();
	m_ResultB = std::make_shared<GeneratorData>();
}

void BiomeFilterStack::Resize(size_t dataSize, int resolution)
{
	m_DataSize = dataSize;
	m_Resolution = glm::max(resolution, 1);
	for (const auto& temp : m_TempBuffers) temp->Resize(dataSize);
	m_ResultA->Resize(dataSize);
	m_ResultB->Resize(dataSize);
	for (const auto& filter : m_Filters) filter->Resize(m_Resolution);
	m_RequireUpdation = true;
}

void BiomeFilterStack::EnsureTempBufferCount(size_t count)
{
	while (m_TempBuffers.size() < count)
	{
		auto buffer = std::make_shared<GeneratorData>();
		buffer->Resize(m_DataSize);
		m_TempBuffers.push_back(std::move(buffer));
	}
}

int BiomeFilterStack::AddFilter(const std::shared_ptr<BiomeFilterDefinition>& definition)
{
	if (definition == nullptr) return -1;
	m_Filters.push_back(std::make_shared<BiomeFilter>(m_AppState, definition));
	m_Filters.back()->Resize(m_Resolution);
	m_RequireUpdation = true;
	return static_cast<int>(m_Filters.size()) - 1;
}

bool BiomeFilterStack::ShowSettings(int filterIndex)
{
	if (filterIndex < 0 || filterIndex >= static_cast<int>(m_Filters.size())) return false;
	ImGui::PushID(m_Filters[filterIndex]->GetID().c_str());
	const bool changed = m_Filters[filterIndex]->ShowSettings();
	ImGui::PopID();
	if (changed) m_RequireUpdation = true;
	return changed;
}

namespace
{
	void SetUniformFromParameter(const std::shared_ptr<ComputeShader>& shader, const std::string& uniformName,
		const CustomInspectorValue& value, int& textureSlot)
	{
		switch (value.GetType())
		{
		case CustomInspectorValueType_Int: shader->SetUniform1i(uniformName, value.GetInt()); break;
		case CustomInspectorValueType_Float: shader->SetUniform1f(uniformName, value.GetFloat()); break;
		case CustomInspectorValueType_Bool: shader->SetUniform1i(uniformName, value.GetBool() ? 1 : 0); break;
		case CustomInspectorValueType_Vector2: shader->SetUniform2f(uniformName, value.GetVector2()); break;
		case CustomInspectorValueType_Vector3: shader->SetUniform3f(uniformName, value.GetVector3()); break;
		case CustomInspectorValueType_Vector4: shader->SetUniform4f(uniformName, value.GetVector4()); break;
		case CustomInspectorValueType_Texture:
			if (value.GetTexture() != nullptr) shader->SetUniform1i(uniformName, value.GetTexture()->Bind(textureSlot++));
			break;
		case CustomInspectorValueType_String:
		case CustomInspectorValueType_Unknown:
		default:
			break;
		}
	}

	void SetUniformFromJson(const std::shared_ptr<ComputeShader>& shader, const std::string& uniformName,
		const nlohmann::json& binding, int& textureSlot)
	{
		const nlohmann::json* value = &binding;
		std::string type;
		if (binding.is_object())
		{
			type = binding.value("Type", "");
			if (binding.contains("Value")) value = &binding["Value"];
			else if (binding.contains("Constant")) value = &binding["Constant"];
			else return;
		}

		if (type == "Bool" || (type.empty() && value->is_boolean()))
		{
			shader->SetUniform1i(uniformName, value->get<bool>() ? 1 : 0);
		}
		else if (type == "Int" || (type.empty() && value->is_number_integer()))
		{
			shader->SetUniform1i(uniformName, value->get<int>());
		}
		else if (type == "Float" || (type.empty() && value->is_number()))
		{
			shader->SetUniform1f(uniformName, value->get<float>());
		}
		else if ((type == "Vector2" || (type.empty() && value->is_array() && value->size() == 2)) && value->is_array())
		{
			shader->SetUniform2f(uniformName, value->at(0).get<float>(), value->at(1).get<float>());
		}
		else if ((type == "Vector3" || (type.empty() && value->is_array() && value->size() == 3)) && value->is_array())
		{
			shader->SetUniform3f(uniformName, value->at(0).get<float>(), value->at(1).get<float>(), value->at(2).get<float>());
		}
		else if ((type == "Vector4" || (type.empty() && value->is_array() && value->size() == 4)) && value->is_array())
		{
			shader->SetUniform4f(uniformName, value->at(0).get<float>(), value->at(1).get<float>(), value->at(2).get<float>(), value->at(3).get<float>());
		}
		else if (type == "Texture")
		{
			(void)textureSlot;
		}
	}
}

void BiomeFilterStack::SetPassUniforms(const std::shared_ptr<BiomeFilter>& filter, const std::shared_ptr<ComputeShader>& shader, const nlohmann::json& bindings)
{
	if (!bindings.is_object()) return;
	int textureSlot = 4;
	for (const auto& [uniformName, binding] : bindings.items())
	{
		if (binding.is_object() && binding.contains("Parameter"))
		{
			const std::string parameterName = binding["Parameter"].get<std::string>();
			const auto parameter = filter->GetParameters().find(parameterName);
			if (parameter != filter->GetParameters().end())
				SetUniformFromParameter(shader, uniformName, parameter->second, textureSlot);
			continue;
		}
		SetUniformFromJson(shader, uniformName, binding, textureSlot);
	}
}

void BiomeFilterStack::RunPhase(const std::shared_ptr<BiomeFilter>& filter, const nlohmann::json& pass, GeneratorData* input, GeneratorData* output)
{
	const std::string phase = pass.value("Phase", "");
	const auto shader = filter->GetPhaseShader(m_AppState, phase);
	if (shader == nullptr) return;
	input->Bind(0);
	output->Bind(2);
	shader->Bind();
	shader->SetUniform1i("u_Resolution", m_Resolution);
	SetPassUniforms(filter, shader, pass.value("Uniforms", nlohmann::json::object()));
	const auto workgroupSize = m_AppState->constants.gpuWorkgroupSize;
	const auto dispatchSize = (m_Resolution + workgroupSize - 1) / workgroupSize;
	shader->Dispatch(dispatchSize, dispatchSize, 1);
	shader->SetMemoryBarrier();
}

void BiomeFilterStack::RunMergePhase(const std::shared_ptr<BiomeFilter>& filter, const nlohmann::json& merge, GeneratorData* input, GeneratorData* operation, GeneratorData* output)
{
	const std::string phase = merge.value("Phase", "");
	const auto shader = filter->GetPhaseShader(m_AppState, phase);
	if (shader == nullptr) return;
	input->Bind(0);
	operation->Bind(1);
	output->Bind(2);
	shader->Bind();
	shader->SetUniform1i("u_Resolution", m_Resolution);
	shader->SetUniform1f("u_Strength", filter->GetStrength());
	shader->SetUniform1i("u_MergeMode", static_cast<int>(filter->GetMergeMode()));
	shader->SetUniform1i("u_UseMask", filter->UsesMask() ? 1 : 0);
	shader->SetUniform1i("u_InvertMask", filter->InvertsMask() ? 1 : 0);
	SetPassUniforms(filter, shader, merge.value("Uniforms", nlohmann::json::object()));
	if (filter->UsesMask())
	{
		filter->GetMaskTexture()->Bind(3);
		shader->SetUniform1i("u_MaskTexture", 3);
	}
	const auto workgroupSize = m_AppState->constants.gpuWorkgroupSize;
	const auto dispatchSize = (m_Resolution + workgroupSize - 1) / workgroupSize;
	shader->Dispatch(dispatchSize, dispatchSize, 1);
	shader->SetMemoryBarrier();
}

void BiomeFilterStack::RunFilter(const std::shared_ptr<BiomeFilter>& filter, GeneratorData* input, GeneratorData* output)
{
	const auto& metadata = filter->GetDefinition()->GetMetadata();
	const auto& execution = metadata.contains("Execution")
		? metadata["Execution"]
		: nlohmann::json::object();
	std::vector<nlohmann::json> passes;
	if (execution.contains("Passes") && execution["Passes"].is_array())
	{
		for (const auto& pass : execution["Passes"])
		{
			if (pass.is_object()) passes.push_back(pass);
		}
	}
	nlohmann::json merge = execution.value("Merge", nlohmann::json::object());
	if (passes.empty() || !merge.is_object() || merge.value("Phase", "").empty())
	{
		TF3D_LOG_ERROR("Filter '{}' has an incomplete phase execution description.", filter->GetName());
		input->CopyTo(output);
		return;
	}

	std::unordered_map<std::string, GeneratorData*> resources;
	resources.emplace("Current", input);
	resources.emplace("Next", output);
	resources.emplace("Output", output);

	const auto resourcesDescription = metadata.value("Resources", nlohmann::json::object());
	if (!resourcesDescription.is_object())
	{
		TF3D_LOG_ERROR("Filter '{}' has an invalid Resources object.", filter->GetName());
		input->CopyTo(output);
		return;
	}
	const auto tempDescription = resourcesDescription.value("Temps", nlohmann::json::array());
	if (!tempDescription.is_array())
	{
		TF3D_LOG_ERROR("Filter '{}' has an invalid Resources.Temps declaration.", filter->GetName());
		input->CopyTo(output);
		return;
	}

	EnsureTempBufferCount(tempDescription.size());
	for (size_t tempIndex = 0; tempIndex < tempDescription.size(); tempIndex++)
	{
		const auto& declaration = tempDescription.at(tempIndex);
		if (!declaration.is_object())
		{
			TF3D_LOG_ERROR("Filter '{}' has a non-object temporary resource declaration.", filter->GetName());
			input->CopyTo(output);
			return;
		}
		const std::string name = declaration.value("Name", "");
		const std::string type = declaration.value("Type", "Field");
		if (name.empty() || type != "Field" || resources.find(name) != resources.end())
		{
			TF3D_LOG_ERROR("Filter '{}' has an invalid or duplicate temporary resource '{}'.", filter->GetName(), name);
			input->CopyTo(output);
			return;
		}
		resources.emplace(name, m_TempBuffers[tempIndex].get());
	}

	const std::string iterationsParameter = execution.value("IterationsParameter", "");
	const int iterations = glm::clamp(iterationsParameter.empty() ? 1 : filter->GetIntegerParameter(iterationsParameter, 1), 0, 64);
	if (iterations == 0 || filter->GetStrength() <= 0.0f)
	{
		input->CopyTo(output);
		return;
	}
	for (const auto& pass : passes)
	{
		const std::string phase = pass.value("Phase", "");
		if (phase.empty())
		{
			TF3D_LOG_ERROR("Filter '{}' has a pass without a phase.", filter->GetName());
			input->CopyTo(output);
			return;
		}
		if (filter->GetPhaseShader(m_AppState, phase) == nullptr)
		{
			TF3D_LOG_ERROR("Filter '{}' is missing phase '{}'.", filter->GetName(), phase);
			input->CopyTo(output);
			return;
		}
		const std::string inputName = pass.value("Input", "");
		const std::string outputName = pass.value("Output", "");
		if (inputName.empty() || outputName.empty() || resources.find(inputName) == resources.end() || resources.find(outputName) == resources.end())
		{
			TF3D_LOG_ERROR("Filter '{}' has a pass with an unknown or missing Input/Output resource.", filter->GetName());
			input->CopyTo(output);
			return;
		}
	}
	if (filter->GetPhaseShader(m_AppState, merge.value("Phase", "")) == nullptr)
	{
		TF3D_LOG_ERROR("Filter '{}' is missing merge phase '{}'.", filter->GetName(), merge.value("Phase", ""));
		input->CopyTo(output);
		return;
	}
	const std::string mergeInputName = merge.value("Input", "");
	const std::string mergeOperationName = merge.value("Operation", "");
	const std::string mergeOutputName = merge.value("Output", "");
	if (mergeInputName.empty() || mergeOperationName.empty() || mergeOutputName.empty()
		|| resources.find(mergeInputName) == resources.end()
		|| resources.find(mergeOperationName) == resources.end()
		|| resources.find(mergeOutputName) == resources.end()
		|| resources.at(mergeOutputName) != output
		|| resources.at(mergeOperationName) == output)
	{
		TF3D_LOG_ERROR("Filter '{}' has a merge with an unknown or missing Input/Operation/Output resource.", filter->GetName());
		input->CopyTo(output);
		return;
	}

	for (int iteration = 0; iteration < iterations; iteration++)
	{
		for (const auto& pass : passes)
		{
			const std::string inputName = pass.value("Input", "");
			const std::string outputName = pass.value("Output", "");
			RunPhase(filter, pass, resources.at(inputName), resources.at(outputName));
		}
	}
	RunMergePhase(filter, merge, resources.at(mergeInputName), resources.at(mergeOperationName), resources.at(mergeOutputName));
}

void BiomeFilterStack::Load(SerializerNode data)
{
	if (data == nullptr) return;
	m_Filters.clear();
	for (const auto& filterNode : data->GetNodeArray("Filters"))
	{
		auto definition = m_Catalog->FindByID(filterNode->GetString("DefinitionID"));
		const int index = AddFilter(definition);
		if (index >= 0) m_Filters[index]->Load(filterNode);
	}
	m_RequireUpdation = true;
}

SerializerNode BiomeFilterStack::Save() const
{
	auto node = CreateSerializerNode();
	node->CreateNodeArray("Filters");
	for (const auto& filter : m_Filters) node->PushToNodeArray("Filters", filter->Save());
	return node;
}

void BiomeFilterStack::Update(GeneratorData* baseResult)
{
	if (baseResult == nullptr || m_Filters.empty())
	{
		m_RequireUpdation = false;
		return;
	}

	GeneratorData* current = baseResult;
	GeneratorData* next = m_ResultA.get();
	bool applied = false;
	for (const auto& filter : m_Filters)
	{
		if (!filter->IsEnabled()) continue;
		filter->UpdateGeneratedMask(current);
		RunFilter(filter, current, next);
		current = next;
		next = current == m_ResultA.get() ? m_ResultB.get() : m_ResultA.get();
		applied = true;
	}

	if (applied && current != baseResult)
	{
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
		current->CopyTo(baseResult);
	}
	m_RequireUpdation = false;
}
