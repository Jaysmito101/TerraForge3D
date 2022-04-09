---------------------------------------------------------------
-- This file belongs to the TerraForge3D project.
--
-- Maintainers:
--   - Jaysmito Mukherjee
--
-- This is under MIT license.
---------------------------------------------------------------

-- The required global variables
include "./Dependencies.lua"

workspace "TerraForge3D"
    -- TerraForge3D is a 64-bit application only
    architecture "x86_64"
    -- The main application porject
    startproject "TerraForge3D"

    -- The build configurations
    configurations 
    {
        "DebugVkCompute",
        "DebugOpenCL",
        "ReleaseVkCompute",
        "ReleaseOpenCL"
    }

    -- Enable Milti-Processor Build
    flags
    {
        "MultiProcessorCompile"
    }
    
    -- Include TerraForge3DLib
    include "TerraForge3D/TerraForge3DLib"
    -- Include TerraForge3D Main Application
    include "TerraForge3D/TerraForge3D"

    -- Include the Dependencies
    group "Dependencies"
    -- TODO: Add the dependencies here
    group ""
