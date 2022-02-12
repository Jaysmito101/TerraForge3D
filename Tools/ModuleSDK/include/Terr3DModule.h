#pragma once
#include "Defines.h"

#include "BaseModuleExport.h"
#include "BaseModuleInterface.h"

#ifdef NODE_MODULE
#include "NodeModuleExport.h"
#include "NodeModuleInterface.h"
#endif //  NODE_MODULE

#ifdef NOISE_LAYER_MODULE
#include "NoiseLayerModuleExport.h"
#include "NoiseLayerModuleInterface.h"
#endif //  NOISE_LAYER_MODULE

#ifdef UI_MODULE
#include "UIModuleExport.h"
#include "UIModuleInterface.h"
#endif // UI_MODULE
