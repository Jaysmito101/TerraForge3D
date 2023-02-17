#pragma once

#include <chrono>


#define START_PROFILER() auto timeBegin_auto_timer = std::chrono::high_resolution_clock::now();


#define END_PROFILER(variable)		auto timeEnd_auto_timer = std::chrono::high_resolution_clock::now();\
									std::chrono::duration<double, std::milli> timeElapsed_auto_timer = timeEnd_auto_timer - timeBegin_auto_timer; \
									variable = timeElapsed_auto_timer.count();
