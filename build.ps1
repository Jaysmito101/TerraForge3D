$ErrorActionPreference = "Stop"

$Command = "help"
$Configuration = "Debug"
$GeneratorInput = "visualstudio"
$Architecture = "x64"
$BuildDir = ""
$NoSetup = $false
$Reconfigure = $false
$CleanAll = $false
$CMakeArgs = @()
$ScriptArgs = @($args)

$RootDir = (Resolve-Path $PSScriptRoot).Path
$DefaultBuildRoot = Join-Path $RootDir "build"

function Show-Help {
    Write-Host @"
TerraForge3D build helper

Usage:
  .\build.ps1 setup
  .\build.ps1 configure --generator visualstudio --configuration Debug
  .\build.ps1 build --generator ninja --configuration Release
  .\build.ps1 run --generator visualstudio --configuration Debug
  .\build.ps1 clean --generator ninja
  .\build.ps1 all --generator visualstudio --configuration Release

Commands:
  setup       Initialize and update all Git submodules.
  configure   Generate the selected CMake build tree.
  build       Configure when needed, then build terraforge3d.
  run         Build when needed, then run terraforge3d.
  clean       Remove generated build/CMake files; use --all for every build tree.
  all         Run setup, configure, and build.

Options (defaults are shown in brackets):
  -g, --generator <name>       visualstudio, ninja, make, or xcode [visualstudio]
  -c, --configuration <name>   Debug, Release, RelWithDebInfo, or MinSizeRel [Debug]
  -a, --architecture <name>    x64 or win32; Visual Studio only [x64]
      --build-dir <path>       Custom directory inside build\ [build\windows.$($GeneratorInfo.Id).$Configuration]
      --no-setup                Skip automatic submodule setup
      --reconfigure             Force CMake regeneration
      --all                     With clean, remove every build tree too
      --cmake-arg <arg>         Pass an additional argument to CMake

Default build trees:
  build\windows.visualstudio.$Configuration\
  build\windows.ninja.$Configuration\

Other output:
  build\compile_commands.json  Root clangd database when supported
"@
}

function Read-OptionValue {
    param(
        [Parameter(Mandatory = $true)][string]$Option,
        [Parameter(Mandatory = $true)][ref]$Index
    )

    if ($Index.Value + 1 -ge $ScriptArgs.Count) {
        throw "$Option requires a value"
    }

    $Index.Value = $Index.Value + 1
    $Value = [string]$ScriptArgs[$Index.Value]
    $Index.Value = $Index.Value + 1
    return $Value
}

function Parse-Arguments {
    if ($ScriptArgs.Count -eq 0) {
        return
    }

    $Index = 0
    if ($ScriptArgs[0] -in @("-h", "--help")) {
        $Command = "help"
    }
    else {
        $Command = ([string]$ScriptArgs[0]).ToLowerInvariant()
    }
    $Index++

    while ($Index -lt $ScriptArgs.Count) {
        $Option = [string]$ScriptArgs[$Index]

        if ($Option -in @("-h", "--help")) {
            $Command = "help"
            $Index++
        }
        elseif ($Option -in @("-g", "--generator")) {
            $GeneratorInput = Read-OptionValue $Option ([ref]$Index)
        }
        elseif ($Option -in @("-c", "--configuration")) {
            $Configuration = Read-OptionValue $Option ([ref]$Index)
        }
        elseif ($Option -in @("-a", "--architecture")) {
            $Architecture = Read-OptionValue $Option ([ref]$Index)
        }
        elseif ($Option -eq "--build-dir") {
            $BuildDir = Read-OptionValue $Option ([ref]$Index)
        }
        elseif ($Option -eq "--no-setup") {
            $NoSetup = $true
            $Index++
        }
        elseif ($Option -eq "--reconfigure") {
            $Reconfigure = $true
            $Index++
        }
        elseif ($Option -eq "--all") {
            $CleanAll = $true
            $Index++
        }
        elseif ($Option -eq "--cmake-arg") {
            $CMakeArgs += Read-OptionValue $Option ([ref]$Index)
        }
        else {
            throw "Unknown argument: $Option"
        }
    }

    Set-Variable -Name Command -Value $Command -Scope 1
    Set-Variable -Name GeneratorInput -Value $GeneratorInput -Scope 1
    Set-Variable -Name Configuration -Value $Configuration -Scope 1
    Set-Variable -Name Architecture -Value $Architecture -Scope 1
    Set-Variable -Name BuildDir -Value $BuildDir -Scope 1
    Set-Variable -Name NoSetup -Value $NoSetup -Scope 1
    Set-Variable -Name Reconfigure -Value $Reconfigure -Scope 1
    Set-Variable -Name CleanAll -Value $CleanAll -Scope 1
    Set-Variable -Name CMakeArgs -Value $CMakeArgs -Scope 1
}

