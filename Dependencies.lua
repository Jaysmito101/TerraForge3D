---------------------------------------------------------------
-- This file belongs to the TerraForge3D project.
--
-- Maintainers:
--   - Jaysmito Mukherjee
--
-- This is under MIT license.
---------------------------------------------------------------

-- This file is to declare global varibles

-- The path to thte Vulkan SDK (please run setup if this doesnt work)
VULKAN_SDK = os.getenv("VULKAN_SDK")

-- The list of dependencies and other include directories
IncludeDirectories = {}
-- TODO: Add the dependencies here
IncludeDirectories["TerraForge3DLib"] = "./TerraForge3D/TerraForge3DLib"
IncludeDirectories["TerraForge3D"] = "./TerraForge3D/TerraForge3D"
IncludeDirectories["VulkanSDK"] = "%{VULKAN_SDK}/Include"

-- The list of libraries and other library directories
LibraryDirectories = {}

LibraryDirectories["VulkanSDK"] = "%{VULKAN_SDK}/Lib"

-- The list of libraries
Libraries = {}

Libraries["Vulkan"] = "%{LibraryDirectories.VulkanSDK}/vulkan-1.lib"
Libraries["VulkanUtils"] = "%{LibraryDirectories.VulkanSDK}/vulkan_utils.lib"

OutputDir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"