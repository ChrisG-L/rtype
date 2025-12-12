{
  description = "Environnement de développement C++ (Clang + Vcpkg + SDL2)";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };
        stdenv = pkgs.clangStdenv;
      in
      {
        devShells.default = pkgs.mkShell.override { inherit stdenv; } {
          
          nativeBuildInputs = with pkgs; [
            cmake ninja pkg-config git unzip zip vcpkg
            llvmPackages.clang-tools
            # Autotools for vcpkg dependencies
            autoconf automake libtool autoconf-archive
          ];

          buildInputs = with pkgs; [
            xorg.libX11 xorg.libXi xorg.libXrandr xorg.libXcursor xorg.libXinerama xorg.libXext
            libGL libGLU systemd
            openal flac libvorbis freetype
            gcc.cc.lib cacert openssl
            # SDL2/SDL3 dependencies (sdl2-compat requires SDL3)
            SDL2 SDL2_image sdl3
          ];
          hardeningDisable = [ "fortify" ];
          
          shellHook = ''
            # --- FIX SSL (MongoDB) ---
            export SSL_CERT_FILE="${pkgs.cacert}/etc/ssl/certs/ca-bundle.crt"
            export NIX_SSL_CERT_FILE="${pkgs.cacert}/etc/ssl/certs/ca-bundle.crt"

            # --- FIX VCPKG ---
            export VCPKG_FORCE_SYSTEM_BINARIES=1

            # --- FIX CRITIQUE POUR HYPRLAND / SFML / NVIDIA ---
            # Corrige l'erreur BadMatch (Major opcode 78) avec NVIDIA + XWayland
            export XLIB_SKIP_ARGB_VISUALS=1

            # Force le visual non-composite pour NVIDIA
            export __GL_ALLOW_FXAA_USAGE=0
            export __GL_FSAA_MODE=0

            # Désactive la synchronisation qui peut causer des problèmes
            export vblank_mode=0

            # Force GLX à utiliser le provider NVIDIA correctement
            export __GLX_VENDOR_LIBRARY_NAME=nvidia


            # --- FIX GRAPHIQUE & LD_LIBRARY_PATH ---
            export LD_LIBRARY_PATH="/run/opengl-driver/lib:${pkgs.lib.makeLibraryPath [
              pkgs.libGL
              pkgs.libGLU
              pkgs.xorg.libX11
              pkgs.xorg.libXcursor
              pkgs.xorg.libXrandr
              pkgs.xorg.libXi
              pkgs.xorg.libXext
              pkgs.xorg.libXinerama
              pkgs.gcc.cc
              pkgs.systemd
              pkgs.SDL2
              pkgs.SDL2_image
              pkgs.sdl3
            ]}:$LD_LIBRARY_PATH"

            echo "🚀 Environnement Clang + SDL2 (Flake) chargé !"
            echo "   Compilateur : $(clang++ --version | head -n1)"
          '';
        };
      }
    );
}