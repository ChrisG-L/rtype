{
  description = "Environnement de développement C++ (Clang + Vcpkg + SFML)";

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
            cmake ninja pkg-config git unzip zip vcpkg clang-tools
          ];

          buildInputs = with pkgs; [
            xorg.libX11 xorg.libXi xorg.libXrandr xorg.libXcursor xorg.libXinerama xorg.libXext
            libGL libGLU systemd
            openal flac libvorbis freetype
            gcc.cc.lib cacert openssl
          ];
          hardeningDisable = [ "fortify" ];
          shellHook = ''
            # --- FIX SSL (MongoDB) ---
            export SSL_CERT_FILE="${pkgs.cacert}/etc/ssl/certs/ca-bundle.crt"
            export NIX_SSL_CERT_FILE="${pkgs.cacert}/etc/ssl/certs/ca-bundle.crt"

            # --- FIX VCPKG ---
            export VCPKG_FORCE_SYSTEM_BINARIES=1

            # --- FIX GRAPHIQUE & EXECUTION (BadMatch / libstdc++) ---
            # 1. On ajoute les drivers OpenGL du système (/run/opengl-driver/lib)
            # 2. On ajoute toutes les libs X11 et GL au LD_LIBRARY_PATH
            # 3. On garde gcc.cc pour vcpkg
            export LD_LIBRARY_PATH="/run/opengl-driver/lib:$LD_LIBRARY_PATH"
            
            export LD_LIBRARY_PATH="${pkgs.lib.makeLibraryPath [
              pkgs.libGL
              pkgs.libGLU
              pkgs.xorg.libX11
              pkgs.xorg.libXcursor
              pkgs.xorg.libXrandr
              pkgs.xorg.libXi
              pkgs.gcc.cc        # Pour le C++ standard
              pkgs.systemd       # Pour libudev
            ]}:$LD_LIBRARY_PATH"

            echo "🚀 Environnement Clang (Flake) chargé !"
            echo "   Compilateur : $(clang++ --version | head -n1)"
            echo "   Fix Graphique : Appliqué (/run/opengl-driver/lib)"
          '';
        };
      }
    );
}