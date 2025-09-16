#pragma once

#include <cstdint>

#ifdef _WIN32

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

#else

/// Kind of a Windows polyfill for Linux?

#include <csignal>

using HMODULE = void*;

#define __cdecl __attribute__((__cdecl__))
#define __fastcall
#define __stdcall __attribute__((__stdcall__))
#define __declspec(spec) __attribute__((spec))

#define TEXT(...) __VA_ARGS__

#define OutputDebugString(MESSAGE) fprintf(stderr, MESSAGE)

#define DebugBreak() raise(SIGTRAP)

#define MAIN main

#define TCHAR char

#endif

#if defined(_WIN64) || defined(__x86_64__) || defined(__amd64__)
#define KB_32 0
#define KB_64 1
#else
#define KB_32 1
#define KB_64 0
#endif