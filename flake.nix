{
  description = "software-watermark development environment";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-26.05";
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
        pkgs = import nixpkgs {
          inherit system;
        };

        libfec = pkgs.stdenv.mkDerivation rec {
          pname = "libfec";
          version = "3.0.1";

          src = pkgs.fetchFromGitHub {
            owner = "Opendigitalradio";
            repo = "ka9q-fec";
            rev = "master";
            hash = "sha256-dfryxSoem5+vxfpiOKzqIrdpzsZZMsFF8Pz/IYn6zVQ=";
          };

          nativeBuildInputs = with pkgs; [
            autoreconfHook
          ];

          installPhase = ''
            mkdir -p $out/lib
            mkdir -p $out/include

            cp libfec.a libfec.so $out/lib/
            cp fec.h $out/include/
          '';
        };

      in
      {
        devShells.default = pkgs.mkShell {
          packages = with pkgs; [
            llvmPackages_22.clang
            llvmPackages_22.openmp
            cmake
            pkg-config
            libfec
          ];
          NIX_DEV_SHELL = 1;
          NIX_DEV_FLAKE = "software-watermark";
        };
      }
    );
}
