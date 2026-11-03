#pragma once
#include <vm/vmstructs.h>

#include <unordered_set>

// Traces and explores all VM basic blocks assembling a partial CFG
class VMTracer {
  public:
    void TraceRoutines(VMImage& Image);

  private:
    void TraceRoutine(VMImage& Image, VMRoutine& Routine);
    bool DecodeIns(const VMImage& Image, VMRoutine& Routine, uint64_t VIP, VMIns& Result);
    bool IsValidBlock(const VMImage& Image, VMRoutine& Routine, uint64_t VIP);
    void TraceBlock(VMImage& Image, VMRoutine& Routine, VMBlock& Block, std::unordered_set<uint64_t>& Explored);
};
