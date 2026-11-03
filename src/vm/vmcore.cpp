#include "vmcore.h"
#include <loguru.hpp>
#include <peconv.h>

// Arbitrary image base to force relocations to be applied
constexpr uint64_t DesiredImageBase = 0x7FF700000000;

VMCore::VMCore(const std::string& FilePath, size_t VirtSectIdx) {
    m_Valid = LoadVMPImage(FilePath, VirtSectIdx, m_Image);
}
VMCore::~VMCore() {
    UnloadVMPImage(m_Image);
}

bool VMCore::LoadPEImage(const std::string& FilePath, std::byte*& ImageData, size_t& ImageSize) {
    ImageData = reinterpret_cast<std::byte*>(
        peconv::load_pe_module(FilePath.c_str(), ImageSize, false, true, DesiredImageBase));
    if (!ImageData) {
        LOG_F(ERROR, "Failed to load PE image!");
        return false;
    }

    return true;
}
void VMCore::UnloadPEImage(std::byte* ImageData) {
    peconv::free_pe_buffer(reinterpret_cast<BYTE*>(ImageData));
}

bool VMCore::LoadVMPImage(const std::string& FilePath, size_t VirtSectIdx, VMImage& Image) {
    if (!LoadPEImage(FilePath, Image.ImageData, Image.ImageSize))
        return false;

    PIMAGE_NT_HEADERS64 NtHeaders = peconv::get_nt_hdrs64(reinterpret_cast<BYTE*>(Image.ImageData));
    if (!NtHeaders) {
        LOG_F(ERROR, "Failed to get image NtHeaders!");
        return false;
    }

    Image.ImageBase = DesiredImageBase;
    Image.ImageDefaultBase = NtHeaders->OptionalHeader.ImageBase;

    size_t SectCount = peconv::get_sections_count(reinterpret_cast<BYTE*>(Image.ImageData), Image.ImageSize);
    for (size_t i = 0; i < SectCount; i++) {
        PIMAGE_SECTION_HEADER Sect =
            peconv::get_section_hdr(reinterpret_cast<BYTE*>(Image.ImageData), Image.ImageSize, i);
        if (!Sect) {
            LOG_F(WARNING, "Failed to get section at index %llu! Skipping...", i);
            continue;
        }

        // Copy section name safely ensuring null termintaor
        char SectionName[9] = {};
        std::memcpy(SectionName, Sect->Name, 8);

        bool IsCodeSect = Sect->Characteristics & IMAGE_SCN_CNT_CODE;
        if (i == VirtSectIdx) {
            // VM section should be executable
            if (!IsCodeSect)
                LOG_F(WARNING, "VM section %s is not marked as a code section!", SectionName);

            Image.VirtSection = Sect;
            LOG_F(INFO, "Found VM section %s stopping section scan", SectionName);
            break;
        }

        if (IsCodeSect) {
            LOG_F(INFO, "Found code section %s at index %llu", SectionName, i);
            Image.CodeSections.push_back(Sect);
        }
    }

    if (!Image.VirtSection) {
        LOG_F(ERROR, "Failed to find VMP virtualization section at index %llu!", VirtSectIdx);
        return false;
    }

    if (Image.CodeSections.empty()) {
        LOG_F(ERROR, "Failed to find any code sections!");
        return false;
    }

    return true;
}
void VMCore::UnloadVMPImage(VMImage& Image) {
    if (Image.ImageData)
        UnloadPEImage(Image.ImageData);
}

bool VMCore::Process() {
    if (!Finder.FindRoutines(m_Image))
        return false;

    if (!Analyzer.AnalyzeRoutines(m_Image))
        return false;

    if (!Profiler.ProfileRoutines(m_Image))
        return false;

    if (!Classifier.ClassifyRoutines(m_Image))
        return false;

    Tracer.TraceRoutines(m_Image);

    return true;
}
