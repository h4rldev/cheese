{
  description = "The dev flake for cheese, an immediate mode gui library.";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    bread.url = "git+https://codeberg.org/h4rl/bread";
    butter.url = "git+https://codeberg.org/h4rl/butter";
    # butter.url = "path:/home/h4rl/projects/butter";
    conjure.url = "git+https://codeberg.org/h4rl/conjure";
    htils.url = "github:h4rldev/htils";
  };

  outputs = {
    self,
    flake-utils,
    nixpkgs,
    bread,
    butter,
    conjure,
    htils,
  }:
    flake-utils.lib.eachDefaultSystem (system: let
      pkgs = import nixpkgs {inherit system;};
      pversion = "0.1.0";

      nativeInputs = [
        conjure.packages.${system}.default
        pkgs.gcc
        pkgs.mold
        pkgs.harfbuzz
        pkgs.freetype
        htils.packages.${system}.htils-threadsafe
      ];

      mkCheese = {
        name,
        profile,
        artifact,
        pc,
      }: let
        artifactStem =
          pkgs.lib.removePrefix "lib"
          (pkgs.lib.removeSuffix ".so"
            (pkgs.lib.removeSuffix ".a" (baseNameOf artifact)));
      in
        pkgs.stdenv.mkDerivation {
          pname = name;
          version = pversion;

          src = ./.;

          nativeBuildInputs = nativeInputs;

          buildPhase = ''
            runHook preBuild
            conjure as ${profile} build
            runHook postBuild
          '';

          installPhase = ''
            runHook preInstall
            mkdir -p $out/lib/pkgconfig
            mkdir -p $out/include/cheese

            cp ${artifact} $out/lib
            sed -e "s|^prefix=.*|prefix=$out|" \
              -e "s|^libdir=.*|libdir=$out/lib|" \
              lib/${profile}/pkgconfig/${artifactStem}.pc \
              > $out/lib/pkgconfig/${pc}

            cp -r include/cheese/* $out/include/cheese
            cp include/cheese.h $out/include

            runHook postInstall
          '';
        };
    in {
      packages = {
        cheese = mkCheese {
          name = "cheese";
          profile = "release";
          artifact = "lib/release/libcheese.so";
          pc = "cheese.pc";
        };

        cheese-static = mkCheese {
          name = "cheese-static";
          profile = "release-static";
          artifact = "lib/release-static/libcheese.a";
          pc = "cheese-static.pc";
        };

        cheese-debug = mkCheese {
          name = "cheese-debug";
          profile = "debug";
          artifact = "lib/debug/libcheese-debug.a";
          pc = "cheese-debug.pc";
        };
      };

      devShells.default = pkgs.mkShell {
        name = "cheese-dev";

        packages = with pkgs; [
          clang-tools
          nixd
          bear
          vulkan-tools
          shaderc
          tokei
          conjure.packages.${system}.default
        ];

        nativeBuildInputs = with pkgs; [
          mold
        ];

        buildInputs = with pkgs; [
          htils.packages.${system}.htils-threadsafe

          bread.packages.${system}.bread-wayland-debug
          bread.packages.${system}.bread-wayland-release
          bread.packages.${system}.bread-x11-debug
          bread.packages.${system}.bread-x11-release

          butter.packages.${system}.butter-wayland-debug
          butter.packages.${system}.butter-wayland-release
          butter.packages.${system}.butter-x11-debug
          butter.packages.${system}.butter-x11-release

          vulkan-headers
          vulkan-validation-layers
          vulkan-loader
          libxcb-wm
          libxcb
          libxcb-cursor
          libxkbcommon
          wayland
          harfbuzz
          freetype
          pkg-config
          gdb
          xxd

          raylib
          sdl3
          sdl2-compat
          sokol
        ];

        shellHook = ''
          export VK_LAYER_PATH="${pkgs.vulkan-validation-layers}/share/vulkan/explicit_layer.d''${VK_LAYER_PATH:+:}$VK_LAYER_PATH"
          export LSAN_OPTIONS="$LSAN_OPTIONS:suppressions=asan.supp"
        '';
      };
    });
}
