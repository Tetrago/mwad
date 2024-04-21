{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { flake-utils, nixpkgs, ... }: flake-utils.lib.eachDefaultSystem (system: let
    pkgs = nixpkgs.legacyPackages.${system};
  in {
    packages.default = pkgs.stdenv.mkDerivation {
      name = "mwad";

      src = ./.;

      installPhase = ''
        mkdir -p $out/bin
        cp ./mwad $out/bin/
      '';
    };
  });
}
