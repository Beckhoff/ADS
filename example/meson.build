project('example', 'cpp',
  version : '0.1',
  default_options : [
    'buildtype=release',
    'warning_level=3',
    'werror=true',
    'b_pie=true',
  ]
)

# some hardening options
add_project_arguments('-D_FORTIFY_SOURCE=2', language: 'cpp')

cxx = meson.get_compiler('cpp')
libs = [
    cxx.find_library('ws2_32', required: false),
    dependency('threads'),
]

example = executable('example',
  'example.cpp',
  include_directories: include_directories('../AdsLib'),
  dependencies: [
    cxx.find_library('AdsLib', dirs: meson.source_root() + '/../build/'),
    libs,
  ]
)

if get_option('tcadsdll_lib') != ''
  libs += cxx.find_library('TcAdsLib', dirs: meson.source_root() + '/../build/')
  libs += cxx.find_library('TcAdsDll', dirs: get_option('tcadsdll_lib'))
else
  libs = disabler()
endif

tcexample = executable('tcexample',
  'example.cpp',
  cpp_args: '-DUSE_TWINCAT_ROUTER',
  include_directories: include_directories('../AdsLib', get_option('tcadsdll_include')),
  dependencies: [
    libs,
  ]
)
