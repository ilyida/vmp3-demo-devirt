#include "vmprofiler.h"
#include <vm/utils/zydisutils.h>
#include <loguru.hpp>
#include <unicorn/unicorn.h>

bool VMProfiler::ProfileRoutines(VMImage& Image) {
    for (auto& Routine : Image.VMRoutines) {
        if (!ProfileRoutine(Image, Routine)) {
            LOG_F(ERROR, "Failed to profile VM routine!");
            return false;
        }
    }

    return true;
}

bool VMProfiler::ProfileRoutine(const VMImage& Image, VMRoutine& Routine) {
    if (!ProfileHT(Image, Routine)) {
        LOG_F(ERROR, "Failed to profile VM routines HandlerTable!");
        return false;
    }

    return true;
}

bool VMProfiler::ProfileHT(const VMImage& Image, VMRoutine& Routine) {
    if (!PopulateHandlers(Image, Routine)) {
        LOG_F(ERROR, "Failed to populate VM routines handlers!");
        return false;
    }

    return true;
}

bool VMProfiler::PopulateHandlers(const VMImage& Image, VMRoutine& Routine) {
    constexpr size_t HandlerCount = 256;
    constexpr size_t HandlerTableSize = HandlerCount * sizeof(uint32_t);

    if (Routine.HTAddress < Image.ImageBase ||
        Routine.HTAddress + HandlerTableSize > Image.ImageBase + Image.ImageSize) {
        LOG_F(ERROR, "Handler table is not within image bounds!");
        return false;
    }

    const std::byte* HTData = Image.ImageData + (Routine.HTAddress - Image.ImageBase);
    for (size_t i = 0; i < HandlerCount; i++) {
        uint32_t RawOffset;
        std::memcpy(&RawOffset, HTData + i * sizeof(uint32_t), sizeof(uint32_t));

        int32_t  RelativeOffset = static_cast<int32_t>(RawOffset);
        uint64_t HandlerAddress = static_cast<uint64_t>(RelativeOffset + Routine.HTAddress);

        Routine.Handlers[i].RuntimeAddress = HandlerAddress;
    }

    return true;
}
