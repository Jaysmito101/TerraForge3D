#include "GPUErosionFilter.h"
#include <imgui.h>
#include <iostream>

#include <ComputeShader.h>
#include <ShaderStorageBuffer.h>

#include <Utils.h>

#include <string>

#define MAX(x, y) x>y?x:y

std::string shadersrc = R"(

#version 430
layout(local_size_x = 1, local_size_y = 1) in;
uniform image2D img_output;
void main(){
// base pixel colour for image
  vec4 pixel = vec4(0.0, 0.0, 0.0, 1.0);
  // get index in global work group i.e x,y position
  ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);
  
  //
  // interesting stuff happens here later
  //
  
  // output to a specific pixel in the image
  imageStore(img_output, pixel_coords, pixel);
}

)";

static Texture2D* tex;
void GPUErosionFilter::Render()
{
	ImGui::DragInt("Num Iterations##GPUErosionFilter", &iterations, 1, 0);
	ImGui::DragFloat("Actual Dimensions##GPUErosionFilter", &dimension, 1, 0);
	ImGui::DragFloat("Strength##GPUErosionFilter", &tfac, 1000);

	ImGui::Image((ImTextureID)tex->GetRendererID(), ImVec2(200, 200));

}

nlohmann::json GPUErosionFilter::Save()
{
	return nlohmann::json();
}

void GPUErosionFilter::Load(nlohmann::json data)
{
}

void GPUErosionFilter::Apply()
{
	model->mesh->RecalculateNormals();



	model->mesh->RecalculateNormals();
	model->UploadToGPU();
}

void GPUErosionFilter::OnAttach()
{
	cCh = new ComputeShader(shadersrc);;
	tex = new Texture2D(GetExecutableDir() + "\\Data\\textures\\white.png");
}
