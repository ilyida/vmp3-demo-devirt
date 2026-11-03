#pragma once
#include <vm/vmstructs.h>
#include <unicorn/unicorn.h>

class VMClassifierEmulator {
  public:
    static constexpr uint64_t MAX_INS_COUNT = 250;
    static constexpr uint64_t STACK_ADDR = 0x10000;

  public:
    VMClassifierEmulator(const VMImage& Image, const VMRoutine& Routine);
    ~VMClassifierEmulator();

  public:
    uc_err   Run(const VMHandler& Handler, const VMRoutine& Routine);
    VMOpcode Return(const VMOpcode Result, const VMRoutine& Routine, VMHandler& Handler);

  public:
    bool       Valid() const noexcept { return m_Valid; }
    uc_engine* UC() noexcept { return m_uc; }

  private:
    uc_engine* m_uc;
    uint64_t   m_StartVIP;
    bool       m_Valid;
};
