project('oki', 'c',
  version : '0.1',
  default_options : ['warning_level=3', 'c_std=gnu23']
)

os = build_machine.system()

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

if get_option('debug')
  conf_data.set('_DEBUG', true)
endif

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
  'testbed',
  testbed_files,
  include_directories : headers_inc,
  link_with : engine,
  c_args : ['-DKIMPORT']
)

install_headers(public_headers, subdir : 'oki')
