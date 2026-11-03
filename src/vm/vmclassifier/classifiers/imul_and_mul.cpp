#include <vm/vmstructs.h>
#include <vm/utils/zydisutils.h>
#include <vm/vmclassifier/classifierutils.h>
#include <unicorn/unicorn.h>
#include <loguru.hpp>
#include <iostream>

// IMUL_Q:
// mov     rdi, [vsp+0]
// mov     rax, [vsp+8]
// sub     vsp, 8
// imul    rdi
// pushfq
// pop     rdi
// mov     [vsp+8], rdx
// mov     [vsp+10h], rax
// mov     [vsp+0], rdi
//
// IMUL_DW:
// mov     ecx, [vsp+0]
// mov     eax, [vsp+4]
// sub     vsp, 8
// imul    ecx
// pushfq
// pop     rcx
// mov     [vsp+8], edx
// mov     [vsp+0Ch], eax
// mov     [vsp+0], rcx
//
// IMUL_W:
// mov     r10w, [vsp+0]
// mov     ax, [vsp+2]
// sub     vsp, 8
// imul    r10w
// pushfq
// pop     r10
// mov     [vsp+8], dx
// mov     [vsp+0Ah], ax
// mov     [vsp+0], r10
//

template <typename T> struct IMUL_Q {
    union {
        struct {
            T fac1;
            T fac2;
        };
        struct {
            int64_t flags;
            T       high;
            T       low;
        };
    };
};

template <typename T, typename UT, VMOpcode IMUL, VMOpcode MUL, int TargetVSPDelta>
static VMOpcode check_imul_or_mul(const VMImage& Image, const VMRoutine& Routine, VMHandler& Handler) {
    VMClassifierEmulator Emulator(Image, Routine);
    if (!Emulator.Valid()) {
        LOG_F(ERROR, "Failed to setup emulator!");
        return VMOpcode::NONE;
    }

    // Populate test data on the virtual stack & run emulation
    //
    uint64_t StartVSP;
    uc_reg_read(Emulator.UC(), ZydisUtils::ZydisRegToUCReg(Routine.RegVSP), &StartVSP);

    IMUL_Q<T> TestValues = {};
    TestValues.fac1 = -1;
    TestValues.fac2 = 2;

    T ExpectedSignedResult = TestValues.fac1 * TestValues.fac2;
    T ExpectedSignedHigh = ExpectedSignedResult >> (8 * sizeof(T));

    // These values will not overflow on an IMUL instruction, but they will on a MUL instruction
    // We will handle the overflow on MUL

    UT ExpectedUnsignedHigh;
    UT ExpectedUnsignedResult = _umul128(TestValues.fac1, TestValues.fac2, (uint64_t*)&ExpectedUnsignedHigh);

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
    uc_mem_read(Emulator.UC(), EndVSP, &TestValues, sizeof(TestValues));

    if (TestValues.low == ExpectedSignedResult && TestValues.high == ExpectedSignedHigh && VSPDelta == TargetVSPDelta) {
        return Emulator.Return(IMUL, Routine, Handler);
    } else if (TestValues.low == static_cast<T>(ExpectedUnsignedResult) &&
               TestValues.high == static_cast<T>(ExpectedUnsignedHigh) && VSPDelta == TargetVSPDelta) {
        return Emulator.Return(MUL, Routine, Handler);
    }

    return VMOpcode::NONE;
}

VMOpcode imul_and_mul(const VMImage& Image, const VMRoutine& Routine, VMHandler& Handler) {
    VMOpcode Op = check_imul_or_mul<int64_t, uint64_t, VMOpcode::IMUL_64, VMOpcode::MUL_64, -8>(Image, Routine, Handler);
    if (Op != VMOpcode::NONE)
        return Op;

    Op = check_imul_or_mul<int32_t, uint32_t, VMOpcode::IMUL_32, VMOpcode::MUL_32, -8>(Image, Routine, Handler);
    if (Op != VMOpcode::NONE)
        return Op;

    Op = check_imul_or_mul<int16_t, uint16_t, VMOpcode::IMUL_16, VMOpcode::MUL_16, -8>(Image, Routine, Handler);
    if (Op != VMOpcode::NONE)
        return Op;

    return VMOpcode::NONE;
}
