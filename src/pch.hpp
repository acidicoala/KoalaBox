#pragma once

#include <cstdint>

// See: https://blog.kowalczyk.info/article/j/guide-to-predefined-macros-in-c-compilers-gcc-clang-msvc-etc..html
// https://learn.microsoft.com/en-us/cpp/preprocessor/predefined-macros?view=msvc-170

// Aliases for 32/64-bit system
#if defined(_WIN64) || defined(__x86_64__) || defined(__amd64__)
#define KB_64
#else
#define KB_32
#endif

// Aliases for Windows/Linux OS
#if defined(_WIN32)
#define KB_WIN
#elif defined(__linux__)
#define KB_LINUX
#endif

#ifndef KB_DEBUG // Defined in CMake
#define KB_RELEASE
#endif

#ifdef KB_WIN

// Windows headers
#define WIN32_LEAN_AND_MEAN
#define _UNICODE
#define UNICODE
#define NOMINMAX
#include <tchar.h>
#include <Windows.h>

// Process Status API must be included after windows
#include <Psapi.h>

#define MAIN wmain

#elifdef KB_LINUX

/// Kind of a Windows polyfill for Linux?

#include <csignal>

// TODO: replace it with void* everywhere
using HMODULE = void*;

// TODO: Delete them after removing their usage
#define __cdecl __attribute__((__cdecl__))
#define __fastcall
#define __stdcall __attribute__((__stdcall__))
#define __declspec(spec) __attribute__((spec))

#define TEXT(...) __VA_ARGS__

#define OutputDebugString(MESSAGE) fprintf(stderr, MESSAGE)

#define DebugBreak() raise(SIGTRAP)

#define MAIN main
#define TCHAR char // for the main signature

#endif
