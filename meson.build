project('AdsLib', 'cpp',
  version : '0.1',
  default_options : [
    'warning_level=3',
  ]
)

common_files = files([
  'AdsLib/AdsDef.cpp',
  'AdsLib/AdsDevice.cpp',
  'AdsLib/AdsLib.cpp',
  'AdsLib/AdsFile.cpp',
  'AdsLib/Log.cpp',
  'AdsLib/Sockets.cpp',
  'AdsLib/Frame.cpp',
])

router_files = files([
  'AdsLib/standalone/AdsLib.cpp',
  'AdsLib/standalone/AmsConnection.cpp',
  'AdsLib/standalone/AmsNetId.cpp',
  'AdsLib/standalone/AmsPort.cpp',
  'AdsLib/standalone/AmsRouter.cpp',
  'AdsLib/standalone/NotificationDispatcher.cpp',
])

inc = include_directories([
  'AdsLib',
  'tools',
])

libs = [
  meson.get_compiler('cpp').find_library('ws2_32', required: false),
  dependency('threads'),
]

adslib = static_library('AdsLib',
  [common_files, router_files],
  include_directories: inc,
)

adslibtest = executable('AdsLibTest',
  'AdsLibTest/main.cpp',
  include_directories: inc,
  dependencies: libs,
  link_with: adslib,
)

adslibooitest = executable('AdsLibOOITest',
  'AdsLibOOITest/main.cpp',
  include_directories: inc,
  dependencies: libs,
  link_with: adslib,
)

if get_option('tcadsdll_include') != ''
  tcadslib = static_library('TcAdsLib',
    [common_files, 'AdsLib/TwinCAT/AdsLib.cpp'],
    cpp_args: '-DUSE_TWINCAT_ROUTER',
    include_directories: [
      get_option('tcadsdll_include'),
      inc,
    ],
  )
endif
