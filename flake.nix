{
  description = "Lumen Rendering Engine";
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
  };

  outputs =
    inputs@{
      self,
      nixpkgs,
      ...
    }:
    let
      allSystems = [
        "x86_64-linux"
        "aarch64-linux"
        "x86_64-darwin"
        "aarch64-darwin"
      ];
      forAllSystems = f: nixpkgs.lib.genAttrs allSystems (system: f (import nixpkgs { inherit system; } ) );
    in
    {
      packages = forAllSystems (pkgs: {
        default = pkgs.callPackage ./Nix/package.nix { inherit inputs; };
      });
      devShells = forAllSystems (pkgs: {
        default = pkgs.callPackage ./Nix/shell.nix { inherit inputs; };
      });
    };
}