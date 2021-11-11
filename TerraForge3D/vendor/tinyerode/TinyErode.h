// SPDX-License-Identifier: MIT
//   _______ _             ______               _
//  |__   __(_)           |  ____|             | |
//     | |   _ _ __  _   _| |__   _ __ ___   __| | ___
//     | |  | | '_ \| | | |  __| | '__/ _ \ / _` |/ _ \
//     | |  | | | | | |_| | |____| | | (_) | (_| |  __/
//     |_|  |_|_| |_|\__, |______|_|  \___/ \__,_|\___|
//                    __/ |
//                   |___/
//
// Copyright (C) 2021 Taylor Holberton
//
// A C++ library for simulating erosion.

#pragma once

#ifndef TINYERODE_H_INCLUDED
#define TINYERODE_H_INCLUDED

#include <algorithm>
#include <array>
#include <numeric>
#include <vector>

#include <cassert>
#include <cmath>

namespace TinyErode {

/// Used for simulating a rainfall event on a terrain.
/// Stores information on the terrain that is required to simulate the effect of
/// hydraulic erosion.
///
/// @note The class should only be used once per rainfall event.
class Simulation final
{
public:
  Simulation(int w = 0, int h = 0);

  void SetTimeStep(float timeStep) noexcept { mTimeStep = timeStep; }

  float GetTimeStep() const noexcept { return mTimeStep; }

  int GetWidth() const noexcept { return mSize[0]; }

  int GetHeight() const noexcept { return mSize[1]; }

  /// Called at the beginning of each iteration.
  /// This function will compute the flow rate of each cell, based on the level
  /// of water and the elevation at each point in the height map.
  ///
  /// @param height The function taking an x and y coordinate and returning the
  ///               height value at each point in the map.
  ///
  /// @param water The function taking an x and y coordinate and returning the
  ///              water level at each point in the map. The water levels can be
  ///              initialized to simulate rainfall or river streams.
  template<typename Height, typename Water>
  void ComputeFlowAndTilt(const Height& height, const Water& water);

  /// This function is called after @ref Simulation::ComputeFlow in order to
  /// determine where the water at each cell is going to be moving.
  ///
  /// @param waterAdder A function taking an x and y coordinate as well as a
  ///                   water value to be added to a cell within the water
  ///                   model.
  template<typename WaterAdder>
  void TransportWater(WaterAdder waterAdder);

  /// Erodes and deposites sediment, and then moves remaining sediment based on
  /// the velocity of the water at each cell.
  ///
  /// @param kC A function taking an x and y coordinate and returning the
  ///           carry capacity constant at that particular location.
  ///
  /// @param kD A function taking an x and y coordinate and returning the
  ///           deposition constant at that particular location.
  ///
  /// @param kE A function taking an x and y coordinate and returning the
  ///           erosion constant at that particular location.
  ///
  /// @param heightAdder A function taking an x and y coordinate, as well as a
  ///                    height delta, and adding the value to the height model.
  ///
  /// @note For simple models, @p Kc @p Ke and @p Kd can both be single, uniform
  ///       values.
  template<typename CarryCapacity,
           typename Deposition,
           typename Erosion,
           typename HeightAdder>
  void TransportSediment(CarryCapacity kC,
                         Deposition kD,
                         Erosion kE,
                         HeightAdder heightAdder);

  /// Evaporates water in the water model, based on evaporation constants.
  ///
  /// @param waterAdder A function taking an x and y coordiante as well as a
  ///                   water value to be added to a cell within the water
  ///                   model.
  ///
  /// @param kEvap A function taking an x and y coordinate and returning the
  ///              evaporation constant at that particular location. It is
  ///              the responsibility of this function to ensure that the water
  ///              level does not become negative at this step.
  template<typename WaterAdder, typename Evaporation>
  void Evaporate(WaterAdder water, Evaporation kEvap);

  /// Deposites all currently suspended sediment into the terrain.
  template<typename HeightAdder>
  void TerminateRainfall(HeightAdder heightAdder);

