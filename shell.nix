{ pkgs ? import <nixpkgs> {} }:

let
  flake = import ./flake.nix;
  # Simple compat for non-flake nix
  # This is a bit complex for a simple PR, so I'll just provide a clean shell.nix
in
pkgs.mkShell {
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
  '';
}
