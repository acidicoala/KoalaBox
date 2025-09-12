{% extends "./templates/README.base.md" %}
{% block content %}

_C++ utilities for koality projects_

## 🛠️ System requirements

### 🪟 Windows

- MSVC toolchain

### 🐧 Linux

- Clang compiler toolchain
- GTK 3

## ❓Trivia

The name of this project is inspired by [BusyBox], which provides several Unix utilities.
In a similar fashion, KoalaBox provides utilities for developing koality projects, hence the name.

[BusyBox]: https://en.wikipedia.org/wiki/BusyBox

{% include "templates/markdown/acknowledgements.md" %}

{% endblock %}