## 🛠️ Installation instructions (🐧 Linux)

> [!NOTE]
> Linux support in {{ project_name }} is highly experimental and has known issues.
> If none of the methods below work for you, then consider running a Windows version of a game
> via Proton compatibility layer and follow the instructions in the Windows section.


### ✔️ Requirements

Linux builds of {{ project_name }} depend on several libraries. Make sure they are installed on your system.
The following list features links in Arch Linux repositories, but if you are using a different distribution,
you should use the equivalent package for your distro.

inja::## set core-32bit=["gcc-libs", "glibc"]
Required libraries:
inja::## for package in ["brotli", "gcc-libs", "glibc", "libidn2", "libnghttp2", "libssh2", "openssl", "zlib", "zstd"]
- [{{ package }}](https://archlinux.org/packages/core/x86_64/{{ package }}/)
  [[32-bit](https://archlinux.org/packages/{% if package in core-32bit %}core{% else %}multilib{% endif %}/x86_64/lib32-{{ package }}/)]

inja::## endfor

Optional libraries:
- [gtk3](https://archlinux.org/packages/extra/x86_64/gtk3/)
  [[32-bit](https://archlinux.org/packages/multilib/x86_64/lib32-gtk3/)]

---

Just like on Windows, {{ project_name }} features 32 and 64-bit Linux builds.
In release archive they are named as {{ unlocker_libs_linux }} respectively.

### 🪝 Hook mode (🐧 Linux)

Linux doesn't have the same easily exploitable library injection mechanism that Windows has.
However, it is possible to force any Linux executable to load any library by using the
`LD_PRELOAD` environment variable.
For example:

1. Extract and paste the {{ unlocker_libs_linux }} next to the game's executable.
2. In Steam _Library_ open game's _Properties_, switch to the _General_ tab, and set the following _LAUNCH OPTIONS_:

| Bitness | Launch Options                                                     |
|---------|--------------------------------------------------------------------|
| 32-bit  | `LD_PRELOAD=./libsmoke_api32.so ./GameExecutable.x86    %command%` |
| 64-bit  | `LD_PRELOAD=./libsmoke_api64.so ./GameExecutable.x86_64 %command%` |

Naturally, the exact options might change depending on how files are located on your filesystem
or depending on other environment variables you might have configured.
If you have other environment variables, and you don't know how to correctly combine them,
then please make heavy use of search engines and LLMs for guidance and examples instead of the official forum topic.

### 🔀 Proxy mode (🐧 Linux)

Same as on Windows:
1. Rename the original {{ sdk_libs_linux }} to {{ sdk_libs_orig_linux }}
2. Extract and paste the {{ unlocker_libs_linux }} to the same directory, renaming it to {{ sdk_libs_linux }}.

### 🐞 Known issues

- Steam overlay is not working in hook mode
