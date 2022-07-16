cmake_minimum_required(VERSION 3.12)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(TerraForge3D)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REUIRED True)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REUIRED True)

add_subdirectory(TerraForge3D/Vendor/Glad)
add_subdirectory(TerraForge3D/Vendor/GLFW)
add_subdirectory(TerraForge3D/Vendor/ImGui)
add_subdirectory(TerraForge3D/Vendor/SPDLog)
add_subdirectory(TerraForge3D/Vendor/ImGuiFileDialog)
add_subdirectory(TerraForge3D/TerraForge3DLib)
