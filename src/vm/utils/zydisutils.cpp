#include "zydisutils.h"

namespace ZydisUtils {
uc_x86_reg ZydisRegToUCReg(ZydisRegister Register) {
    switch (Register) {
    // 64-bit registers
    case ZYDIS_REGISTER_RAX:
        return UC_X86_REG_RAX;
    case ZYDIS_REGISTER_RCX:
        return UC_X86_REG_RCX;
    case ZYDIS_REGISTER_RDX:
        return UC_X86_REG_RDX;
    case ZYDIS_REGISTER_RBX:
        return UC_X86_REG_RBX;
    case ZYDIS_REGISTER_RSP:
        return UC_X86_REG_RSP;
    case ZYDIS_REGISTER_RBP:
        return UC_X86_REG_RBP;
    case ZYDIS_REGISTER_RSI:
        return UC_X86_REG_RSI;
    case ZYDIS_REGISTER_RDI:
        return UC_X86_REG_RDI;
    case ZYDIS_REGISTER_R8:
        return UC_X86_REG_R8;
    case ZYDIS_REGISTER_R9:
        return UC_X86_REG_R9;
    case ZYDIS_REGISTER_R10:
        return UC_X86_REG_R10;
    case ZYDIS_REGISTER_R11:
        return UC_X86_REG_R11;
    case ZYDIS_REGISTER_R12:
        return UC_X86_REG_R12;
    case ZYDIS_REGISTER_R13:
        return UC_X86_REG_R13;
    case ZYDIS_REGISTER_R14:
        return UC_X86_REG_R14;
    case ZYDIS_REGISTER_R15:
        return UC_X86_REG_R15;

    // 32-bit registers
    case ZYDIS_REGISTER_EAX:
        return UC_X86_REG_EAX;
    case ZYDIS_REGISTER_ECX:
        return UC_X86_REG_ECX;
    case ZYDIS_REGISTER_EDX:
        return UC_X86_REG_EDX;
    case ZYDIS_REGISTER_EBX:
        return UC_X86_REG_EBX;
    case ZYDIS_REGISTER_ESP:
        return UC_X86_REG_ESP;
    case ZYDIS_REGISTER_EBP:
        return UC_X86_REG_EBP;
    case ZYDIS_REGISTER_ESI:
        return UC_X86_REG_ESI;
    case ZYDIS_REGISTER_EDI:
        return UC_X86_REG_EDI;
    case ZYDIS_REGISTER_R8D:
        return UC_X86_REG_R8D;
    case ZYDIS_REGISTER_R9D:
        return UC_X86_REG_R9D;
    case ZYDIS_REGISTER_R10D:
        return UC_X86_REG_R10D;
    case ZYDIS_REGISTER_R11D:
        return UC_X86_REG_R11D;
    case ZYDIS_REGISTER_R12D:
        return UC_X86_REG_R12D;
    case ZYDIS_REGISTER_R13D:
        return UC_X86_REG_R13D;
    case ZYDIS_REGISTER_R14D:
        return UC_X86_REG_R14D;
    case ZYDIS_REGISTER_R15D:
        return UC_X86_REG_R15D;

    // 16-bit registers
    case ZYDIS_REGISTER_AX:
        return UC_X86_REG_AX;
    case ZYDIS_REGISTER_CX:
        return UC_X86_REG_CX;
    case ZYDIS_REGISTER_DX:
        return UC_X86_REG_DX;
    case ZYDIS_REGISTER_BX:
        return UC_X86_REG_BX;
    case ZYDIS_REGISTER_SP:
        return UC_X86_REG_SP;
    case ZYDIS_REGISTER_BP:
        return UC_X86_REG_BP;
    case ZYDIS_REGISTER_SI:
        return UC_X86_REG_SI;
    case ZYDIS_REGISTER_DI:
        return UC_X86_REG_DI;
    case ZYDIS_REGISTER_R8W:
        return UC_X86_REG_R8W;
    case ZYDIS_REGISTER_R9W:
        return UC_X86_REG_R9W;
    case ZYDIS_REGISTER_R10W:
        return UC_X86_REG_R10W;
    case ZYDIS_REGISTER_R11W:
        return UC_X86_REG_R11W;
    case ZYDIS_REGISTER_R12W:
        return UC_X86_REG_R12W;
    case ZYDIS_REGISTER_R13W:
        return UC_X86_REG_R13W;
    case ZYDIS_REGISTER_R14W:
        return UC_X86_REG_R14W;
    case ZYDIS_REGISTER_R15W:
        return UC_X86_REG_R15W;

    // 8-bit registers (low)
    case ZYDIS_REGISTER_AL:
        return UC_X86_REG_AL;
    case ZYDIS_REGISTER_CL:
        return UC_X86_REG_CL;
    case ZYDIS_REGISTER_DL:
        return UC_X86_REG_DL;
    case ZYDIS_REGISTER_BL:
        return UC_X86_REG_BL;
    case ZYDIS_REGISTER_SPL:
        return UC_X86_REG_SPL;
    case ZYDIS_REGISTER_BPL:
        return UC_X86_REG_BPL;
    case ZYDIS_REGISTER_SIL:
        return UC_X86_REG_SIL;
    case ZYDIS_REGISTER_DIL:
        return UC_X86_REG_DIL;
    case ZYDIS_REGISTER_R8B:
        return UC_X86_REG_R8B;
    case ZYDIS_REGISTER_R9B:
        return UC_X86_REG_R9B;
    case ZYDIS_REGISTER_R10B:
        return UC_X86_REG_R10B;
    case ZYDIS_REGISTER_R11B:
        return UC_X86_REG_R11B;
    case ZYDIS_REGISTER_R12B:
        return UC_X86_REG_R12B;
    case ZYDIS_REGISTER_R13B:
        return UC_X86_REG_R13B;
    case ZYDIS_REGISTER_R14B:
        return UC_X86_REG_R14B;
    case ZYDIS_REGISTER_R15B:
        return UC_X86_REG_R15B;

    // 8-bit registers (high)
    case ZYDIS_REGISTER_AH:
        return UC_X86_REG_AH;
    case ZYDIS_REGISTER_CH:
        return UC_X86_REG_CH;
    case ZYDIS_REGISTER_DH:
        return UC_X86_REG_DH;
    case ZYDIS_REGISTER_BH:
        return UC_X86_REG_BH;

    default:
        DebugBreak();
        return UC_X86_REG_INVALID;
    }
}
} // namespace ZydisUtils
