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
        lumenCli = import ./Nix/Lumen.nix { inherit pkgs; };

      in
      {
        packages.default = lumenPackage;

        apps = {
          lumen = {
            type = "app";
            program = "${lumenCli}/bin/lumen";
          };
        };

        devShells.default = import ./Nix/Shell.nix {
          inherit pkgs lumenPackage lumenCli;
        };
      }
    );
}
