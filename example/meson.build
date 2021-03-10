project('example', 'cpp',
  version : '0.1',
  default_options : [
    'warning_level=3',
  ]
)

example = executable('example',
  'example.cpp',
  include_directories: include_directories('../AdsLib'),
  dependencies: [
    meson.get_compiler('cpp').find_library('AdsLib', dirs: meson.source_root() + '/../build/'),
    meson.get_compiler('cpp').find_library('ws2_32', required: false),
    dependency('threads'),
  ]
)
