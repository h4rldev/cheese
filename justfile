set shell := ["bash", "-c"]
set quiet := false

## Metadata

lib_name := "libcheese"
test_name := "cheese-test" # Using bread && butter

## Directories

src := 'src'
include := 'include'
out := 'out'
lib := 'lib'
bin := 'bin'

## Type dirs

test_out := out + '/tests'
cheese_out := out + '/cheese'
test_src := src + '/tests'
cheese_src := src + '/cheese'
renderer_src := 'renderers'

## General flags

include_flags := '-I' + include
link_flags := ' -lhtils -lvulkan -Llib -ldl -lharfbuzz -lfreetype -lm'

## Shared flags

shared_flags_debug := '-ggdb -g -Og -fsanitize=address,undefined,leak -fno-sanitize-recover=all -fno-omit-frame-pointer -fno-optimize-sibling-calls -fsanitize-address-use-after-scope -fno-common -DBUTTER_DEBUG'
shared_flags_release := '-O3 -std=gnu11'

## Link flags

release_wayland_link_flags := shared_flags_debug + ' -lwayland-client -lwayland-cursor -lxkbcommon -lbutter-wayland-release -lbread-wayland-release -lcheese-release' + link_flags
release_x11_link_flags := shared_flags_debug + ' -lxkbcommon -lxkbcommon-x11 -lxcb -lxcb-randr -lbutter-x11-release -lbread-x11-release -lcheese-release' + link_flags
debug_wayland_link_flags := shared_flags_debug + ' -lwayland-client -lwayland-cursor -lxkbcommon -lbutter-wayland-release -lbread-wayland-release -lcheese-debug' + link_flags
debug_x11_link_flags := shared_flags_debug + ' -lxkbcommon -lxkbcommon-x11 -lxcb -lxcb-randr -lbutter-x11-debug -lbread-x11-release -lcheese-debug' + link_flags

## Static link flags

release_static_link_flags := shared_flags_release + ' -static -fPIC'
debug_static_link_flags := shared_flags_debug + ' -static -fPIC'

## Compile flags

release_compile_flags := shared_flags_release
debug_compile_flags := shared_flags_debug + ' -Wall -Wextra -Wpedantic -Wno-unused-parameter -DCHEESE_DEBUG'
wayland_compile_flags := '-DBUTTER_WAYLAND'
x11_compile_flags := '-DBUTTER_X11'

## Colors

red := "\\x1b[31m"
green := "\\x1b[32m"
yellow := "\\x1b[33m"
reset := "\\x1b[0m"

default:
    just --list

## Utilities

clean:
    ! [[ -d {{ out }} ]] || rm -fr {{ out }}
    ! [[ -d {{ bin }} ]] || rm -fr {{ bin }}
    ! [[ -d {{ lib }} ]] || rm -fr {{ lib }}

bear:
    bear -- just compile-cheese debug force

## Compile

compile-cheese target="debug" force="dont_force" threads=num_cpus():
    #!/usr/bin/env bash
    shopt -s globstar

    CURRENT_TARGET_COMPILE_FLAGS=""

    [[ -d {{ src }} ]] || exit 0


    if [[ {{ target }} == "release" ]]; then
      CURRENT_TARGET_COMPILE_FLAGS="{{ release_compile_flags }}"
    else
      CURRENT_TARGET_COMPILE_FLAGS="{{ debug_compile_flags }}"
    fi

    [[ -d {{ cheese_out }} ]] || mkdir -p {{ cheese_out }}

    WILL_COMPILE=false

    export WILL_COMPILE
    export CURRENT_PLATFORM_COMPILE_FLAGS
    export CURRENT_TARGET_COMPILE_FLAGS

    check_flags() {
      if [[ {{ force }} == "force" || {{ force }} == "true" ]]; then
        WILL_COMPILE=true; echo -e "Compile (cheese): Forcing"; return
      fi

      for file in {{ cheese_src }}/**/*.c; do
        local current_out_file="$(basename "${file%.c}")-{{ target }}.o"

        if ! [[ -f {{ cheese_out }}/${current_out_file} ]]; then
          WILL_COMPILE=true; return
        fi
        if [[ $file -nt {{ cheese_out }}/${current_out_file} ]]; then
          WILL_COMPILE=true; return
        fi
      done
    }

    compile() {
      local file="$1"
      local current_out_file="$(basename "${file%.c}")-{{ target }}.o"

      echo -e "Compiling {{ green }}$file{{ reset }}..."
      gcc {{ include_flags }} ${CURRENT_TARGET_COMPILE_FLAGS} -c "$file" -o "{{ cheese_out }}/${current_out_file}"
    }
    export -f compile
    check_flags

    if [[ ${WILL_COMPILE} == false ]]; then
      echo -e "Compile (cheese): Nothing to do"
      exit 0
    fi

    echo -e "Using {{ red }}{{ threads }}{{ reset }} threads"
    echo -e "Target: {{ green }}{{ target }}{{ reset }}"

    find {{ cheese_src }} -name "*.c" -print0 | xargs -0 -P{{ threads }} -n1 bash -c 'compile "$0"'

    echo -e "Compile (cheese): Compiling {{ green }}{{ target }}{{ reset }} complete"

