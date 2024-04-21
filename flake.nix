{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { flake-utils, nixpkgs, ... }: flake-utils.lib.eachDefaultSystem (system: let
    pkgs = nixpkgs.legacyPackages.${system};
  in {
    devShells.default = pkgs.mkShell {
      name = "mwad";

      buildInputs = with pkgs; [
        fuse
      ];
    };

    packages.default = pkgs.stdenv.mkDerivation {
      name = "mwad";
      src = ./.;

      buildInputs = with pkgs; [
        fuse
        pkg-config
      ];

      installPhase = ''
        mkdir -p $out/bin
        cp ./mwad $out/bin/
      '';
    };
  });
}
