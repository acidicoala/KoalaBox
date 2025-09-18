#include <cerrno>

#include <dlfcn.h>
#include <gtk/gtk.h>

#include "koalabox.hpp"

namespace koalabox::util {
    void error_box(const std::string& title, const std::string& message) {
        // TODO: verify implementation
        // Make dependency on gtk3 optional. Many modern distros do not include lib32-gtk3 by default.
        void* libgtk3 = dlopen("libgtk-3.so", RTLD_LAZY);
        if(!libgtk3) {
            LOG_WARN("No gtk3 library available. Skipping error box.");
            return;
        }

#define FIND_SYMBOL(FUNC) \
        const auto lib_##FUNC = reinterpret_cast<decltype(&FUNC)>(dlsym(libgtk3, #FUNC));

        FIND_SYMBOL(gtk_init);
        FIND_SYMBOL(gtk_message_dialog_new);
        FIND_SYMBOL(gtk_window_set_title);
        FIND_SYMBOL(gtk_dialog_run);
        FIND_SYMBOL(gtk_widget_destroy);

        lib_gtk_init(nullptr, nullptr);

        auto* dialog = lib_gtk_message_dialog_new(
            nullptr,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_OK,
            "%s",
            message.c_str()
        );
        lib_gtk_window_set_title(reinterpret_cast<GtkWindow*>(dialog), title.c_str());

        lib_gtk_dialog_run(reinterpret_cast<GtkDialog*>(dialog));
        lib_gtk_widget_destroy(dialog);
    }

    [[noreturn]] void panic(const std::string& message) {
        const auto title = std::format("[{}] Panic!", globals::get_project_name());

        LOG_CRITICAL(message);

        error_box(title, message);

        logger::shutdown();

        DebugBreak();

        exit(errno);
    }

    std::string get_env_var(const std::string& key) {
        if(const char* value = getenv(key.c_str())) {
            return value;
        }

        return "";
    }
}
