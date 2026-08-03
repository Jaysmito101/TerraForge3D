#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$SCRIPT_DIR"
BUILD_ROOT="$ROOT_DIR/build"

COMMAND="help"
CONFIGURATION="Debug"
GENERATOR_INPUT="ninja"
GENERATOR_ID="ninja"
ARCHITECTURE="x64"
BUILD_DIR=""
NO_SETUP=0
RECONFIGURE=0
CLEAN_ALL=0
CMAKE_ARGS=()

usage() {
    cat <<EOF
TerraForge3D build helper for macOS

Usage:
  ./build.sh setup
  ./build.sh configure --generator ninja --configuration Debug
  ./build.sh build --generator ninja --configuration Release
  ./build.sh run --generator ninja --configuration Debug
  ./build.sh clean --generator ninja
  ./build.sh all --generator make --configuration Release

Commands:
  setup       Initialize and update all Git submodules.
  configure   Generate the selected CMake build tree.
  build       Configure when needed, then build terraforge3d.
  run         Build when needed, then run terraforge3d.
  clean       Remove generated build/CMake files; use --all for every build tree.
  all         Run setup, configure, and build.

Options (defaults are shown in brackets):
  -g, --generator <name>       visualstudio, ninja, make, or xcode [ninja]
  -c, --configuration <name>   Debug, Release, RelWithDebInfo, or MinSizeRel [Debug]
  -a, --architecture <name>    x64 or win32; Visual Studio only [x64]
      --build-dir <path>       Custom directory inside build/ [build/macos.$GENERATOR_ID.$CONFIGURATION]
      --no-setup                Skip automatic submodule setup
      --reconfigure             Force CMake regeneration
      --all                     With clean, remove every build tree too
      --cmake-arg <arg>         Pass an additional argument to CMake

Default build trees:
  build/macos.ninja.$CONFIGURATION/
  build/macos.make.$CONFIGURATION/
  build/macos.xcode.$CONFIGURATION/

Other output:
  build/compile_commands.json  Root clangd database when supported
EOF
}

die() {
    echo "Error: $*" >&2
    exit 1
}

require_command() {
    command -v "$1" >/dev/null 2>&1 || die "$1 was not found on PATH."
}

lowercase() {
    printf '%s' "$1" | tr '[:upper:]' '[:lower:]'
}

normalize_configuration() {
    case "$(lowercase "$CONFIGURATION")" in
        debug) CONFIGURATION="Debug" ;;
        release) CONFIGURATION="Release" ;;
        relwithdebinfo) CONFIGURATION="RelWithDebInfo" ;;
        minsizerel) CONFIGURATION="MinSizeRel" ;;
        *) die "Unsupported configuration: $CONFIGURATION" ;;
    esac
}

select_generator() {
    case "$(lowercase "$GENERATOR_INPUT")" in
        ninja)
            CMAKE_GENERATOR="Ninja"
            GENERATOR_ID="ninja"
            MULTI_CONFIG=0
            ;;
        make|unixmakefiles|"unix makefiles")
            CMAKE_GENERATOR="Unix Makefiles"
            GENERATOR_ID="make"
            MULTI_CONFIG=0
            ;;
        xcode)
            CMAKE_GENERATOR="Xcode"
            GENERATOR_ID="xcode"
            MULTI_CONFIG=1
            ;;
        visualstudio|visual-studio|"visual studio")
            die "Visual Studio is not available on macOS. Use --generator ninja, make, or xcode."
            ;;
        *) die "Unsupported generator: $GENERATOR_INPUT" ;;
    esac
}

normalize_architecture() {
    case "$(lowercase "$ARCHITECTURE")" in
        x64) ARCHITECTURE="x64" ;;
        win32) ARCHITECTURE="Win32" ;;
        *) die "Unsupported architecture: $ARCHITECTURE" ;;
    esac
}

