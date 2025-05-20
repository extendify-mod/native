{
  stdenv,
  cmake,
}: stdenv.mkDerivation {
  pname = "extendify-wrapper";
  version = "0.0.1";
  
  src = ../deps/wrapper;

  nativeBuildInputs = [ cmake ];
}