set LLVMPATH=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\Llvm\x64
set TBAAFLAG=true
set WARNINGS=-Wno-gnu-inline-cpp-without-extern

"%LLVMPATH%"\bin\clang++ -O3 %WARNINGS% -mlong-double-80 -fno-discard-value-names -fstrict-aliasing -fno-slp-vectorize -mllvm -enable-tbaa=%TBAAFLAG% VMProtectHelpers.cpp -emit-llvm -S -I..\dependencies\remill\include -std=c++17 -o VMProtectHelpers.ll
"%LLVMPATH%"\bin\clang++ -O3 %WARNINGS% -mlong-double-80 -fno-discard-value-names -fstrict-aliasing -fno-slp-vectorize -mllvm -enable-tbaa=%TBAAFLAG% VMProtectHelpers.cpp -emit-llvm -c -I..\dependencies\remill\include -std=c++17 -o VMProtectHelpers.bc