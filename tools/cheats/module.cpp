// SPDX-License-Identifier: GPL-2.0-or-later
#include "triaevum_title_whole_aot_abi.h"
#include "comfort.h"
#include "api.h"
#include "gameplay.h"
#include "levitation.h"
#include "shortcuts.h"
#include "challenges.h"
#include "warps.h"
#include "feedback.h"
#include "controller_rumble.h"
#include <optional>
#include <atomic>
#include <mutex>
#include <nlohmann/json.hpp>
#include <dlfcn.h>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <vector>

using namespace Oot3dNativeGame;
using namespace oot3d::recomp::a32;

namespace {
const Oot3dWholeAotProgramV2* original = nullptr;
std::array<std::atomic<bool>, RetunedComfort::Settings.size()> enabled{};
std::mutex settingsMutex;
std::string settingsError;
thread_local std::string errorCopy;
std::string notice;
uint64_t noticeSerial = 0;
thread_local std::string noticeCopy;
Comfort::LevitationShortcut levitationShortcut;
Comfort::FeedbackTracker feedbackTracker;
Comfort::ControllerRumble controllerRumble;
bool controllerFeedback = true;
std::array<std::optional<uint32_t>, static_cast<size_t>(RetunedComfort::Action::Count)> pending{};
std::filesystem::path directory;
constexpr uint32_t FrameHook = 0x5CF8E4;
constexpr uint32_t ChangeRupees = 0x376A60;
constexpr uint32_t ParseText = 0x4C08C0;
constexpr uint32_t DispatchControl = 0x2E0578;

bool Enabled(RetunedComfort::Cheat cheat) {
    return enabled[static_cast<size_t>(cheat)].load();
}

bool GetCheat(RetunedComfort::Cheat cheat, bool* value) noexcept {
    const auto index = static_cast<size_t>(cheat);
    if (index >= enabled.size() || !value) return false;
    *value = enabled[index].load();
    return true;
}

void PublishNotice(const std::string& text) {
    notice = text;
    ++noticeSerial;
}

bool SetCheat(RetunedComfort::Cheat cheat, bool value) noexcept {
    std::lock_guard lock(settingsMutex);
    const auto index = static_cast<size_t>(cheat);
    if (index >= enabled.size()) { settingsError = "Unknown cheat."; return false; }
    const auto temporary = directory / "settings.pending.json";
    try {
        auto settings = nlohmann::json::parse(std::ifstream(directory / "settings.json"));
        settings[RetunedComfort::Settings[index].Key] = value;
        {
            std::ofstream stream(temporary);
            stream.exceptions(std::ios::failbit | std::ios::badbit);
            stream << settings.dump(2) << '\n';
            stream.close();
        }
        std::filesystem::rename(temporary, directory / "settings.json");
        enabled[index].store(value);
        settingsError.clear();
        if (cheat == RetunedComfort::Cheat::Levitate)
            PublishNotice(value ? "Levitation ON" : "Levitation OFF");
        return true;
    } catch (const std::exception&) {
        std::error_code ignored;
        std::filesystem::remove(temporary, ignored);
        settingsError = "Could not save cheats. Your previous settings remain active. Try again.";
        PublishNotice(settingsError);
        return false;
    }
}

bool RequestAction(RetunedComfort::Action action, uint32_t value) noexcept {
    std::lock_guard lock(settingsMutex);
    const auto index = static_cast<size_t>(action);
    using RetunedComfort::Action;
    bool valid = false;
    if (action == Action::SetTime)
        valid = std::find(RetunedComfort::Times.begin(), RetunedComfort::Times.end(), value) != RetunedComfort::Times.end();
    else if (action == Action::Teleport)
        valid = value < RetunedComfort::Destinations.size();
    else if (action >= Action::Bottle1 && action <= Action::Bottle4)
        valid = value >= 0x14 && value <= 0x20;
    if (index >= pending.size() || !valid) {
        settingsError = "Choose a supported destination, time, or bottle content.";
        return false;
    }
    pending[index] = value;
    settingsError = "Queued. Changes apply when normal play resumes.";
    return true;
}

const char* LastSettingsError() noexcept {
    std::lock_guard lock(settingsMutex);
    errorCopy = settingsError;
    return errorCopy.c_str();
}

const char* PollNotice(uint64_t* serial) noexcept {
    std::lock_guard lock(settingsMutex);
    if (!serial || *serial == noticeSerial) return nullptr;
    *serial = noticeSerial;
    noticeCopy = notice;
    return noticeCopy.c_str();
}

void GameplayFrame(GuestState& state, MemoryBus& memory) {
    if (controllerFeedback) {
        switch (feedbackTracker.Update(memory, state.r[0])) {
            case Comfort::Feedback::Damage: controllerRumble.Pulse(0x7000, 0x5000, 220); break;
            case Comfort::Feedback::Landing: controllerRumble.Pulse(0x3800, 0x1800, 130); break;
            case Comfort::Feedback::Reset: controllerRumble.Stop(); break;
            case Comfort::Feedback::None: break;
        }
    }
    uint32_t player = 0, buttons = 0;
    const bool playing = Comfort::ReadPlayer(memory, state.r[0], &player);
    const bool input = Comfort::ReadButtons(memory, &buttons);
    if (levitationShortcut.Update(buttons, playing && input))
        SetCheat(RetunedComfort::Cheat::Levitate, !Enabled(RetunedComfort::Cheat::Levitate));
    if (!playing) return;
    using RetunedComfort::Cheat;
    if (Enabled(Cheat::Levitate)) Comfort::Levitate(memory, state.r[0]);
    if (Enabled(Cheat::FreezeTimers)) Comfort::FreezeTimers(memory);
    if (Enabled(Cheat::RunFast)) Comfort::RunFast(memory, state.r[0]);
    constexpr std::array ammo{
        Comfort::Ammo::Bombs, Comfort::Ammo::Nuts, Comfort::Ammo::Seeds,
        Comfort::Ammo::Sticks, Comfort::Ammo::Arrows, Comfort::Ammo::Beans};
    for (size_t i = 0; i < ammo.size(); ++i) {
        if (Enabled(static_cast<Cheat>(static_cast<size_t>(Cheat::InfiniteBombs) + i)))
            Comfort::RefillAmmo(memory, ammo[i]);
    }
    std::lock_guard lock(settingsMutex);
    for (size_t i = 0; i < pending.size(); ++i) {
        if (!pending[i]) continue;
        const auto action = static_cast<RetunedComfort::Action>(i);
        const uint32_t value = *pending[i];
        bool success = false;
        if (action == RetunedComfort::Action::SetTime)
            success = Comfort::SetTime(memory, static_cast<uint16_t>(value));
        else if (action == RetunedComfort::Action::Teleport)
            success = Comfort::Warp(memory, state.r[0], value);
        else success = Comfort::SetBottle(memory, static_cast<uint32_t>(i - 1), static_cast<uint8_t>(value), player);
        pending[i].reset();
        if (action == RetunedComfort::Action::Teleport) {
            settingsError = success ? std::string("Teleporting to ") + RetunedComfort::Destinations[value].Name + "."
                : "Teleport unavailable now. Resume normal play and try again.";
        } else settingsError = success ? "Applied."
            : "A change could not apply. Use an owned bottle and unequip it before trying again.";
        PublishNotice(settingsError);
    }
}

struct CallbackContext {
    BlockEntryCallback callback;
    void* user;
    const uint32_t* pcs;
    size_t count;
};

void OnBlock(uint32_t pc, GuestState& state, MemoryBus& memory, void* user) {
    auto& previous = *static_cast<CallbackContext*>(user);
    if (previous.callback && (previous.count == 0 ||
        std::binary_search(previous.pcs, previous.pcs + previous.count, pc))) {
        previous.callback(pc, state, memory, previous.user);
    }
    if (pc == FrameHook) GameplayFrame(state, memory);
    if (Enabled(RetunedComfort::Cheat::InfiniteRupees) && (pc == FrameHook || pc == ChangeRupees)) {
        if (Comfort::RefillWallet(memory)) {
            if (pc == ChangeRupees) state.r[0] = 0;
        }
    }
    if (Enabled(RetunedComfort::Cheat::InstantDialogue) && pc == ParseText && state.r[14] == 0x2E0494) {
        Comfort::BeginInstantText(memory, state.r[5], state.r[1], state.r[9]);
    }
    if (Enabled(RetunedComfort::Cheat::InstantDialogue) && pc == DispatchControl) {
        Comfort::MakeControlSkippable(memory, state.r[6]);
    }
}

bool Execute(uint32_t pc, GuestState& state, NativeA32Memory& memory,
    ExecutionResult* result, Oot3dWholeAotStats* stats, Oot3dWholeAotExternalCall external,
    uint32_t budget, uint32_t* consumed, BlockEntryCallback callback, void* user,
    const uint32_t* pcs, size_t count, const Oot3dAotBlockEntryFilter*,
    bool skipFirst, uint32_t stopPc) {
    CallbackContext previous{callback, user, pcs, count};
    std::vector<uint32_t> combined;
    const bool unfiltered = callback && count == 0;
    Oot3dAotBlockEntryFilter combinedFilter;
    if (!unfiltered) {
        if (callback && count) combined.assign(pcs, pcs + count);
        combined.push_back(FrameHook);
        if (Enabled(RetunedComfort::Cheat::InfiniteRupees)) combined.push_back(ChangeRupees);
        if (Enabled(RetunedComfort::Cheat::InstantDialogue)) { combined.push_back(ParseText); combined.push_back(DispatchControl); }
        std::sort(combined.begin(), combined.end());
        combined.erase(std::unique(combined.begin(), combined.end()), combined.end());
        for (auto entry : combined) combinedFilter.Insert(entry);
    }
    return original->Execute(pc, state, memory, result, stats, external, budget, consumed,
        OnBlock, &previous, combined.data(), combined.size(),
        unfiltered ? nullptr : &combinedFilter, skipFirst, stopPc);
}
}

