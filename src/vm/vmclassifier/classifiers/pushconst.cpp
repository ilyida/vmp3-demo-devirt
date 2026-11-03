#include <vm/vmstructs.h>
#include <vm/utils/zydisutils.h>
#include <vm/vmclassifier/classifierutils.h>
#include <unicorn/unicorn.h>
#include <loguru.hpp>

// LCONST_Q:
// mov     r8, [vip]
// add     vip, 8
// sub     vsp, 8
// mov     [vsp+0], r8
//
// LCONST_DW:
// mov     ecx, [vip]
// add     vip, 4
// sub     vsp, 4
// mov     [vsp+0], ecx
//
// LCONST_W:
// movzx   r10d, word ptr [vip]
// add     vip, 2
// sub     vsp, 2
// mov     [vsp+0], r10w
//
// LCONST_B:
// movzx   eax, byte ptr [vip]
// add     vip, 1
// sub     vsp, 2
// mov     [vsp+0], ax
//

VMOpcode pushconst(const VMImage& Image, const VMRoutine& Routine, VMHandler& Handler) {
    VMClassifierEmulator Emulator(Image, Routine);
    if (!Emulator.Valid()) {
        LOG_F(ERROR, "Failed to setup emulator!");
        return VMOpcode::NONE;
    }

    uint64_t StartVSP;
    uc_reg_read(Emulator.UC(), ZydisUtils::ZydisRegToUCReg(Routine.RegVSP), &StartVSP);

    constexpr uint64_t VIPAddress = 0x1FF000;
    uc_mem_map(Emulator.UC(), VIPAddress, 0x1000, UC_PROT_READ);

    uint64_t TestValue = 0x123456789ABCDEF;
    uc_mem_write(Emulator.UC(), VIPAddress, &TestValue, sizeof(TestValue));
    uc_reg_write(Emulator.UC(), ZydisUtils::ZydisRegToUCReg(Routine.RegVIP), &VIPAddress);

    uc_err err = Emulator.Run(Handler, Routine);
    if (err != UC_ERR_OK)
        return VMOpcode::NONE;

    uint64_t EndVSP;
    uc_reg_read(Emulator.UC(), ZydisUtils::ZydisRegToUCReg(Routine.RegVSP), &EndVSP);
    int VSPDelta = EndVSP - StartVSP;

    uint64_t ActualResult;
    uc_mem_read(Emulator.UC(), EndVSP, &ActualResult, sizeof(ActualResult));

    if (ActualResult == TestValue && VSPDelta == -8)
        return Emulator.Return(VMOpcode::PUSH_IMM_64, Routine, Handler);
    else if (ActualResult == static_cast<uint32_t>(TestValue) && VSPDelta == -4)
        return Emulator.Return(VMOpcode::PUSH_IMM_32, Routine, Handler);
    else if (ActualResult == static_cast<uint16_t>(TestValue) && VSPDelta == -2)
        return Emulator.Return(VMOpcode::PUSH_IMM_16, Routine, Handler);
    else if (ActualResult == static_cast<uint8_t>(TestValue) && VSPDelta == -2)
        return Emulator.Return(VMOpcode::PUSH_IMM_8, Routine, Handler);

    return VMOpcode::NONE;
}
