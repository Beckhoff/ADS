project('AdsLib', 'cpp',
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
add_project_arguments(get_option('default_loglevel'), language: 'cpp')

common_files = files([
  'AdsLib/AdsDef.cpp',
  'AdsLib/AdsDevice.cpp',
  'AdsLib/AdsFile.cpp',
  'AdsLib/AdsLib.cpp',
  'AdsLib/ECatAccess.cpp',
  'AdsLib/Frame.cpp',
  'AdsLib/LicenseAccess.cpp',
  'AdsLib/Log.cpp',
  'AdsLib/RTimeAccess.cpp',
  'AdsLib/RegistryAccess.cpp',
  'AdsLib/RouterAccess.cpp',
  'AdsLib/Sockets.cpp',
  'AdsLib/SymbolAccess.cpp',
  'AdsLib/bhf/ParameterList.cpp',
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
  install: true,
)

adslib_dep = declare_dependency(
  include_directories : inc,
  link_with: adslib,
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

adstool = executable('adstool',
  'AdsTool/main.cpp',
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

  tcadsdll_deps = [
      meson.get_compiler('cpp').find_library('TcAdsDll', dirs: get_option('tcadsdll_lib')),
      libs,
  ]

  tcadstool = executable('tcadstool',
    'AdsTool/main.cpp',
    cpp_args: '-DUSE_TWINCAT_ROUTER',
    include_directories: [
      get_option('tcadsdll_include'),
      inc,
    ],
    dependencies: [
      tcadsdll_deps
    ],
    link_with: tcadslib,
  )
endif
