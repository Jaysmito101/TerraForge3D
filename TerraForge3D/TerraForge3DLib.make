# Alternative GNU Make project makefile autogenerated by Premake

ifndef config
  config=debug
endif

ifndef verbose
  SILENT = @
endif

.PHONY: clean prebuild

SHELLTYPE := posix
ifeq (.exe,$(findstring .exe,$(ComSpec)))
	SHELLTYPE := msdos
endif

# Configurations
# #############################################

RESCOMP = windres
INCLUDES += -Ivendor/assimp/include -Iinclude -Iinclude/Utils -Iinclude/Base -Ivendor -Ivendor/openssl -Ivendor/glfw/include -Ivendor/glad/include -Ivendor/glm -Ivendor/zip/src -Ivendor/imgui -Ivendor/implot -Ivendor/lua -Ivendor/muparser/include -Ivendor/json -Ivendor/FastNoiseLite -Ivendor/imgui-node-editor
FORCE_INCLUDE +=
ALL_CPPFLAGS += $(CPPFLAGS) -MMD -MP $(DEFINES) $(INCLUDES)
ALL_RESFLAGS += $(RESFLAGS) $(DEFINES) $(INCLUDES)
LINKCMD = $(AR) -rcs "$@" $(OBJECTS)
define PREBUILDCMDS
endef
define PRELINKCMDS
endef
define POSTBUILDCMDS
endef

ifeq ($(config),debug)
TARGETDIR = ../bin/Debug-linux-x86_64
TARGET = $(TARGETDIR)/libTerraForge3DLib.a
OBJDIR = ../bin/intermediates/Debug-linux-x86_64/TerraForge3DLib
DEFINES += -DNULL=0 -DTERR3D_DEBUG
ALL_CFLAGS += $(CFLAGS) $(ALL_CPPFLAGS) -m64 -g -msse4.1
ALL_CXXFLAGS += $(CXXFLAGS) $(ALL_CPPFLAGS) -m64 -g -msse4.1 -std=c++17
LIBS += vendor/glfw/bin/Debug-linux-x86_64/GLFW/libGLFW.a vendor/glad/bin/Debug-linux-x86_64/Glad/libGlad.a vendor/zip/bin/Debug-linux-x86_64/Zip/libZip.a vendor/imgui/bin/Debug-linux-x86_64/ImGui/libImGui.a vendor/imnodes/bin/Debug-linux-x86_64/ImNodes/libImNodes.a vendor/implot/bin/Debug-linux-x86_64/ImPlot/libImPlot.a vendor/imgui-node-editor/bin/Debug-linux-x86_64/ImGuiNodeEditor/libImGuiNodeEditor.a vendor/text-editor/bin/Debug-linux-x86_64/ImColorTextEdit/libImColorTextEdit.a vendor/lua/bin/Debug-linux-x86_64/Lua/libLua.a vendor/muparser/bin/Debug-linux-x86_64/MuParser/libMuParser.a vendor/assimp/bin/Debug-linux-x86_64/libAssimp.a -lUrlmon.lib -lCrypt32 -lws2_32 -lPathcch -lopengl32 -lOpenCL -llibcryptoMTd.lib -llibsslMTd.lib
LDDEPS += vendor/glfw/bin/Debug-linux-x86_64/GLFW/libGLFW.a vendor/glad/bin/Debug-linux-x86_64/Glad/libGlad.a vendor/zip/bin/Debug-linux-x86_64/Zip/libZip.a vendor/imgui/bin/Debug-linux-x86_64/ImGui/libImGui.a vendor/imnodes/bin/Debug-linux-x86_64/ImNodes/libImNodes.a vendor/implot/bin/Debug-linux-x86_64/ImPlot/libImPlot.a vendor/imgui-node-editor/bin/Debug-linux-x86_64/ImGuiNodeEditor/libImGuiNodeEditor.a vendor/text-editor/bin/Debug-linux-x86_64/ImColorTextEdit/libImColorTextEdit.a vendor/lua/bin/Debug-linux-x86_64/Lua/libLua.a vendor/muparser/bin/Debug-linux-x86_64/MuParser/libMuParser.a vendor/assimp/bin/Debug-linux-x86_64/libAssimp.a
ALL_LDFLAGS += $(LDFLAGS) -L../libs/win32/x64 -L/usr/lib64 -m64

