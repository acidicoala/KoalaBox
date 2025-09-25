#include <koalabox/str.hpp>

#include "koalabox_tools/cmd.hpp"

namespace koalabox::tools::cmd {
    normalized_args_t normalize_args(const int argc, const TCHAR** argv) {
        normalized_args_t normalized;
        normalized.args_containers.reserve(argc);
        normalized.argv.reserve(argc);

        for(auto i = 0; i < argc; ++i) {
            normalized.args_containers.push_back(str::to_str(argv[i]));
            normalized.argv.push_back(normalized.args_containers.at(i).c_str());
        }

        return normalized;
    }
}
