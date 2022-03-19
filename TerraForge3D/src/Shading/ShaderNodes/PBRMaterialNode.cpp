#include "Shading/ShaderNodes/PBRMaterialNode.h"

#include <iostream>

static void LoadPBRFunctions(GLSLHandler *handler)
{
	GLSLFunction DistributionGGX("DistributionGGX", "vec3 N, vec3 H, float roughness", "float");
	DistributionGGX.AddLine(GLSLLine("float a = roughness * roughness;"));
	DistributionGGX.AddLine(GLSLLine("float a2 = a * a;"));
	DistributionGGX.AddLine(GLSLLine("float NdotH = max(dot(N, H), 0.0f);"));
	DistributionGGX.AddLine(GLSLLine("float NdotH2 = NdotH * NdotH;"));
	DistributionGGX.AddLine(GLSLLine("float denom = NdotH2 * (a2 - 1.0) + 1.0;"));
	DistributionGGX.AddLine(GLSLLine("return a2 / (PI * denom * denom);"));
	handler->AddFunction(DistributionGGX);
	GLSLFunction GeometrySchlickGGX("GeometrySchlickGGX", "float NdotV, float roughness", "float");
	GeometrySchlickGGX.AddLine(GLSLLine("float r = (roughness + 1.0);"));
	GeometrySchlickGGX.AddLine(GLSLLine("float k = (r*r) / 8.0;"));
	GeometrySchlickGGX.AddLine(GLSLLine(""));
	GeometrySchlickGGX.AddLine(GLSLLine("float nom   = NdotV;"));
	GeometrySchlickGGX.AddLine(GLSLLine("float denom = NdotV * (1.0 - k) + k;"));
	GeometrySchlickGGX.AddLine(GLSLLine(""));
	GeometrySchlickGGX.AddLine(GLSLLine("return nom / denom;"));
	handler->AddFunction(GeometrySchlickGGX);
	GLSLFunction GeometrySmith("GeometrySmith", "vec3 N, vec3 V, vec3 L, float roughness", "float");
	GeometrySmith.AddLine(GLSLLine("float NdotV = max(dot(N, V), 0.0);"));
	GeometrySmith.AddLine(GLSLLine("float NdotL = max(dot(N, L), 0.0);"));
	GeometrySmith.AddLine(GLSLLine(""));
	GeometrySmith.AddLine(GLSLLine("float ggx2 = GeometrySchlickGGX(NdotV, roughness);"));
	GeometrySmith.AddLine(GLSLLine("float ggx1 = GeometrySchlickGGX(NdotL, roughness);"));
	GeometrySmith.AddLine(GLSLLine(""));
	GeometrySmith.AddLine(GLSLLine("return ggx1 * ggx2;"));
	handler->AddFunction(GeometrySmith);
	GLSLFunction frenselSchlick("fresnelSchlick", "float cosTheta, vec3 F0", "vec3");
	frenselSchlick.AddLine(GLSLLine("return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);"));
	handler->AddFunction(frenselSchlick);
}

