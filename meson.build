project('oki', 'c',
  version : '0.1',
  default_options : ['warning_level=3', 'c_std=gnu23']
)

os = build_machine.system()
cc = meson.get_compiler('c')
conf_data = configuration_data()

if os == 'linux'
  conf_data.set('KPLATFORM_LINUX', true)
elif os == 'windows'
  conf_data.set('KPLATFORM_LINUX', false)
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
  include_directories : headers_inc,
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
