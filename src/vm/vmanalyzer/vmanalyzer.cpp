#include "vmanalyzer.h"
#include <vm/utils/zydisutils.h>
#include <unordered_map>
#include <loguru.hpp>
#include <unicorn/unicorn.h>
#include <peconv.h>

struct HookCodeData {
    const VMImage* Image;
    VMRoutine*     Routine;
};

static bool IsVMEnterFinalIns(const ZydisDisassembledInstruction& Ins, VMRoutine& Routine) {
    // VMEnter handlers end with:
    // movzx   reg32, byte ptr [vip]
    //
    // This instruction gets the opcode at the current VIP and moves it to a random register, which means we have hit
    // the FDE loop of the VM.

    if (Ins.info.mnemonic != ZYDIS_MNEMONIC_MOVZX)
        return false;

    if (Ins.info.operand_count_visible != 2)
        return false;

    if (Ins.operands[0].type != ZYDIS_OPERAND_TYPE_REGISTER || Ins.operands[0].element_size != 32)
        return false;

    if (Ins.operands[1].type != ZYDIS_OPERAND_TYPE_MEMORY || Ins.operands[1].element_size != 8 ||
        Ins.operands[1].mem.base != Routine.RegVIP || Ins.operands[1].mem.disp.value != 0)
        return false;

    return true;
}

static void HookCode(uc_engine* uc, uint64_t address, uint32_t size, HookCodeData* user_data) {
    ZydisDisassembledInstruction Ins;
    if (ZYAN_FAILED(ZydisDisassembleIntel(ZYDIS_MACHINE_MODE_LONG_64, address,
                                          user_data->Image->ImageData + (address - user_data->Image->ImageBase), size,
                                          &Ins))) {
        LOG_F(ERROR, "Failed to dissasemble instruction!");
        uc_emu_stop(uc);
        return;
    }

    if (IsVMEnterFinalIns(Ins, *user_data->Routine)) {
        uc_reg_read(uc, ZydisUtils::ZydisRegToUCReg(user_data->Routine->RegVIP), &user_data->Routine->VIPAddress);
        uc_reg_read(uc, ZydisUtils::ZydisRegToUCReg(user_data->Routine->RegHT), &user_data->Routine->HTAddress);
        uc_emu_stop(uc);

        user_data->Routine->FDEAddress = Ins.runtime_address;

        LOG_F(INFO, "Found VMEnter end signature! (0x%llX, %s)", user_data->Routine->FDEAddress, Ins.text);
    }
}

bool VMAnalyzer::AnalyzeRoutines(VMImage& Image) {
    for (auto& Routine : Image.VMRoutines) {
        if (!AnalyzeRoutineEntry(Image, Routine)) {
            LOG_F(ERROR, "Failed to analyze routine!");
            return false;
        }
    }

    return true;
}