void PBRMaterialNode::OnEvaluate(GLSLFunction *function, GLSLLine *line)
{
	LoadPBRFunctions(handler);
	function->AddLine(GLSLLine("vec3 " + VAR("albedo") + " = vec3(1.0f);", "The albedo color of the material."));
	function->AddLine(GLSLLine("vec3 " + VAR("normal") + " = vec3(1.0f);", "The normal of the material."));
	function->AddLine(GLSLLine("float " + VAR("metallic") + " = 1.0f;", "The metalic of the material."));
	function->AddLine(GLSLLine("float " + VAR("roughness") + " = 1.0f;", "The roughness of the material."));
	function->AddLine(GLSLLine("float " + VAR("ao") + " = 1.0f;", "The ambient occlusion of the material."));

	if(inputPins[0]->IsLinked())
	{
		GLSLLine tmp("", "");
		inputPins[0]->other->Evaluate(GetParams(function, &tmp));
		function->AddLine(GLSLLine("if(" + SDATA(0) + " == 1.0f)", "If gamma corection is enabled adjust the texture accordingly."));
		function->AddLine(GLSLLine("\t" + VAR("albedo") + " = pow(" + tmp.line + ", vec3(gamma));", "The albedo color of the material."));
		function->AddLine(GLSLLine("else", "If gamma corection is disabled just use the texture as it is."));
		function->AddLine(GLSLLine("\t" + VAR("albedo") + " = " + tmp.line + ";", "The albedo color of the material."));
	}

	if(inputPins[1]->IsLinked())
	{
		GLSLLine tmp("", "");
		inputPins[1]->other->Evaluate(GetParams(function, &tmp));
		function->AddLine(GLSLLine(VAR("normal") + " = " + tmp.line + ";", "The normal map sampled for current fragment"));
		function->AddLine(GLSLLine(VAR("normal") + " = TBN * (" + VAR("normal") + " * 2.0f - 1.0f);", "Scale the normal to -1.0f to 1.0f and transform to tangent space"));
	}

	if(inputPins[5]->IsLinked())
	{
		GLSLLine tmp("", "");
		inputPins[5]->other->Evaluate(GetParams(function, &tmp));
		function->AddLine(GLSLLine("", "Using the ARM texture for ao, roughness and metalic"));
		function->AddLine(GLSLLine("vec3 " + VAR("tmp") + " = " + tmp.line + ";", "Temporatily storing the arm data."));
		function->AddLine(GLSLLine(VAR("ao") + " = " + VAR("tmp") + ".x;", "The ambient occlusion of the material."));
		function->AddLine(GLSLLine(VAR("roughness") + " = " + VAR("tmp") + ".y;", "The roughness of the material."));
		function->AddLine(GLSLLine(VAR("metallic") + " = " + VAR("tmp") + ".z;", "The metalic of the material."));
	}

	else
	{
		if(inputPins[2]->IsLinked())
		{
			GLSLLine tmp("", "");
			inputPins[2]->other->Evaluate(GetParams(function, &tmp));
			function->AddLine(GLSLLine(VAR("metallic") + " = " + tmp.line + ".x;", "The metalic of the material."));
		}

		if(inputPins[3]->IsLinked())
		{
			GLSLLine tmp("", "");
			inputPins[3]->other->Evaluate(GetParams(function, &tmp));
			function->AddLine(GLSLLine(VAR("roughness") + " = " + tmp.line + ".x;", "The roughness of the material."));
		}

		if(inputPins[4]->IsLinked())
		{
			GLSLLine tmp("", "");
			inputPins[4]->other->Evaluate(GetParams(function, &tmp));
			function->AddLine(GLSLLine(VAR("ao") + " = " + tmp.line + ".x;", "The ambient occlusion of the material."));
		}
	}

	function->AddLine(GLSLLine("vec3 " + VAR("N") + " = " + VAR("normal") + ";", "The normal vector"));
	function->AddLine(GLSLLine("vec3 " + VAR("V") + " = normalize(_CameraPos - FragPos.xyz);", "The view vector"));
	function->AddLine(GLSLLine("", ""));
	function->AddLine(GLSLLine("", "Calculate reflectance at normal incidence; if dia-electric (like plastic) use F0"));
	function->AddLine(GLSLLine("", "of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)"));
	function->AddLine(GLSLLine("vec3 " + VAR("F0") + " = vec3(0.04f);", ""));
	function->AddLine(GLSLLine(VAR("F0") + " = mix(" + VAR("F0") + ", " + VAR("albedo") + ", " + VAR("metallic") + " );", ""));
	function->AddLine(GLSLLine("", "Reflectance equation"));
	function->AddLine(GLSLLine("vec3 " + VAR("Lo") + " = vec3(0.0f);", ""));
	function->AddLine(GLSLLine("// for(int i = 0 ; i < _NumLights ; i++ )", ""));
	function->AddLine(GLSLLine("// {", ""));
	function->AddLine(GLSLLine("", "Calculate per-light radiance"));
	function->AddLine(GLSLLine("vec3 " + VAR("L") + " = normalize(_LightPosition - FragPos.xyz);", ""));
	function->AddLine(GLSLLine("vec3 " + VAR("H") + " = normalize(" + VAR("V") + " + " + VAR("L") + ");", ""));
	function->AddLine(GLSLLine("float " + VAR("distance") + " = length( _LightPosition - FragPos.xyz );", ""));
	function->AddLine(GLSLLine("float " + VAR("attenuation") + " = 1.0f / (" + VAR("distance") + " * " + VAR("distance") + ");", ""));
	function->AddLine(GLSLLine("vec3  " + VAR("radiance") + " = _LightColor * _LightStrength * " + VAR("attenuation") + ";", ""));
	function->AddLine(GLSLLine("", ""));
	function->AddLine(GLSLLine("", "Color-Torrance BRDF"));
	function->AddLine(GLSLLine("float " + VAR("NDF") + " = DistributionGGX(" + VAR("N") + ", " + VAR("H") + ", " + VAR("roughness") + ");", ""));
	function->AddLine(GLSLLine("float " + VAR("G") + " = GeometrySmith(" + VAR("N") + ", " + VAR("V") + ", " + VAR("L") + ", " + VAR("roughness") + ");", ""));
	function->AddLine(GLSLLine("vec3 " + VAR("F") + " = fresnelSchlick(max(dot(" + VAR("H") + ", " + VAR("V") + "), 0.0f), " + VAR("F0") + ");", ""));
	function->AddLine(GLSLLine("", ""));
	function->AddLine(GLSLLine("vec3 " + VAR("numerator") + " = " + VAR("NDF") + " * " + VAR("G") + " * " + VAR("F") + ";", ""));
	function->AddLine(GLSLLine("float " + VAR("denominator") + " = 4.0f * max(dot(" + VAR("N") + ", " + VAR("V") + "), 0.0f) * max(dot(" + VAR("N") + ", " + VAR("L") + "), 0.0f) + 0.0001f;", "+ 0.0001f to prevent divide by zero"));
	function->AddLine(GLSLLine("vec3 " + VAR("specular") + " = " + VAR("numerator") + " / " + VAR("denominator") + ";", ""));
	function->AddLine(GLSLLine("", ""));
	function->AddLine(GLSLLine("vec3 " + VAR("kS") + " = " + VAR("F") + ";", "kS is equal to Fresnel"));
	function->AddLine(GLSLLine("", ""));
	function->AddLine(GLSLLine("", "For energy conservation, the diffuse and specular light can't"));
	function->AddLine(GLSLLine("", "be above 1.0 (unless the surface emits light); to preserve this"));
	function->AddLine(GLSLLine("", "relationship the diffuse component (kD) should equal 1.0 - kS."));
	function->AddLine(GLSLLine("vec3 " + VAR("kD") + " = vec3(1.0f) - " + VAR("kS") + ";", ""));
	function->AddLine(GLSLLine("", ""));
	function->AddLine(GLSLLine("", "Multiply kD by the inverse metalness such that only non-metals"));
	function->AddLine(GLSLLine("", "have diffuse lighting, or a linear blend if partly metal (pure metals"));
	function->AddLine(GLSLLine("", "have no diffuse light)."));
	function->AddLine(GLSLLine(VAR("kD") + " *= 1.0 - " + VAR("metallic") + ";", ""));
	function->AddLine(GLSLLine("", ""));
	function->AddLine(GLSLLine("", "Scale light by NdotL"));
	function->AddLine(GLSLLine("float " + VAR("NdotL") + " = max(dot(" + VAR("N") + ", " + VAR("L") + "), 0.0f); ", ""));
	function->AddLine(GLSLLine("", ""));
	function->AddLine(GLSLLine("", "// Add to outgoing radiance Lo"));
	function->AddLine(GLSLLine(VAR("Lo") + " += (" + VAR("kD") + " * " + VAR("albedo") + " / PI + " + VAR("specular") + ") * " + VAR("radiance") + " * " + VAR("NdotL") + ";", "Note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again"));
	function->AddLine(GLSLLine("", ""));
	function->AddLine(GLSLLine("// }", ""));
	function->AddLine(GLSLLine("", ""));
	function->AddLine(GLSLLine("", "Ambient lighting (note that the next IBL tutorial will replace"));
	function->AddLine(GLSLLine("", "this ambient lighting with environment lighting)."));
	function->AddLine(GLSLLine("vec3 " + VAR("ambient") + " = vec3(0.03f) * " + VAR("albedo") + " * " + VAR("ao") + ";", ""));
	function->AddLine(GLSLLine("", ""));
	function->AddLine(GLSLLine("vec3 " + VAR("result") + " = " + VAR("ambient") + " + " + VAR("Lo") + ";", ""));
	function->AddLine(GLSLLine("", ""));
	function->AddLine(GLSLLine("if(" + SDATA(1) + " == 1.0f)", "If HDR Tonemapping is enable tonemap the result"));
	function->AddLine(GLSLLine("{", ""));
	function->AddLine(GLSLLine("\t" + VAR("result") + " = " + VAR("result") + " / (" + VAR("result") + " + vec3(1.0f));", ""));
	function->AddLine(GLSLLine("}", ""));
	function->AddLine(GLSLLine("", ""));
	function->AddLine(GLSLLine("if(" + SDATA(0) + " == 1.0f)", "If Gamma Correction is enable gamma correct the result"));
	function->AddLine(GLSLLine("{", ""));
	function->AddLine(GLSLLine("\t" + VAR("result") + " = pow(" + VAR("result") + ", vec3(1.0f/gamma));", ""));
	function->AddLine(GLSLLine("}", ""));
	function->AddLine(GLSLLine("", ""));
	line->line = VAR("result");
}

