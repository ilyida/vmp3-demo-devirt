#include "vmfinder.h"
#include <Zydis/Zydis.h>
#include <loguru.hpp>

bool VMFinder::FindRoutines(VMImage& Image) {
    for (const auto& Sect : Image.CodeSections) {
        FindRoutinesInSection(Image, Sect);
    }

    if (Image.VMRoutines.empty()) {
        LOG_F(ERROR, "Failed to find any patched VM routines! No virtualization?");
        return false;
    }

    return true;
}

bool VMFinder::IsAddressInSect(uint64_t RuntimeAddress, uint64_t ImageBase, const PIMAGE_SECTION_HEADER Sect) const {
    uint64_t SectRuntimeAddress = ImageBase + Sect->VirtualAddress;
    return RuntimeAddress >= SectRuntimeAddress && RuntimeAddress < SectRuntimeAddress + Sect->Misc.VirtualSize;
}
void VMFinder::FindRoutinesInSection(VMImage& Image, PIMAGE_SECTION_HEADER Sect) {
    const uint64_t SectionStart = Image.ImageBase + Sect->VirtualAddress;
    const uint64_t SectionEnd = SectionStart + Sect->SizeOfRawData;

    for (uint64_t RuntimeAddress = SectionStart; RuntimeAddress < SectionEnd;) {
        const uint64_t RVA = RuntimeAddress - Image.ImageBase;
        if (RVA >= Image.ImageSize)
            break;

        const std::byte* InstrPtr = Image.ImageData + RVA;
        const size_t     RemainingSize = Image.ImageSize - RVA;

        ZydisDisassembledInstruction Ins;
        if (ZYAN_SUCCESS(
                ZydisDisassembleIntel(ZYDIS_MACHINE_MODE_LONG_64, RuntimeAddress, InstrPtr, RemainingSize, &Ins))) {
            RuntimeAddress += Ins.info.length;

            if (Ins.info.mnemonic != ZYDIS_MNEMONIC_JMP)
                continue;

            ZyanU64 JumpAddress = 0;
            if (!ZYAN_SUCCESS(ZydisCalcAbsoluteAddress(&Ins.info, &Ins.operands[0], Ins.runtime_address, &JumpAddress)))
                continue;

            if (IsAddressInSect(JumpAddress, Image.ImageBase, Image.VirtSection)) {
                LOG_F(INFO, "Found patched VM routine JMP at 0x%llX to 0x%llX", RuntimeAddress, JumpAddress);
                Image.VMRoutines.emplace_back(JumpAddress);
            }
        } else {
            // Zydis will fail on junk bytes, so we should scan entire section no matter what
            ++RuntimeAddress;
        }
    }
}
