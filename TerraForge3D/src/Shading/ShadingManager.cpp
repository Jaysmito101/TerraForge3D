#include "Shading/ShadingManager.h"
#include "Utils/Utils.h"
#include "Data/ApplicationState.h"
#include "Profiler.h"
#include "imgui/imgui.h"

#include "Shading/ShaderNodes/ShaderOutputNode.h"

ShadingManager::ShadingManager(ApplicationState *as)
{
	appState = as;
	
	vsh = new GLSLHandler("Primary Vertex Shader");
	gsh = new GLSLHandler("Primary Geometry Shader");
	fsh = new GLSLHandler("Primary Fragment Shader");

	sharedMemoryManager = new SharedMemoryManager();
	sharedMemoryManager->AddItem();

	NodeEditorConfig config;
	config.makeNodeFunc = [&]()
	{

	};
	config.insNodeFunc = [&](nlohmann::json data) -> NodeEditorNode*
	{
		SNENode* node = nullptr;
		if(data["type"] = "ShaderOutput")
		{
			node = new ShaderOutputNode(fsh);
		}
		return node;
	};
	shaderNodeEditor = new NodeEditor(config);
	shaderNodeEditor->name = "Shader Nodes";
	shaderNodeEditor->SetOutputNode(new ShaderOutputNode(fsh));
}

ShadingManager::~ShadingManager()
{
	delete vsh;
	delete gsh;
	delete fsh;
	delete sharedMemoryManager;
}

void ShadingManager::UpdateShaders()
{
	sharedMemoryManager->UpdateShader(appState->shaders.terrain);
}

void ShadingManager::ReCompileShaders()
{
	logs.clear();
	START_PROFILER();
	PrepVertShader();
	vertexSource = vsh->GenerateGLSL();
	PrepGeomShader();
	geometrySource = gsh->GenerateGLSL();
	PrepFragShader();
	fragmentSource = fsh->GenerateGLSL();
	appState->shaders.terrain = new Shader(vertexSource, fragmentSource, geometrySource);
	double time = 0.0f;
	END_PROFILER(time);
	logs.push_back("Generated & Compiled Shaders in " + std::to_string(time) + "ms");
}

void ShadingManager::ShowSettings(bool *pOpen)
{
	ImGui::Begin("Shading##ShadingManager", pOpen);

	if(ImGui::Button("Compile Shaders"))
	{
		ReCompileShaders();
	}

	ImGui::SameLine();

	if(ImGui::Button("Export GLSL"))
	{
		std::string path = ShowSaveFileDialog("*.glsl\0");
		if(path.size() > 3)
		{
			SaveToFile(path, fragmentSource);
		}
	}

	if(ImGui::CollapsingHeader("Logs"))
	{
		if(logs.size() > 0)
		{
			ImGui::Text("Logs:");

			for(std::string &s : logs)
			{
				ImGui::Text(s.data());
			}
		}
	}


	if(ImGui::CollapsingHeader("Shader Nodes"))
	{
		shaderNodeEditor->Render();
	}

	ImGui::End();
}

