#include <vm/vmstructs.h>
#include <vm/utils/zydisutils.h>
#include <vm/vmclassifier/classifierutils.h>
#include <unicorn/unicorn.h>
#include <loguru.hpp>

// SHRD_Q:
// mov     r10, [vsp+0]
// mov     rdi, [vsp+8]
// mov     cl, [vsp+10h]
// add     vsp, 2
// shrd    r10, rdi, cl
// pushfq
// pop     rdi
// mov     [vsp+8], r10
// mov     [vsp+0], rdi
//
// SHRD_DW:
// mov     r10d, [vsp+0]
// mov     edx, [vsp+4]
// mov     cl, [vsp+8]
// sub     vsp, 2
// shrd    r10d, edx, cl
// pushfq
// pop     rdx
// mov     [vsp+8], r10d
// mov     [vsp+0], rdx
//
// SHRD_W:
// mov     di, [vsp+0]
// mov     dx, [vsp+2]
// mov     cl, [vsp+4]
// sub     vsp, 4
// shrd    di, dx, cl
// pushfq
// pop     rdx
// mov     [vsp+8], di
// mov     [vsp+0], rdx
//

template <typename T> T ShrdIntrin(T x, T y, unsigned n) {
    return (x >> n) | (y << (std::numeric_limits<T>::digits - n));
}

VMOpcode shrd(const VMImage& Image, const VMRoutine& Routine, VMHandler& Handler) {
    VMClassifierEmulator Emulator(Image, Routine);
    if (!Emulator.Valid()) {
        LOG_F(ERROR, "Failed to setup emulator!");
        return VMOpcode::NONE;
    }

    // Populate test data on the virtual stack & run emulation
    //
    uint64_t StartVSP;
    uc_reg_read(Emulator.UC(), ZydisUtils::ZydisRegToUCReg(Routine.RegVSP), &StartVSP);

    uint64_t TestValues[3] = {0x12345678901CDEF, 0x1212343456567878, 0x2222222222222222};
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

    uint64_t ExpectedResult_Q = ShrdIntrin(TestValues[0], TestValues[1], static_cast<uint8_t>(TestValues[2]));
    uint32_t ExpectedResult_DW = ShrdIntrin(TestValuesDW[0], TestValuesDW[1], static_cast<uint8_t>(TestValuesDW[2]));
    uint16_t ExpectedResult_W = ShrdIntrin(TestValuesW[0], TestValuesW[1], static_cast<uint8_t>(TestValuesW[2]));

    if (ActualResult_Q == ExpectedResult_Q && VSPDelta == 2)
        return Emulator.Return(VMOpcode::SHRD_64, Routine, Handler);
    else if (static_cast<uint32_t>(ActualResult_Q) == ExpectedResult_DW && VSPDelta == -2)
        return Emulator.Return(VMOpcode::SHRD_32, Routine, Handler);
    else if (static_cast<uint16_t>(ActualResult_Q) == ExpectedResult_W && VSPDelta == -4)
        return Emulator.Return(VMOpcode::SHRD_16, Routine, Handler);

    return VMOpcode::NONE;
}
