{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {

	buildInputs = [
			pkgs.clang
			pkgs.gnumake
			pkgs.ncurses
			pkgs.rustup
			pkgs.tree-sitter
			pkgs.nodejs
			pkgs.python313Packages.python-lsp-server
	];

}
