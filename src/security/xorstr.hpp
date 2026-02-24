// =============================================================================
// xorstr.hpp — Compile-Time String Obfuscation
// =============================================================================
// Protege as strings de serem lidas no .exe final por ferramentas
// de engenharia reversa (strings.exe, IDA Pro, Ghidra).
// Descriptografa apenas na memoria interativamente.
// =============================================================================

#pragma once

#include <array>
#include <cstdint>
#include <string>


// Macro magica para seed baseada no TIME e DATE do compilador
#define COMPILER_SEED                                                          \
  ((__TIME__[7] - '0') * 1ull + (__TIME__[6] - '0') * 10ull +                  \
   (__TIME__[4] - '0') * 60ull + (__TIME__[3] - '0') * 600ull +                \
   (__TIME__[1] - '0') * 3600ull + (__TIME__[0] - '0') * 36000ull)

// Gerador congruente linear para produzir pseudo-random em compile-time
constexpr uint64_t linear_congruent_generator(unsigned rounds) {
  return 1013904223ull +
         1664525ull * ((rounds > 0) ? linear_congruent_generator(rounds - 1)
                                    : COMPILER_SEED);
}

#define RANDOM_SEED(min, max)                                                  \
  (min + (linear_congruent_generator(10) % (max - min + 1)))

template <size_t Size> struct XorString {
  uint8_t buffer[Size];
  uint8_t key;

  constexpr XorString(const char *str, uint8_t initialKey, int)
      : buffer{}, key(initialKey) {
    for (size_t i = 0; i < Size; ++i) {
      buffer[i] = static_cast<uint8_t>(str[i]) ^ (key + i); // Criptografando
    }
  }

  __forceinline std::string decrypt() const {
    std::string result(Size - 1, '\0');
    for (size_t i = 0; i < Size - 1; ++i) {
      result[i] = static_cast<char>(buffer[i] ^
                                    (key + i)); // Descriptografando em runtime
    }
    return result;
  }
};

// =============================================================================
// MACRO USO PRINCIPAL
// =============================================================================
// Ex: auto api = XOR("http://api.minha.com");
//     api.decrypt().c_str();  (Se for passar para funcao C)
//     api.decrypt();          (Se for std::string)

#define XOR(str)                                                               \
  (XorString<sizeof(str)>(str, static_cast<uint8_t>(RANDOM_SEED(0, 255)),      \
                          __COUNTER__)                                         \
       .decrypt())
