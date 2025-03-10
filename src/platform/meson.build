platform_files = []

if os == 'windows'
  platform_files += files('platform_win32.c')
elif os == 'linux'
  platform_files += files('platform_linux.c')
  if wayland_build
    platform_files += files('platform_linux_wayland.c')
    wl_scanner = find_program('wayland-scanner')
    wl_scanner_dep = dependency('wayland-scanner', required : true)

    wl_scanner_pkgdatadir = wl_scanner_dep.get_variable(
      'pkgdatadir',
      default_value : '/usr/share/wayland',
    )
    wl_protos_pkgdatadir = wl_protos_dep.get_variable(
      'pkgdatadir',
      default_value : '/usr/share/wayland-protocols',
    )
    wl_client_path = wl_scanner_pkgdatadir / 'wayland.xml'
    xdg_shell_path = wl_protos_pkgdatadir / 'stable/xdg-shell/xdg-shell.xml'

    wl_client_header = custom_target(
      'wl_client_header',
      input : wl_client_path,
      output : 'wayland-client-protocol.h',
      command : [wl_scanner, 'client-header', '@INPUT@', '@OUTPUT@'],
    )
    wl_client_code = custom_target(
      'wl_client_code',
      input : wl_client_path,
      output : 'wayland-client-protocol-code.h',
      command : [wl_scanner, 'private-code', '@INPUT@', '@OUTPUT@'],
    )
    xdg_shell_header = custom_target(
      'xdg_shell_header',
      input : xdg_shell_path,
      output : 'wayland-xdg-shell-client-protocol.h',
      command : [wl_scanner, 'client-header', '@INPUT@', '@OUTPUT@'],
    )
    xdg_shell_code = custom_target(
      'xdg_shell_code',
      input : xdg_shell_path,
      output : 'wayland-xdg-shell-client-protocol-code.h',
      command : [wl_scanner, 'private-code', '@INPUT@', '@OUTPUT@'],
    )

    platform_files += [wl_client_header, wl_client_code, xdg_shell_header, xdg_shell_code]
  endif
  if x11_build
    platform_files += files('platform_linux_x11.c')
  endif
endif
