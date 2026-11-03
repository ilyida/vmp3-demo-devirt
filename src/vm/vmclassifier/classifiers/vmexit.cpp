#include <vm/vmstructs.h>
#include <vm/utils/zydisutils.h>
#include <vm/vmclassifier/classifierutils.h>
#include <unicorn/unicorn.h>
#include <loguru.hpp>

// VMEXIT:
// mov     rsp, vsp
// pop     rax
// pop     rcx
// pop     rdx
// pop     rbx
// pop     rsi
// pop     rdi
// pop     rsp
// pop     rbp
// pop     r8
// pop     r9
// pop     r10
// pop     r11
// pop     r12
// pop     r13
// pop     r14
// pop     r15
// popfq
// ret
//

VMOpcode vmexit(const VMImage& Image, const VMRoutine& Routine, VMHandler& Handler) {
    VMClassifierEmulator Emulator(Image, Routine);
    if (!Emulator.Valid()) {
        LOG_F(ERROR, "Failed to setup emulator!");
        return VMOpcode::NONE;
    }

    uint64_t VSP;
    uc_reg_read(Emulator.UC(), ZydisUtils::ZydisRegToUCReg(Routine.RegVSP), &VSP);

    // Emulator will fail on ret so we cannot check for success
    //
    Emulator.Run(Handler, Routine);

    uint64_t RSP;
    uc_reg_read(Emulator.UC(), UC_X86_REG_RSP, &RSP);

    // Account for 16 restored registers and one extra 64-bit int from ret instructions implicit add rsp, 8
    //
    return (VSP + 0x88) == RSP ? Emulator.Return(VMOpcode::EXIT, Routine, Handler) : VMOpcode::NONE;
}
