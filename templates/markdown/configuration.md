## ⚙ Configuration

{{ project_name }} does not require any manual configuration.
By default, it uses the most reasonable options and tries to unlock all DLCs that it can.
However, there might be circumstances in which you need more custom-tailored behaviour, such as disabling certain DLCs, or selectively enabling just a few of them.
In this case you can use a configuration file [{{ config_filename }}](res/{{config_filename}}) that you can find here in this repository or in the release zip.
To use it, simply place it next to the {{ project_name }} DLL.
It will be read upon each launch of a game.

The config file is expected to conform to the JSON standard. It is recommended to use a text editor with JSON schema supports. This greatly assists with editing since it warns not just about syntax errors, but also about invalid values. One such editor is [Visual Studio Code](https://code.visualstudio.com/).
<details><summary>VS Code demo</summary>

This example showcases how VS code highlights an invalid value and displays a list of valid values that are accepted.
![VS Code demo](https://i.ibb.co/0y45qgtQ/schema-config-demo.jpg)
</details>

Below you can find the detailed description of each available config option. In the absence of the config file, default values specified below will be used.

{{ jsonSchemaToConfigTable(json_schema_path, false) }}

<details><summary>Advanced options</summary>

> [!NOTE] These options do not affect the unlocker, and should be left unmodified.
> They serve as utilities for text or GUI editors.

{{ jsonSchemaToConfigTable(json_schema_path, true) }}
</details>

---

Below you can find an example config where nearly every option has been customized.

<details><summary>Complete example</summary>

```json
{{
  jsonSchemaToExample(json_schema_path)
}}
```
</details>