else ifeq ($(config),release)
TARGETDIR = ../bin/Release-linux-x86_64
TARGET = $(TARGETDIR)/libTerraForge3DLib.a
OBJDIR = ../bin/intermediates/Release-linux-x86_64/TerraForge3DLib
DEFINES += -DNULL=0 -DTERR3D_RELEASE
ALL_CFLAGS += $(CFLAGS) $(ALL_CPPFLAGS) -m64 -O3 -msse4.1 /Qpar /fp:fast
ALL_CXXFLAGS += $(CXXFLAGS) $(ALL_CPPFLAGS) -m64 -O3 -msse4.1 -std=c++17 /Qpar /fp:fast
LIBS += vendor/glfw/bin/Release-linux-x86_64/GLFW/libGLFW.a vendor/glad/bin/Release-linux-x86_64/Glad/libGlad.a vendor/zip/bin/Release-linux-x86_64/Zip/libZip.a vendor/imgui/bin/Release-linux-x86_64/ImGui/libImGui.a vendor/imnodes/bin/Release-linux-x86_64/ImNodes/libImNodes.a vendor/implot/bin/Release-linux-x86_64/ImPlot/libImPlot.a vendor/imgui-node-editor/bin/Release-linux-x86_64/ImGuiNodeEditor/libImGuiNodeEditor.a vendor/text-editor/bin/Release-linux-x86_64/ImColorTextEdit/libImColorTextEdit.a vendor/lua/bin/Release-linux-x86_64/Lua/libLua.a vendor/muparser/bin/Release-linux-x86_64/MuParser/libMuParser.a vendor/assimp/bin/Release-linux-x86_64/libAssimp.a -lUrlmon.lib -lCrypt32 -lws2_32 -lPathcch -lopengl32 -lOpenCL -llibcryptoMT.lib -llibsslMT.lib
LDDEPS += vendor/glfw/bin/Release-linux-x86_64/GLFW/libGLFW.a vendor/glad/bin/Release-linux-x86_64/Glad/libGlad.a vendor/zip/bin/Release-linux-x86_64/Zip/libZip.a vendor/imgui/bin/Release-linux-x86_64/ImGui/libImGui.a vendor/imnodes/bin/Release-linux-x86_64/ImNodes/libImNodes.a vendor/implot/bin/Release-linux-x86_64/ImPlot/libImPlot.a vendor/imgui-node-editor/bin/Release-linux-x86_64/ImGuiNodeEditor/libImGuiNodeEditor.a vendor/text-editor/bin/Release-linux-x86_64/ImColorTextEdit/libImColorTextEdit.a vendor/lua/bin/Release-linux-x86_64/Lua/libLua.a vendor/muparser/bin/Release-linux-x86_64/MuParser/libMuParser.a vendor/assimp/bin/Release-linux-x86_64/libAssimp.a
ALL_LDFLAGS += $(LDFLAGS) -L../libs/win32/x64 -L/usr/lib64 -m64 -s

endif

# Per File Configurations
# #############################################


# File sets
# #############################################

GENERATED :=
OBJECTS :=

