#include "MathFunctionNode.h"
#include "MeshNodeEditor.h"

#include "muParser.h"

#include <locale>
#include <codecvt>
#include <string>
#include <iostream>


static void SetupParser(mu::Parser* parser) {
    // TODO
}

NodeOutput MathFunctionNode::Evaluate(NodeInputParam input, NodeEditorPin* pin)
{
    try {
        x = input.x;
        y = input.y;
        z = input.z;
        return NodeOutput({(float)parser->Eval()});
    }
    catch (...)
    {
    }
    return NodeOutput({0.0f});
}

void MathFunctionNode::Load(nlohmann::json data)
{
    std::string expr = data["expr"];
    memcpy(inputExpression, expr.data(), data.size());
    mathInputWidth = data["mathInputWidth"];
}

nlohmann::json MathFunctionNode::Save()
{
    nlohmann::json data;
    data["type"] = MeshNodeEditor::MeshNodeType::MathFunction;
    data["expr"] = std::string(inputExpression);
    data["mathInputWidth"] = mathInputWidth;
    return data;
}

void MathFunctionNode::OnRender()
{
    DrawHeader("Custom Math Function");
    
    ImGui::Dummy(ImVec2(mathInputWidth + 20, 10));
    ImGui::SameLine();
    ImGui::Text("Time");
    outputPins[0]->Render();

    ImGui::NewLine();

    ImGui::Text("Math Input Size : ");
    ImGui::SameLine();
    ImGui::PushItemWidth(100);
    ImGui::DragInt(MAKE_IMGUI_ID(outputPins[0]->id), &mathInputWidth, 0.5f);
    ImGui::PopItemWidth();

    ImGui::Text("Output : ");
    ImGui::SameLine();
    ImGui::PushItemWidth(mathInputWidth);
    if (ImGui::InputText(MAKE_IMGUI_ID(id), inputExpression, 1024*4))
        compiled = false;
    ImGui::PopItemWidth();

    if (!compiled)
    {
        if (ImGui::Button("Compile"))
        {
            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
            std::wstring wide = converter.from_bytes(inputExpression);
            try {
                parser->SetExpr(wide);
                compiled = true;
            }
            catch (...) {

            }
        }
    }
}

MathFunctionNode::MathFunctionNode()
{
    headerColor = ImColor(VALUE_NODE_COLOR);
    outputPins.push_back(new NodeEditorPin(NodeEditorPinType::Output));
    parser = new mu::Parser();
    memset(inputExpression, 0, sizeof(inputExpression));
    x = y = z = 0;
    mathInputWidth = 100;
    compiled = false;
    SetupParser(parser);
    parser->DefineVar(L"x", &x);
    parser->DefineVar(L"y", &y);
    parser->DefineVar(L"z", &z);
}
