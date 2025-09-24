# KoalaBox tools

## Exports generator

### Windows

Generates linker exports header that forwards all exports to original library

### Linux

Generates c++ source file that implements exported functions,
which dynamically call and return results from original library.

## Sync

Synchronizes or generates files via project-specific `sync.json` config file.
Helps in maintaining consistency across several projects that:
- share common files such as:
    - license
    - release zip readme
    - IDE inspections
- derive file content from other sources like:
    - Unlocker config from JSON schema
    - Config options and example from JSON schema