GENERATED += $(OBJDIR)/AbsNode.o
GENERATED += $(OBJDIR)/AddNode.o
GENERATED += $(OBJDIR)/AdvancedErosionFilter.o
GENERATED += $(OBJDIR)/AppStyles.o
GENERATED += $(OBJDIR)/Application.o
GENERATED += $(OBJDIR)/ApplicationState.o
GENERATED += $(OBJDIR)/BlendNode.o
GENERATED += $(OBJDIR)/CPUNodeEditor.o
GENERATED += $(OBJDIR)/CPUNoiseLayersGenerator.o
GENERATED += $(OBJDIR)/Camera.o
GENERATED += $(OBJDIR)/ClampNode.o
GENERATED += $(OBJDIR)/ClearMeshGenerator.o
GENERATED += $(OBJDIR)/ComputeKernel.o
GENERATED += $(OBJDIR)/ComputeShader.o
GENERATED += $(OBJDIR)/CosNode.o
GENERATED += $(OBJDIR)/CubeMap.o
GENERATED += $(OBJDIR)/CurveNode.o
GENERATED += $(OBJDIR)/DivNode.o
GENERATED += $(OBJDIR)/DrawFilter.o
GENERATED += $(OBJDIR)/DummyNode.o
GENERATED += $(OBJDIR)/DuplicateNode.o
GENERATED += $(OBJDIR)/ErosionFilter.o
GENERATED += $(OBJDIR)/ExplorerControls.o
GENERATED += $(OBJDIR)/ExportManager.o
GENERATED += $(OBJDIR)/ExportTexture.o
GENERATED += $(OBJDIR)/FiltersManager.o
GENERATED += $(OBJDIR)/FoliagePlacement.o
GENERATED += $(OBJDIR)/FrameBuffer.o
GENERATED += $(OBJDIR)/GPUErosionFilter.o
GENERATED += $(OBJDIR)/GPUNoiseLayerGenerator.o
GENERATED += $(OBJDIR)/GeneratorMask.o
GENERATED += $(OBJDIR)/HillNode.o
GENERATED += $(OBJDIR)/ImGuiCurveEditor.o
GENERATED += $(OBJDIR)/ImGuiImplGLFW.o
GENERATED += $(OBJDIR)/ImGuiImplOpenGL.o
GENERATED += $(OBJDIR)/ImGuiShapes.o
GENERATED += $(OBJDIR)/LayeredNoiseManager.o
GENERATED += $(OBJDIR)/LightManager.o
GENERATED += $(OBJDIR)/LogHandler.o
GENERATED += $(OBJDIR)/Logger.o
GENERATED += $(OBJDIR)/MainMenu.o
GENERATED += $(OBJDIR)/MathFunctionNode.o
GENERATED += $(OBJDIR)/MaxMeshCoordinatesNode.o
GENERATED += $(OBJDIR)/Mesh.o
GENERATED += $(OBJDIR)/MeshCoordinatesNode.o
GENERATED += $(OBJDIR)/MeshGeneratorManager.o
GENERATED += $(OBJDIR)/MinMeshCoordinatesNode.o
GENERATED += $(OBJDIR)/MinValNode.o
GENERATED += $(OBJDIR)/Model.o
GENERATED += $(OBJDIR)/ModelImporter.o
GENERATED += $(OBJDIR)/Module.o
GENERATED += $(OBJDIR)/ModuleManager.o
GENERATED += $(OBJDIR)/MulNode.o
GENERATED += $(OBJDIR)/NodeEditor.o
GENERATED += $(OBJDIR)/NoiseCellularNode.o
GENERATED += $(OBJDIR)/NoiseLayer.o
GENERATED += $(OBJDIR)/NoiseOpenSimplex2Node.o
GENERATED += $(OBJDIR)/NoiseOpenSimplex2SNode.o
GENERATED += $(OBJDIR)/NoisePerlinNode.o
GENERATED += $(OBJDIR)/NoiseValueCubicNode.o
GENERATED += $(OBJDIR)/NoiseValueNode.o
GENERATED += $(OBJDIR)/OSLiscences.o
GENERATED += $(OBJDIR)/OutputNode.o
GENERATED += $(OBJDIR)/PixelateNode.o
GENERATED += $(OBJDIR)/ProjectData.o
GENERATED += $(OBJDIR)/QuickHack.o
GENERATED += $(OBJDIR)/RandomNumberNode.o
GENERATED += $(OBJDIR)/Renderer.o
GENERATED += $(OBJDIR)/SeaManager.o
GENERATED += $(OBJDIR)/Serializer.o
GENERATED += $(OBJDIR)/Shader.o
GENERATED += $(OBJDIR)/ShaderStorageBuffer.o
GENERATED += $(OBJDIR)/ShaderUtils.o
GENERATED += $(OBJDIR)/SinNode.o
GENERATED += $(OBJDIR)/SkySettings.o
GENERATED += $(OBJDIR)/SplashScreen.o
GENERATED += $(OBJDIR)/SquareNode.o
GENERATED += $(OBJDIR)/SubNode.o
GENERATED += $(OBJDIR)/SupportersTribute.o
GENERATED += $(OBJDIR)/TanNode.o
GENERATED += $(OBJDIR)/Texture2D.o
GENERATED += $(OBJDIR)/TextureCoordinatesNode.o
GENERATED += $(OBJDIR)/TextureCubemap.o
GENERATED += $(OBJDIR)/TextureNode.o
GENERATED += $(OBJDIR)/TextureStore.o
GENERATED += $(OBJDIR)/TimeBasedSeedNode.o
GENERATED += $(OBJDIR)/UIFontManager.o
GENERATED += $(OBJDIR)/Utils.o
GENERATED += $(OBJDIR)/VisualizerNode.o
GENERATED += $(OBJDIR)/Window.o
OBJECTS += $(OBJDIR)/AbsNode.o
OBJECTS += $(OBJDIR)/AddNode.o
OBJECTS += $(OBJDIR)/AdvancedErosionFilter.o
OBJECTS += $(OBJDIR)/AppStyles.o
OBJECTS += $(OBJDIR)/Application.o
OBJECTS += $(OBJDIR)/ApplicationState.o
OBJECTS += $(OBJDIR)/BlendNode.o
OBJECTS += $(OBJDIR)/CPUNodeEditor.o
OBJECTS += $(OBJDIR)/CPUNoiseLayersGenerator.o
OBJECTS += $(OBJDIR)/Camera.o
OBJECTS += $(OBJDIR)/ClampNode.o
OBJECTS += $(OBJDIR)/ClearMeshGenerator.o
OBJECTS += $(OBJDIR)/ComputeKernel.o
OBJECTS += $(OBJDIR)/ComputeShader.o
OBJECTS += $(OBJDIR)/CosNode.o
OBJECTS += $(OBJDIR)/CubeMap.o
OBJECTS += $(OBJDIR)/CurveNode.o
OBJECTS += $(OBJDIR)/DivNode.o
OBJECTS += $(OBJDIR)/DrawFilter.o
OBJECTS += $(OBJDIR)/DummyNode.o
OBJECTS += $(OBJDIR)/DuplicateNode.o
OBJECTS += $(OBJDIR)/ErosionFilter.o
OBJECTS += $(OBJDIR)/ExplorerControls.o
OBJECTS += $(OBJDIR)/ExportManager.o
OBJECTS += $(OBJDIR)/ExportTexture.o
OBJECTS += $(OBJDIR)/FiltersManager.o
OBJECTS += $(OBJDIR)/FoliagePlacement.o
OBJECTS += $(OBJDIR)/FrameBuffer.o
OBJECTS += $(OBJDIR)/GPUErosionFilter.o
OBJECTS += $(OBJDIR)/GPUNoiseLayerGenerator.o
OBJECTS += $(OBJDIR)/GeneratorMask.o
OBJECTS += $(OBJDIR)/HillNode.o
OBJECTS += $(OBJDIR)/ImGuiCurveEditor.o
OBJECTS += $(OBJDIR)/ImGuiImplGLFW.o
OBJECTS += $(OBJDIR)/ImGuiImplOpenGL.o
OBJECTS += $(OBJDIR)/ImGuiShapes.o
OBJECTS += $(OBJDIR)/LayeredNoiseManager.o
OBJECTS += $(OBJDIR)/LightManager.o
OBJECTS += $(OBJDIR)/LogHandler.o
OBJECTS += $(OBJDIR)/Logger.o
OBJECTS += $(OBJDIR)/MainMenu.o
OBJECTS += $(OBJDIR)/MathFunctionNode.o
OBJECTS += $(OBJDIR)/MaxMeshCoordinatesNode.o
OBJECTS += $(OBJDIR)/Mesh.o
OBJECTS += $(OBJDIR)/MeshCoordinatesNode.o
OBJECTS += $(OBJDIR)/MeshGeneratorManager.o
OBJECTS += $(OBJDIR)/MinMeshCoordinatesNode.o
OBJECTS += $(OBJDIR)/MinValNode.o
OBJECTS += $(OBJDIR)/Model.o
OBJECTS += $(OBJDIR)/ModelImporter.o
OBJECTS += $(OBJDIR)/Module.o
OBJECTS += $(OBJDIR)/ModuleManager.o
OBJECTS += $(OBJDIR)/MulNode.o
OBJECTS += $(OBJDIR)/NodeEditor.o
OBJECTS += $(OBJDIR)/NoiseCellularNode.o
OBJECTS += $(OBJDIR)/NoiseLayer.o
OBJECTS += $(OBJDIR)/NoiseOpenSimplex2Node.o
OBJECTS += $(OBJDIR)/NoiseOpenSimplex2SNode.o
OBJECTS += $(OBJDIR)/NoisePerlinNode.o
OBJECTS += $(OBJDIR)/NoiseValueCubicNode.o
OBJECTS += $(OBJDIR)/NoiseValueNode.o
OBJECTS += $(OBJDIR)/OSLiscences.o
OBJECTS += $(OBJDIR)/OutputNode.o
OBJECTS += $(OBJDIR)/PixelateNode.o
OBJECTS += $(OBJDIR)/ProjectData.o
OBJECTS += $(OBJDIR)/QuickHack.o
OBJECTS += $(OBJDIR)/RandomNumberNode.o
OBJECTS += $(OBJDIR)/Renderer.o
OBJECTS += $(OBJDIR)/SeaManager.o
OBJECTS += $(OBJDIR)/Serializer.o
OBJECTS += $(OBJDIR)/Shader.o
OBJECTS += $(OBJDIR)/ShaderStorageBuffer.o
OBJECTS += $(OBJDIR)/ShaderUtils.o
OBJECTS += $(OBJDIR)/SinNode.o
OBJECTS += $(OBJDIR)/SkySettings.o
OBJECTS += $(OBJDIR)/SplashScreen.o
OBJECTS += $(OBJDIR)/SquareNode.o
OBJECTS += $(OBJDIR)/SubNode.o
OBJECTS += $(OBJDIR)/SupportersTribute.o
OBJECTS += $(OBJDIR)/TanNode.o
OBJECTS += $(OBJDIR)/Texture2D.o
OBJECTS += $(OBJDIR)/TextureCoordinatesNode.o
OBJECTS += $(OBJDIR)/TextureCubemap.o
OBJECTS += $(OBJDIR)/TextureNode.o
OBJECTS += $(OBJDIR)/TextureStore.o
OBJECTS += $(OBJDIR)/TimeBasedSeedNode.o
OBJECTS += $(OBJDIR)/UIFontManager.o
OBJECTS += $(OBJDIR)/Utils.o
OBJECTS += $(OBJDIR)/VisualizerNode.o
OBJECTS += $(OBJDIR)/Window.o

