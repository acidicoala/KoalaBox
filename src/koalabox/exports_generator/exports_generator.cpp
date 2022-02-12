// Windows headers
#define UNICODE
#include <Windows.h>

// C++ std lib headers
#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>
#include <iostream>
#include <vector>
#include <set>
#include <regex>

// Ensure that assert macros work
#undef NDEBUG

using namespace std;

set<string> get_implemented_functions(const filesystem::path& path);

vector<string> get_exported_functions(filesystem::path& dll_path);

std::string to_string(const std::wstring& wstr);

bool is_function_mangled(const string& name);

bool is_64_bit();

/**
 * Args <br>
 * 0: program name <br>
 * 1: forwarded dll name <br>
 * 2: dll input path <br>
 * 3: headers input path <br>
 * 4: header output path <br>
 */
int wmain(const int argc, const wchar_t* argv[]) {
    assert(argc == 5);

    string forwarded_dll_name(to_string(argv[1]));
    filesystem::path dll_input_path(argv[2]);
    filesystem::path headers_input_path(argv[3]);
    filesystem::path header_output_path(argv[4]);

    auto implemented_functions = get_implemented_functions(headers_input_path);

    auto exported_functions = get_exported_functions(dll_input_path);

    // Create directories for export header, if necessary
    filesystem::create_directories(header_output_path.parent_path());

    // Open the export header file for writing
    ofstream export_file(header_output_path, ofstream::out | ofstream::trunc);
    assert(export_file.is_open());

    // Add header guard
    export_file << "#pragma once" << endl << endl;

    // Iterate over exported functions
    for (const auto& name: exported_functions) {
        // Iterate over implemented functions and check if it is there
        for (const auto& implemented_function: implemented_functions) {
            bool is_implemented = (is_64_bit() or not is_function_mangled(name))
                ? name == implemented_function
                : name.find("_" + implemented_function + "@") != string::npos;

            if (is_implemented) {
                export_file << "// ";
            }
        }

        export_file << "#pragma comment(linker, " << '"' << "/export:" << name << '='
                    << forwarded_dll_name << '.' << name << '"' << ')' << endl;
    }
}

// Adapted from: https://github.com/mborne/dll2def/blob/master/dll2def.cpp
vector<string> get_exported_functions(filesystem::path& dll_path) {
    auto exported_functions = vector<string>();

    HMODULE lib = LoadLibraryExW(
        dll_path.c_str(),
        nullptr,
        DONT_RESOLVE_DLL_REFERENCES
    );

    assert(((PIMAGE_DOS_HEADER) lib)->e_magic == IMAGE_DOS_SIGNATURE);
    auto header = (PIMAGE_NT_HEADERS) ((BYTE*) lib + ((PIMAGE_DOS_HEADER) lib)->e_lfanew);
    assert(header->Signature == IMAGE_NT_SIGNATURE);
    assert(header->OptionalHeader.NumberOfRvaAndSizes > 0);
    auto exports = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(
        (BYTE*) lib +
        header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress
    );
    PVOID names = (BYTE*) lib + exports->AddressOfNames;

    // Iterate over the names and add them to the vector
    for (unsigned int i = 0; i < exports->NumberOfNames; i++) {
        auto name = (char*) lib + ((DWORD*) names)[i];
        exported_functions.emplace_back(name);
    }

    return exported_functions;
}

/**
 * Returns a list of functions parsed from the sources
 * in a given directory. Edge cases: Comments
 */
set<string> get_implemented_functions(const filesystem::path& path) {
    set<string> implemented_functions;

    for (auto& p: filesystem::recursive_directory_iterator(path)) {
        auto file_path = p.path();

        ifstream ifs(file_path);
        std::string file_content(istreambuf_iterator<char>{ ifs }, {});

        // Matches function name in the 1st group
        static regex func_name_pattern(R"(\s*DLL_EXPORT\([\w|\s]+\**\)\s*(\w+)\s*\()");
        smatch match;
        while (regex_search(file_content, match, func_name_pattern)) {
            auto func_name = match.str(1);

            if(not implemented_functions.contains(func_name)){
                implemented_functions.insert(func_name);
                cout << "Implemented: " << func_name << endl;
            }

            file_content = match.suffix();
        }
    }

    return implemented_functions;
}

std::string to_string(const wstring& wstr) {
    if (wstr.empty()) {
        return {};
    }
    int size_needed = WideCharToMultiByte(
        CP_UTF8, 0, &wstr[0], static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr
    );
    std::string string(size_needed, 0);
    WideCharToMultiByte(
        CP_UTF8, 0, &wstr[0], (int) wstr.size(), &string[0], size_needed, nullptr, nullptr
    );
    return string;
}

bool is_64_bit() {
    // Static variables are used to suppress
    // "Condition always true/false" warnings
#ifdef _WIN64
    static auto result = true;
#else
    static auto result = false;
#endif
    return result;
}

bool is_function_mangled(const string& name) {
    auto contains_underscore = name.starts_with("_");
    auto contains_at_sign = name.find('@') != string::npos;

    return contains_underscore and contains_at_sign;
}
