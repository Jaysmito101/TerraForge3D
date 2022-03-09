#pragma once

void SetupExplorerControls();

void UpdateExplorerControls(float *pos, float *rot, bool isIEx, float *xO, float *yO);

void ExPRestoreCamera(float *pos, float *rot);

void ExPSaveCamera(float *pos, float *rot);