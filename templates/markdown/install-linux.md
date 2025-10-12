## 🛠️ Installation instructions (🐧 Linux)

> [!NOTE]
> Linux support in {{ project_name }} is highly experimental/unstable and has known issues.
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
However, unlike Windows, it is recommended to use proxy mode, rather than hook mode.
This is because the current hook mode installation method has to directly launch game
executable. However, by default {{ store_name }} doesn't do that, instead it launches certain wrappers
that setup game environment with optimal parameters. Hence, launching a game without those
wrappers might cause issues in theory. However, in practice real tests show that hook mode has higher
chance of success compared to proxy mode. So, at the end of the day, try both modes to see which one works
best for you.


### 🔀 Proxy mode (🐧 Linux)

Same as on Windows:
1. Rename the original {{ sdk_libs_linux }} to {{ sdk_libs_orig_linux }}
2. Extract and paste the {{ unlocker_libs_linux }} to the same directory, renaming it to {{ sdk_libs_linux }}.

### 🪝 Hook mode (🐧 Linux)

Linux doesn't have the same easily exploitable library injection mechanism that Windows has.
However, it is possible to force any Linux executable to load any library by using the
`LD_PRELOAD` environment variable. In fact, {{ store_name }} itself already uses that to inject its overlay,
hence we can use it as well. But we have to include that overlay library as well when specifying
`LD_PRELOAD`, otherwise the game will be launched with {{ project_name }}, but without {{ store_name }} overlay.

For example:

1. Extract and paste the {{ unlocker_libs_linux }} in the root of game's installation directory.
2. In Steam _Library_ open game's _Properties_, switch to the _General_ tab, and set the following _LAUNCH OPTIONS_:

| Bitness | Launch Options                                                                                                         |
|---------|------------------------------------------------------------------------------------------------------------------------|
| 32-bit  | `LD_PRELOAD="./libsmoke_api32.so $HOME/.local/share/Steam/ubuntu12_32/gameoverlayrenderer.so" ./<GameExe32> %command%` |
| 64-bit  | `LD_PRELOAD="./libsmoke_api64.so $HOME/.local/share/Steam/ubuntu12_64/gameoverlayrenderer.so" ./<GameExe64> %command%` |

Where `<GameExe32>` and `<GameExe64>` correspond to the actual filename of the game executable. For example:
- `TheEscapists2.x86` (32-bit)
- `TheEscapists2.x86_64` (64-bit)
- `_linux/darkest.bin.x86` (32-bit)
- `_linux/darkest.bin.x86_64` (64-bit)
- `bin/linux_x64/eurotrucks2` (64-bit)
- `binaries/victoria3` (64-bit)

And so on. Notice that Linux executables do not have `.exe` extension like on Windows, so make sure to copy the entire
file name as it is. Naturally, the exact options might change depending on where files are located on your filesystem
and other environment variables you might have specified previously.
If you have other environment variables, and you don't know how to correctly combine them,
then please make extensive use of search engines and LLMs for guidance and examples
before seeking help the official forum topic.