# Rules
# #############################################

all: $(TARGET)
	@:

$(TARGET): $(GENERATED) $(OBJECTS) $(LDDEPS) | $(TARGETDIR)
	$(PRELINKCMDS)
	@echo Linking TerraForge3DLib
	$(SILENT) $(LINKCMD)
	$(POSTBUILDCMDS)

$(TARGETDIR):
	@echo Creating $(TARGETDIR)
ifeq (posix,$(SHELLTYPE))
	$(SILENT) mkdir -p $(TARGETDIR)
else
	$(SILENT) mkdir $(subst /,\\,$(TARGETDIR))
endif

$(OBJDIR):
	@echo Creating $(OBJDIR)
ifeq (posix,$(SHELLTYPE))
	$(SILENT) mkdir -p $(OBJDIR)
else
	$(SILENT) mkdir $(subst /,\\,$(OBJDIR))
endif

clean:
	@echo Cleaning TerraForge3DLib
ifeq (posix,$(SHELLTYPE))
	$(SILENT) rm -f  $(TARGET)
	$(SILENT) rm -rf $(GENERATED)
	$(SILENT) rm -rf $(OBJDIR)
else
	$(SILENT) if exist $(subst /,\\,$(TARGET)) del $(subst /,\\,$(TARGET))
	$(SILENT) if exist $(subst /,\\,$(GENERATED)) rmdir /s /q $(subst /,\\,$(GENERATED))
	$(SILENT) if exist $(subst /,\\,$(OBJDIR)) rmdir /s /q $(subst /,\\,$(OBJDIR))
