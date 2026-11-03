#include <vm/vmstructs.h>
#include <vm/utils/zydisutils.h>
#include <vm/vmclassifier/classifierutils.h>
#include <unicorn/unicorn.h>
#include <loguru.hpp>

// READ_Q:
// mov     r9, [vsp+0]
// mov     rbx, [r9]
// mov     [vsp+0], rbx
//
// READ_DW:
// mov     rcx, [vsp+0]
// mov     r9d, [rcx]
// add     vsp, 4
// mov     [vsp+0], r9d
//
// READ_W:
// mov     rdi, [vsp+0]
// mov     bx, [rdi]
// add     vsp, 6
// mov     [vsp+0], bx
//
// READ_B:
// mov     r9, [vsp+0]
// movzx   ax, byte ptr [r9]
// add     vsp, 6
// mov     [vsp+0], ax
//

VMOpcode read(const VMImage& Image, const VMRoutine& Routine, VMHandler& Handler) {
    VMClassifierEmulator Emulator(Image, Routine);
    if (!Emulator.Valid()) {
        LOG_F(ERROR, "Failed to setup emulator!");
        return VMOpcode::NONE;
    }

    // Avoid clashing with READGS
    //
    uint64_t GS = 0x123456789ABCDEF;
    uc_reg_write(Emulator.UC(), UC_X86_REG_GS_BASE, &GS);

    uint64_t StartVSP;
    uc_reg_read(Emulator.UC(), ZydisUtils::ZydisRegToUCReg(Routine.RegVSP), &StartVSP);

    constexpr uint64_t DataAddress = 0x1FF000;
    uc_mem_map(Emulator.UC(), DataAddress, 0x1000, UC_PROT_READ);

    uint64_t TestValue = 0x123456789ABCDEF;
    uc_mem_write(Emulator.UC(), DataAddress, &TestValue, sizeof(TestValue));
    uc_mem_write(Emulator.UC(), StartVSP, &DataAddress, sizeof(DataAddress));

    uc_err err = Emulator.Run(Handler, Routine);
    if (err != UC_ERR_OK)
        return VMOpcode::NONE;

    uint64_t EndVSP;
    uc_reg_read(Emulator.UC(), ZydisUtils::ZydisRegToUCReg(Routine.RegVSP), &EndVSP);
    int VSPDelta = EndVSP - StartVSP;

    uint64_t ActualResult;
    uc_mem_read(Emulator.UC(), EndVSP, &ActualResult, sizeof(ActualResult));

    if (ActualResult == TestValue && VSPDelta == 0)
        return Emulator.Return(VMOpcode::LOAD_64, Routine, Handler);
    else if (ActualResult == static_cast<uint32_t>(TestValue) && VSPDelta == 4)
        return Emulator.Return(VMOpcode::LOAD_32, Routine, Handler);
    else if (ActualResult == static_cast<uint16_t>(TestValue) && VSPDelta == 6)
        return Emulator.Return(VMOpcode::LOAD_16, Routine, Handler);
    else if (ActualResult == static_cast<uint8_t>(TestValue) && VSPDelta == 6)
        return Emulator.Return(VMOpcode::LOAD_8, Routine, Handler);

    return VMOpcode::NONE;
}