parse_arguments() {
    if [[ $# -gt 0 ]]; then
        case "$1" in
            -h|--help)
                usage
                exit 0
                ;;
            *)
                COMMAND="$1"
                shift
                ;;
        esac
    fi

    while [[ $# -gt 0 ]]; do
        case "$1" in
            -h|--help)
                usage
                exit 0
                ;;
            -c|--configuration)
                [[ $# -ge 2 ]] || die "$1 requires a value"
                CONFIGURATION="$2"
                shift 2
                ;;
            -g|--generator)
                [[ $# -ge 2 ]] || die "$1 requires a value"
                GENERATOR_INPUT="$2"
                shift 2
                ;;
            -a|--architecture)
                [[ $# -ge 2 ]] || die "$1 requires a value"
                ARCHITECTURE="$2"
                shift 2
                ;;
            --build-dir)
                [[ $# -ge 2 ]] || die "$1 requires a value"
                BUILD_DIR="$2"
                shift 2
                ;;
            --no-setup)
                NO_SETUP=1
                shift
                ;;
            --reconfigure)
                RECONFIGURE=1
                shift
                ;;
            --all)
                CLEAN_ALL=1
                shift
                ;;
            --cmake-arg)
                [[ $# -ge 2 ]] || die "$1 requires a value"
                CMAKE_ARGS+=("$2")
                shift 2
                ;;
            *)
                die "Unknown argument: $1"
                ;;
        esac
    done
}

setup_submodules() {
    git -C "$ROOT_DIR" submodule sync --recursive
    git -C "$ROOT_DIR" submodule update --init --recursive
}

configure_project() {
    local args=(
        -S "$ROOT_DIR"
        -B "$BUILD_DIR"
        -G "$CMAKE_GENERATOR"
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    )

    if [[ "$MULTI_CONFIG" -eq 0 ]]; then
        args+=("-DCMAKE_BUILD_TYPE=$CONFIGURATION")
    fi

    if [[ "${#CMAKE_ARGS[@]}" -gt 0 ]]; then
        args+=("${CMAKE_ARGS[@]}")
    fi

    cmake "${args[@]}"
    sync_compile_commands
}

sync_compile_commands() {
    local source_path="$BUILD_DIR/compile_commands.json"
    local destination_path="$BUILD_ROOT/compile_commands.json"

    if [[ -f "$source_path" ]]; then
        mkdir -p "$BUILD_ROOT"
        cp -f -- "$source_path" "$destination_path"
        echo "Copied compile_commands.json to $destination_path"
    else
        echo "Warning: this generator did not produce compile_commands.json; use Ninja for clangd support." >&2
    fi
}

ensure_configured() {
    if [[ "$RECONFIGURE" -eq 1 || ! -f "$BUILD_DIR/CMakeCache.txt" ]]; then
        configure_project
    fi
}

build_project() {
    ensure_configured

    local args=(--build "$BUILD_DIR" --target terraforge3d --parallel)
    if [[ "$MULTI_CONFIG" -eq 1 ]]; then
        args+=(--config "$CONFIGURATION")
    fi

    cmake "${args[@]}"
    sync_compile_commands
}

executable_path() {
    if [[ "$MULTI_CONFIG" -eq 1 ]]; then
        printf '%s\n' "$BUILD_DIR/$CONFIGURATION/terraforge3d"
    else
        printf '%s\n' "$BUILD_DIR/terraforge3d"
    fi
}

run_project() {
    local executable
    executable="$(executable_path)"
    if [[ ! -x "$executable" ]]; then
        build_project
    fi

    [[ -x "$executable" ]] || die "Build completed without producing $executable"
    "$executable"
}

root_path_is_tracked() {
    git -C "$ROOT_DIR" ls-files --error-unmatch -- "$1" >/dev/null 2>&1
}

remove_generated_root_path() {
    local relative_path="$1"
    local target_path="$ROOT_DIR/$relative_path"

    [[ -e "$target_path" || -L "$target_path" ]] || return 0

    case "$target_path" in
        "$ROOT_DIR"/*) ;;
        *) die "Refusing to remove a path outside $ROOT_DIR: $target_path" ;;
    esac

    if root_path_is_tracked "$relative_path"; then
        echo "Preserved tracked path: $relative_path"
        return 0
    fi

    rm -rf -- "$target_path"
    echo "Removed generated path: $relative_path"
}

remove_generated_root_files() {
    local relative_path
    local pattern
    local target_path

    for relative_path in \
        CMakeCache.txt \
        CMakeFiles \
        CMakeScripts \
        cmake_install.cmake \
        CTestTestfile.cmake \
        install_manifest.txt \
        Makefile \
        Debug \
        Release \
        gladsources \
        x64 \
        .vs \
        build/compile_commands.json; do
        remove_generated_root_path "$relative_path"
    done

    for pattern in '*.sln' '*.vcxproj' '*.vcxproj.filters' '*.vcxproj.user' '*.aps' '*.make'; do
        for target_path in "$ROOT_DIR"/$pattern; do
            [[ -f "$target_path" ]] || continue
            relative_path="${target_path#"$ROOT_DIR/"}"
            remove_generated_root_path "$relative_path"
        done
    done

    for target_path in "$ROOT_DIR"/*.dir; do
        [[ -d "$target_path" ]] || continue
        relative_path="${target_path#"$ROOT_DIR/"}"
        remove_generated_root_path "$relative_path"
    done
}

clean_project() {
    local build_root="${BUILD_ROOT%/}"
    local allowed_prefix="$build_root/"

    if [[ "$CLEAN_ALL" -eq 1 ]]; then
        remove_generated_root_path "build"
    else
        [[ "$BUILD_DIR" == "$allowed_prefix"* && "$BUILD_DIR" != "$build_root" ]] || \
            die "Refusing to clean a directory outside $build_root"

        if [[ -e "$BUILD_DIR" ]]; then
            rm -rf -- "$BUILD_DIR"
            echo "Removed $BUILD_DIR"
        fi
    fi

    remove_generated_root_files
}

parse_arguments "$@"
normalize_configuration
select_generator
normalize_architecture

[[ "$CLEAN_ALL" -eq 0 || "$COMMAND" == "clean" ]] || die "--all is only valid with the clean command."

if [[ -z "$BUILD_DIR" ]]; then
    BUILD_DIR="$BUILD_ROOT/macos.$GENERATOR_ID.$CONFIGURATION"
elif [[ "$BUILD_DIR" != /* ]]; then
    BUILD_DIR="$ROOT_DIR/$BUILD_DIR"
fi

case "$BUILD_DIR" in
    "$BUILD_ROOT"/*) ;;
    *) die "Build directory must be inside $BUILD_ROOT" ;;
esac

case "$COMMAND" in
    configure|build|run|all)
        require_command cmake
        [[ "$GENERATOR_ID" != "ninja" ]] || require_command ninja
        ;;
esac

case "$COMMAND" in
    help)
        usage
        ;;
    setup)
        setup_submodules
        ;;
    configure)
        [[ "$NO_SETUP" -eq 1 ]] || setup_submodules
        configure_project
        ;;
    build)
        [[ "$NO_SETUP" -eq 1 ]] || setup_submodules
        build_project
        ;;
    run)
        [[ "$NO_SETUP" -eq 1 ]] || setup_submodules
        run_project
        ;;
    clean)
        clean_project
        ;;
    all)
        [[ "$NO_SETUP" -eq 1 ]] || setup_submodules
        configure_project
        build_project
        ;;
    *)
        die "Unknown command: $COMMAND"
        ;;
esac
