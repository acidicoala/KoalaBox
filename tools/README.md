# KoalaBox tools

## Exports generator

Generates linker exports that enable automatic proxy DLL injection via search order exploit.

## Sync [Work-In-Progress]

Synchronizes or generates files via project-specific `sync.json` config file.
Helps to maintain consistency across several projects that:
- share common files such as:
    - license
    - release zip readme
    - IDE inspections
- derive file content from other sources like:
    - $version in JSON schema from c++ headers
    - Unlocker config from JSON schema
    - README.md sections from JSON schema
