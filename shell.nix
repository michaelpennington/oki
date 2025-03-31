{pkgs ? import <nixpkgs> {}}:
pkgs.mkShellNoCC {
  buildInputs = with pkgs; [
    meson
    ninja
    clang
    llvm
    lld
    pkg-config
    wayland
    wayland-protocols
    wayland-scanner
    xorg.libX11
    xorg.libxcb
    libxkbcommon
    valgrind
    vulkan-loader
    vulkan-headers
    vulkan-tools-lunarg
    vulkan-caps-viewer
    vulkan-validation-layers
    vulkan-utility-libraries
  ];
  VK_LAYER_PATH = "${pkgs.vulkan-validation-layers}/share/vulkan/explicit_layer.d";
  CC = "clang";
  CC_LD = "lld";
}
