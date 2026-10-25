// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <cstdint>
#include <dlfcn.h>

namespace Comfort {
class ControllerRumble {
  public:
    bool Pulse(uint16_t low, uint16_t high, uint32_t milliseconds) {
        if (!Available()) return false;
        lock();
        auto* controller = fromPlayer ? fromPlayer(0) : nullptr;
        if (!controller || !hasRumble(controller)) {
            controller = nullptr;
            for (int i = 0; i < count(); ++i) {
                auto* candidate = fromInstance(instanceAt(i));
                if (candidate && hasRumble(candidate)) {
                    // Do not vibrate an arbitrary controller when several pads are attached.
                    if (controller) { unlock(); return false; }
                    controller = candidate;
                }
            }
        }
        bool success = false;
        if (controller) {
            const int result = rumble(controller, low, high, milliseconds);
            success = result == 0;
            if (success) lastInstance = instanceOf(joystickOf(controller));
        }
        unlock();
        return success;
    }
    void Stop() {
        if (lastInstance < 0 || !Available()) return;
        lock();
        if (auto* controller = fromInstance(lastInstance)) rumble(controller, 0, 0, 0);
        lastInstance = -1;
        unlock();
    }
  private:
    template<class T> static T Resolve(const char* symbol) {
        // Reuse the host's SDL instance, including its already-open gamepads.
        return reinterpret_cast<T>(dlsym(RTLD_DEFAULT, symbol));
    }
    bool Available() const { return count && instanceAt && fromInstance && hasRumble && rumble &&
        joystickOf && instanceOf && lock && unlock; }
    struct Controller;
    struct Joystick;
    int (*count)() = Resolve<decltype(count)>("SDL_NumJoysticks");
    int32_t (*instanceAt)(int) = Resolve<decltype(instanceAt)>("SDL_JoystickGetDeviceInstanceID");
    Controller* (*fromInstance)(int32_t) = Resolve<decltype(fromInstance)>("SDL_GameControllerFromInstanceID");
    Controller* (*fromPlayer)(int) = Resolve<decltype(fromPlayer)>("SDL_GameControllerFromPlayerIndex");
    int (*hasRumble)(Controller*) = Resolve<decltype(hasRumble)>("SDL_GameControllerHasRumble");
    int (*rumble)(Controller*,uint16_t,uint16_t,uint32_t) = Resolve<decltype(rumble)>("SDL_GameControllerRumble");
    Joystick* (*joystickOf)(Controller*) = Resolve<decltype(joystickOf)>("SDL_GameControllerGetJoystick");
    int32_t (*instanceOf)(Joystick*) = Resolve<decltype(instanceOf)>("SDL_JoystickInstanceID");
    void (*lock)() = Resolve<decltype(lock)>("SDL_LockJoysticks");
    void (*unlock)() = Resolve<decltype(unlock)>("SDL_UnlockJoysticks");
    int32_t lastInstance = -1;
};
}
