#pragma once
#include <Zydis/Zydis.h>
#include <unicorn/unicorn.h>

namespace ZydisUtils {
uc_x86_reg ZydisRegToUCReg(ZydisRegister Register);
}
