{pkgs ? import <nixpkgs> {}}:
pkgs.mkShellNoCC {
  buildInputs = with pkgs; [
    meson
    ninja
    clang
    llvm
    lld
  ];
  CC = "clang";
  CC_LD = "lld";
}
