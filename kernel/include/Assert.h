#pragma once

[[noreturn]] void Poof(const char* assertion, const char* file, unsigned int line, const char* function);

#define Panic() Poof("None", __FILE__, __LINE__, __func__)
#define Assert(assertion) if (!(assertion)) Poof(#assertion, __FILE__, __LINE__, __func__)