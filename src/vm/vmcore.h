#pragma once
#include <string>
#include <vm/vmstructs.h>
#include <vm/vmfinder/vmfinder.h>
#include <vm/vmanalyzer/vmanalyzer.h>
#include <vm/vmprofiler/vmprofiler.h>
#include <vm/vmclassifier/vmclassifier.h>
#include <vm/vmtracer/vmtracer.h>

class VMCore {
  public:
    VMCore(const std::string& FilePath, size_t VirtSectIdx);
    ~VMCore();

  private:
    bool LoadPEImage(const std::string& FilePath, std::byte*& ImageData, size_t& ImageSize);
    void UnloadPEImage(std::byte* ImageData);

    bool LoadVMPImage(const std::string& FilePath, size_t VirtSectIdx, VMImage& Image);
    void UnloadVMPImage(VMImage& Image);

  public:
    bool IsValid() const noexcept { return m_Valid; }

  public:
    bool Process();

  private:
    VMFinder     Finder;
    VMAnalyzer   Analyzer;
    VMProfiler   Profiler;
    VMClassifier Classifier;
    VMTracer     Tracer;

  private:
    VMImage m_Image = {};
    bool    m_Valid = false;
};
