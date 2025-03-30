project('oki', 'c',
  version : '0.1.0',
  default_options : ['warning_level=3', 'c_std=gnu23']
)

os = build_machine.system()
cc = meson.get_compiler('c')
conf_data = configuration_data()

wayland_build = false
x11_build = false


engine_dependencies = []

vulkan_dep = dependency('vulkan')
engine_dependencies += vulkan_dep

versions = meson.project_version().split('.')

conf_data.set('KVERSION_MAJOR', versions[0].to_int())
conf_data.set('KVERSION_MINOR', versions[1].to_int())
conf_data.set('KVERSION_PATCH', versions[2].to_int())

if os == 'linux'
  conf_data.set('KPLATFORM_LINUX', true)
  dl_dep = cc.find_library('dl', required : true)
  engine_dependencies += dl_dep

  x11_dep = dependency('x11', required : get_option('x11'))
  x11_xcb_dep = dependency('x11-xcb', required : get_option('x11'))
  xkb_x11_dep = dependency('xkbcommon-x11', required : get_option('x11'))
  xkbcommon_dep = dependency('xkbcommon', required : true)

  engine_dependencies += xkbcommon_dep

  if x11_dep.found() and x11_xcb_dep.found() and xkb_x11_dep.found()
    x11_build = true
    conf_data.set('KBUILD_X11', true)
    engine_dependencies += [x11_dep, xkb_x11_dep, x11_xcb_dep]
  endif

  wl_dep = dependency('wayland-client', required : get_option('wayland'))
  wl_protos_dep = dependency('wayland-protocols', required : get_option('wayland'))
  if wl_dep.found() and wl_protos_dep.found()
    wayland_build = true
    engine_dependencies += [wl_dep, wl_protos_dep]
    
    conf_data.set('KBUILD_WAYLAND', true)
  endif

  if not (x11_build or wayland_build)
    error('At least one of x11 or wayland must be selected on Linux')
  endif
elif os == 'windows'
  conf_data.set('KPLATFORM_WINDOWS', true)
else
  error('Only windows and linux are supported at the moment, not ', os)
endif

if build_machine.cpu_family() != 'x86_64'
  error('Must be built on an x86_64 machine, cpu_family returned ', build_machine.cpu_family())
endif

debug = get_option('debug')
conf_data.set('_DEBUG', debug)

levels = {'error': 4, 'warn': 3, 'info': 2, 'debug': 1, 'trace': 0}

log_level = get_option('log_level')

log = levels.get(log_level, 0)

if log <= 3
  conf_data.set('LOG_WARN_ENABLED', true)
endif
if log <= 2
  conf_data.set('LOG_INFO_ENABLED', true)
endif
if log <= 1 and not (get_option('buildtype') == 'release')
  conf_data.set('LOG_DEBUG_ENABLED', true)
endif
if log == 0 and not (get_option('buildtype') == 'release')
  conf_data.set('LOG_TRACE_ENABLED', true)
endif

if get_option('assertions')
  conf_data.set('KASSERTIONS_ENABLED', true)
endif

add_project_arguments(cc.get_supported_arguments('-fvisibility=hidden'), language : 'c')

subdir('include')
subdir('src')
subdir('testbed')

engine = library(
  'oki',
  library_files,
  include_directories : [headers_inc, internal_inc],
  dependencies : engine_dependencies,
  c_args : ['-DKEXPORT']
)

testbed = executable(
  'tester',
  testbed_files,
  include_directories : headers_inc,
  link_with : engine,
  c_args : ['-DKIMPORT']
)

install_headers(public_headers, subdir : 'oki')
