#pragma once

#include "Base/Texture2D.h"
#include "Base/FrameBuffer.h"
#include "Base/ExportTexture.h"
#include "Base/Camera.h"
#include "Base/Shader.h"
#include "Base/Model.h"

#include "json/json.hpp"

static const char* textueBakerSlots[] = { 
    "Slot 0 (Height Map)",
    "Slot 1 (Custom)",
    "Slot 2 (Custom)",
    "Slot 3 (Custom)",
    "Slot 4 (Custom)",
    "Slot 5 (Custom)",
    "Slot 6 (Custom)",
    "Slot 7 (Custom)",
    "Slot 8 (Custom)",
    "Slot 9 (Custom)"
};

struct ApplicationState;

class TextureBaker
{
public:
    TextureBaker(ApplicationState* appState);
    ~TextureBaker();

    void ShowSettings(bool* pOpen);

    void Render(bool isPreview = true);

    nlohmann::json Save();
    void Load(nlohmann::json data);

private:
    void CalculateHeightMapMinMax();
    void Update();
    void Bake();

public:
    int tileResolution = 512;
    int tileCount = 2;
    int tileX = 0;
    int tileY = 0;
    bool useTiledExport = false;
    float heightMapMinMax[2] = { -1.0f, 1.0f };
    bool autoCalculateHeightMapMinMax = false;

    int currentTextureSlot = 0;

    Camera textureBakerCamera;

    ApplicationState* appState = nullptr;
    FrameBuffer* previewFrameBuffer = nullptr;
    FrameBuffer* bakeFrameBuffer = nullptr;
};