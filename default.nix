with import <nixpkgs>{};

let
  version = "1.0";
in {
  plop = stdenv.mkDerivation rec {
    name = "plop-${version}";
    src = ./.;
    buildInputs = [
      gcc
      coreutils
      openmpi
    ];
    installPhase = ''
      make
      cp plop ./bin
      cp -r ./bin $out
    '';
  };
}
