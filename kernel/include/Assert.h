#pragma once

[[noreturn]] void KernelPanic(const char* assertion, const char* file, unsigned int line, const char* function);
void KernelWarn(const char* message, const char* file, unsigned int line);

#define Panic() KernelPanic("None", __FILE__, __LINE__, __func__)
#define Assert(assertion) if (!(assertion)) KernelPanic(#assertion, __FILE__, __LINE__, __func__)
#define Warn(message) KernelWarn(message, __FILE__, __LINE__)