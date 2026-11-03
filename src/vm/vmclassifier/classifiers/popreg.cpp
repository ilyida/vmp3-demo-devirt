#include <vm/vmstructs.h>
#include <vm/utils/zydisutils.h>
#include <vm/vmclassifier/classifierutils.h>
#include <unicorn/unicorn.h>
#include <loguru.hpp>

// POP_REG_Q:
// mov     rbx, [vsp+0]
// add     vsp, 8
// movzx   ecx, word ptr [vip]
// add     vip, 2
// mov     [rsp+rcx+0], rbx
//
// POP_REG_DW:
// mov     eax, [vsp+0]
// add     vsp, 4
// movzx   r10d, byte ptr [vip]
// add     vip, 1
// mov     [rsp+r10+0], eax
//
// POP_REG_W:
// mov     r10w, [vsp+0]
// add     vsp, 2
// movzx   ecx, byte ptr [vip]
// add     vip, 1
// mov     [rsp+rcx+0], r10w
//
// POP_REG_B:
// mov     cx, [vsp+0]
// add     vsp, 2
// movzx   eax, byte ptr [vip]
// add     vip, 1
// mov     [rsp+rax+0], cl
//

VMOpcode POP_REG(const VMImage& Image, const VMRoutine& Routine, VMHandler& Handler) {
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
    uc_reg_write(Emulator.UC(), ZydisUtils::ZydisRegToUCReg(Routine.RegVIP), &VIPAddress);
    uc_mem_write(Emulator.UC(), StartVSP, &TestValue, sizeof(TestValue));

    uc_err err = Emulator.Run(Handler, Routine);
    if (err != UC_ERR_OK)
        return VMOpcode::NONE;

    uint64_t EndVSP;
    uc_reg_read(Emulator.UC(), ZydisUtils::ZydisRegToUCReg(Routine.RegVSP), &EndVSP);
    int VSPDelta = EndVSP - StartVSP;

    uint64_t VREG;
    uc_reg_read(Emulator.UC(), ZydisUtils::ZydisRegToUCReg(Routine.RegVREG), &VREG);

    uint64_t ActualResult;
    uc_mem_read(Emulator.UC(), VREG, &ActualResult, sizeof(ActualResult));

    if (ActualResult == TestValue && VSPDelta == 8)
        return Emulator.Return(VMOpcode::POP_VMREG_64, Routine, Handler);
    else if (ActualResult == static_cast<uint32_t>(TestValue) && VSPDelta == 4)
        return Emulator.Return(VMOpcode::POP_VMREG_32, Routine, Handler);
    else if (ActualResult == static_cast<uint16_t>(TestValue) && VSPDelta == 2)
        return Emulator.Return(VMOpcode::POP_VMREG_16, Routine, Handler);
    else if (ActualResult == static_cast<uint8_t>(TestValue) && VSPDelta == 2)
        return Emulator.Return(VMOpcode::POP_VMREG_8, Routine, Handler);

    return VMOpcode::NONE;
}
