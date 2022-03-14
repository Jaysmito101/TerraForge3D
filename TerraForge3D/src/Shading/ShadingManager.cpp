#include "Shading/ShadingManager.h"
#include "Utils/Utils.h"
#include "Data/ApplicationState.h"

#include "imgui/imgui.h"

ShadingManager::ShadingManager(ApplicationState* as)
{
	appState = as;
	vsh = new GLSLHandler("Primary Vertex Shader");
	gsh = new GLSLHandler("Primary Geometry Shader");
	fsh = new GLSLHandler("Primary Fragment Shader");

	PrepVertShader();
}

ShadingManager::~ShadingManager()
{
	delete vsh;
	delete gsh;
	delete fsh;
}


void ShadingManager::ReCompileShaders()
{
	PrepVertShader();
	vetexSource = vsh->GenerateGLSL();
}

void ShadingManager::ShowSettings(bool* pOpen)
{
	ImGui::Begin("Shading##ShadingManager", pOpen);

	if(ImGui::Button("Compile Shaders"))
		ReCompileShaders();

	ImGui::Text("VS : \n%s", vetexSource.data());

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
} data_out
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