  void Resize(int w, int h);

  /// Gets the sediment levels at each cell. Useful primarily for debugging.
  auto GetSediment() const noexcept -> const std::vector<float>&
  {
    return mSediment;
  }

  void SetMetersPerX(float metersPerX) noexcept
  {
    mPipeLengths[0] = metersPerX;
  }

  void SetMetersPerY(float metersPerY) noexcept
  {
    mPipeLengths[1] = metersPerY;
  }

private:
  using Velocity = std::array<float, 2>;

  using Flow = std::array<float, 4>;

  template<typename Height, typename Water>
  void ComputeFlowAndTiltAt(const Height& height,
                            const Water& water,
                            int x,
                            int y);

  template<typename WaterAdder>
  void TransportWaterAt(WaterAdder& water, int x, int y);

  template<typename CarryCapacity,
           typename Deposition,
           typename Erosion,
           typename HeightAdder>
  void ErodeAndDeposit(CarryCapacity& kC,
                       Deposition& kD,
                       Erosion& kE,
                       HeightAdder& heightAdder,
                       int x,
                       int y);

  const Flow& GetFlow(int x, int y) const noexcept
  {
    return mFlow[(y * GetWidth()) + x];
  }

  Flow& GetFlow(int x, int y) noexcept { return mFlow[(y * GetWidth()) + x]; }

  bool InBounds(int x, int y) const noexcept
  {
    return (x >= 0) && (x < GetWidth()) && (y >= 0) && (y < GetHeight());
  }

  Flow GetInflow(int x, int y) const noexcept;

  float GetScalingFactor(const Flow& flow, float waterLevel) noexcept;

  int ToIndex(int x, int y) const noexcept { return (y * GetWidth()) + x; }

private:
  float mTimeStep = 0.0125;

  float mGravity = 9.8;

  std::array<float, 2> mPipeLengths{ 1, 1 };

  std::array<int, 2> mSize{ 0, 0 };

  std::vector<Flow> mFlow;

  std::vector<float> mSediment;

  std::vector<Velocity> mVelocity;

