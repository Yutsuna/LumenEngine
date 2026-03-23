{
  description = "LumenEngine C++ Game Engine";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs =
    {
      self,
      nixpkgs,
      flake-utils,
    }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = import nixpkgs { inherit system; };

        version = pkgs.lib.strings.trim (builtins.readFile ./VERSION.md);

        lumenPackage = pkgs.callPackage ./Nix/Package.nix {
          inherit version;
        };
      in
      {
        packages.default = lumenPackage;

        devShells.default = import ./Nix/Shell.nix {
          inherit pkgs lumenPackage;
        };
      }
    );
}
