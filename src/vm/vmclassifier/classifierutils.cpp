#include "classifierutils.h"
#include <vm/utils/zydisutils.h>
#include <loguru.hpp>

VMClassifierEmulator::VMClassifierEmulator(const VMImage& Image, const VMRoutine& Routine) : m_Valid(false) {
    uc_err err = uc_open(UC_ARCH_X86, UC_MODE_64, &m_uc);
    if (err != UC_ERR_OK) {
        LOG_F(ERROR, "Failed to open unicorn-engine for classifying handler! %s", uc_strerror(err));
        return;
    }

    err = uc_mem_map(m_uc, Image.ImageBase, Image.ImageSize, UC_PROT_ALL);
    if (err != UC_ERR_OK) {
        LOG_F(ERROR, "Failed to map image to unicorn-engine memory! %s", uc_strerror(err));
        return;
    }

    err = uc_mem_write(m_uc, Image.ImageBase, Image.ImageData, Image.ImageSize);
    if (err != UC_ERR_OK) {
        LOG_F(ERROR, "Failed to write image to unicorn-engine memory! %s", uc_strerror(err));
        return;
    }

    err = uc_mem_map(m_uc, STACK_ADDR, sizeof(VMStack), UC_PROT_READ | UC_PROT_WRITE);
    if (err != UC_ERR_OK) {
        LOG_F(ERROR, "Failed to allocate stack! %s", uc_strerror(err));
        return;
    }

    uint64_t VirtualStackOff = offsetof(VMStack, VirtualStack);
    VirtualStackOff += sizeof(VMStack::VirtualStack) - 0x100; // Dummy size for instruction operands, should be enough
    uint64_t StackOff = offsetof(VMStack, Stack);
    StackOff += sizeof(VMStack::Stack) - 0x8;

    uint64_t rsp = STACK_ADDR + StackOff;
    uc_reg_write(m_uc, UC_X86_REG_RSP, &rsp);
    uc_reg_write(m_uc, UC_X86_REG_RBP, &rsp);

    uint64_t CurrentVSP = STACK_ADDR + VirtualStackOff;
    uc_reg_write(m_uc, ZydisUtils::ZydisRegToUCReg(Routine.RegVSP), &CurrentVSP);

    m_Valid = true;
}
VMClassifierEmulator ::~VMClassifierEmulator() {
    if (m_uc)
        uc_close(m_uc);
}

uc_err VMClassifierEmulator::Run(const VMHandler& Handler, const VMRoutine& Routine) {
    uc_reg_read(m_uc, ZydisUtils::ZydisRegToUCReg(Routine.RegVIP), &m_StartVIP);
    return uc_emu_start(m_uc, Handler.RuntimeAddress, Routine.FDEAddress, 0, MAX_INS_COUNT);
}
VMOpcode VMClassifierEmulator::Return(const VMOpcode Result, const VMRoutine& Routine, VMHandler& Handler) {
    if (Result == VMOpcode::NONE)
        return Result;

    uint64_t EndVIP;
    uc_reg_read(m_uc, ZydisUtils::ZydisRegToUCReg(Routine.RegVIP), &EndVIP);

    Handler.VIPDelta = EndVIP - m_StartVIP;
    return Result;
}
