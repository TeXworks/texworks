#ifndef UTILS_CompilerWarnings_H
#define UTILS_CompilerWarnings_H

#if defined(__clang__)
#define DIAG_DO_PRAGMA(x) _Pragma(#x)
#define WARNINGS_PUSH() _Pragma("clang diagnostic push")
#define WARNINGS_POP() _Pragma("clang diagnostic pop")
#define WARNINGS_DISABLE(warn) DIAG_DO_PRAGMA(clang diagnostic ignored warn)
#elif defined(__GNUC__)
#define DIAG_DO_PRAGMA(x) _Pragma(#x)
#define WARNINGS_PUSH() _Pragma("GCC diagnostic push")
#define WARNINGS_POP() _Pragma("GCC diagnostic pop")
#define WARNINGS_DISABLE(warn) DIAG_DO_PRAGMA(GCC diagnostic ignored warn)
#elif defined(_MSC_VER)
#define WARNINGS_PUSH() __pragma("warning(push)")
#define WARNINGS_POP() __pragma("warning(pop)")
#define WARNINGS_DISABLE(warn)
#else
#define WARNINGS_PUSH()
#define WARNINGS_POP()
#define WARNINGS_DISABLE(warn)
#endif


#endif // !defined(UTILS_CompilerWarnings_H)
