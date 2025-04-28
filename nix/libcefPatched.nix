# fix to make clangd pickup on the headers
# CEF uses relative includes such as #include "include/header.h"
# clangd doesnt recognize the leading include folder
pkgs: prev: {
  cefPatched = prev.libcef.overrideAttrs (
    old:
    let
      platforms = {
        "aarch64-linux" = {
          platformStr = "linuxarm64";
          projectArch = "arm64";
        };
        "x86_64-linux" = {
          platformStr = "linux64";
          projectArch = "x86_64";
        };
      };
      platforms."aarch64-linux".sha256 = pkgs.lib.fakeHash;
      platforms."x86_64-linux".sha256 = "sha256-zzHYUU8w7rSWvpPe3hpojeFNKTkjcvjibFTPwIJUeQE=";

      platformInfo =
        platforms.${prev.stdenv.hostPlatform.system}
          or (throw "unsupported system ${prev.stdenv.hostPlatform.system}");
      cefSources = pkgs.fetchFromBitbucket {
        owner = "chromiumembedded";
        repo = "cef";
        rev = "1d7a1f96bf2ab9923d785d8595d164701a886f17";
        hash = "sha256-PT3RzrKD/GvRY8idpvmfX5BsOXQWwAGwooya40xCwEs=";
      };
    in
    rec {
      version = "128.4.12";
      gitRevision = "1d7a1f9";
      chromiumVersion = "128.0.6613.138";
      src = prev.fetchurl {
        url = "https://cef-builds.spotifycdn.com/cef_binary_${version}+g${gitRevision}+chromium-${chromiumVersion}_${platformInfo.platformStr}.tar.bz2";
        inherit (platformInfo) sha256;
      };
      installPhase =
        old.installPhase
        + ''
          TEMPD=$(mktemp -d)
          mv $out/include/* $TEMPD
          mkdir $out/include/include
          mv $TEMPD/* $out/include/include
          cp -r $out/include/include/* $out/include
          mkdir -p $out/include/libcef_dll
          cp -r ${cefSources}/libcef_dll $out/libcef_dll
          cp -r $out/libcef_dll/* $out/include/libcef_dll
        '';
    }
  );
}