Parse-Arguments

switch ($Configuration.ToLowerInvariant()) {
    "debug" { $Configuration = "Debug" }
    "release" { $Configuration = "Release" }
    "relwithdebinfo" { $Configuration = "RelWithDebInfo" }
    "minsizerel" { $Configuration = "MinSizeRel" }
    default { throw "Unsupported configuration: $Configuration" }
}

switch ($GeneratorInput.ToLowerInvariant()) {
    "visualstudio" { $Generator = "visualstudio" }
    "visual-studio" { $Generator = "visualstudio" }
    "ninja" { $Generator = "ninja" }
    "make" { $Generator = "make" }
    "xcode" { $Generator = "xcode" }
    default { throw "Unsupported generator: $GeneratorInput" }
}

switch ($Architecture.ToLowerInvariant()) {
    "x64" { $Architecture = "x64" }
    "win32" { $Architecture = "Win32" }
    default { throw "Unsupported architecture: $Architecture" }
}

function Invoke-Checked {
    param(
        [Parameter(Mandatory = $true)][string]$Tool,
        [Parameter(Mandatory = $true)][string[]]$Arguments
    )

    & $Tool @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$Tool failed with exit code $LASTEXITCODE"
    }
}

function Get-GeneratorInfo {
    if ($Generator -eq "visualstudio") {
        return @{
            Name = "Visual Studio 17 2022"
            Id = "visualstudio"
            MultiConfig = $true
        }
    }

    if ($Generator -eq "ninja") {
        return @{
            Name = "Ninja"
            Id = "ninja"
            MultiConfig = $false
        }
    }

    throw "Generator '$Generator' is not supported by build.ps1. Use visualstudio or ninja on Windows."
}

$GeneratorInfo = Get-GeneratorInfo

if ($CleanAll -and $Command -ne "clean") {
    throw "--all is only valid with the clean command."
}

if ([string]::IsNullOrWhiteSpace($BuildDir)) {
    $BuildDir = Join-Path $DefaultBuildRoot ("windows." + $GeneratorInfo.Id + "." + $Configuration)
}
elseif (-not [IO.Path]::IsPathRooted($BuildDir)) {
    $BuildDir = Join-Path $RootDir $BuildDir
}
$BuildDir = [IO.Path]::GetFullPath($BuildDir)

$BuildRootFull = [IO.Path]::GetFullPath($DefaultBuildRoot).TrimEnd([IO.Path]::DirectorySeparatorChar)
$BuildRootPrefix = $BuildRootFull + [IO.Path]::DirectorySeparatorChar
if (-not $BuildDir.StartsWith($BuildRootPrefix, [StringComparison]::OrdinalIgnoreCase)) {
    throw "BuildDir must be inside $BuildRootFull"
}

function Invoke-Setup {
    Invoke-Checked "git" @("-C", $RootDir, "submodule", "sync", "--recursive")
    Invoke-Checked "git" @("-C", $RootDir, "submodule", "update", "--init", "--recursive")
}

function Invoke-Configure {
    $Arguments = @(
        "-S", $RootDir,
        "-B", $BuildDir,
        "-G", $GeneratorInfo.Name,
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
    )

    if ($GeneratorInfo.MultiConfig) {
        $Arguments += @("-A", $Architecture)
    }
    else {
        $Arguments += "-DCMAKE_BUILD_TYPE=$Configuration"
    }

    if ($CMakeArgs.Count -gt 0) {
        $Arguments += $CMakeArgs
    }

    Invoke-Checked "cmake" $Arguments
    Sync-CompileCommands
}

function Sync-CompileCommands {
    $SourcePath = Join-Path $BuildDir "compile_commands.json"
    $DestinationPath = Join-Path $DefaultBuildRoot "compile_commands.json"

    if (Test-Path $SourcePath) {
        New-Item -ItemType Directory -Path $DefaultBuildRoot -Force | Out-Null
        Copy-Item -LiteralPath $SourcePath -Destination $DestinationPath -Force
        Write-Host "Copied compile_commands.json to $DestinationPath"
    }
    else {
        Write-Warning "This generator did not produce compile_commands.json; use Ninja for clangd support."
    }
}

function Ensure-Configured {
    $CachePath = Join-Path $BuildDir "CMakeCache.txt"
    if ($Reconfigure -or -not (Test-Path $CachePath)) {
        Invoke-Configure
    }
}