assemble-cheese target="debug" static="dynamic" force="dont_force":
    #!/usr/bin/env bash
    shopt -s globstar

    [[ -d {{ cheese_out }} ]] || just compile-cheese {{ target }} {{ force }}
    [[ -d {{ lib }} ]] || mkdir -p {{ lib }}

    ASSEMBLE_STATIC=false
    WILL_ASSEMBLE=false

    if [[ {{ target }} == "debug" ]]; then
      ASSEMBLE_STATIC=true
    elif [[ {{ target }} == "release" && {{ static }} == "static" || {{ static }} == "true" ]]; then
      ASSEMBLE_STATIC=true
    else
      ASSEMBLE_STATIC=false
    fi

    export ASSEMBLE_STATIC

    check_flags() {
      if [[ {{ force }} == "true" || {{ force }} == "force" ]]; then
        WILL_ASSEMBLE=true; echo -e "Assemble: Forcing"; return
      fi

      if ! [[ -f {{ lib }}/{{ lib_name }}-{{ target }}.a ]] && [[ ${ASSEMBLE_STATIC} == "true" ]]; then
        WILL_ASSEMBLE=true; return
      fi

      if ! [[ -f {{ lib }}/{{ lib_name }}-{{ target }}.so ]] && [[ ${ASSEMBLE_STATIC} == "false" ]]; then
        WILL_ASSEMBLE=true; return
      fi

      for file in {{ cheese_out }}/*-{{ target }}.o; do
        if [[ "${ASSEMBLE_STATIC}" == "true" ]]; then
          if [[ $file -nt {{ lib }}/{{ lib_name }}-{{ target }}.a ]]; then
            WILL_ASSEMBLE=true; return
          fi
        else
          if [[ $file -nt {{ lib }}/{{ lib_name }}-{{ target }}.so ]]; then
            WILL_ASSEMBLE=true; return
          fi
        fi
      done
    }

    check_flags

    if [[ $WILL_ASSEMBLE == false ]]; then
      echo -e "Assemble (cheese): Nothing to do"
      exit 0
    fi

    echo -e "Assemble (cheese)"
    echo -e "Assembling {{ green }}{{ target }}{{ reset }}..."

    FILES=$(find {{ cheese_out }} -name "*-{{ target }}.o")
    echo -e "Files to assemble: {{ green }}"
    printf "%s\n" "${FILES[@]}" | sed 's/^/    /'
    echo -e "{{ reset }}"

    if [[ "${ASSEMBLE_STATIC}" == "true" ]]; then
      ar rcs {{ lib }}/{{ lib_name }}-{{ target }}.a {{ cheese_out }}/*-{{ target }}.o
      if [[ {{ target }} == "release" ]]; then
        strip --strip-debug {{ lib }}/{{ lib_name }}-{{ target }}.a
      fi
      ranlib {{ lib }}/{{ lib_name }}-{{ target }}.a
    else
      gcc -shared -o {{ lib }}/{{ lib_name }}-{{ target }}.so {{ cheese_out }}/*-{{ target }}.o
      strip {{ lib }}/{{ lib_name }}-{{ target }}.so
    fi

    echo -e "Assemble (cheese): Assembling {{ green }}{{ target }}{{ reset }} complete"

compile-test platform="wayland" target="debug" force="dont_force" threads=num_cpus():
    #!/usr/bin/env bash
    shopt -s globstar

    [[ -d {{ test_out }} ]] || mkdir -p {{ test_out }}
    [[ -d {{ lib }} ]] || just debug

    WILL_COMPILE=false
    CURRENT_PLATFORM_COMPILE_FLAGS=""
    CURRENT_TARGET_COMPILE_FLAGS=""

    if [[ {{ platform }} == "wayland" ]]; then
      CURRENT_PLATFORM_COMPILE_FLAGS="{{ wayland_compile_flags }}"
    else
      CURRENT_PLATFORM_COMPILE_FLAGS="{{ x11_compile_flags }}"
    fi

    if [[ {{ target }} == "release" ]]; then
      CURRENT_TARGET_COMPILE_FLAGS="{{ release_compile_flags }}"
    else
      CURRENT_TARGET_COMPILE_FLAGS="{{ debug_compile_flags }}"
    fi

    export WILL_COMPILE
    export CURRENT_PLATFORM_COMPILE_FLAGS
    export CURRENT_TARGET_COMPILE_FLAGS

    check_flags() {
      if [[ {{ force }} == "true" || {{ force }} == "force" ]]; then
        WILL_COMPILE=true; echo -e "Compile: Forcing"; return
      fi

      if ! [[ -d {{ test_out }} ]]; then
        WILL_COMPILE=true; return
      fi

      for file in {{ test_src }}/*.c; do
        current_out_file="$(basename "${file%.c}").o"
        if ! [[ -f {{ test_out }}/${current_out_file} ]]; then
          WILL_COMPILE=true; return
        fi

        if [[ $file -nt {{ test_out }}/${current_out_file} ]]; then
          WILL_COMPILE=true; return
        fi
      done
    }

    check_flags

    if [[ ${WILL_COMPILE} == false ]]; then
      echo -e "Compile (test): Nothing to do"
      exit 0
    fi

    compile() {
      local file="$1"
      local current_out_file="$(basename "${file%.c}").o"

      local path


      if [[ $file == {{ renderer_src }}/*.c ]]; then
        path={{ test_out }}/renderers
      else
        path={{ test_out }}/$(basename "${file%.c}")
      fi
      mkdir -p $path


      echo -e "Compiling {{ green }}$file{{ reset }}..."
      gcc {{ include_flags }} ${CURRENT_TARGET_COMPILE_FLAGS} ${CURRENT_PLATFORM_COMPILE_FLAGS} -c "$file" -o "$path/${current_out_file}"
    }

    export -f compile

    echo -e "Compile (test):"
    echo -e "Platform: {{ green }}{{ platform }}{{ reset }}"
    echo -e "Using {{ red }}{{ threads }}{{ reset }} threads"

    find {{ test_src }} -name "*.c" -print0 | xargs -0 -P{{ threads }} -n1 bash -c 'compile "$0"'
    find {{ renderer_src }} -name "*.c" -print0 | xargs -0 -P{{ threads }} -n1 bash -c 'compile "$0"'

    echo -e "Compile (test): Compiling {{ green }}{{ platform }}{{ reset }} complete"

link-test platform="wayland" renderer="butter" target="debug" force="dont_force":
    #!/usr/bin/env bash
    shopt -s globstar

    [[ -d {{ test_out }} ]] || just compile-test {{ platform }} {{ force }}
    [[ -d {{ bin }} ]] || mkdir -p {{ bin }}

    # Variable for checking if we should link.
    WILL_LINK=false
    LINK_STATIC=false

    CURRENT_PLATFORM_LINK_FLAGS=""

    if [[ {{ platform }} == "wayland" ]]; then
      if [[ {{ target }} == "release" ]]; then
        CURRENT_PLATFORM_LINK_FLAGS="{{ release_wayland_link_flags }}"
      else
        CURRENT_PLATFORM_LINK_FLAGS="{{ debug_wayland_link_flags }}"
      fi
    else
      if [[ {{ target }} == "release" ]]; then
        CURRENT_PLATFORM_LINK_FLAGS="{{ release_x11_link_flags }}"
      else
        CURRENT_PLATFORM_LINK_FLAGS="{{ debug_x11_link_flags }}"
      fi
    fi

    export CURRENT_PLATFORM_LINK_FLAGS
    export WILL_LINK

    check_flags() {
      if [[ {{ force }} == "true" || {{ force }} == "force" ]]; then
        WILL_LINK=true; echo -e "Link: Forcing"; return
      fi

      if ! [[ -f {{ bin }}/{{ test_name }}-{{ platform }} ]]; then
        WILL_LINK=true; return
      fi

      for file in {{ test_out }}/**/*-{{ platform }}.o; do
        if [[ $file -nt {{ bin }}/{{ test_name }}-{{ platform }} ]]; then
          WILL_LINK=true; return
        fi
      done
    }

    link() {
      echo -e "Link (test):"
      echo -e "Platform: {{ green }}{{ platform }}{{ reset }}"
      echo -e "Renderer: {{ green }}{{ renderer }}{{ reset }}"

      local file="$1"
      local out_sub=$(basename "${file%.o}")

      gcc $file {{ test_out }}/renderers/{{ renderer }}.o ${CURRENT_PLATFORM_LINK_FLAGS} -o {{ bin }}/cheese-test-$out_sub-{{ renderer }} -fuse-ld=mold
    }

    check_flags

    if [[ ${WILL_LINK} == false ]]; then
      echo -e "Link: Nothing to do"
      exit 0
    fi

    files_to_link=$(find {{ test_out }} -name renderers -prune -o -type f -name "*.o" -print)
    for file in $files_to_link; do
      echo -e "Linking files: $file"
      link $file
    done

## Aliases

build-cheese target="debug" force="false" static="dynamic" threads=num_cpus():
    just compile-cheese {{ target }} {{ force }} {{ threads }}
    just assemble-cheese {{ target }} {{ static }} {{ force }}

build-test platform="wayland" renderer="butter" target="debug" force="false" threads=num_cpus():
    just {{ target }} {{ force }} static {{ threads }}

    just compile-test {{ platform }} {{ target }} {{ force }} {{ threads }}
    just link-test {{ platform }} {{ renderer }} {{ target }} {{ force }}

release force="false" static="dynamic" threads=num_cpus():
    just build-cheese release {{ force }} {{ static }} {{ threads }}

debug force="false" static="static" threads=num_cpus():
    just build-cheese debug {{ force }} {{ static }} {{ threads }}
