#include <koalabox/paths.hpp>
#include <koalabox/globals.hpp>
#include <koalabox/loader.hpp>

namespace koalabox::paths {

    namespace {
        String get_file_name(String suffix) {
            return koalabox::globals::get_project_name() + suffix;
        }
    }

    KOALABOX_API(Path) get_self_path() {
        static const auto self_handle = koalabox::globals::get_self_handle();
        static const auto self_path = koalabox::loader::get_module_dir(self_handle);
        return self_path;
    }

    KOALABOX_API(Path) get_config_path() {
        static const auto path = get_self_path() / get_file_name(".config.json");
        return path;
    }

    KOALABOX_API(Path) get_cache_path() {
        static const auto path = get_self_path() / get_file_name(".cache.json");
        return path;
    }

    KOALABOX_API(Path) get_log_path() {
        static const auto path = get_self_path() / get_file_name(".log.log");
        return path;
    }

    KOALABOX_API(Path) get_ca_key_path() {
        static const auto project_name = koalabox::globals::get_project_name();
        static const auto path = get_self_path() / (project_name + ".ca.key");
        return path;
    }

    KOALABOX_API(Path) get_ca_cert_path() {
        static const auto project_name = koalabox::globals::get_project_name();
        static const auto path = get_self_path() / (project_name + ".ca.crt");
        return path;
    }

}
