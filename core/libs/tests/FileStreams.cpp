#include "prx/libc/include/FileStream.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <array>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

extern "C" {
FileStream* APS5_VABI fopen_nid_postfix(const char* filename, const char* mode);
FileStream* APS5_VABI freopen_nid_postfix(const char* filename, const char* mode, FileStream* stream);
int APS5_VABI fclose_nid_postfix(FileStream* stream);
std::size_t APS5_VABI fread_nid_postfix(void* buffer, std::size_t size, std::size_t count, FileStream* stream);
std::size_t APS5_VABI fwrite_nid_postfix(const void* buffer, std::size_t size, std::size_t count, FileStream* stream);
int APS5_VABI fseek_nid_postfix(FileStream* stream, std::int64_t offset, int origin);
std::int64_t APS5_VABI ftell_nid_postfix(FileStream* stream);
int APS5_VABI fputs_nid_postfix(const char* str, FileStream* stream);
int APS5_VABI fflush_nid_postfix(FileStream* stream);
int APS5_VABI ferror_nid_postfix(FileStream* stream);
void APS5_VABI clearerr_nid_postfix(FileStream* stream);
}

static void Require(bool condition) {
    if (!condition) throw std::runtime_error("File stream check failed");
}

template<typename TAction>
static void ExpectException(TAction action) {
    try {
        action();
    } catch (const std::runtime_error&) {
        return;
    }
    throw std::runtime_error("Expected a stream exception");
}

int main(int argc, char** argv) {
    std::cout << "Starting test\n" << std::flush;
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <temp-file>\n";
        return 1;
    }

    std::cout << "Test 1: write to read-only stream\n" << std::flush;
    auto* readonly = fopen_nid_postfix(argv[1], "wb");
    if (!readonly) {
        std::cerr << "Failed to open file for writing: errno=" << errno << " " << std::strerror(errno) << "\n";
        return 1;
    }
    constexpr char marker[] = "readonly";
    auto wrote = fwrite_nid_postfix(marker, 1, sizeof(marker), readonly);
    if (wrote != sizeof(marker)) {
        std::cerr << "Initial write failed\n";
        return 1;
    }
    fclose_nid_postfix(readonly);
    readonly = fopen_nid_postfix(argv[1], "rb");
    if (!readonly) {
        std::cerr << "Failed to reopen file for reading\n";
        return 1;
    }
    std::cout << "  fwrite to rb stream\n" << std::flush;
    const auto writeResult = fwrite_nid_postfix(marker, 1, sizeof(marker), readonly);
    std::cout << "  fwrite returned: " << writeResult << "\n" << std::flush;
    if (writeResult != 0) {
        std::cerr << "Expected fwrite to return 0, got " << writeResult << "\n";
        return 1;
    }
    std::cout << "  ferror check\n" << std::flush;
    if (!ferror_nid_postfix(readonly)) {
        std::cerr << "Expected ferror to be set\n";
        return 1;
    }
    clearerr_nid_postfix(readonly);
    std::cout << "  fputs to rb stream\n" << std::flush;
    const auto putsResult = fputs_nid_postfix(marker, readonly);
    std::cout << "  fputs returned: " << putsResult << "\n" << std::flush;
    if (putsResult != EOF) {
        std::cerr << "Expected fputs to return EOF, got " << putsResult << "\n";
        return 1;
    }
    if (!ferror_nid_postfix(readonly)) {
        std::cerr << "Expected ferror to be set after fputs\n";
        return 1;
    }
    fclose_nid_postfix(readonly);

    std::cout << "Test 2: read from write-only stream\n" << std::flush;
    auto* writeonly = fopen_nid_postfix(argv[1], "wb");
    if (!writeonly) {
        std::cerr << "Failed to open file for writing (test 2)\n";
        return 1;
    }
    std::array<char, 8> readbuf{};
    std::cout << "  fread from wb stream\n" << std::flush;
    const auto readResult = fread_nid_postfix(readbuf.data(), 1, readbuf.size(), writeonly);
    std::cout << "  fread returned: " << readResult << "\n" << std::flush;
    if (readResult != 0) {
        std::cerr << "Expected fread to return 0, got " << readResult << "\n";
        return 1;
    }
    if (!ferror_nid_postfix(writeonly)) {
        std::cerr << "Expected ferror to be set after fread\n";
        return 1;
    }
    fclose_nid_postfix(writeonly);

    std::remove(argv[1]);
    std::cout << "PASS: I/O error handling returns error codes instead of throwing\n";
    return 0;
}
