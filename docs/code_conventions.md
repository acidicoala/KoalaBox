# Code conventions

## Imports

Imports are sorted by alphabetical order and grouped by their category:

1. System library (e.g. Windows SDK, C++ standard library)
2. Third party library (e.g. spdlog, json)
3. Local file (e.g. `koalabox/util.hpp`)

Groups are separated by a single new line. Example:

```c++
#include <Windows.h>
#include <string>

#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

#include <koalabox/util.hpp>
```