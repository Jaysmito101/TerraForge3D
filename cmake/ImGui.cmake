set(TERRA_FORGE_VENDOR_DIR "${CMAKE_CURRENT_LIST_DIR}/../TerraForge3D/vendor")

set(TERRA_FORGE_IMGUI_SOURCES
    "${TERRA_FORGE_VENDOR_DIR}/imgui/imgui.cpp"
    "${TERRA_FORGE_VENDOR_DIR}/imgui/imgui_demo.cpp"
    "${TERRA_FORGE_VENDOR_DIR}/imgui/imgui_draw.cpp"
    "${TERRA_FORGE_VENDOR_DIR}/imgui/imgui_tables.cpp"
    "${TERRA_FORGE_VENDOR_DIR}/imgui/imgui_widgets.cpp"
    "${TERRA_FORGE_VENDOR_DIR}/imgui-node-editor/crude_json.cpp"
    "${TERRA_FORGE_VENDOR_DIR}/imgui-node-editor/imgui_bezier_math.inl"
    "${TERRA_FORGE_VENDOR_DIR}/imgui-node-editor/imgui_canvas.cpp"
    "${TERRA_FORGE_VENDOR_DIR}/imgui-node-editor/imgui_extra_math.inl"
    "${TERRA_FORGE_VENDOR_DIR}/imgui-node-editor/imgui_node_editor_api.cpp"
    "${TERRA_FORGE_VENDOR_DIR}/imgui-node-editor/imgui_node_editor_internal.inl"
    "${TERRA_FORGE_VENDOR_DIR}/imgui-node-editor/imgui_node_editor.cpp"
)

add_library(ImGui STATIC ${TERRA_FORGE_IMGUI_SOURCES})

target_include_directories(ImGui PUBLIC
    "${TERRA_FORGE_VENDOR_DIR}/imgui"
    "${TERRA_FORGE_VENDOR_DIR}/imgui/backends"
    "${TERRA_FORGE_VENDOR_DIR}/imgui-node-editor"
)

target_compile_definitions(ImGui PUBLIC IMGUI_DEFINE_MATH_OPERATORS)
