# KoalaBox tools

## Exports generator

Generates linker exports that enable automatic proxy DLL injection via search order exploit.

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
