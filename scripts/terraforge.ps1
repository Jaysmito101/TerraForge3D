[CmdletBinding()]
param(
    [Parameter(Position = 0)]
    [ValidateSet("setup", "configure", "build", "run", "clean", "all", "help")]
    [string]$Command = "help",

    [ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")]
    [string]$Configuration = "Debug",

    [ValidateSet("VisualStudio", "Ninja")]
    [string]$Generator = "VisualStudio",

    [ValidateSet("x64", "Win32")]
    [string]$Architecture = "x64",

    [string]$BuildDir = "",
    [switch]$NoSetup,
    [switch]$Reconfigure,
    [string[]]$CMakeArg
)

$ErrorActionPreference = "Stop"

$RootDir = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$DefaultBuildRoot = Join-Path $RootDir "build"

function Show-Help {
    Write-Host @"
TerraForge3D build helper

Usage:
  .\scripts\terraforge.ps1 setup
  .\scripts\terraforge.ps1 configure -Generator VisualStudio -Configuration Debug
  .\scripts\terraforge.ps1 build -Generator Ninja -Configuration Release
  .\scripts\terraforge.ps1 run -Generator VisualStudio -Configuration Debug
  .\scripts\terraforge.ps1 clean -Generator Ninja
  .\scripts\terraforge.ps1 all -Generator VisualStudio -Configuration Release

Commands:
  setup       Initialize and update all git submodules.
  configure   Generate the selected CMake build tree.
  build       Configure when needed, then build terraforge3d.
  run         Build when needed, then run terraforge3d.
  clean       Remove the selected build tree.
  all         Run setup, configure, and build.

Options:
  -Generator VisualStudio|Ninja       Default: VisualStudio
  -Configuration Debug|Release|RelWithDebInfo|MinSizeRel
  -Architecture x64|Win32            Visual Studio only; default: x64
  -BuildDir <path>                   Override it inside the root build directory
  -NoSetup                           Skip automatic submodule setup
  -Reconfigure                       Force CMake regeneration
  -CMakeArg <arg>                    Pass an additional argument to CMake

Build trees:
  build\windows.visualstudio\
  build\windows.ninja\
  build\compile_commands.json        clangd database when supported
"@
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
    if ($Generator -eq "VisualStudio") {
        return @{
            Name = "Visual Studio 17 2022"
            Id = "visualstudio"
            MultiConfig = $true
        }
    }

    return @{
        Name = "Ninja"
        Id = "ninja"
        MultiConfig = $false
    }
}

$GeneratorInfo = Get-GeneratorInfo

if ([string]::IsNullOrWhiteSpace($BuildDir)) {
    $BuildDir = Join-Path $DefaultBuildRoot ("windows." + $GeneratorInfo.Id)
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

    if ($CMakeArg) {
        $Arguments += $CMakeArg
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

function Invoke-Clean {
    $BuildRoot = [IO.Path]::GetFullPath($DefaultBuildRoot).TrimEnd([IO.Path]::DirectorySeparatorChar)
    $AllowedPrefix = $BuildRoot + [IO.Path]::DirectorySeparatorChar

    if (-not $BuildDir.StartsWith($AllowedPrefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to clean a directory outside $BuildRoot"
    }

    if (Test-Path $BuildDir) {
        Remove-Item -LiteralPath $BuildDir -Recurse -Force
        Write-Host "Removed $BuildDir"
    }
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
