include_directories(
	../imgui
	../imgui/backends
	../imgui-node-editor
	)


set(SOURCES
	# ImGui
    ../imgui/imgui.cpp
    ../imgui/imgui_demo.cpp
    ../imgui/imgui_draw.cpp
    ../imgui/imgui_tables.cpp
    ../imgui/imgui_widgets.cpp
    # ImGui Node Editor
    ../imgui-node-editor/crude_json.cpp
    ../imgui-node-editor/imgui_bezier_math.inl
    ../imgui-node-editor/imgui_canvas.cpp
    ../imgui-node-editor/imgui_extra_math.inl
    ../imgui-node-editor/imgui_node_editor_api.cpp
    ../imgui-node-editor/imgui_node_editor_internal.inl
    ../imgui-node-editor/imgui_node_editor.cpp
	)

add_library(ImGui ${SOURCES})

target_compile_definitions(ImGui PUBLIC IMGUI_DEFINE_MATH_OPERATORS)
