// SPDX-License-Identifier: GPL-2.0-or-later
#include "../tools/frontend/motion_recenter.h"
#include <cassert>
#include <cmath>

using ThreeDsRecomp::Input::MotionObservation;
constexpr std::array<float, 3> Bias{0.3f, -0.1f, 0.2f};

bool Near(const std::array<float, 3>& a, const std::array<float, 3>& b) {
    for (size_t i = 0; i < a.size(); ++i)
        if (std::abs(a[i] - b[i]) > 0.0001f) return false;
    return true;
}

MotionObservation Sample(std::array<float, 3> gravity, std::array<float, 3> rate = Bias) {
    MotionObservation sample;
    sample.AccelerometerValid = sample.GyroscopeValid = true;
    sample.Accelerometer = gravity;
    sample.GyroscopeDegreesPerSecond = rate;
    return sample;
}

int main() {
    RetunedFrontend::MotionRecenter recenter;
    const auto neutral = Sample({0, -1, 0});
    const auto tilted = Sample({0, 0, -1}, {Bias[0], Bias[1] + 10, Bias[2]});
    assert(Near(recenter.Apply(neutral, false, Bias, 1.0 / 30).Accelerometer, neutral.Accelerometer));
    for (int i = 0; i < 30; ++i) {
        const auto frozen = recenter.Apply(tilted, true, Bias, 1.0 / 30);
        assert(Near(frozen.Accelerometer, neutral.Accelerometer));
        assert(Near(frozen.GyroscopeDegreesPerSecond, Bias));
    }
    const auto resumed = recenter.Apply(tilted, false, Bias, 1.0 / 30);
    assert(Near(resumed.Accelerometer, neutral.Accelerometer));
    assert(Near(resumed.GyroscopeDegreesPerSecond, {Bias[0], Bias[1], Bias[2] - 10}));
    auto idle = tilted;
    idle.GyroscopeDegreesPerSecond = Bias;
    assert(Near(recenter.Apply(idle, false, Bias, 1.0 / 30).GyroscopeDegreesPerSecond, Bias));

    // A second hold must retain the previous reference instead of undoing it.
    recenter.Apply(neutral, true, Bias, 1.0 / 30);
    const auto preview = recenter.Apply(neutral, false, Bias, 1.0 / 30, false);
    assert(Near(preview.Accelerometer, resumed.Accelerometer));
    assert(Near(recenter.Apply(tilted, true, Bias, 1.0 / 30).Accelerometer, resumed.Accelerometer));
    assert(Near(recenter.Apply(neutral, false, Bias, 1.0 / 30).Accelerometer, resumed.Accelerometer));

    const auto unavailable = recenter.Apply({}, true, Bias, 1.0 / 30);
    assert(!unavailable.GyroscopeValid && !unavailable.AccelerometerValid);
    recenter = {};
    assert(Near(recenter.Apply(tilted, false, Bias, 1.0 / 30).Accelerometer, tilted.Accelerometer));

    RetunedFrontend::MotionRecenter gyroOnly;
    auto gyro = neutral;
    gyro.AccelerometerValid = false;
    const auto held = gyroOnly.Apply(gyro, true, Bias, 1.0 / 30);
    assert(!held.AccelerometerValid && held.GyroscopeValid);
    assert(Near(held.GyroscopeDegreesPerSecond, Bias));

    ThreeDsRecomp::Input::MappingConfig config;
    config.NativeMotionSource = ThreeDsRecomp::Input::MotionSource::ControllerMotion;
    config.GyroscopeBiasDegreesPerSecond = Bias;
    ThreeDsRecomp::Input::PhysicalInputState physical;
    physical.ControllerMotion = held;
    const auto frame = ThreeDsRecomp::Input::ResolveInput(config, physical, {});
    assert(Near(frame.Hid.GyroscopeDegreesPerSecond, {}));
    assert(frame.Hid.Buttons == 0);
}
