#include "vmclassifier.h"
#include <functional>
#include <loguru.hpp>

extern VMOpcode add(const VMImage&, const VMRoutine&, VMHandler&);
extern VMOpcode sub(const VMImage&, const VMRoutine&, VMHandler&);
extern VMOpcode _div(const VMImage&, const VMRoutine&, VMHandler&);
extern VMOpcode imul_and_mul(const VMImage&, const VMRoutine&, VMHandler&);
extern VMOpcode nand(const VMImage&, const VMRoutine&, VMHandler&);
extern VMOpcode nor(const VMImage&, const VMRoutine&, VMHandler&);
extern VMOpcode shl(const VMImage&, const VMRoutine&, VMHandler&);
extern VMOpcode shr(const VMImage&, const VMRoutine&, VMHandler&);
extern VMOpcode shld(const VMImage&, const VMRoutine&, VMHandler&);
extern VMOpcode shrd(const VMImage&, const VMRoutine&, VMHandler&);
extern VMOpcode PUSH_REG(const VMImage&, const VMRoutine&, VMHandler&);
extern VMOpcode POP_REG(const VMImage&, const VMRoutine&, VMHandler&);
extern VMOpcode pushconst(const VMImage&, const VMRoutine&, VMHandler&);
extern VMOpcode lflags(const VMImage&, const VMRoutine&, VMHandler&);
extern VMOpcode pushvsp(const VMImage&, const VMRoutine&, VMHandler&);
extern VMOpcode popvsp(const VMImage&, const VMRoutine&, VMHandler&);
extern VMOpcode read(const VMImage&, const VMRoutine&, VMHandler&);
extern VMOpcode readgs(const VMImage&, const VMRoutine&, VMHandler&);
extern VMOpcode write(const VMImage&, const VMRoutine&, VMHandler&);
extern VMOpcode jmp(const VMImage&, const VMRoutine&, VMHandler&);
extern VMOpcode call(const VMImage&, const VMRoutine&, VMHandler&);
extern VMOpcode cpuid(const VMImage&, const VMRoutine&, VMHandler&);
extern VMOpcode rdtsc(const VMImage&, const VMRoutine&, VMHandler&);
extern VMOpcode vmexit(const VMImage&, const VMRoutine&, VMHandler&);

static std::array<std::function<VMOpcode(const VMImage&, const VMRoutine&, VMHandler&)>, 24> HandlerClassifiers = {
    add,       sub,    _div,    imul_and_mul, nand, nor,    shl,   shr, shld, shrd,  PUSH_REG, POP_REG,
    pushconst, lflags, pushvsp, popvsp,       read, readgs, write, jmp, call, cpuid, rdtsc,    vmexit};

bool VMClassifier::ClassifyRoutines(VMImage& Image) {
    for (auto& Routine : Image.VMRoutines) {
        if (!ClassifyRoutineHandlerTable(Image, Routine)) {
            LOG_F(ERROR, "Failed to classify routine handler table!");
            return false;
        }
    }

    return true;
}

bool VMClassifier::ClassifyRoutineHandlerTable(const VMImage& Image, VMRoutine& Routine) {
    int ClassifiedCount = 0;
    for (auto& Handler : Routine.Handlers) {
        Handler.Opcode = ClassifyHandler(Image, Routine, Handler);
        if (Handler.Opcode == VMOpcode::NONE) {
            LOG_F(ERROR, "Failed to classify handler! %p", Handler.RuntimeAddress);
        } else {
            ClassifiedCount += 1;
        }
    }

    LOG_F(INFO, "Classified %d/%d (%.1f%%)", ClassifiedCount, Routine.Handlers.size(),
          ((float)ClassifiedCount / (float)Routine.Handlers.size()) * 100.f);

    return true;
}

VMOpcode VMClassifier::ClassifyHandler(const VMImage& Image, const VMRoutine& Routine, VMHandler& Handler) {
    VMOpcode Result = VMOpcode::NONE;
    VMOpcode ResultValid = VMOpcode::NONE;

    for (const auto& Classifier : HandlerClassifiers) {
        Result = Classifier(Image, Routine, Handler);
        if (Result != VMOpcode::NONE) {
            if (ResultValid != VMOpcode::NONE) {
                LOG_F(ERROR, "Found double classification! %s and %s", VMOpcodeToString(Result).c_str(),
                      VMOpcodeToString(ResultValid).c_str());
            }

            ResultValid = Result;
            Result = VMOpcode::NONE;
        }
    }

    return ResultValid;
}
