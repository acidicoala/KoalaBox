# Code conventions

## Imports

Imports are sorted by alphabetical order and grouped by their category:

1. System library (e.g. Windows SDK, C++ standard library)
2. Third party library (e.g. spdlog, json)
3. Koalabox library (e.g. `koalabox/logger.hpp`)
4. Generates files (e.g. `build_config.h`)
5. Project file (e.g. `main.hpp`)

Groups are separated by a single new line. Example:

```c++
#include <Windows.h>
#include <string>

#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

#include <koalabox/logger.hpp>
#include <koalabox/util.hpp>

#include "build_config.h"

#include "main.hpp"
```

Additionally, includes for libraries outside the current project should be using angle brackets
(e.g. `#include <string>`), whereas includes for project files (including generated) should use
double quotes (`#include "dllmain.hpp"`).

## Code style

Projects should use the style stored in IDE, specifically
[acidicoala.xml](https://github.com/acidicoala/acidicoala/blob/main/idea-configs/style/acidicoala.xml).

## Code inspections

Projects should use the inspections stored in IDE, specifically
[acidicoala.xml](https://github.com/acidicoala/acidicoala/blob/main/idea-configs/inspections/acidicoala.xml).

## Global variables

Do NOT use global variables without explicit initialization. MSVC initializes them correctly, but Clang fails to do so.
