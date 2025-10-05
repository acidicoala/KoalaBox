{% extends "./templates/README.base.md" %}
{% block content %}

_C++ utilities for koality projects_

## 🛠️ System requirements

### 🪟 Windows

- MSVC toolchain

### 🐧 Linux

inja::## for package in ["brotli", "gcc", "clang", "gtk3", "libidn2", "libnghttp2", "libssh2", "openssl", "zlib", "zstd"]
- [{{ package }}](https://archlinux.org/packages/core/x86_64/{{ package }}/)
inja::## endfor

## ❓Trivia

The name of this project is inspired by [BusyBox], which provides several Unix utilities.
In a similar fashion, KoalaBox provides utilities for developing koality projects, hence the name.

[BusyBox]: https://en.wikipedia.org/wiki/BusyBox

{% include "templates/markdown/acknowledgements.md" %}

{% endblock %}