  std::vector<float> mTilt;
};

// Implementation details beyond this point.

inline Simulation::Simulation(int w, int h)
{
  Resize(w, h);
}

template<typename WaterAdder>
void
Simulation::TransportWater(WaterAdder water)
{
#ifdef _OPENMP
#pragma omp parallel for
#endif

  for (int y = 0; y < GetHeight(); y++) {
    for (int x = 0; x < GetWidth(); x++) {
      TransportWaterAt(water, x, y);
    }
  }
}

template<typename WaterAdder>
void
Simulation::TransportWaterAt(WaterAdder& water, int x, int y)
{
  auto& flow = GetFlow(x, y);

  auto inflow = GetInflow(x, y);

  auto inflowSum = std::accumulate(inflow.begin(), inflow.end(), 0.0f);

  auto outflowSum = std::accumulate(flow.begin(), flow.end(), 0.0f);

  auto volumeDelta = (inflowSum - outflowSum) * mTimeStep;

  auto waterDelta = volumeDelta / (mPipeLengths[0] * mPipeLengths[1]);

  float waterLevel = water(x, y, waterDelta);

  // Compute Water Velocity

  float dx = 0.5f * ((inflow[1] - flow[1]) + (flow[2] - inflow[2]));
  float dy = 0.5f * ((flow[0] - inflow[0]) + (inflow[3] - flow[3]));

  float avgWaterLevel = waterLevel - (waterDelta * 0.5f);

  Velocity velocity{ { 0, 0 } };

  if (avgWaterLevel != 0.0f) {
    velocity[0] = dx / (mPipeLengths[0] * avgWaterLevel);
    velocity[1] = dy / (mPipeLengths[1] * avgWaterLevel);
  }

  mVelocity[ToIndex(x, y)] = velocity;
}

template<typename Height, typename Water>
void
Simulation::ComputeFlowAndTilt(const Height& height, const Water& water)
{
#ifdef _OPENMP
#pragma omp parallel for
#endif

  for (int y = 0; y < GetHeight(); y++) {
    for (int x = 0; x < GetWidth(); x++)
      ComputeFlowAndTiltAt(height, water, x, y);
  }
}

template<typename Height, typename Water>
void
Simulation::ComputeFlowAndTiltAt(const Height& height,
                                 const Water& water,
                                 int x,
                                 int y)
{
  auto& center = GetFlow(x, y);

  std::array<int, 4> xDeltas{ { 0, -1, 1, 0 } };
  std::array<int, 4> yDeltas{ { -1, 0, 0, 1 } };

  auto centerH = height(x, y);
  auto centerW = water(x, y);

  std::array<float, 4> heightNeighbors{ centerH, centerH, centerH, centerH };

  std::array<int, 4> pipeLengthIndices{ { 1, 0, 0, 1 } };

  for (int i = 0; i < 4; i++) {

    auto neighborX = x + xDeltas[i];
    auto neighborY = y + yDeltas[i];

    if (!InBounds(neighborX, neighborY))
      continue;

    heightNeighbors[i] = height(neighborX, neighborY);

    auto neighborH = heightNeighbors[i];

    auto neighborW = water(neighborX, neighborY);

    auto heightDiff = (centerH + centerW) - (neighborH + neighborW);

    // Cross sectional area of the virtual pipe.
    float area = 1;

    // Length of the virtual pipe.
    float pipeLength = mPipeLengths[pipeLengthIndices[i]];

    auto c = mTimeStep * area * (mGravity * heightDiff) / pipeLength;

    center[i] = std::max(0.0f, center[i] + c);
  }

  float totalOutputVolume =
    std::accumulate(center.begin(), center.end(), 0.0f) * mTimeStep;

  if (totalOutputVolume > (centerW * mPipeLengths[0] * mPipeLengths[1])) {

    auto k = GetScalingFactor(center, centerW);

    for (auto& n : center)
      n *= k;
  }

  // Compute Tilt

  float avgDeltaY = 0;
  avgDeltaY += (centerH - heightNeighbors[0]);
  avgDeltaY += (heightNeighbors[3] - centerH);
  avgDeltaY *= 0.5f;

  float avgDeltaX = 0;
  avgDeltaX += (centerH - heightNeighbors[1]);
  avgDeltaX += (heightNeighbors[2] - centerH);
  avgDeltaX *= 0.5f;

  float a = avgDeltaX * avgDeltaX;
  float b = avgDeltaY * avgDeltaY;

  auto abSum = a + b;

  mTilt[ToIndex(x, y)] = std::sqrt(abSum) / std::sqrt(1 + abSum);
}

template<typename CarryCapacity,
         typename Deposition,
         typename Erosion,
         typename HeightAdder>
void
Simulation::TransportSediment(CarryCapacity kC,
                              Deposition kD,
                              Erosion kE,
                              HeightAdder heightAdder)
{
#ifdef _OPENMP
#pragma omp parallel for
#endif

  for (int y = 0; y < GetHeight(); y++) {
    for (int x = 0; x < GetWidth(); x++)
      ErodeAndDeposit(kC, kD, kE, heightAdder, x, y);
  }

  std::vector<float> nextSediment(GetWidth() * GetHeight());

#ifdef _OPENMP
#pragma omp parallel for
#endif

  for (int y = 0; y < GetHeight(); y++) {

    for (int x = 0; x < GetWidth(); x++) {

      auto index = ToIndex(x, y);

      auto vel = mVelocity[index];
      auto xf = x - (vel[0] * mTimeStep);
      auto yf = y + (vel[1] * mTimeStep);

      auto xfi = int(xf);
      auto yfi = int(yf);

      auto u = xf - xfi;
      auto v = yf - yfi;

      std::array<float, 4> s{ { 0, 0, 0, 0 } };

      if (InBounds(xfi + 0, yfi + 0))
        s[0] = mSediment[ToIndex(xfi + 0, yfi + 0)];

      if (InBounds(xfi + 1, yfi + 0))
        s[1] = mSediment[ToIndex(xfi + 1, yfi + 0)];

      if (InBounds(xfi + 0, yfi + 1))
        s[2] = mSediment[ToIndex(xfi + 0, yfi + 1)];

      if (InBounds(xfi + 1, yfi + 1))
        s[3] = mSediment[ToIndex(xfi + 1, yfi + 1)];

      float sx1 = s[0] + (u * (s[1] - s[0]));
      float sx2 = s[2] + (u * (s[3] - s[2]));

      nextSediment[index] = sx1 + (v * (sx2 - sx1));
    }
  }

  mSediment = std::move(nextSediment);
}

template<typename HeightAdder>
void
Simulation::TerminateRainfall(HeightAdder heightAdder)
{
#ifdef _OPENMP
#pragma omp parallel for
#endif

  for (int y = 0; y < GetHeight(); y++) {

    for (int x = 0; x < GetWidth(); x++) {

      auto index = ToIndex(x, y);

      float sediment = mSediment[index];

      heightAdder(x, y, sediment / (mPipeLengths[0] * mPipeLengths[1]));

      mSediment[index] = 0;

      mVelocity[index][0] = 0;
      mVelocity[index][1] = 0;
    }
  }
}

template<typename CarryCapacity,
         typename Deposition,
         typename Erosion,
         typename HeightAdder>
void
Simulation::ErodeAndDeposit(CarryCapacity& kC,
                            Deposition& kD,
                            Erosion& kE,
                            HeightAdder& heightAdder,
                            int x,
                            int y)
{
  auto vel = mVelocity[ToIndex(x, y)];

  auto velocityMagnitude = std::sqrt((vel[0] * vel[0]) + (vel[1] * vel[1]));

  float tiltAngle = mTilt[ToIndex(x, y)];

  float capacity = kC(x, y) * std::max(0.01f, tiltAngle) * velocityMagnitude;

  float sediment = mSediment[ToIndex(x, y)];

  float factor = (capacity > sediment) ? kE(x, y) : kD(x, y);

  heightAdder(x, y, -(factor * (capacity - sediment)));

  mSediment[ToIndex(x, y)] += factor * (capacity - sediment);
}

template<typename WaterAdder, typename Evaporation>
void
Simulation::Evaporate(WaterAdder water, Evaporation kEvap)
{
#ifdef _OPENMP
#pragma omp parallel for
#endif

  for (int y = 0; y < GetHeight(); y++) {
    for (int x = 0; x < GetWidth(); x++)
      water(x, y, -mTimeStep * kEvap(x, y));
  }
}

inline auto
Simulation::GetInflow(int centerX, int centerY) const noexcept -> Flow
{
  std::array<int, 4> xDeltas{ { 0, -1, 1, 0 } };
  std::array<int, 4> yDeltas{ { -1, 0, 0, 1 } };

  std::array<float, 4> inflow{ { 0, 0, 0, 0 } };

  for (int i = 0; i < 4; i++) {

    int x = centerX + xDeltas[i];
    int y = centerY + yDeltas[i];

    if (InBounds(x, y))
      inflow[i] = GetFlow(x, y)[3 - i];
  }

  return inflow;
}

inline float
Simulation::GetScalingFactor(const Flow& flow, float waterLevel) noexcept
{
  auto volume = std::accumulate(flow.begin(), flow.end(), 0.0f) * mTimeStep;

  if (volume == 0.0f)
    return 1.0f;

  return std::min(1.0f,
                  (waterLevel * mPipeLengths[0] * mPipeLengths[1]) / volume);
}

inline void
Simulation::Resize(int w, int h)
{
  assert(w >= 0);
  assert(h >= 0);

  w = std::max(w, 0);
  h = std::max(h, 0);

  mFlow.resize(w * h);

  mSediment.resize(w * h);

  mVelocity.resize(w * h);

  mTilt.resize(w * h);

  mSize[0] = w;
  mSize[1] = h;
}

} // namespace TinyErode

#endif // TINYERODE_H_INCLUDED