#include <vm/vmstructs.h>
#include <vm/utils/zydisutils.h>
#include <vm/vmclassifier/classifierutils.h>
#include <unicorn/unicorn.h>
#include <loguru.hpp>

// PUSH_REG_Q:
// movzx   edi, byte ptr [vip]
// add     vip, 1
// mov     rcx, [rsp+rdi+0]
// sub     vsp, 8
// mov     [rbp+0], rcx
//
// PUSH_REG_DW:
// movzx   ebx, byte ptr [vip]
// add     vip, 1
// mov     cx, [rsp+rbx+0]
// sub     vsp, 2
// mov     [vsp+0], cx
//
// PUSH_REG_W:
// movzx   eax, word ptr [vip]
// add     vip, 1
// mov     bx, [rsp+rax+0]
// sub     vsp, 2
// mov     [vsp+0], bx
//
// PUSH_REG_B:
// movzx   r10d, byte ptr [vip]
// add     vip, 1
// movzx   r8w, byte ptr [rsp+r10+0]
// sub     vsp, 2
// mov     [vsp+0], r8w
//

VMOpcode PUSH_REG(const VMImage& Image, const VMRoutine& Routine, VMHandler& Handler) {
    VMClassifierEmulator Emulator(Image, Routine);
    if (!Emulator.Valid()) {
        LOG_F(ERROR, "Failed to setup emulator!");
        return VMOpcode::NONE;
    }

    uint64_t StartVSP;
    uc_reg_read(Emulator.UC(), ZydisUtils::ZydisRegToUCReg(Routine.RegVSP), &StartVSP);

    constexpr uint64_t VIPAddress = 0x1FF000;
    uc_mem_map(Emulator.UC(), VIPAddress, 0x1000, UC_PROT_READ);
    uc_reg_write(Emulator.UC(), ZydisUtils::ZydisRegToUCReg(Routine.RegVIP), &VIPAddress);

    uint64_t VREG;
    uc_reg_read(Emulator.UC(), ZydisUtils::ZydisRegToUCReg(Routine.RegVREG), &VREG);

    uint64_t TestValue = 0x123456789ABCDEF;
    uc_mem_write(Emulator.UC(), VREG, &TestValue, sizeof(TestValue));

    uc_err err = Emulator.Run(Handler, Routine);
    if (err != UC_ERR_OK)
        return VMOpcode::NONE;

    uint64_t EndVSP;
    uc_reg_read(Emulator.UC(), ZydisUtils::ZydisRegToUCReg(Routine.RegVSP), &EndVSP);
    int VSPDelta = EndVSP - StartVSP;

    uint64_t ActualResult;
    uc_mem_read(Emulator.UC(), EndVSP, &ActualResult, sizeof(ActualResult));

    if (ActualResult == TestValue && VSPDelta == -8)
        return Emulator.Return(VMOpcode::PUSH_VMREG_64, Routine, Handler);
    else if (ActualResult == static_cast<uint32_t>(TestValue) && VSPDelta == -4)
        return Emulator.Return(VMOpcode::PUSH_VMREG_32, Routine, Handler);
    else if (ActualResult == static_cast<uint16_t>(TestValue) && VSPDelta == -2)
        return Emulator.Return(VMOpcode::PUSH_VMREG_16, Routine, Handler);
    else if (ActualResult == static_cast<uint8_t>(TestValue) && VSPDelta == -2)
        return Emulator.Return(VMOpcode::PUSH_VMREG_8, Routine, Handler);

    return VMOpcode::NONE;
}
