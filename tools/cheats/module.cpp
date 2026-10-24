// SPDX-License-Identifier: GPL-2.0-or-later
#include "triaevum_title_whole_aot_abi.h"
#include "comfort.h"
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
bool infiniteRupees = false;
bool instantText = false;
std::filesystem::path directory;
constexpr uint32_t FrameHook = 0x5CF8E4;
constexpr uint32_t ChangeRupees = 0x376A60;
constexpr uint32_t ParseText = 0x4C08C0;
constexpr uint32_t DispatchControl = 0x2E0578;

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
    if (infiniteRupees && (pc == FrameHook || pc == ChangeRupees)) {
        if (Comfort::RefillWallet(memory)) {
            if (pc == ChangeRupees) state.r[0] = 0;
        }
    }
    if (instantText && pc == ParseText && state.r[14] == 0x2E0494) {
        Comfort::BeginInstantText(memory, state.r[5], state.r[1], state.r[9]);
    }
    if (instantText && pc == DispatchControl) {
        Comfort::MakeControlSkippable(memory, state.r[6]);
    }
}

bool Execute(uint32_t pc, GuestState& state, NativeA32Memory& memory,
    ExecutionResult* result, Oot3dWholeAotStats* stats, Oot3dWholeAotExternalCall external,
    uint32_t budget, uint32_t* consumed, BlockEntryCallback callback, void* user,
    const uint32_t* pcs, size_t count, const Oot3dAotBlockEntryFilter* filter,
    bool skipFirst, uint32_t stopPc) {
    if (!infiniteRupees && !instantText) {
        return original->Execute(pc, state, memory, result, stats, external, budget, consumed,
            callback, user, pcs, count, filter, skipFirst, stopPc);
    }
    CallbackContext previous{callback, user, pcs, count};
    std::vector<uint32_t> combined;
    const bool unfiltered = callback && count == 0;
    Oot3dAotBlockEntryFilter combinedFilter;
    if (!unfiltered) {
        if (callback && count) combined.assign(pcs, pcs + count);
        if (infiniteRupees) { combined.push_back(FrameHook); combined.push_back(ChangeRupees); }
        if (instantText) { combined.push_back(ParseText); combined.push_back(DispatchControl); }
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
        const bool requestedRupees = settings.at("infinite_rupees").get<bool>();
        const bool requestedText = settings.at("instant_dialogue").get<bool>();
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
        infiniteRupees = requestedRupees;
        instantText = requestedText;
        program.Execute = Execute;
        return &program;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "Comfort patch: %s\n", error.what());
        return nullptr;
    }
}
