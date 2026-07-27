{
  description = "Crib - An IDE.";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    self.submodules = true;
  };

  outputs =
    { nixpkgs, ... }:
    let
      system = "x86_64-linux";
      pkgs = nixpkgs.legacyPackages.${system}.pkgsStatic;
      hostPkgs = nixpkgs.legacyPackages.${system};

      mruby = pkgs.stdenv.mkDerivation {
        pname = "mruby";
        version = "0.0.1";

        src = pkgs.lib.cleanSourceWith {
          src = ./libs;
          name = "mruby-src";
        };

        patches = [
          ./libs/changes.patch
        ];

        nativeBuildInputs = with hostPkgs; [
          ruby
          gnumake
        ];

        buildPhase = ''
          cd mruby
          rake
        '';

        installPhase = ''
          mkdir -p $out

          cp -r build/host/lib $out/lib
          cp -r build/host/include $out/include

          mkdir -p $out/bin
          cp build/host/bin/mrbc $out/bin/mrbc
        '';
      };

      crib = pkgs.stdenv.mkDerivation {
        pname = "crib";
        version = "0.1.0";

        src = ./.;

        nativeBuildInputs = with hostPkgs; [
          pkg-config
          gnumake
          xxd
          bash
        ];

        buildInputs = with pkgs; [
          libgrapheme

          mruby
        ];

        buildPhase = ''
          make \
            MRBC=${mruby}/bin/mrbc \
            MRUBY_INCLUDE=${mruby}/include \
            MRUBY_LIB=${mruby}/lib/libmruby.a
        '';

        installPhase = ''
          mkdir -p $out/bin
          cp bin/crib $out/bin/crib
        '';
      };
    in
    {
      packages.${system} = {
        default = crib;
        crib = crib;
        mruby = mruby;
      };

      devShells.${system}.default = pkgs.mkShell {
        inputsFrom = [ crib ];

        packages = [
          hostPkgs.clang-tools
          hostPkgs.pkg-config
          hostPkgs.ccache
          hostPkgs.gnumake
          hostPkgs.bear
          hostPkgs.ruby
          hostPkgs.xxd
          hostPkgs.gdb
          hostPkgs.binutils

          pkgs.libgrapheme
          pkgs.pcre2
        ];

        shellHook = ''
          export CC="ccache gcc"
          export CXX="ccache g++"

          export MRBC=${mruby}/bin/mrbc
          export MRUBY_INCLUDE=${mruby}/include
          export MRUBY_LIB=${mruby}/lib/libmruby.a
        '';
      };
    };
}
