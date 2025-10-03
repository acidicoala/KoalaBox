## 🛠 Installation instructions (🐧 Linux)

Just like on Windows, {{ project_name }} features 32 and 64-bit Linux builds.
In release archive they are named as {{ unlocker_libs_linux }} respectively.

### 🪝 Hook mode (🐧 Linux)

Linux doesn't have the same easily exploitable library injection mechanism that Windows has.
However, it is possible to force any Linux executable to load any library by using the
`LD_PRELOAD` environment variable.
For example:

1. Extract and paste the {{ unlocker_libs_linux }} next to the game's executable.
2. In Steam _Library_ open game's _Properties_, switch to the _General_ tab, and set the following _LAUNCH OPTIONS_:

| Bitness | Launch Options                                           |
|---------|----------------------------------------------------------|
| 32-bit  | `LD_PRELOAD=./libsmoke_api32.so ./GameExecutable.x86`    |
| 64-bit  | `LD_PRELOAD=./libsmoke_api64.so ./GameExecutable.x86_64` |

Naturally, the exact options might change depending on how files are located on your filesystem
or depending on other environment variables you might have configured.
If you have other environment variables, and you don't know how to correctly combine them,
then please make heavy use of search engines and LLMs for guidance examples instead of the official forum topic.

### 🔀 Proxy mode (🐧 Linux)

Same as on Windows:
1. Rename the original {{ sdk_libs_linux }} to {{ sdk_libs_orig_linux }}
2. Extract and paste the {{ unlocker_libs_linux }} to the same directory, renaming it to {{ sdk_libs_linux }}.