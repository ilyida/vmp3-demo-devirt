#pragma once
#include <vector>
#include <array>
#include <string>
#include <unordered_map>

#include <Windows.h>

#include <Zydis/Zydis.h>
#include <unicorn/unicorn.h>

enum class VMOpcode {
#define EXPAND_VM_OPCODES(X)                                                                                           \
    X(NONE)                                                                                                            \
                                                                                                                       \
    /* Register operations */                                                                                          \
    X(POP_VMREG_64)                                                                                                    \
    X(POP_VMREG_32)                                                                                                    \
    X(POP_VMREG_16)                                                                                                    \
    X(POP_VMREG_8)                                                                                                     \
                                                                                                                       \
    X(PUSH_VMREG_64)                                                                                                   \
    X(PUSH_VMREG_32)                                                                                                   \
    X(PUSH_VMREG_16)                                                                                                   \
    X(PUSH_VMREG_8)                                                                                                    \
                                                                                                                       \
    /* Register operations */                                                                                          \
    X(POP_REG)                                                                                                          \
    X(PUSH_REG)                                                                                                         \
                                                                                                                       \
    /* Constant operations */                                                                                          \
    X(PUSH_IMM_64)                                                                                                     \
    X(PUSH_IMM_32)                                                                                                     \
    X(PUSH_IMM_16)                                                                                                     \
    X(PUSH_IMM_8)                                                                                                      \
                                                                                                                       \
    /* Stack operations */                                                                                             \
    X(PUSH_VSP_64)                                                                                                     \
    X(PUSH_VSP_32)                                                                                                     \
    X(PUSH_VSP_16)                                                                                                     \
    X(PUSH_VSP_8)                                                                                                      \
                                                                                                                       \
    X(POP_VSP_64)                                                                                                      \
    X(POP_VSP_32)                                                                                                      \
    X(POP_VSP_16)                                                                                                      \
    X(POP_VSP_8)                                                                                                       \
                                                                                                                       \
    /* Arithmetic operations */                                                                                        \
    X(ADD_64)                                                                                                          \
    X(ADD_32)                                                                                                          \
    X(ADD_16)                                                                                                          \
    X(ADD_8)                                                                                                           \
                                                                                                                       \
    X(SUB_64)                                                                                                          \
    X(SUB_32)                                                                                                          \
    X(SUB_16)                                                                                                          \
    X(SUB_8)                                                                                                           \
                                                                                                                       \
    X(DIV_64)                                                                                                          \
    X(DIV_32)                                                                                                          \
    X(DIV_16)                                                                                                          \
    X(DIV_8)                                                                                                           \
                                                                                                                       \
    X(IMUL_64)                                                                                                         \
    X(IMUL_32)                                                                                                         \
    X(IMUL_16)                                                                                                         \
                                                                                                                       \
    X(MUL_64)                                                                                                          \
    X(MUL_32)                                                                                                          \
    X(MUL_16)                                                                                                          \
                                                                                                                       \
    /* Bitwise operations */                                                                                           \
    X(NOR_64)                                                                                                          \
    X(NOR_32)                                                                                                          \
    X(NOR_16)                                                                                                          \
    X(NOR_8)                                                                                                           \
                                                                                                                       \
    X(NAND_64)                                                                                                         \
    X(NAND_32)                                                                                                         \
    X(NAND_16)                                                                                                         \
    X(NAND_8)                                                                                                          \
                                                                                                                       \
    /* Shift operations */                                                                                             \
    X(SHR_64)                                                                                                          \
    X(SHR_32)                                                                                                          \
    X(SHR_16)                                                                                                          \
    X(SHR_8)                                                                                                           \
                                                                                                                       \
    X(SHL_64)                                                                                                          \
    X(SHL_32)                                                                                                          \
    X(SHL_16)                                                                                                          \
    X(SHL_8)                                                                                                           \
                                                                                                                       \
    X(SHRD_64)                                                                                                         \
    X(SHRD_32)                                                                                                         \
    X(SHRD_16)                                                                                                         \
                                                                                                                       \
    X(SHLD_64)                                                                                                         \
    X(SHLD_32)                                                                                                         \
    X(SHLD_16)                                                                                                         \
                                                                                                                       \
    /* Memory operations */                                                                                            \
    X(LOAD_64)                                                                                                         \
    X(LOAD_32)                                                                                                         \
    X(LOAD_16)                                                                                                         \
    X(LOAD_8)                                                                                                          \
                                                                                                                       \
    X(READGS_64)                                                                                                       \
    X(READGS_32)                                                                                                       \
    X(READGS_16)                                                                                                       \
    X(READGS_8)                                                                                                        \
                                                                                                                       \
    X(STORE_64)                                                                                                        \
    X(STORE_32)                                                                                                        \
    X(STORE_16)                                                                                                        \
    X(STORE_8)                                                                                                         \
                                                                                                                       \
    /* Special operations */                                                                                           \
    X(LFLAGS_64)                                                                                                       \
                                                                                                                       \
    /* Control flow */                                                                                                 \
    X(CALL)                                                                                                            \
    X(JUMP)                                                                                                            \
    X(RDTSC)                                                                                                           \
    X(CPUID)                                                                                                           \
    X(EXIT)

// Define the enum values
#define ENUM_CASE(name) name,
    EXPAND_VM_OPCODES(ENUM_CASE)
#undef ENUM_CASE
};

