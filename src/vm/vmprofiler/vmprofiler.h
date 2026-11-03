#pragma once
#include <vm/vmstructs.h>

class VMProfiler {
  public:
    bool ProfileRoutines(VMImage& Image);

  private:
    bool ProfileRoutine(const VMImage& Image, VMRoutine& Routine);
    bool ProfileHT(const VMImage& Image, VMRoutine& Routine);
    bool PopulateHandlers(const VMImage& Image, VMRoutine& Routine);
    bool ProfileHandler(const VMImage& Image, const VMRoutine& Routine, VMHandler& Handler);
};
