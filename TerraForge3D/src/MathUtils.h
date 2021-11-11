#pragma once

template <typename t>
t Clamp01(t val) {
	if (val > 1.0f)
		return 1.0f;
	if (val < 0.0f)
		return 0.0f;
	return val;
}
