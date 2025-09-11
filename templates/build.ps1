# Usage:
# PS ..\{{ project_name }}> .\build.ps1 64 Debug

Set-Location (Get-Item $PSScriptRoot)

. ./KoalaBox/build.ps1 {{ project_name }} @args

Build-Project
