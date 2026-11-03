#include <vm/vmstructs.h>
#include <vm/utils/zydisutils.h>
#include <vm/vmclassifier/classifierutils.h>
#include <unicorn/unicorn.h>
#include <loguru.hpp>

// ADD_Q:
// mov     rbx, [vsp+0]
// mov     r9, [vsp+8]
// add     rbx, r9
// pushfq
// pop     r9
// mov     [vsp+8], rbx
// mov     [vsp+0], r9
//
// ADD_DW:
// mov     r8d, [vsp+0]
// mov     ebx, [vsp+4]
// sub     vsp, 4
// add     r8d, ebx
// pushfq
// pop     rbx
// mov     [vsp+8], r8d
// mov     [vsp+0], rbx
//
// ADD_W:
// mov     cx, [vsp+0]
// mov     r8w, [vsp+2]
// sub     vsp, 6
// add     cx, r8w
// pushfq
// pop     r8
// mov     [vsp+8], cx
// mov     [vsp+0], r8
//
// ADD_B:
// movzx   bx, byte ptr [vsp+0]
// mov     dil, [vsp+2]
// sub     vsp, 6
// add     bl, dil
// pushfq
// pop     rdi
// mov     [vsp+8], bx
// mov     [vsp+0], rdi
//

VMOpcode add(const VMImage& Image, const VMRoutine& Routine, VMHandler& Handler) {
    VMClassifierEmulator Emulator(Image, Routine);
    if (!Emulator.Valid()) {
        LOG_F(ERROR, "Failed to setup emulator!");
        return VMOpcode::NONE;
    }

    // Populate test data on the virtual stack & run emulation
    //
    uint64_t StartVSP;
    uc_reg_read(Emulator.UC(), ZydisUtils::ZydisRegToUCReg(Routine.RegVSP), &StartVSP);

    uint64_t TestValues[2] = {0x123456789ABCDEF, 0xFEDCBA987654321};
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
    uint64_t ActualResult;
    uc_mem_read(Emulator.UC(), EndVSP + 0x8, &ActualResult, sizeof(ActualResult));

    uint32_t* TestValuesDW = reinterpret_cast<uint32_t*>(TestValues);
    uint16_t* TestValuesW = reinterpret_cast<uint16_t*>(TestValues);

    uint64_t ExpectedResult_Q = TestValues[0] + TestValues[1];
    uint32_t ExpectedResult_DW = TestValuesDW[0] + TestValuesDW[1];
    uint16_t ExpectedResult_W = TestValuesW[0] + TestValuesW[1];
    uint8_t  ExpectedResult_B = static_cast<uint8_t>(TestValuesW[0]) + static_cast<uint8_t>(TestValuesW[1]);

    if (ActualResult == ExpectedResult_Q && VSPDelta == 0)
        return Emulator.Return(VMOpcode::ADD_64, Routine, Handler);
    else if (static_cast<uint32_t>(ActualResult) == ExpectedResult_DW && VSPDelta == -4)
        return Emulator.Return(VMOpcode::ADD_32, Routine, Handler);
    else if (static_cast<uint16_t>(ActualResult) == ExpectedResult_W && VSPDelta == -6)
        return Emulator.Return(VMOpcode::ADD_16, Routine, Handler);
    else if (static_cast<uint8_t>(ActualResult) == ExpectedResult_B && VSPDelta == -6)
        return Emulator.Return(VMOpcode::ADD_8, Routine, Handler);

    return VMOpcode::NONE;
}