void ShadingManager::PrepVertShader()
{
	vsh->Clear();
	vsh->AddTopLine(GLSLLine("layout (location = 0) in vec4 aPos;", "The world position of the vertex"));
	vsh->AddTopLine(GLSLLine("layout (location = 1) in vec4 aNorm;", "The vertex normals"));
	vsh->AddTopLine(GLSLLine("layout (location = 2) in vec2 aTexCoord;", "The texture coordinates of the vertex"));
	vsh->AddTopLine(GLSLLine("layout (location = 3) in vec4 aExtras1;", "Extra data like generated height, erosion delta, etc."));
	vsh->AddTopLine(GLSLLine("", "The Output passed to the Geometry Shader"));
	vsh->AddTopLine(GLSLLine(R"(
out DATA
{
	float height;
	vec3 FragPos;
	vec3 Normal;
	vec2 TexCoord;
} data_out;
)"));

	vsh->AddUniform(GLSLUniform("_Model", "mat4"));
	vsh->AddUniform(GLSLUniform("_PV", "mat4"));
	vsh->AddUniform(GLSLUniform("_TextureBake", "float", "0.0f"));
	vsh->AddUniform(GLSLUniform("_Scale", "float", "1.0f"));
	
	GLSLFunction main("main");
	main.AddLine(GLSLLine("", "TEMP"));
	main.AddLine(GLSLLine(R"(
if(_TextureBake == 0.0f)
	gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0f);
else
	gl_Position = vec4(aPos.x/_Scale, aPos.z/_Scale, 0.0f, 1.0f);
)"));
	main.AddLine(GLSLLine("data_out.height = aExtras1.x;", "The actual generated noise value"));
	main.AddLine(GLSLLine("data_out.FragPos = vec3(aPos.x, aPos.y, aPos.z);", "The world position"));
	main.AddLine(GLSLLine("data_out.Normal = vec3(aNorm.x, aNorm.y, aNorm.z);", "The vertex normals"));
	main.AddLine(GLSLLine("data_out.TexCoord = aTexCoord;", "The texture coordinates"));


	vsh->AddFunction(main);
}

void ShadingManager::PrepGeomShader()
{
	gsh->Clear();

	gsh->AddTopLine(GLSLLine("layout(triangles) in;", "Take 3 vertices of a triangles as input"));
	gsh->AddTopLine(GLSLLine("layout(triangle_strip, max_vertices = 3) out;"));
	gsh->AddTopLine(GLSLLine("", "The data passed in from the vertex shader"));
	gsh->AddTopLine(GLSLLine(R"(
in DATA
{
	float height;
	vec3 FragPos;
	vec3 Normal;
	vec2 TexCoord;
} data_in[];
)"));
	gsh->AddTopLine(GLSLLine(""));
	gsh->AddTopLine(GLSLLine("out float height;", "Pass the height to the fragment shader"));
	gsh->AddTopLine(GLSLLine("out vec4 FragPos;", "Pass the position to the fragment shader"));
	gsh->AddTopLine(GLSLLine("out vec3 Normal;", "Pass the normal to the fragment shader"));
	gsh->AddTopLine(GLSLLine("out vec2 TexCoord;", "Pass the texture coordinates to the fragment shader"));
	gsh->AddTopLine(GLSLLine("out mat3 TBN;", "Pass the Tangent Bitangent Normal matrix to the fragment shader"));

	gsh->AddUniform(GLSLUniform("_PV", "mat4"));
	gsh->AddUniform(GLSLUniform("_FlatShade", "float"));
	
	GLSLFunction main("main");
	main.AddLine(GLSLLine("vec3 a  = (_PV * gl_in[0].gl_Position).xyz;", "Vertex a of triangle"));
	main.AddLine(GLSLLine("vec3 b  = (_PV * gl_in[1].gl_Position).xyz;", "Vertex b of triangle"));
	main.AddLine(GLSLLine("vec3 c  = (_PV * gl_in[2].gl_Position).xyz;", "Vertex c of triangle"));
	main.AddLine(GLSLLine(""));
	main.AddLine(GLSLLine("vec3 edge0 = b - c;"));
	main.AddLine(GLSLLine("vec3 edge1 = c - a;"));
	main.AddLine(GLSLLine(""));
	main.AddLine(GLSLLine("vec2 deltaUV0 = data_in[1].TexCoord - data_in[0].TexCoord;"));
	main.AddLine(GLSLLine("vec2 deltaUV1 = data_in[2].TexCoord - data_in[0].TexCoord;"));
	main.AddLine(GLSLLine(""));
	main.AddLine(GLSLLine("float invDet = 1.0f / (deltaUV0.x * deltaUV1.y - deltaUV1.x * deltaUV0.y);", "One over the determinant"));
	main.AddLine(GLSLLine(""));
	main.AddLine(GLSLLine("vec3 tangent = vec3(invDet * (deltaUV1.y * edge0 - deltaUV0.y * edge1));"));
	main.AddLine(GLSLLine("vec3 bitangent = vec3(invDet * (-deltaUV1.x * edge0 + deltaUV0.x * edge1));"));
	main.AddLine(GLSLLine("mat4 model = mat4(1);", "A default model matrix"));
	main.AddLine(GLSLLine(""));
	main.AddLine(GLSLLine("vec3 T = normalize(vec3(model * vec4(tangent, 0.0f)));"));
	main.AddLine(GLSLLine("vec3 B = normalize(vec3(model * vec4(bitangent, 0.0f)));"));
	main.AddLine(GLSLLine("vec3 N = normalize(vec3(model * vec4(cross(edge1, edge0), 0.0f)));"));
	main.AddLine(GLSLLine(""));
	main.AddLine(GLSLLine("TBN = mat3(T, B, N);"));
	main.AddLine(GLSLLine(""));
	main.AddLine(GLSLLine("vec3 n = normalize(cross(edge1, edge0));", "Calculate face normal for flat shading"));
	main.AddLine(GLSLLine(""));

	for(int i=0;i<3;i++)
	{
		main.AddLine(GLSLLine("gl_Position = _PV * gl_in[" + std::to_string(i) + "].gl_Position;"));
		main.AddLine(GLSLLine("height = data_in[" + std::to_string(i) + "].height;"));
		main.AddLine(GLSLLine("TexCoord = data_in[" + std::to_string(i) + "].TexCoord;"));
		main.AddLine(GLSLLine("FragPos = vec4(data_in[" + std::to_string(i) + "].FragPos, 1.0f);"));
		main.AddLine(GLSLLine(R"(
		if(_FlatShade > 0.5f)
			Normal = n;
		else
			Normal = data_in[)" + std::to_string(i) + R"(].Normal;
		)"));
	main.AddLine(GLSLLine("EmitVertex();", "Emit the vertex"));
}

main.AddLine(GLSLLine("EndPrimitive();", "Emit the triangle"));


gsh->AddFunction(main);

}

void ShadingManager::PrepFragShader()
{
	fsh->Clear();
	fsh->AddTopLine(GLSLLine("out vec4 FragColor;", "The result of the fragment shader"));
	fsh->AddTopLine(GLSLLine(""));
	fsh->AddTopLine(GLSLLine("in float height;", "Take the height passed in fromt the Geometry Shader"));
	fsh->AddTopLine(GLSLLine("in vec4 FragPos;", "Take the world position passed in fromt the Geometry Shader"));
	fsh->AddTopLine(GLSLLine("in vec3 Normal;", "Take the normal passed in fromt the Geometry Shader"));
	fsh->AddTopLine(GLSLLine("in vec2 TexCoord;", "Take the texture coordinates passed in fromt the Geometry Shader"));
	fsh->AddTopLine(GLSLLine(""));
	fsh->AddTopLine(GLSLLine(R"(
	struct DataBlob
	{
		float d[32];
	};
	)"));
	fsh->AddUniform(GLSLUniform("_LightPosition", "vec3"));
	fsh->AddUniform(GLSLUniform("_LightColor", "vec3"));
	fsh->AddUniform(GLSLUniform("_HMapMinMax", "vec3"));
	fsh->AddUniform(GLSLUniform("_TextureBake", "float", "0.0f"));
	fsh->AddUniform(GLSLUniform("_Mode", "float", "0.0f"));
	GLSLSSBO dataBlobs("dataBlobs", std::to_string(sharedMemoryManager->ssboBinding), "These are small data blobs which may be used for nodes, etc.");
	dataBlobs.AddLine(GLSLLine("DataBlob data[];"));
	fsh->AddSSBO(dataBlobs);
	GLSLFunction main("main");
	main.AddLine(GLSLLine("vec3 objectColor = vec3(1.0f);"));
	main.AddLine(GLSLLine(""));
	main.AddLine(GLSLLine(R"(
	if(_TextureBake == 1.0f)
	{
		if(_Mode == 0.0f)
			objectColor = vec3(1.0);
		else if(_Mode == 1.0f)
			objectColor = vec3( (height - _HMapMinMax.x) / (_HMapMinMax.y - _HMapMinMax.x) );
		else if(_Mode == 2.0f)
			objectColor = Normal;
		FragColor = vec4(objectColor, 1.0f);
		return;
	}	
	)"));
	main.AddLine(GLSLLine(""));
	GLSLLine tmp("", "");
	NodeInputParam param;
	param.userData1 = &main;
	param.userData2 = &tmp;
	shaderNodeEditor->outputNode->Evaluate(param, nullptr);
	main.AddLine(GLSLLine("FragColor = vec4(" + tmp.line + ", 1.0f);"));
	fsh->AddFunction(main);
}

