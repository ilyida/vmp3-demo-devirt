#include "vmtracer.h"
#include <unordered_map>
#include <loguru.hpp>

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalValue.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/Utils/ValueMapper.h>

#include <llvm/ADT/SCCIterator.h>
#include <llvm/Analysis/CFG.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/IR/InstIterator.h>

#include <llvm/IR/LLVMContext.h>
#include "llvm/AsmParser/Parser.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/IR/IRBuilder.h"
#include <unordered_set>

void VMTracer::TraceRoutines(VMImage& Image) {
    for (auto& Routine : Image.VMRoutines) {
        TraceRoutine(Image, Routine);
    }
}

void VMTracer::TraceRoutine(VMImage& Image, VMRoutine& Routine) {
    VMBlock EntryBlock = VMBlock(Routine.VIPAddress);

    std::unordered_set<uint64_t> Explored;
    TraceBlock(Image, Routine, EntryBlock, Explored);

    LOG_F(INFO, "Found %llu basic blocks in routine 0x%llX", Routine.BBs.size(), Routine.VIPAddress);
    for (const auto& BB : Routine.BBs) {
        LOG_F(INFO, "BB: 0x%llX", BB.Address);
        for (const auto& Edge : BB.PotentialEdges) {
            LOG_F(INFO, "   Edge: 0x%llX", Edge);
        }
    }
}

bool VMTracer::DecodeIns(const VMImage& Image, VMRoutine& Routine, uint64_t VIP, VMIns& Result) {
    uint8_t* VIPData = (uint8_t*)(Image.ImageData + (VIP - Image.ImageBase));

    if ((uint64_t)VIPData < (uint64_t)Image.ImageData)
        return false;

    // Ensure room for largest possible instruction
    if ((uint64_t)(VIPData + 9) >= (uint64_t)Image.ImageData + Image.ImageSize)
        return false;

    VMHandler& Handler = Routine.Handlers[*VIPData++];

    Result.Opcode = Handler.Opcode;
    Result.OperandRawSize = Handler.VIPDelta;
    Result.Text = VMOpcodeToString(Result.Opcode);

    if (Result.Opcode == VMOpcode::NONE)
        return false;

    switch (Result.Opcode) {
    case VMOpcode::POP_VMREG_64:
    case VMOpcode::POP_VMREG_32:
    case VMOpcode::POP_VMREG_16:
    case VMOpcode::POP_VMREG_8:
    case VMOpcode::PUSH_VMREG_64:
    case VMOpcode::PUSH_VMREG_32:
    case VMOpcode::PUSH_VMREG_16:
    case VMOpcode::PUSH_VMREG_8:
        if (Handler.VIPDelta != 1)
            throw std::runtime_error("Invalid operand size!");

        Result.OperandType = VMOperandType::vreg;
        Result.vreg.Value = VMRegister(*VIPData / 8);
        Result.Text += " vreg[" + std::to_string(std::underlying_type_t<VMRegister>(Result.vreg.Value)) + "]";
        break;

    case VMOpcode::PUSH_IMM_64:
        if (Handler.VIPDelta != 8)
            throw std::runtime_error("Invalid operand size!");

        Result.OperandType = VMOperandType::imm;
        Result.imm.Size = 64;
        Result.imm.u64 = *(uint64_t*)VIPData;
        Result.Text += " " + std::to_string(Result.imm.u64);
        break;
    case VMOpcode::PUSH_IMM_32:
        if (Handler.VIPDelta != 4)
            throw std::runtime_error("Invalid operand size!");

        Result.OperandType = VMOperandType::imm;
        Result.imm.Size = 32;
        Result.imm.u32 = *(uint32_t*)VIPData;
        Result.Text += " " + std::to_string(Result.imm.u32);
        break;
    case VMOpcode::PUSH_IMM_16:
        if (Handler.VIPDelta != 2)
            throw std::runtime_error("Invalid operand size!");

        Result.OperandType = VMOperandType::imm;
        Result.imm.Size = 16;
        Result.imm.u16 = *(uint16_t*)VIPData;
        Result.Text += " " + std::to_string(Result.imm.u16);
        break;
    case VMOpcode::PUSH_IMM_8:
        if (Handler.VIPDelta != 1)
            throw std::runtime_error("Invalid operand size!");

        Result.OperandType = VMOperandType::imm;
        Result.imm.Size = 8;
        Result.imm.u8 = *(uint8_t*)VIPData;
        Result.Text += " " + std::to_string(Result.imm.u8);
        break;
    }

    return true;
}

bool VMTracer::IsValidBlock(const VMImage& Image, VMRoutine& Routine, uint64_t VIP) {
    if (VIP - Image.ImageBase < Image.VirtSection->VirtualAddress)
        return false;

    if (VIP - Image.ImageBase >= Image.VirtSection->VirtualAddress + Image.VirtSection->Misc.VirtualSize)
        return false;

    // All blocks must start by popping all arguments from the stack
    int VMArgCount = _countof(VMStack::VMArgs);
    for (int i = 0; i < VMArgCount; i++) {
        VMIns Ins;
        if (!DecodeIns(Image, Routine, VIP, Ins))
            return false;

        if (Ins.Opcode != VMOpcode::POP_VMREG_64)
            return false;
    }

    return true;
}

void VMTracer::TraceBlock(VMImage& Image, VMRoutine& Routine, VMBlock& Block, std::unordered_set<uint64_t>& Explored) {
    if (Explored.contains(Block.Address))
        return;

    Explored.insert(Block.Address);

    // LOG_F(INFO, "Exploring: %llX", Block.Address);

    uint64_t VIP = Block.Address;
    while (true) {
        VMIns Ins;
        if (!DecodeIns(Image, Routine, VIP, Ins)) {
            LOG_F(ERROR, "Failed to decode instruction!");
            break;
        }

        // Attempt to find new basic block addresses
        if (Ins.Opcode == VMOpcode::PUSH_IMM_64) {
            uint64_t NewVIP = Ins.imm.u64 + (Image.ImageBase - Image.ImageDefaultBase);
            if (IsValidBlock(Image, Routine, NewVIP)) {
                Block.PotentialEdges.push_back(NewVIP);
                // LOG_F(INFO, "Found potential BB: 0x%llX", NewVIP);
            }
        }

        Block.Instructions.push_back(Ins);

        VIP += Ins.OperandRawSize + 1;
        if (Ins.Opcode == VMOpcode::EXIT || Ins.Opcode == VMOpcode::JUMP)
            break;
    }

    Routine.BBs.push_back(Block);

    for (const auto& Edge : Block.PotentialEdges) {
        VMBlock NewBlock = VMBlock(Edge);
        TraceBlock(Image, Routine, NewBlock, Explored);
    }
}