enum class VMRegister {
    NONE = 0,
    VREG0,
    VREG1,
    VREG2,
    VREG3,
    VREG4,
    VREG5,
    VREG6,
    VREG7,
    VREG8,
    VREG9,
    VREG10,
    VREG11,
    VREG12,
    VREG13,
    VREG14,
    VREG15,
    VREG16,
    VREG17,
    VREG18,
    VREG19,
    VREG20,
    VREG21,
    VREG22,
    VREG23,
    VREG24,
    VREG25,
    VREG26,
    VREG27,
    VREG28,
    VREG29,
};

enum class VMOperandType { NONE = 0, reg, vreg, imm };

struct VMIns {
    VMOpcode      Opcode = VMOpcode::NONE;
    VMOperandType OperandType = VMOperandType::NONE;
    std::string   Text;
    uint8_t       OperandRawSize = 0;
    union {
        struct {
            int Value;
        } reg;
        struct {
            VMRegister Value;
        } vreg;
        struct {
            size_t Size;
            union {
                uint64_t u64;
                uint32_t u32;
                uint16_t u16;
                uint8_t  u8;
            };
        } imm;
    };
};

struct VMEdge {
    bool     IsValid = false;
    uint64_t Value;
};

struct VMBlock {
    uint64_t           Address;
    std::vector<VMIns> Instructions;

    std::vector<uint64_t> PotentialEdges;

    //VMEdge             Unconditional;
    //VMEdge             Conditional;
};

struct VMStack {
    uint64_t VMArgs[17]; // Contains all standard modifiable registers as well as flags register and a few other things
    uint8_t  Stack[0xF78];
    uint8_t  VirtualStack[0x1000];
};

struct VMHandler {
    uint64_t                                  RuntimeAddress;
    std::vector<ZydisDisassembledInstruction> Trace;
    VMOpcode                                  Opcode;
    int                                       VIPDelta;
};

struct VMRoutine {
    uint64_t RuntimeAddress;
    uint64_t VIPAddress;
    uint64_t FDEAddress;
    uint64_t HTAddress;

    std::array<VMHandler, 256> Handlers;

    static constexpr int             REL_ADDR = -1;        // Used to signify REL_ADDR param
    std::vector<std::pair<int, int>> RegisterStackOffsets; // Stored from lowest stack offset to highest

    ZydisRegister RegVIP = ZYDIS_REGISTER_RSI;
    ZydisRegister RegVSP = ZYDIS_REGISTER_RBP;
    ZydisRegister RegVREG = ZYDIS_REGISTER_RSP;
    ZydisRegister RegHT = ZYDIS_REGISTER_R11;

    std::vector<VMBlock> BBs;
};

struct VMImage {
    std::byte* ImageData;
    size_t     ImageSize;
    uint64_t   ImageBase;
    uint64_t   ImageDefaultBase;

    std::vector<PIMAGE_SECTION_HEADER> CodeSections;
    PIMAGE_SECTION_HEADER              VirtSection;

    std::vector<VMRoutine> VMRoutines;
};

inline std::string VMOpcodeToString(VMOpcode opcode) {
    switch (opcode) {
#define STRING_CASE(name)                                                                                              \
    case VMOpcode::name:                                                                                               \
        return #name;
        EXPAND_VM_OPCODES(STRING_CASE)
#undef STRING_CASE
    default:
        return "UNKNOWN";
    }
}
