// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "three_ds_input.h"

namespace RetunedFrontend {
class MotionRecenter {
  public:
    ThreeDsRecomp::Input::MotionObservation Apply(
        ThreeDsRecomp::Input::MotionObservation sample, bool held,
        const std::array<float, 3>& bias, double seconds, bool advance = true) {
        if (!sample.GyroscopeValid && !sample.AccelerometerValid) return sample;
        auto next = *this;
        if (held) {
            if (!next.composition.HasOutput && sample.AccelerometerValid)
                next.composition.RestoreGravity(sample.Accelerometer);
            sample.GyroscopeDegreesPerSecond = bias;
            if (sample.AccelerometerValid && next.composition.HasOutput)
                sample.Accelerometer = next.composition.LastGravity;
        } else {
            if (next.wasHeld) next.composition.ResetReference();
            auto rate = sample.GyroscopeDegreesPerSecond;
            for (size_t i = 0; i < rate.size(); ++i) rate[i] -= bias[i];
            const auto motion = next.composition.Sample(
                sample.Accelerometer, sample.AccelerometerValid,
                sample.GyroscopeValid ? rate : std::array<float, 3>{},
                0, 0, seconds, true, true);
            if (sample.AccelerometerValid) sample.Accelerometer = motion.Gravity;
            // The shared input mapper subtracts the sensor bias after this transform.
            if (sample.GyroscopeValid)
                for (size_t i = 0; i < rate.size(); ++i)
                    sample.GyroscopeDegreesPerSecond[i] = motion.AngularVelocity[i] + bias[i];
        }
        next.wasHeld = held;
        if (advance) *this = next;
        return sample;
    }
  private:
    ThreeDsRecomp::Input::MotionCompositionState composition;
    bool wasHeld = false;
};
}
