## 🛠 Installation instructions

> [!NOTE]
> To determine the bitness of a game you can open _Task Manager_, navigate to _Details_ tab,
> right-click on the column headers, click _Select columns_, tick checkbox next to _Platform_ and click _OK_.
> This will allow you to see a game's bitness in the _Details_ tab while a game is running.

### 🪝 Hook mode

Hook mode itself has two sub-modes: Self-Hook and Hook with injector.

#### 🪝 Self-Hook mode

In self-hook mode {{ project_name }} is injected automatically without the help of third-party injectors.
It works best when a game doesn't have any protections against DLL injection.
The main advantage of this mode is its minimal setup, which adds only 1 new DLL to the game folder.

- Download the [latest {{ project_name }} release zip].
- From this downloaded zip extract {{ unlocker_dll_names }}, depending on a game's bitness.
- Rename the unzipped DLL to {{ self_inject_dll }}.
- Place this {{ self_inject_dll }} next to the game's `.exe` file.

#### 🪝 Hook mode with Koaloader

If a game doesn't load {{ self_inject_dll }}, you can use an alternative injector to load
{{ project_name }} into the game process.
One such injector is [Koaloader], which supports different DLLs that a typical game might load.
For example, assuming that the game loads `d3d11.dll`:

- Install Koaloader:
    - Download the [latest Koaloader release zip].
    - From this downloaded zip extract `d3d11.dll` from `d3d11-32` or `d3d11-64`, depending on a game's bitness.
    - Place `d3d11.dll` next to the game's `.exe` file.
- Install {{ project_name }}
    - Download the [latest {{ project_name }} release zip].
    - From this downloaded zip extract {{ unlocker_dll_names }}, depending on a game's bitness.
    - Place {{ unlocker_dll_names }} next to the game's `.exe` file.

#### 🪝 Hook mode with Special K

There are games which have extra protections that break hook mode.
In such cases, it might be worth trying [Special K], which can inject {{ project_name }} as a [custom plugin].

### 🔀 Proxy mode

- Find a {{ sdk_dll_names }} file in game directory, and rename it to {{ sdk_dll_orig_names }}.
- Download the [latest {{ project_name }} release zip].
- From this downloaded zip extract {{ unlocker_dll_names }}, depending on a game's bitness.
- Rename this extracted DLL to {{ sdk_dll_names }}, depending on a game's bitness.
- Place this renamed unlocker DLL next to the {{ sdk_dll_orig_names }} file.

[latest {{ project_name }} release zip]: {{ github_repo_url }}/releases/latest
[latest Koaloader release zip]: https://github.com/acidicoala/Koaloader/releases/latest
[Special K]: https://www.special-k.info
[custom plugin]: https://wiki.special-k.info/en/SpecialK/Tools#custom-plugin