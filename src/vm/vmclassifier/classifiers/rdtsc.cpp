#include <vm/vmstructs.h>
#include <vm/utils/zydisutils.h>
#include <vm/vmclassifier/classifierutils.h>
#include <unicorn/unicorn.h>
#include <loguru.hpp>

// CPUID:
// rdtsc
// sub     vsp, 8
// mov     [vsp+4], eax
// mov     [vsp+0], edx
//

constexpr uint32_t ValueRDTSC = 0x13131313;

static void CodeHook(uc_engine* uc, uint64_t address, uint32_t size, void* user_data) {
    uint8_t Opcode[2];
    uc_mem_read(uc, address, Opcode, 2);

    if (Opcode[0] == 0x0F && Opcode[1] == 0x31) {
        uc_reg_write(uc, UC_X86_REG_EAX, &ValueRDTSC);
        uc_reg_write(uc, UC_X86_REG_EDX, &ValueRDTSC);

        uint64_t rip;
        uc_reg_read(uc, UC_X86_REG_RIP, &rip);
        rip += 2;
        uc_reg_write(uc, UC_X86_REG_RIP, &rip);
    }
}

VMOpcode rdtsc(const VMImage& Image, const VMRoutine& Routine, VMHandler& Handler) {
    VMClassifierEmulator Emulator(Image, Routine);
    if (!Emulator.Valid()) {
        LOG_F(ERROR, "Failed to setup emulator!");
        return VMOpcode::NONE;
    }

    uc_hook hook;
    uc_err  err = uc_hook_add(Emulator.UC(), &hook, UC_HOOK_CODE, CodeHook, nullptr, 1, 0);
    if (err != UC_ERR_OK) {
        LOG_F(ERROR, "Failed to create code hook!");
        return VMOpcode::NONE;
    }

    err = Emulator.Run(Handler, Routine);
    if (err != UC_ERR_OK)
        return VMOpcode::NONE;

    // Compare result with known expected results with identical behaviour to VM instruction
    //
    uint64_t VSP;
    uc_reg_read(Emulator.UC(), ZydisUtils::ZydisRegToUCReg(Routine.RegVSP), &VSP);

    uint32_t RDTSC[2];
    uc_mem_read(Emulator.UC(), VSP, &RDTSC, sizeof(RDTSC));

    if (RDTSC[0] == ValueRDTSC && RDTSC[1] == ValueRDTSC)
        return Emulator.Return(VMOpcode::RDTSC, Routine, Handler);

    return VMOpcode::NONE;
}
