Set-Location (Get-Item $PSScriptRoot)

$generator = 'Visual Studio 16 2019'
$platform = 'x64'
$build_dir = './build/64'
$config = 'Debug'

cmake -G "$generator" -A "$platform" -B "$build_dir"

cmake --build "$build_dir" --config "$config"
