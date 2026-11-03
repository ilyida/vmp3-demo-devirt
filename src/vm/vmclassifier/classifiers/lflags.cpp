#include <vm/vmstructs.h>
#include <vm/utils/zydisutils.h>
#include <vm/vmclassifier/classifierutils.h>
#include <unicorn/unicorn.h>
#include <loguru.hpp>

// LFLAGS_Q:
// mov     r8, [vsp+0]
// add     vsp, 8
// push    r8
// popfq
//

VMOpcode lflags(const VMImage& Image, const VMRoutine& Routine, VMHandler& Handler) {
    VMClassifierEmulator Emulator(Image, Routine);
    if (!Emulator.Valid()) {
        LOG_F(ERROR, "Failed to setup emulator!");
        return VMOpcode::NONE;
    }

    uint64_t StartVSP;
    uc_reg_read(Emulator.UC(), ZydisUtils::ZydisRegToUCReg(Routine.RegVSP), &StartVSP);

    uint64_t TestValue = 0x00000202;
    uc_mem_write(Emulator.UC(), StartVSP, &TestValue, sizeof(TestValue));

    uc_err err = Emulator.Run(Handler, Routine);
    if (err != UC_ERR_OK)
        return VMOpcode::NONE;

    uint64_t EndVSP;
    uc_reg_read(Emulator.UC(), ZydisUtils::ZydisRegToUCReg(Routine.RegVSP), &EndVSP);
    int VSPDelta = EndVSP - StartVSP;

    uint64_t ActualResult;
    uc_reg_read(Emulator.UC(), UC_X86_REG_RFLAGS, &ActualResult);

    if (ActualResult == TestValue && VSPDelta == 8)
        return Emulator.Return(VMOpcode::LFLAGS_64, Routine, Handler);

    return VMOpcode::NONE;
}
