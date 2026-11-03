#pragma once
#include <vm/vmstructs.h>

// Scans image for virtualized routines by searching for patched JMPs to VM section
class VMFinder {
  public:
    bool FindRoutines(VMImage& Image);

  private:
    bool IsAddressInSect(uint64_t RuntimeAddress, uint64_t ImageBase, const PIMAGE_SECTION_HEADER Sect) const;
    void FindRoutinesInSection(VMImage& Image, PIMAGE_SECTION_HEADER Sect);
};
