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
REBUILD=0
CLEAN_ALL=0
CMAKE_ARGS=()
BUILD_LOCK_DIR="$ROOT_DIR/.terraforge3d-build.lock"
BUILD_LOCK_HELD=0

usage() {
    cat <<EOF
TerraForge3D build helper for macOS

Usage:
  ./build.sh setup
  ./build.sh configure --generator ninja --configuration Debug
  ./build.sh build --generator ninja --configuration Release
  ./build.sh format
  ./build.sh run --generator ninja --configuration Debug
  ./build.sh clean --generator ninja
  ./build.sh all --generator make --configuration Release

Commands:
  setup       Initialize and update all Git submodules.
  configure   Generate the selected CMake build tree.
  build       Configure when needed, then build terraforge3d.
  format      Run clang-format on TerraForge3D-owned C/C++ files.
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
      --rebuild                 Clean and rebuild before running/building
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

cleanup_build_lock() {
    if [[ "$BUILD_LOCK_HELD" -eq 1 ]]; then
        rm -rf -- "$BUILD_LOCK_DIR"
        BUILD_LOCK_HELD=0
    fi
}

interrupt_build() {
    cleanup_build_lock
    exit "$1"
}

trap cleanup_build_lock EXIT
trap 'interrupt_build 130' INT
trap 'interrupt_build 143' TERM

build_lock_owner_is_alive() {
    local owner_pid="$1"
    local owner_start="$2"
    local current_start

    [[ "$owner_pid" =~ ^[0-9]+$ ]] || return 1
    kill -0 "$owner_pid" 2>/dev/null || return 1

    if [[ -n "$owner_start" ]]; then
        current_start="$(ps -p "$owner_pid" -o lstart= 2>/dev/null | sed 's/^[[:space:]]*//')"
        [[ -z "$current_start" || "$current_start" == "$owner_start" ]] || return 1
    fi

    return 0
}

acquire_build_lock() {
    local owner_file
    local owner_pid
    local owner_start
    local stale_lock
    local wait_seconds=0
    local wait_logged=0

    while ! mkdir -- "$BUILD_LOCK_DIR" 2>/dev/null; do
        if [[ "$wait_logged" -eq 0 ]]; then
            echo "Another TerraForge3D session is already running; waiting for it to finish..." >&2
            wait_logged=1
        fi

        owner_file="$BUILD_LOCK_DIR/owner"
        if [[ ! -f "$owner_file" ]]; then
            sleep 1
            wait_seconds=$((wait_seconds + 1))
            if (( wait_seconds % 5 == 0 )); then
                echo "Still waiting for the active TerraForge3D session to finish..." >&2
            fi
            stale_lock="${BUILD_LOCK_DIR}.stale.$$.$RANDOM"
            if mv -- "$BUILD_LOCK_DIR" "$stale_lock" 2>/dev/null; then
                rm -rf -- "$stale_lock"
            fi
            continue
        fi

        IFS=' ' read -r owner_pid owner_start < "$owner_file" || true
        if ! build_lock_owner_is_alive "$owner_pid" "$owner_start"; then
            stale_lock="${BUILD_LOCK_DIR}.stale.$$.$RANDOM"
            if mv -- "$BUILD_LOCK_DIR" "$stale_lock" 2>/dev/null; then
                rm -rf -- "$stale_lock"
            fi
            continue
        fi

        wait_seconds=$((wait_seconds + 1))
        if (( wait_seconds == 1 || wait_seconds % 5 == 0 )); then
            echo "Waiting for TerraForge3D session PID $owner_pid to finish..." >&2
        fi
        sleep 1
    done

    local owner_tmp="$BUILD_LOCK_DIR/owner.tmp.$$"
    printf '%s %s\n' "$$" "$(ps -p "$$" -o lstart= 2>/dev/null | sed 's/^[[:space:]]*//')" > "$owner_tmp"
    mv -- "$owner_tmp" "$BUILD_LOCK_DIR/owner"
    BUILD_LOCK_HELD=1
    if [[ "$wait_logged" -eq 1 ]]; then
        echo "Previous TerraForge3D session finished; continuing." >&2
    fi
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
            --rebuild)
                REBUILD=1
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
    format_project
    ensure_configured

    local args=(--build "$BUILD_DIR" --target terraforge3d --parallel)
    if [[ "$REBUILD" -eq 1 ]]; then
        args+=(--clean-first)
    fi
    if [[ "$MULTI_CONFIG" -eq 1 ]]; then
        args+=(--config "$CONFIGURATION")
    fi

    cmake "${args[@]}"
    sync_compile_commands
}

format_project() {
    require_command clang-format
    require_command rg
    [[ -f "$ROOT_DIR/.clang-format" ]] || die "The clang-format configuration was not found at $ROOT_DIR/.clang-format"

    local -a rg_args=(
        --files
        --glob '*.c'
        --glob '*.cc'
        --glob '*.cpp'
        --glob '*.cxx'
        --glob '*.h'
        --glob '*.hh'
        --glob '*.hpp'
        --glob '*.hxx'
        --glob '*.inl'
        --glob '!vendor/**'
        --glob '!**/vendor/**'
        --glob '!build/**'
        --glob '!**/build/**'
        --glob '!build-*/*'
        --glob '!**/build-*/*'
    )
    local relative_files
    if relative_files="$(cd "$ROOT_DIR" && rg "${rg_args[@]}")"; then
        :
    else
        local rg_status=$?
        [[ "$rg_status" -eq 1 ]] || die "rg failed while discovering files for formatting with exit code $rg_status"
    fi

    local -a files=()
    local relative_file
    while IFS= read -r relative_file; do
        [[ -n "$relative_file" ]] || continue
        files+=("$ROOT_DIR/$relative_file")
    done <<< "$relative_files"

    if [[ "${#files[@]}" -eq 0 ]]; then
        echo "No project C/C++ files found to format."
        return 0
    fi

    echo "Formatting ${#files[@]} project C/C++ files with clang-format..."
    clang-format \
        -i \
        --style=file \
        --fallback-style=none \
        --sort-includes=false \
        "${files[@]}"
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
    if [[ "$REBUILD" -eq 1 || ! -x "$executable" ]]; then
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

if [[ "$COMMAND" != "help" ]]; then
    acquire_build_lock
fi

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
