#include "Shading/ShadingManager.h"
#include "Utils/Utils.h"
#include "Data/ApplicationState.h"
#include "Profiler.h"
#include "Platform.h"
#include "imgui/imgui.h"
// #include "glsl_optimizer.h"

#include "Shading/ShaderNodes/ShaderOutputNode.h"
#include "Shading/ShaderNodes/ShaderTextureNode.h"
#include "Shading/ShaderNodes/Float3Node.h"
#include "Shading/ShaderNodes/FloatNode.h"
#include "Shading/ShaderNodes/PBRMaterialNode.h"
#include "Shading/ShaderNodes/CustomShaderNode.h"


#include <filesystem>
namespace fs = std::filesystem;

static char *stristr4(const char *str, const char *pattern)
{
	size_t i;

	if (!*pattern)
	{
		return (char *)str;
	}

	for (; *str; str++)
	{
		if (toupper((unsigned char)*str) == toupper((unsigned char)*pattern))
		{
			for (i = 1;; i++)
			{
				if (!pattern[i])
				{
					return (char *)str;
				}

				if (toupper((unsigned char)str[i]) != toupper((unsigned char)pattern[i]))
				{
					break;
				}
			}
		}
	}

	return NULL;
}


#define NODE_MAKER_COND(x) length == 0 || stristr4(x, data)
#define NODE_MAKER_SHOW(x, y, z) if (NODE_MAKER_COND(y)) {if (ImGui::Button((y + std::string("##SHADERNODEMAKER")).c_str())) { editor->AddNode(new x(handler)); z = z || true; ImGui::CloseCurrentPopup(); } }


static bool ShowNodeMaker(NodeEditor *editor, GLSLHandler *handler, ShaderTextureManager *shaderTextureManager, std::vector<DefaultCustomNode> &defaultCustomNodes)
{
	static char data[1000];
	static bool didMake = false;
	didMake = false;
	ImGui::InputTextWithHint("##SearchShaderNodes", "Search ...", data, sizeof(data));
	int length = strlen(data);
	ImGui::BeginChild("##ShaderNodesMaker", ImVec2(200, 250));

	NODE_MAKER_SHOW(Float3Node, "Float3", didMake);

	NODE_MAKER_SHOW(FloatNode, "Float Value", didMake);

	if (NODE_MAKER_COND("Texture"))
	{
		if (ImGui::Button(("Texture" + std::string("##SHADERNODEMAKER")).c_str()))
		{
			editor->AddNode(new ShaderTextureNode(handler, shaderTextureManager));
			didMake = didMake || true;
			ImGui::CloseCurrentPopup();
		}
	}

	NODE_MAKER_SHOW(PBRMaterialNode, "PBR Material", didMake);

	for (auto &node : defaultCustomNodes)
	{
		if (NODE_MAKER_COND(node.name.data()))
		{
			if (ImGui::Button((node.name + std::string("##SHADERNODEMAKER")).c_str()))
			{
				editor->AddNode(new CustomShaderNode(handler, node.content));
				didMake = didMake || true;
				ImGui::CloseCurrentPopup();
			}
		}
	}

	if (NODE_MAKER_COND("Custom Shader"))
	{
		if (ImGui::Button(("Custom Shader" + std::string("##SHADERNODEMAKER")).c_str()))
		{
			std::string path = ShowOpenFileDialog("*.json\0");

			if(path.size() > 3)
			{
				bool tmp = false;
				editor->AddNode(new CustomShaderNode(handler, ReadShaderSourceFile(path, &tmp)));
				didMake = didMake || true;
				ImGui::CloseCurrentPopup();
			}
		}
	}

	ImGui::EndChild();
	return didMake;
}


ShadingManager::ShadingManager(ApplicationState *as)
{
	appState = as;
	vsh = new GLSLHandler("Primary Vertex Shader");
	gsh = new GLSLHandler("Primary Geometry Shader");
	fsh = new GLSLHandler("Primary Fragment Shader");
	sharedMemoryManager = new SharedMemoryManager();
	shaderTextureManager = new ShaderTextureManager();
	NodeEditorConfig config;
	config.makeNodeFunc = [&]()
	{
		ImGuiNodeEditor::Suspend();
		ImGui::OpenPopup("NodeMakerDropped");
		ImGuiNodeEditor::Resume();
	};
	config.insNodeFunc = [&](nlohmann::json data) -> NodeEditorNode*
	{
		SNENode *node = nullptr;

		if(data["type"] == "ShaderOutput")
		{
			node = new ShaderOutputNode(fsh);
		}

		else if(data["type"] == "Float3")
		{
			node = new Float3Node(fsh);
		}

		else if(data["type"] == "Float")
		{
			node = new FloatNode(fsh);
		}

		else if(data["type"] == "ShaderTexture")
		{
			node = new ShaderTextureNode(fsh, shaderTextureManager);
		}

		else if(data["type"] == "PBRMaterial")
		{
			node = new PBRMaterialNode(fsh);
		}

		else if(data["type"] == "CustomShader")
		{
			node = new CustomShaderNode(fsh, data["shader"]);
		}

		return node;
	};
	config.saveFile = GetExecutableDir() + PATH_SEPERATOR + "Data" + PATH_SEPERATOR + "configs" + PATH_SEPERATOR + "ShaderNodes.json";
	shaderNodeEditor = new NodeEditor(config);
	shaderNodeEditor->name = "Shader Nodes";
	shaderNodeEditor->SetOutputNode(new ShaderOutputNode(fsh));
	LoadDefaultCustomNodes();
	ReCompileShaders();
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
	// SNENode* outputNode = static_cast<SNENode*>(shaderNodeEditor->outputNode);
	// outputNode->UpdateShaders();
	// for(auto& it : shaderNodeEditor->nodes)
	// {
	// 	SNENode* node = static_cast<SNENode*>(it.second);
	// 	node->UpdateShaders();
	// }
	sharedMemoryManager->UpdateShader(appState->shaders.terrain);
	shaderTextureManager->Bind(7);
	appState->shaders.terrain->SetUniformi("_Textures", 7);
}

