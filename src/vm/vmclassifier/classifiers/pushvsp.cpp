#include <vm/vmstructs.h>
#include <vm/utils/zydisutils.h>
#include <vm/vmclassifier/classifierutils.h>
#include <unicorn/unicorn.h>
#include <loguru.hpp>

// PUSHVSP_Q:
// mov     rdi, vsp
// sub     vsp, 8
// mov     [vsp+0], rdi
//
// PUSHVSP_DW:
// mov     rax, vsp
// sub     vsp, 4
// mov     [vsp+0], eax
//
// PUSHVSP_W:
// mov     rbx, vsp
// sub     vsp, 2
// mov     [vsp+0], bx
//
// PUSHVSP_B: (unfound, just a guess)
// mov     rbx, vsp
// sub     vsp, 2
// mov     [vsp+0], bl
//

VMOpcode pushvsp(const VMImage& Image, const VMRoutine& Routine, VMHandler& Handler) {
    VMClassifierEmulator Emulator(Image, Routine);
    if (!Emulator.Valid()) {
        LOG_F(ERROR, "Failed to setup emulator!");
        return VMOpcode::NONE;
    }

    uint64_t StartVSP;
    uc_reg_read(Emulator.UC(), ZydisUtils::ZydisRegToUCReg(Routine.RegVSP), &StartVSP);

    uc_err err = Emulator.Run(Handler, Routine);
    if (err != UC_ERR_OK)
        return VMOpcode::NONE;

    uint64_t EndVSP;
    uc_reg_read(Emulator.UC(), ZydisUtils::ZydisRegToUCReg(Routine.RegVSP), &EndVSP);
    int VSPDelta = EndVSP - StartVSP;

    uint64_t ActualResult;
    uc_mem_read(Emulator.UC(), EndVSP, &ActualResult, sizeof(ActualResult));

    if (ActualResult == StartVSP && VSPDelta == -8)
        return Emulator.Return(VMOpcode::PUSH_VSP_64, Routine, Handler);
    else if (ActualResult == static_cast<uint32_t>(StartVSP) && VSPDelta == -4)
        return Emulator.Return(VMOpcode::PUSH_VSP_32, Routine, Handler);
    else if (ActualResult == static_cast<uint16_t>(StartVSP) && VSPDelta == -2)
        return Emulator.Return(VMOpcode::PUSH_VSP_16, Routine, Handler);
    else if (ActualResult == static_cast<uint8_t>(StartVSP) && VSPDelta == -2)
        return Emulator.Return(VMOpcode::PUSH_VSP_8, Routine, Handler);

    return VMOpcode::NONE;
}
