with import <nixpkgs>{};

let
  version = "1.0";
in {
  camping = stdenv.mkDerivation rec {
    name = "Camping-${version}";
    pname = "camping";
    src = ./.;
    nativeBuildInputs = [ makeWrapper ];
    buildInputs = [
      gcc
      coreutils
      openmpi
    ];
    installPhase = ''
      make
      cp camping ./bin
      cp -r ./bin $out
    '';
  };
}