void ShadingManager::LoadDefaultCustomNodes()
{
	std::string nodesDir = GetExecutableDir() + PATH_SEPERATOR + "Data" + PATH_SEPERATOR + "shader_nodes";
	defaultCustomNodes.clear();
	bool tmp = false;

	for (const auto &entry : fs::directory_iterator(nodesDir))
	{
		if(entry.path().extension().string() != ".json")	
			continue;
		DefaultCustomNode n;
		n.name = entry.path().filename().string();
		n.name = n.name.substr(0, n.name.find_last_of("."));
		n.content = ReadShaderSourceFile(entry.path().string(), &tmp);
		defaultCustomNodes.push_back(n);
	}

	extraSource = ReadShaderSourceFile(nodesDir + PATH_SEPERATOR + "extras.glsl", &tmp);
	
}

void ShadingManager::ReCompileShaders()
{
	logs.clear();
	START_PROFILER();
	sharedMemoryManager->Clear();
	SNENode *outputNode = static_cast<SNENode *>(shaderNodeEditor->outputNode);
	outputNode->dataBlobOffset = sharedMemoryManager->AddItem();

	for(auto &it : shaderNodeEditor->nodes)
	{
		SNENode *node = static_cast<SNENode *>(it.second);
		node->dataBlobOffset = sharedMemoryManager->AddItem();
	}

	outputNode->sharedData = sharedMemoryManager->At(outputNode->dataBlobOffset);
	outputNode->UpdateShaders();

	for(auto &it : shaderNodeEditor->nodes)
	{
		SNENode *node = static_cast<SNENode *>(it.second);
		node->sharedData = sharedMemoryManager->At(node->dataBlobOffset);
		node->UpdateShaders();
	}

	shaderTextureManager->UpdateShaders();
	PrepVertShader();
	vertexSource = vsh->GenerateGLSL();
	PrepGeomShader();
	geometrySource = gsh->GenerateGLSL();
	PrepFragShader();
	fragmentSource = fsh->GenerateGLSL();

	if(optimizeGLSL)
	{
		// glslopt_ctx* ctx = glslopt_initialize(glslopt_target::kGlslTargetOpenGL);
		// glslopt_shader* shader = glslopt_optimize(ctx, glslopt_shader_type::kGlslOptShaderFragment, fragmentSource.data(), 0);
		// if (glslopt_get_status (shader)) 
		// {
		// 	fragmentSource = std::string(glslopt_get_output(shader));
		// }
		// else
		// {
		// 	logs.push_back(std::string(glslopt_get_log (shader)));
		// }
		// glslopt_shader_delete (shader);
		// glslopt_cleanup (ctx);
	}

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

	if(ImGui::Button("Print GLSL"))
	{
		std::cout << fragmentSource << std::endl;
	}

	ImGui::SameLine();

	ImGui::SameLine();

	if(ImGui::Checkbox("Optimize GLSL", &optimizeGLSL))
	{
		ReCompileShaders();
	}

	//ImGui::Text("SMM Size : %d", sharedMemoryManager->sharedMemoryBlobs.size());
	//ImGui::Text("SMM : %f, %f, %f", sharedMemoryManager->sharedMemoryBlobs[0].d0, sharedMemoryManager->sharedMemoryBlobs[0].d1, sharedMemoryManager->sharedMemoryBlobs[0].d2);

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
		if (ImGui::Button("Add Node##ShaderNodeMaker"))
		{
			ImGui::OpenPopup("NodeMakerDropped");
		}

		ImGui::SameLine();

		if (ImGui::Button("Reset##ShaderNodeMaker"))
		{
			shaderNodeEditor->Reset();
			ReCompileShaders();
		}

		ImGui::SameLine();

		if (ImGui::Button("Export##ShaderNodeMaker"))
		{
			std::string file = ShowSaveFileDialog("*.terr3d");

			if (file.size() > 3)
			{
				if (file.find(".terr3d") == std::string::npos)
				{
					file += ".terr3d";
				}

				SaveToFile(file, shaderNodeEditor->Save().dump(4));
			}
		}

		ImGui::SameLine();

		if (ImGui::Button("Import##CPUNE"))
		{
			std::string file = ShowOpenFileDialog("*.terr3d");

			if (file.size() > 3)
			{
				bool tmp = false;
				shaderNodeEditor->Reset();
				shaderNodeEditor->Load(nlohmann::json::parse(ReadShaderSourceFile(file, &tmp)));
				ReCompileShaders();
			}
		}

		ImGui::SameLine();

		if(ImGui::Button("Reload Nodes"))
		{
			LoadDefaultCustomNodes();
		}

		int nC = shaderNodeEditor->nodes.size();
		int lC = shaderNodeEditor->links.size();
		int pC = shaderNodeEditor->pins.size();
		shaderNodeEditor->Render();

		if ((nC != shaderNodeEditor->nodes.size()) || (lC != shaderNodeEditor->links.size()) || (pC != shaderNodeEditor->pins.size()))
		{
			ReCompileShaders();
		}
	}

	if (ImGui::IsWindowFocused() && (((IsKeyDown(TERR3D_KEY_RIGHT_SHIFT) || IsKeyDown(TERR3D_KEY_LEFT_SHIFT)) && IsKeyDown(TERR3D_KEY_A)) || IsMouseButtonDown(TERR3D_MOUSE_BUTTON_MIDDLE)))
	{
		ImGui::OpenPopup("NodeMakerDropped");
	}

	if(ImGui::BeginPopup("NodeMakerDropped"))
	{
		if(ShowNodeMaker(shaderNodeEditor, fsh, shaderTextureManager, defaultCustomNodes))
		{
			ReCompileShaders();
		}

		if (ImGui::Button("Close##ShaderNodeMaker"))
		{
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
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
	main.AddLine(GLSLLine("vec3 N = normalize(vec3(model * vec4(-1.0f*cross(edge1, edge0), 0.0f)));"));
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
	fsh->AddTopLine(GLSLLine("in float height;", "Take the height passed in from the Geometry Shader"));
	fsh->AddTopLine(GLSLLine("in vec4 FragPos;", "Take the world position passed in from the Geometry Shader"));
	fsh->AddTopLine(GLSLLine("in vec3 Normal;", "Take the normal passed in from the Geometry Shader"));
	fsh->AddTopLine(GLSLLine("in vec2 TexCoord;", "Take the texture coordinates passed in from the Geometry Shader"));
	fsh->AddTopLine(GLSLLine("in mat3 TBN;", "Take the tangent bitangent normal matrix in from the Geometry Shader"));
	fsh->AddTopLine(GLSLLine(""));
	fsh->AddTopLine(GLSLLine(extraSource));
	fsh->AddTopLine(GLSLLine(""));
	fsh->AddTopLine(GLSLLine(R"(
	struct DataBlob
	{
		float d[32];
	};
	)"));
	fsh->AddTopLine(GLSLLine("const float PI = 3.14159265359;"));
	fsh->AddTopLine(GLSLLine("const float gamma = 2.2f;"));
	fsh->AddUniform(GLSLUniform("_LightPosition", "vec3"));
	fsh->AddUniform(GLSLUniform("_LightColor", "vec3"));
	fsh->AddUniform(GLSLUniform("_LightStrength", "float"));
	fsh->AddUniform(GLSLUniform("_CameraPos", "vec3"));
	fsh->AddUniform(GLSLUniform("_HMapMinMax", "vec3"));
	fsh->AddUniform(GLSLUniform("_TextureBake", "float", "0.0f"));
	fsh->AddUniform(GLSLUniform("_Mode", "float", "0.0f"));
	fsh->AddUniform(GLSLUniform("_Textures", "sampler2DArray"));
	GLSLSSBO dataBlobs("dataBlobs", std::to_string(sharedMemoryManager->ssboBinding), "These are small data blobs which may be used for nodes, etc.");
	dataBlobs.AddLine(GLSLLine("DataBlob data[];"));
	fsh->AddSSBO(dataBlobs);
	GLSLFunction main("main");
	main.AddLine(GLSLLine("vec3 objectColor = vec3(1.0f);"));
	main.AddLine(GLSLLine(""));
	GLSLLine tmp("", "");
	NodeInputParam param;
	param.userData1 = &main;
	param.userData2 = &tmp;
	shaderNodeEditor->outputNode->Evaluate(param, nullptr);
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
	main.AddLine(GLSLLine("FragColor = vec4(" + tmp.line + ", 1.0f);"));
	fsh->AddFunction(main);
}

