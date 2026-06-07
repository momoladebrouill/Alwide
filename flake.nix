{
  description = "Alwide - A modern terminal-based code editor";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
      in
      {
        packages.default = pkgs.stdenv.mkDerivation {
          pname = "alwide";
          version = "0.1.0";

          src = ./.;

          nativeBuildInputs = [
            pkgs.pkg-config
            pkgs.clang
            pkgs.gnumake
            pkgs.rustc
            pkgs.cargo
            pkgs.rustPlatform.cargoSetupHook
            pkgs.nodejs
            pkgs.tree-sitter
          ];

          buildInputs = [
            pkgs.ncurses
          ];

          # Since the project uses many submodules and custom build steps, 
          # we manually call make. 
          # Note: In a pure Nix build, network access is disabled, 
          # so 'cargo build' might fail unless we provide cargoSha256.
          # For now, we focus on providing a perfect devShell.
          
          buildPhase = ''
            export HOME=$TMPDIR
            make release
          '';

          installPhase = ''
            mkdir -p $out/bin
            cp al $out/bin/
            mkdir -p $out/share/alwide
            cp -r assets/* $out/share/alwide/
          '';
        };

        devShells.default = pkgs.mkShell {
          name = "alwide-dev";
          
          nativeBuildInputs = [
            pkgs.pkg-config
            pkgs.clang
            pkgs.gnumake
            pkgs.rustup
            pkgs.nodejs
            pkgs.tree-sitter
          ];

          buildInputs = [
            pkgs.ncurses
          ];

          shellHook = ''
            export CC=clang
            echo "Welcome to Alwide development environment!"
            echo "Dependencies loaded: clang, gnumake, ncurses, rustup, nodejs, tree-sitter"
          '';
        };
      }
    );
}
