#include <vm/vmstructs.h>
#include <vm/utils/zydisutils.h>
#include <vm/vmclassifier/classifierutils.h>
#include <unicorn/unicorn.h>
#include <loguru.hpp>

// SHR_Q:
// mov     r9, [vsp+0]
// mov     cl, [vsp+8]
// sub     vsp, 6
// shr     r9, cl
// pushfq
// pop     rdi
// mov     [vsp+8], r9
// mov     [vsp+0], rdi
//
// SHR_DW:
// mov     edx, [vsp+0]
// mov     cl, [vsp+4]
// sub     vsp, 6
// shr     edx, cl
// pushfq
// pop     rdi
// mov     [vsp+8], edx
// mov     [vsp+0], rdi
//
// SHR_W:
// mov     r9w, [vsp+0]
// mov     cl, [vsp+2]
// sub     vsp, 6
// shr     r9w, cl
// pushfq
// pop     rdx
// mov     [vsp+8], r9w
// mov     [vsp+0], rdx
//
// SHR_B:
// movzx   ax, byte ptr [vsp+0]
// mov     cl, [vsp+2]
// sub     vsp, 6
// shr     al, cl
// pushfq
// pop     rdx
// mov     [vsp+8], ax
// mov     [vsp+0], rdx
//

VMOpcode shr(const VMImage& Image, const VMRoutine& Routine, VMHandler& Handler) {
    VMClassifierEmulator Emulator(Image, Routine);
    if (!Emulator.Valid()) {
        LOG_F(ERROR, "Failed to setup emulator!");
        return VMOpcode::NONE;
    }

    // Populate test data on the virtual stack & run emulation
    //
    uint64_t StartVSP;
    uc_reg_read(Emulator.UC(), ZydisUtils::ZydisRegToUCReg(Routine.RegVSP), &StartVSP);

    uint64_t TestValues[2] = {0x12345678901CDEF, 0x111111111111111};
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
    uint64_t ActualResult_Q;
    uc_mem_read(Emulator.UC(), EndVSP + 0x8, &ActualResult_Q, sizeof(ActualResult_Q));

    uint64_t Flags;
    uc_mem_read(Emulator.UC(), EndVSP, &Flags, sizeof(Flags));

    uint32_t* TestValuesDW = reinterpret_cast<uint32_t*>(TestValues);
    uint16_t* TestValuesW = reinterpret_cast<uint16_t*>(TestValues);

    uint64_t ExpectedResult_Q = TestValues[0] >> static_cast<uint8_t>(TestValues[1]);
    uint32_t ExpectedResult_DW = TestValuesDW[0] >> static_cast<uint8_t>(TestValuesDW[1]);
    uint16_t ExpectedResult_W = TestValuesW[0] >> static_cast<uint8_t>(TestValuesW[1]);
    uint8_t  ExpectedResult_B = static_cast<uint8_t>(TestValuesW[0]) >> static_cast<uint8_t>(TestValuesW[1]);

    if (ActualResult_Q == ExpectedResult_Q && VSPDelta == -6)
        return Emulator.Return(VMOpcode::SHR_64, Routine, Handler);
    else if (static_cast<uint32_t>(ActualResult_Q) == ExpectedResult_DW && VSPDelta == -6)
        return Emulator.Return(VMOpcode::SHR_32, Routine, Handler);
    else if (static_cast<uint16_t>(ActualResult_Q) == ExpectedResult_W && VSPDelta == -6)
        return Emulator.Return(VMOpcode::SHR_16, Routine, Handler);
    else if (static_cast<uint8_t>(ActualResult_Q) == ExpectedResult_B && VSPDelta == -6)
        return Emulator.Return(VMOpcode::SHR_8, Routine, Handler);

    return VMOpcode::NONE;
}
