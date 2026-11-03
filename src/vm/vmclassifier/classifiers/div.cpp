#include <vm/vmstructs.h>
#include <vm/utils/zydisutils.h>
#include <vm/vmclassifier/classifierutils.h>
#include <unicorn/unicorn.h>
#include <loguru.hpp>

// DIV_Q:
// mov     rdx, [vsp+0]
// mov     rax, [vsp+8]
// mov     r9, [vsp+10h]
// add     vsp, 8
// div     r9
// mov     [vsp+0], rdx
// mov     [vsp+8], rax
//

VMOpcode _div(const VMImage& Image, const VMRoutine& Routine, VMHandler& Handler) {
    VMClassifierEmulator Emulator(Image, Routine);
    if (!Emulator.Valid()) {
        LOG_F(ERROR, "Failed to setup emulator!");
        return VMOpcode::NONE;
    }

    // Populate test data on the virtual stack & run emulation
    //
    uint64_t StartVSP;
    uc_reg_read(Emulator.UC(), ZydisUtils::ZydisRegToUCReg(Routine.RegVSP), &StartVSP);

    uint64_t TestValues[3] = {0, 0x1000, 0x101};
    uc_mem_write(Emulator.UC(), StartVSP, &TestValues, sizeof(TestValues));

    uc_err err = Emulator.Run(Handler, Routine);
    if (err != UC_ERR_OK)
        return VMOpcode::NONE;

    // Get VSP delta
    //
    uint64_t EndVSP;
    uc_reg_read(Emulator.UC(), ZydisUtils::ZydisRegToUCReg(Routine.RegVSP), &EndVSP);
    int VSPDelta = EndVSP - StartVSP;

    // Compare result with known expected results with identical behaviour to VM instruction
    //
    uint64_t ActualResult[2];
    uc_mem_read(Emulator.UC(), EndVSP, &ActualResult, sizeof(ActualResult));

    uint64_t ExpectedResult_Q = TestValues[1] / TestValues[2];

    if (ActualResult[1] == ExpectedResult_Q && VSPDelta == 8)
        return Emulator.Return(VMOpcode::DIV_64, Routine, Handler);

    return VMOpcode::NONE;
}
