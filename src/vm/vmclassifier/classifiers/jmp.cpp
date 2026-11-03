#include <vm/vmstructs.h>
#include <vm/utils/zydisutils.h>
#include <vm/vmclassifier/classifierutils.h>
#include <unicorn/unicorn.h>
#include <loguru.hpp>

// JMP:
// mov     rbx, [vsp+0]
// mov     rsp, rbx
// 

VMOpcode jmp(const VMImage& Image, const VMRoutine& Routine, VMHandler& Handler) {
    VMClassifierEmulator Emulator(Image, Routine);
    if (!Emulator.Valid()) {
        LOG_F(ERROR, "Failed to setup emulator!");
        return VMOpcode::NONE;
    }

    // Populate test data on the virtual stack & run emulation
    //
    uint64_t VSP;
    uc_reg_read(Emulator.UC(), ZydisUtils::ZydisRegToUCReg(Routine.RegVSP), &VSP);

    uint64_t TestValue = 0x123456789ABCDEF;
    uc_mem_write(Emulator.UC(), VSP, &TestValue, sizeof(TestValue));

    uc_err err = Emulator.Run(Handler, Routine);
    if (err != UC_ERR_OK)
        return VMOpcode::NONE;

    // Compare result with known expected results with identical behaviour to VM instruction
    //
    uint64_t VIP;
    uc_reg_read(Emulator.UC(), ZydisUtils::ZydisRegToUCReg(Routine.RegVIP), &VIP);

    return VIP == TestValue ? Emulator.Return(VMOpcode::JUMP, Routine, Handler) : VMOpcode::NONE;
}