extern "C" __attribute__((visibility("default"))) const void*
triaevum_title_aot_query(uint32_t) noexcept { return nullptr; }

extern "C" __attribute__((visibility("default"))) const Oot3dWholeAotProgramV2*
triaevum_title_whole_aot_query(uint32_t abi) noexcept {
    static Oot3dWholeAotProgramV2 program{};
    if (original) return abi == program.AbiVersion ? &program : nullptr;
    try {
        Dl_info info{};
        if (!dladdr(reinterpret_cast<void*>(&triaevum_title_whole_aot_query), &info)) return nullptr;
        directory = std::filesystem::path(info.dli_fname).parent_path();
        auto settings = nlohmann::json::parse(std::ifstream(directory / "settings.json"));
        std::array<bool, RetunedComfort::Settings.size()> requested{};
        for (size_t i = 0; i < requested.size(); ++i) {
            const auto& setting = RetunedComfort::Settings[i];
            requested[i] = settings.value(setting.Key, false);
        }
        const bool requestedFeedback = settings.value("controller_rumble", true);
        auto path = settings.at("original_module").get<std::string>();
        void* handle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
        if (!handle) { std::fprintf(stderr, "Comfort patch: %s\n", dlerror()); return nullptr; }
        using Query = const Oot3dWholeAotProgramV2* (*)(uint32_t) noexcept;
        auto query = reinterpret_cast<Query>(dlsym(handle, "triaevum_title_whole_aot_query"));
        const auto* candidate = query ? query(abi) : nullptr;
        if (!candidate || candidate->AbiVersion != kOot3dWholeAotPluginAbiV2 ||
            candidate->StructSize != sizeof(program) || !candidate->Execute) return nullptr;
        program = *candidate;
        original = candidate;
        controllerFeedback = requestedFeedback;
        for (size_t i = 0; i < requested.size(); ++i) enabled[i].store(requested[i]);
        program.Execute = Execute;
        return &program;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "Comfort patch: %s\n", error.what());
        return nullptr;
    }
}

extern "C" __attribute__((visibility("default"))) const RetunedComfort::Api*
retuned_comfort_query(uint32_t version) noexcept {
    if (version != 3 || !triaevum_title_whole_aot_query(kOot3dWholeAotPluginAbiV2)) return nullptr;
    static const RetunedComfort::Api api{3, sizeof(RetunedComfort::Api), GetCheat, SetCheat, RequestAction, LastSettingsError, PollNotice};
    return &api;
}