bool VMAnalyzer::AnalyzeRoutineEntry(const VMImage& Image, VMRoutine& Routine) {
    constexpr uint64_t STACK_ADDR = 0x0FFF0000;
    constexpr uint64_t STACK_SIZE = 0x10000;
    constexpr uint64_t MAX_INS_COUNT = 150;

    uc_engine* uc = nullptr;
    uc_err     err = uc_open(UC_ARCH_X86, UC_MODE_64, &uc);
    if (err != UC_ERR_OK) {
        LOG_F(ERROR, "Failed to open unicorn-engine for analyzing VM routine! %s", uc_strerror(err));
        return false;
    }

    err = uc_mem_map(uc, Image.ImageBase, Image.ImageSize, UC_PROT_ALL);
    if (err != UC_ERR_OK) {
        LOG_F(ERROR, "Failed to map image to unicorn-engine memory! %s", uc_strerror(err));
        uc_close(uc);
        return false;
    }

    err = uc_mem_write(uc, Image.ImageBase, Image.ImageData, Image.ImageSize);
    if (err != UC_ERR_OK) {
        LOG_F(ERROR, "Failed to write image to unicorn-engine memory! %s", uc_strerror(err));
        uc_close(uc);
        return false;
    }

    err = uc_mem_map(uc, STACK_ADDR, STACK_SIZE, UC_PROT_ALL);
    if (err != UC_ERR_OK) {
        LOG_F(ERROR, "Failed to map stack for unicorn-engine! %s", uc_strerror(err));
        uc_close(uc);
        return false;
    }

    uint64_t rsp = STACK_ADDR + STACK_SIZE - 0x8;

    uint64_t ReturnAddress = 0x3857195879821341;
    uc_mem_write(uc, rsp, &ReturnAddress, sizeof(ReturnAddress));
    rsp -= sizeof(ReturnAddress);

    // Write dummy values to each register (besides RSP, that's required as it points to the mapped mem)
    // This way we can track the layout of pushed user registers
    //
    std::unordered_map<int, uint64_t> RegisterValues = {
        {UC_X86_REG_RFLAGS, 2 /*Scuffed way of doing this but RFLAGS seems to be 2 always so this is fine for now*/},
        {UC_X86_REG_RAX, 0x8571934719826522},
        {UC_X86_REG_RBX, 0x7C9E13A5D2F467B1},
        {UC_X86_REG_RCX, 0xA54BE91D37C8024F},
        {UC_X86_REG_RDX, 0x1D46FEA8BC37E90C},
        {UC_X86_REG_RSI, 0x85BE6C41D29703FA},
        {UC_X86_REG_RDI, 0x3B7D9AE4F1056C82},
        {UC_X86_REG_RBP, 0x48C9D17A2639FE0D},
        {UC_X86_REG_RSP, rsp}, // Stack pointer should not be random num
        {UC_X86_REG_R8, 0xDE79F13BA048C1EF},
        {UC_X86_REG_R9, 0x6B4ED05FA93C271A},
        {UC_X86_REG_R10, 0xACD3180F64B2E5D3},
        {UC_X86_REG_R11, 0x2E17C953B0DAF846},
        {UC_X86_REG_R12, 0x9F83E17264AC0D7B},
        {UC_X86_REG_R13, 0xC7159B3FA6D04B29},
        {UC_X86_REG_R14, 0x1A94F02EC7DB59E3},
        {UC_X86_REG_R15, 0x5D63A49EC8F03AB6},
        {VMRoutine::REL_ADDR, Image.ImageBase - Image.ImageDefaultBase}};

    for (const auto& Value : RegisterValues) {
        uc_reg_write(uc, Value.first, &Value.second);
    }

    HookCodeData Data;
    Data.Image = &Image;
    Data.Routine = &Routine;

    uc_hook trace;
    err = uc_hook_add(uc, &trace, UC_HOOK_CODE, HookCode, &Data, 1, 0);
    if (err != UC_ERR_OK) {
        LOG_F(ERROR, "Failed to add code hook in unicorn-engine! %s", uc_strerror(err));
        uc_close(uc);
        return false;
    }

    err = uc_emu_start(uc, Routine.RuntimeAddress, Image.ImageBase + Image.ImageSize, 0, MAX_INS_COUNT);
    if (err != UC_ERR_OK) {
        LOG_F(ERROR, "Failed to execute execute instruction in unicorn-engine! %s", uc_strerror(err));
        uc_close(uc);
        return false;
    }

    if (!Routine.VIPAddress) {
        LOG_F(ERROR, "Failed to find initial VIP address for VM routine!");
        uc_close(uc);
        return false;
    }

    // Attempt to find the values on the stack
    //
    uint64_t VSP;
    uc_reg_read(uc, ZydisUtils::ZydisRegToUCReg(Routine.RegVSP), &VSP);

    std::array<uint64_t, 17> OriginalReg;
    uc_mem_read(uc, VSP, OriginalReg.data(), sizeof(OriginalReg));

    std::unordered_map<int, int> RegisterStackOffsets = {
        {UC_X86_REG_RAX, -1},     {UC_X86_REG_RBX, -1}, {UC_X86_REG_RCX, -1}, {UC_X86_REG_RDX, -1},
        {UC_X86_REG_RSI, -1},     {UC_X86_REG_RDI, -1}, {UC_X86_REG_RBP, -1}, {UC_X86_REG_R8, -1},
        {UC_X86_REG_R9, -1},      {UC_X86_REG_R10, -1}, {UC_X86_REG_R11, -1}, {UC_X86_REG_R12, -1},
        {UC_X86_REG_R13, -1},     {UC_X86_REG_R14, -1}, {UC_X86_REG_R15, -1}, {UC_X86_REG_RFLAGS, -1},
        {VMRoutine::REL_ADDR, -1}};

    uint64_t UnfoundStackOffset = -1;
    int      RemainingSlots = OriginalReg.size();
    for (size_t i = 0; i < OriginalReg.size(); i++) {
        uint64_t StackValue = OriginalReg[i];
        uint64_t StackOffset = i * 8;

        bool Found = false;
        for (const auto& Value : RegisterValues) {
            if (Value.second == StackValue) {
                if (RegisterStackOffsets[Value.first] != -1) {
                    LOG_F(ERROR, "Found register twice on stack! (%d)", Value.first);
                    break;
                }

                RegisterStackOffsets[Value.first] = StackOffset;
                --RemainingSlots;
                Found = true;
            }
        }

        if (!Found)
            UnfoundStackOffset = StackOffset;
    }

    if (RemainingSlots == 1 && UnfoundStackOffset != -1 && RegisterStackOffsets[VMRoutine::REL_ADDR] == -1) {
        LOG_F(INFO, "Assuming data at %llu is REL_ADDR", UnfoundStackOffset);
        RegisterStackOffsets[VMRoutine::REL_ADDR] = UnfoundStackOffset;
        RemainingSlots--;
    }

    if (RemainingSlots) {
        LOG_F(ERROR, "Failed to trace register stack locations for VMENTER! (%d remaining)", RemainingSlots);
        uc_close(uc);
        return true;
    }

    // Verify the order and distance between each register on the stack
    //
    for (const auto& Register : RegisterStackOffsets) {
        Routine.RegisterStackOffsets.push_back({Register.first, Register.second});
    }
    std::sort(Routine.RegisterStackOffsets.begin(), Routine.RegisterStackOffsets.end(),
              [](const auto& a, const auto& b) { return a.second < b.second; });

    bool ValidOffsets = true;
    int  ExpectedOffset = 0;

    for (const auto& Register : Routine.RegisterStackOffsets) {
        if (Register.second != ExpectedOffset) {
            LOG_F(ERROR, "Register have more than 8 byte gap on the stack!");
            ValidOffsets = false;
        }
        ExpectedOffset += 8;
    }

    if (!ValidOffsets) {
        LOG_F(ERROR, "Stack offsets not valid!");
        uc_close(uc);
        return true;
    }

    uc_close(uc);
    return true;
}
