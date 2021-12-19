#pragma once

#include <string>



std::string GetName();

std::string GetVersion();

bool OnModuleUpdate(std::string path);

void OnModuleLoad();

void OnModuleUnload();