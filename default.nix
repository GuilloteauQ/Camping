with import <nixpkgs>{};

let
  version = "1.0";
in {
  camping = stdenv.mkDerivation rec {
    name = "CaMPIng-${version}";
    pname = "camping";
    src = ./.;
    buildInputs = [
      gcc
      coreutils
      openmpi
    ];
    installPhase = ''
      make
      cp campign ./bin
      cp -r ./bin $out
    '';
  };
}
