#include <vm/vmstructs.h>
#include <vm/utils/zydisutils.h>
#include <vm/vmclassifier/classifierutils.h>
#include <unicorn/unicorn.h>
#include <loguru.hpp>

// CALL:
// mov     rbx, [vsp+0]
// add     vsp, 8
// call    rbx
//

VMOpcode call(const VMImage& Image, const VMRoutine& Routine, VMHandler& Handler) {
    VMClassifierEmulator Emulator(Image, Routine);
    if (!Emulator.Valid()) {
        LOG_F(ERROR, "Failed to setup emulator!");
        return VMOpcode::NONE;
    }

    // Populate test data on the virtual stack & run emulation
    //
    uint64_t VSP;
    uc_reg_read(Emulator.UC(), ZydisUtils::ZydisRegToUCReg(Routine.RegVSP), &VSP);

    uint64_t TestValue = 0xFF00000;
    uc_mem_write(Emulator.UC(), VSP, &TestValue, sizeof(TestValue));

    uint8_t Int3 = 0xCC;
    uc_mem_map(Emulator.UC(), TestValue, 0x1000, UC_PROT_EXEC | UC_PROT_READ);
    uc_mem_write(Emulator.UC(), TestValue, &Int3, sizeof(Int3));

    Emulator.Run(Handler, Routine);

    // Compare result with known expected results with identical behaviour to VM instruction
    //
    uint64_t RIP;
    uc_reg_read(Emulator.UC(), UC_X86_REG_RIP, &RIP);

    return RIP == TestValue ? Emulator.Return(VMOpcode::CALL, Routine, Handler) : VMOpcode::NONE;
}
