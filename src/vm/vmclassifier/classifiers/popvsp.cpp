#include <vm/vmstructs.h>
#include <vm/utils/zydisutils.h>
#include <vm/vmclassifier/classifierutils.h>
#include <unicorn/unicorn.h>
#include <loguru.hpp>

// POPVSP_Q:
// mov     vsp, [vsp+0]
//
// POPVSP_DW:
// mov     ebp, [vsp+0]
//
// POPVSP_W:
// mov     bp, [vsp+0]
//
// POPVSP_B: (unfound, just a guess)
// mov     bl, [vsp+0]
//

VMOpcode popvsp(const VMImage& Image, const VMRoutine& Routine, VMHandler& Handler) {
    VMClassifierEmulator Emulator(Image, Routine);
    if (!Emulator.Valid()) {
        LOG_F(ERROR, "Failed to setup emulator!");
        return VMOpcode::NONE;
    }

    uint64_t StartVSP;
    uc_reg_read(Emulator.UC(), ZydisUtils::ZydisRegToUCReg(Routine.RegVSP), &StartVSP);

    uint64_t TestValue = 0x123456789ABCDEF;
    uc_mem_write(Emulator.UC(), StartVSP, &TestValue, sizeof(TestValue));

    uc_err err = Emulator.Run(Handler, Routine);
    if (err != UC_ERR_OK)
        return VMOpcode::NONE;

    uint64_t EndVSP;
    uc_reg_read(Emulator.UC(), ZydisUtils::ZydisRegToUCReg(Routine.RegVSP), &EndVSP);

    if (EndVSP == TestValue)
        return Emulator.Return(VMOpcode::POP_VSP_64, Routine, Handler);
    else if (static_cast<uint32_t>(EndVSP) == static_cast<uint32_t>(TestValue))
        return Emulator.Return(VMOpcode::POP_VSP_32, Routine, Handler);
    else if (static_cast<uint16_t>(EndVSP) == static_cast<uint16_t>(TestValue))
        return Emulator.Return(VMOpcode::POP_VSP_16, Routine, Handler);
    else if (static_cast<uint8_t>(EndVSP) == static_cast<uint8_t>(TestValue))
        return Emulator.Return(VMOpcode::POP_VSP_8, Routine, Handler);

    return VMOpcode::NONE;
}
