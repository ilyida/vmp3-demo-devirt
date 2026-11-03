#pragma once
#include <vm/vmstructs.h>

class VMAnalyzer {
  public:
    bool AnalyzeRoutines(VMImage& Image);

  private:
    bool AnalyzeRoutineEntry(const VMImage& Image, VMRoutine& Routine);
};
