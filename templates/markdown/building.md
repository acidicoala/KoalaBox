## 🏗️ Building from source

### ✔️ Requirements

- [CMake] v3.24 or newer (Make sure that cmake is available from powershell)
- [Visual Studio Build Tools 2022] with `Desktop Development for C++` workload installed
    - Tested on Windows 11 SDK (10.0.26100.4188)

### 👨‍💻 Commands

Build the project

```powershell
.\build.ps1 $arch $config
```

where

| Variable | Valid values         |
|----------|----------------------|
| $arch    | `32` or `64`         |
| $config  | `Debug` or `Release` |

For example:

```powershell
.\build.ps1 64 Release
```

[Visual Studio Build Tools 2022]: https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022
[CMake]: https://cmake.org/