function Invoke-Build {
    Ensure-Configured

    $Arguments = @(
        "--build", $BuildDir,
        "--target", "terraforge3d",
        "--parallel"
    )

    if ($GeneratorInfo.MultiConfig) {
        $Arguments += @("--config", $Configuration)
    }

    Invoke-Checked "cmake" $Arguments
    Sync-CompileCommands
}

function Get-ExecutablePath {
    if ($GeneratorInfo.MultiConfig) {
        return Join-Path $BuildDir (Join-Path $Configuration "terraforge3d.exe")
    }

    return Join-Path $BuildDir "terraforge3d.exe"
}

function Invoke-Run {
    $Executable = Get-ExecutablePath
    if (-not (Test-Path $Executable)) {
        Invoke-Build
    }

    if (-not (Test-Path $Executable)) {
        throw "Build completed without producing $Executable"
    }

    & $Executable
    if ($LASTEXITCODE -ne 0) {
        throw "terraforge3d exited with code $LASTEXITCODE"
    }
}

function Test-RootPathTracked {
    param([Parameter(Mandatory = $true)][string]$RelativePath)

    & git -C $RootDir ls-files --error-unmatch -- $RelativePath *> $null
    return $LASTEXITCODE -eq 0
}

function Remove-GeneratedRootPath {
    param([Parameter(Mandatory = $true)][string]$RelativePath)

    $Path = Join-Path $RootDir $RelativePath
    if (-not (Test-Path -LiteralPath $Path)) {
        return
    }

    if (Test-RootPathTracked $RelativePath) {
        Write-Host "Preserved tracked path: $RelativePath"
        return
    }

    Remove-Item -LiteralPath $Path -Recurse -Force
    Write-Host "Removed generated path: $RelativePath"
}

function Remove-GeneratedRootFiles {
    $GeneratedPaths = @(
        "CMakeCache.txt",
        "CMakeFiles",
        "CMakeScripts",
        "cmake_install.cmake",
        "CTestTestfile.cmake",
        "install_manifest.txt",
        "Makefile",
        "Debug",
        "Release",
        "gladsources",
        "x64",
        ".vs",
        "build\compile_commands.json"
    )

    foreach ($RelativePath in $GeneratedPaths) {
        Remove-GeneratedRootPath $RelativePath
    }

    $GeneratedFilePatterns = @("*.sln", "*.vcxproj", "*.vcxproj.filters", "*.vcxproj.user", "*.aps", "*.make")
    foreach ($Pattern in $GeneratedFilePatterns) {
        Get-ChildItem -LiteralPath $RootDir -File -Force -Filter $Pattern | ForEach-Object {
            Remove-GeneratedRootPath $_.Name
        }
    }

    Get-ChildItem -LiteralPath $RootDir -Directory -Force |
        Where-Object { $_.Name -like "*.dir" } |
        ForEach-Object { Remove-GeneratedRootPath $_.Name }
}

function Invoke-Clean {
    $BuildRoot = [IO.Path]::GetFullPath($DefaultBuildRoot).TrimEnd([IO.Path]::DirectorySeparatorChar)
    $AllowedPrefix = $BuildRoot + [IO.Path]::DirectorySeparatorChar

    if ($CleanAll) {
        Remove-GeneratedRootPath "build"
    }
    else {
        if (-not $BuildDir.StartsWith($AllowedPrefix, [StringComparison]::OrdinalIgnoreCase)) {
            throw "Refusing to clean a directory outside $BuildRoot"
        }

        if (Test-Path $BuildDir) {
            Remove-Item -LiteralPath $BuildDir -Recurse -Force
            Write-Host "Removed $BuildDir"
        }
    }

    Remove-GeneratedRootFiles
}

if ($Command -eq "help") {
    Show-Help
    exit 0
}

if ($Command -eq "setup") {
    Invoke-Setup
    exit 0
}

if ($Command -in @("configure", "build", "run", "all")) {
    if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
        throw "CMake was not found on PATH."
    }

    if ($GeneratorInfo.Id -eq "ninja" -and -not (Get-Command ninja -ErrorAction SilentlyContinue)) {
        throw "Ninja was selected but was not found on PATH. Install Ninja or use -Generator VisualStudio."
    }
}

if (-not $NoSetup -and $Command -in @("configure", "build", "run", "all")) {
    Invoke-Setup
}

switch ($Command) {
    "configure" { Invoke-Configure }
    "build" { Invoke-Build }
    "run" { Invoke-Run }
    "clean" { Invoke-Clean }
    "all" {
        Invoke-Configure
        Invoke-Build
    }
}
