#pragma once
#include <vm/vmstructs.h>
#include <unicorn/unicorn.h>

class VMClassifier {
  public:
    bool ClassifyRoutines(VMImage& Image);

  private:
    bool     ClassifyRoutineHandlerTable(const VMImage& Image, VMRoutine& Routine);
    VMOpcode ClassifyHandler(const VMImage& Image, const VMRoutine& Routine, VMHandler& Handler);
};
