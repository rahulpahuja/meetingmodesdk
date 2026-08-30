#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

// Portable driver for LLVMFuzzerTestOneInput: replays each file given on the command line
// (typically a saved corpus or a crash reproducer). Needs no libFuzzer runtime, so it builds
// and runs under plain ASan/UBSan on any toolchain — the coverage-guided `fuzz_c_abi` target
// is the Clang-with-fuzzer-runtime counterpart used for actual fuzzing.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size);

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: " << argv[0] << " <input-file> [more ...]\n";
        return 2;
    }
    for (int i = 1; i < argc; ++i) {
        std::ifstream in(argv[i], std::ios::binary);
        if (!in) {
            std::cerr << "cannot open " << argv[i] << "\n";
            return 1;
        }
        const std::vector<char> bytes((std::istreambuf_iterator<char>(in)),
                                      std::istreambuf_iterator<char>());
        LLVMFuzzerTestOneInput(reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size());
    }
    return 0;
}
