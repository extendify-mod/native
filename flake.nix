{
  description = "A basic flake with a shell";
  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
  inputs.systems.url = "github:nix-systems/default";
  inputs.flake-utils = {
    url = "github:numtide/flake-utils";
    inputs.systems.follows = "systems";
  };

  outputs =
    { nixpkgs, flake-utils, ... }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = import nixpkgs {
          inherit system;
          config = {
            allowUnfree = true;
          };
        };
        _libcef = pkgs.callPackage ./nix/libcef.nix { };
        libcef = pkgs.enableDebugging _libcef;
        _wrapper = pkgs.callPackage ./nix/wrapper.nix { };
        wrapper = pkgs.enableDebugging _wrapper;
        spotify = pkgs.callPackage ./nix/spotify.nix {
          # cef = libcef;
          extendifyWrapper = wrapper;
        };
      in
      {
        devShells.default =
          let
            deps = with pkgs; [
              curlWithGnuTls
            ];
            inherit (pkgs) lib stdenv;
          in
          pkgs.mkShell {
            NIX_LD_LIBRARY_PATH = lib.makeLibraryPath deps;
            NIX_LD = lib.fileContents "${stdenv.cc}/nix-support/dynamic-linker";
            LD_LIBRARY_PATH = lib.makeLibraryPath (with pkgs; [
              libgcc.lib
              alsa-lib
              at-spi2-atk
              at-spi2-core
              atk
              cairo
              cups
              dbus
              expat
              ffmpeg_4 # Requires libavcodec < 59 as of 1.2.9.743.g85d9593d
              fontconfig
              freetype
              gdk-pixbuf
              glib
              gtk3
              harfbuzz
              libayatana-appindicator
              libdbusmenu
              libdrm
              libgcrypt
              libGL
              libnotify
              libpng
              libpulseaudio
              libxkbcommon
              libgbm
              nss_latest
              pango
              stdenv.cc.cc
              systemd
              xorg.libICE
              xorg.libSM
              xorg.libX11
              xorg.libxcb
              xorg.libXcomposite
              xorg.libXcursor
              xorg.libXdamage
              xorg.libXext
              xorg.libXfixes
              xorg.libXi
              xorg.libXrandr
              xorg.libXrender
              xorg.libXScrnSaver
              xorg.libxshmfence
              xorg.libXtst
              zlib
              openssl
            ]);
            libcef_ROOT = "${libcef}";
            libcef_INCLUDE_DIR = "${libcef}/include";
            shellHook = ''
              export EXTENDIFY_LIB_PATH="$(pwd)/build/libextendify.so"
            '';
          };
        packages = {
          inherit libcef spotify wrapper;
        };
      }
    );
}
