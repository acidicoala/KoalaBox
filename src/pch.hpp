#pragma once

// Windows headers
#define WIN32_LEAN_AND_MEAN
#define _UNICODE
#define UNICODE
#define NOMINMAX
#include <tchar.h>
#include <Windows.h>

// Process Status API must be included after windows
#include <Psapi.h>