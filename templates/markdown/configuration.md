## ⚙ Configuration

{{ project_name }} does not require any manual configuration.
By default, it uses the most reasonable options and tries to unlock all DLCs that it can.
However, there might be circumstances in which you need more custom-tailored behaviour, such as disabling certain DLCs, or selectively enabling just a few of them.
In this case you can use a configuration file [{{ config_filename }}](res/{{config_filename}}) that you can find here in this repository or in the release zip.
To use it, simply place it next to the {{ project_name }} DLL.
It will be read upon each launch of a game.

The config file is expected to conform to the JSON standard. It is recommended to use a text editor with JSON schema supports. This greatly assists with editing since it warns not just about syntax errors, but also about invalid values.

Below you can find the detailed description of each available config option. In the absence of the config file, default values specified below will be used.

[//]: # (TODO: Add VS Code example screenshot)

| Option | Description | Type | Default | Valid values |
|--------|-------------|------|---------|--------------|
{{ jsonSchemaToConfigTable(json_schema_path, false) }}

[//]: # (TODO: Use footnotes to link to DB URL: https://docs.github.com/en/get-started/writing-on-github/getting-started-with-writing-and-formatting-on-github/basic-writing-and-formatting-syntax#footnotes)

[//]: # (TODO: Add not about previous config version)