void PBRMaterialNode::Load(nlohmann::json data)
{
	gammaCorrection = data["gammaCorrection"];
	hdrTonemapping = data["hdrTonemapping"];
}

nlohmann::json PBRMaterialNode::Save()
{
	nlohmann::json data;
	data["type"] = "PBRMaterial";
	data["gammaCorrection"] = gammaCorrection;
	data["hdrTonemapping"] = hdrTonemapping;
	return data;
}

void PBRMaterialNode::UpdateShaders()
{
	sharedData->d0 = gammaCorrection ? 1.0f : 0.0f;
	sharedData->d1 = hdrTonemapping ? 1.0f : 0.0f;
}

void PBRMaterialNode::OnRender()
{
	DrawHeader("PBR Material");
	inputPins[0]->Render();
	ImGui::SameLine();
	ImGui::Text("Albedo");
	ImGui::SameLine();
	outputPins[0]->Render();
	inputPins[1]->Render();
	ImGui::SameLine();
	ImGui::Text("Normal");
	inputPins[2]->Render();
	ImGui::SameLine();
	ImGui::Text("Metallic");
	inputPins[3]->Render();
	ImGui::SameLine();
	ImGui::Text("Roughness");
	inputPins[4]->Render();
	ImGui::SameLine();
	ImGui::Text("AO");
	inputPins[5]->Render();
	ImGui::SameLine();
	ImGui::Text("ARM");

	if (ImGui::Checkbox("Gamma Correction", &gammaCorrection))
	{
		sharedData->d0 = gammaCorrection ? 1.0f : 0.0f;
	}

	if (ImGui::Checkbox("HDR Tonemapping", &hdrTonemapping))
	{
		sharedData->d1 = hdrTonemapping ? 1.0f : 0.0f;
	}
}

PBRMaterialNode::PBRMaterialNode(GLSLHandler *handler)
	:SNENode(handler)
{
	name = "PBR Material";
	headerColor = ImColor(SHADER_MATERIAL_NODE_COLOR);
	inputPins.push_back(new SNEPin(NodeEditorPinType::Input, SNEPinType::SNEPinType_Float3)); // Albedo
	inputPins.push_back(new SNEPin(NodeEditorPinType::Input, SNEPinType::SNEPinType_Float3)); // Normal
	inputPins.push_back(new SNEPin(NodeEditorPinType::Input, SNEPinType::SNEPinType_Float3)); // Metallic
	inputPins.push_back(new SNEPin(NodeEditorPinType::Input, SNEPinType::SNEPinType_Float3)); // Roughness
	inputPins.push_back(new SNEPin(NodeEditorPinType::Input, SNEPinType::SNEPinType_Float3)); // Ambient Occlusion
	inputPins.push_back(new SNEPin(NodeEditorPinType::Input, SNEPinType::SNEPinType_Float3)); // ARM
	outputPins.push_back(new SNEPin(NodeEditorPinType::Output, SNEPinType::SNEPinType_Float3)); // Result
}


PBRMaterialNode::~PBRMaterialNode()
{
}