endif

prebuild: | $(OBJDIR)
	$(PREBUILDCMDS)

ifneq (,$(PCH))
$(OBJECTS): $(GCH) | $(PCH_PLACEHOLDER)
$(GCH): $(PCH) | prebuild
	@echo $(notdir $<)
	$(SILENT) $(CXX) -x c++-header $(ALL_CXXFLAGS) -o "$@" -MF "$(@:%.gch=%.d)" -c "$<"
$(PCH_PLACEHOLDER): $(GCH) | $(OBJDIR)
ifeq (posix,$(SHELLTYPE))
	$(SILENT) touch "$@"
else
	$(SILENT) echo $null >> "$@"
endif
else
$(OBJECTS): | prebuild
endif


# File Rules
# #############################################

$(OBJDIR)/Application.o: src/Base/Application.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/Camera.o: src/Base/Camera.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/ComputeShader.o: src/Base/ComputeShader.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/ExportTexture.o: src/Base/ExportTexture.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/FrameBuffer.o: src/Base/FrameBuffer.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/ImGuiCurveEditor.o: src/Base/ImGuiCurveEditor.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/ImGuiImplGLFW.o: src/Base/ImGuiImplGLFW.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/ImGuiImplOpenGL.o: src/Base/ImGuiImplOpenGL.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/ImGuiShapes.o: src/Base/ImGuiShapes.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/LogHandler.o: src/Base/Logging/LogHandler.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/Logger.o: src/Base/Logging/Logger.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/Mesh.o: src/Base/Mesh.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/Model.o: src/Base/Model.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/ModelImporter.o: src/Base/ModelImporter.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/NodeEditor.o: src/Base/NodeEditor/NodeEditor.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/ComputeKernel.o: src/Base/OpenCL/ComputeKernel.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/QuickHack.o: src/Base/QuickHack.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/Renderer.o: src/Base/Renderer.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/Shader.o: src/Base/Shader.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/ShaderStorageBuffer.o: src/Base/ShaderStorageBuffer.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/ShaderUtils.o: src/Base/ShaderUtils.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/SplashScreen.o: src/Base/SplashScreen.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/Texture2D.o: src/Base/Texture2D.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/TextureCubemap.o: src/Base/TextureCubemap.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/UIFontManager.o: src/Base/UIFontManager.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/Window.o: src/Base/Window.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/ApplicationState.o: src/Data/ApplicationState.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/ProjectData.o: src/Data/ProjectData.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/Serializer.o: src/Data/Serializer.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/AdvancedErosionFilter.o: src/Filters/AdvancedErosionFilter.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/DrawFilter.o: src/Filters/DrawFilter.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/ErosionFilter.o: src/Filters/ErosionFilter.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/FiltersManager.o: src/Filters/FiltersManager.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/GPUErosionFilter.o: src/Filters/GPUErosionFilter.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/FoliagePlacement.o: src/Foliage/FoliagePlacement.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/CPUNodeEditor.o: src/Generators/CPUNodeEditor/CPUNodeEditor.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/AbsNode.o: src/Generators/CPUNodeEditor/Nodes/AbsNode.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/AddNode.o: src/Generators/CPUNodeEditor/Nodes/AddNode.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/BlendNode.o: src/Generators/CPUNodeEditor/Nodes/BlendNode.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/ClampNode.o: src/Generators/CPUNodeEditor/Nodes/ClampNode.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/CosNode.o: src/Generators/CPUNodeEditor/Nodes/CosNode.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/CurveNode.o: src/Generators/CPUNodeEditor/Nodes/CurveNode.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/DivNode.o: src/Generators/CPUNodeEditor/Nodes/DivNode.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/DummyNode.o: src/Generators/CPUNodeEditor/Nodes/DummyNode.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/DuplicateNode.o: src/Generators/CPUNodeEditor/Nodes/DuplicateNode.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/HillNode.o: src/Generators/CPUNodeEditor/Nodes/HillNode.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/MathFunctionNode.o: src/Generators/CPUNodeEditor/Nodes/MathFunctionNode.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/MaxMeshCoordinatesNode.o: src/Generators/CPUNodeEditor/Nodes/MaxMeshCoordinatesNode.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/MeshCoordinatesNode.o: src/Generators/CPUNodeEditor/Nodes/MeshCoordinatesNode.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/MinMeshCoordinatesNode.o: src/Generators/CPUNodeEditor/Nodes/MinMeshCoordinatesNode.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/MinValNode.o: src/Generators/CPUNodeEditor/Nodes/MinValNode.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/MulNode.o: src/Generators/CPUNodeEditor/Nodes/MulNode.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/NoiseCellularNode.o: src/Generators/CPUNodeEditor/Nodes/NoiseCellularNode.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/NoiseOpenSimplex2Node.o: src/Generators/CPUNodeEditor/Nodes/NoiseOpenSimplex2Node.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/NoiseOpenSimplex2SNode.o: src/Generators/CPUNodeEditor/Nodes/NoiseOpenSimplex2SNode.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/NoisePerlinNode.o: src/Generators/CPUNodeEditor/Nodes/NoisePerlinNode.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/NoiseValueCubicNode.o: src/Generators/CPUNodeEditor/Nodes/NoiseValueCubicNode.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/NoiseValueNode.o: src/Generators/CPUNodeEditor/Nodes/NoiseValueNode.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/OutputNode.o: src/Generators/CPUNodeEditor/Nodes/OutputNode.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/PixelateNode.o: src/Generators/CPUNodeEditor/Nodes/PixelateNode.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/RandomNumberNode.o: src/Generators/CPUNodeEditor/Nodes/RandomNumberNode.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/SinNode.o: src/Generators/CPUNodeEditor/Nodes/SinNode.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/SquareNode.o: src/Generators/CPUNodeEditor/Nodes/SquareNode.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/SubNode.o: src/Generators/CPUNodeEditor/Nodes/SubNode.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/TanNode.o: src/Generators/CPUNodeEditor/Nodes/TanNode.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/TextureCoordinatesNode.o: src/Generators/CPUNodeEditor/Nodes/TextureCoordinatesNode.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/TextureNode.o: src/Generators/CPUNodeEditor/Nodes/TextureNode.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/TimeBasedSeedNode.o: src/Generators/CPUNodeEditor/Nodes/TimeBasedSeedNode.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/VisualizerNode.o: src/Generators/CPUNodeEditor/Nodes/VisualizerNode.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/CPUNoiseLayersGenerator.o: src/Generators/CPUNoiseLayersGenerator.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/ClearMeshGenerator.o: src/Generators/ClearMeshGenerator.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/GPUNoiseLayerGenerator.o: src/Generators/GPUNoiseLayerGenerator.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/GeneratorMask.o: src/Generators/GeneratorMask.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/MeshGeneratorManager.o: src/Generators/MeshGeneratorManager.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/LightManager.o: src/Lighting/LightManager.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/MainMenu.o: src/Menu/MainMenu.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/AppStyles.o: src/Misc/AppStyles.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/ExplorerControls.o: src/Misc/ExplorerControls.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/ExportManager.o: src/Misc/ExportManager.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/OSLiscences.o: src/Misc/OSLiscences.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/SupportersTribute.o: src/Misc/SupportersTribute.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/Module.o: src/Modules/Module.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/ModuleManager.o: src/Modules/ModuleManager.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/LayeredNoiseManager.o: src/NoiseLayers/LayeredNoiseManager.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/NoiseLayer.o: src/NoiseLayers/NoiseLayer.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/SeaManager.o: src/Sea/SeaManager.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/CubeMap.o: src/Sky/CubeMap.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/SkySettings.o: src/Sky/SkySettings.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/TextureStore.o: src/TextureStore/TextureStore.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/Utils.o: src/Utils/Utils.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"

-include $(OBJECTS:%.o=%.d)
ifneq (,$(PCH))
  -include $(PCH_PLACEHOLDER).d
endif