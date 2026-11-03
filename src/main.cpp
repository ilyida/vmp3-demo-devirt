#include <loguru.hpp>
#include <argparse/argparse.hpp>
#include <vm/vmcore.h>

void ParseArgs(int argc, char* argv[], argparse::ArgumentParser& args) {
    args.add_argument("-if", "-infile").required();
    args.add_argument("-of", "-outfile").required();

    try {
        args.parse_args(argc, argv);
    } catch (const std::exception& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << args;
        std::exit(1);
    }
}

void InitLoguru(int argc, char* argv[]) {
    loguru::g_preamble_thread = false;
    loguru::g_preamble_date = false;
    loguru::g_preamble_time = false;
    loguru::init(argc, argv);
}

int main(int argc, char* argv[]) {
    argparse::ArgumentParser args("Alcatraz-Deobfuscator");
    // ParseArgs(argc, argv, args);
    InitLoguru(argc, argv);

    // const std::string InFilePath = args.get("-if");
    // const std::string OutFilePath = args.get("-of");
    const std::string InFilePath =
        "C:\\Users\\raax\\source\\repos\\ConsoleApplication17\\x64\\Release\\ConsoleApplication17.vmp.exe";
    const size_t VirtSectIdx = 4;
    // const std::string OutFilePath = args.get("-of");

    VMCore VM(InFilePath, VirtSectIdx);
    if (!VM.IsValid()) {
        LOG_F(ERROR, "Failed to load image!");
        return 1;
    }

    if (!VM.Process()) {
        LOG_F(ERROR, "Failed to process image!");
        return 1;
    }
}
