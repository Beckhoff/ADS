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
  'AdsLib/MasterDcStatAccess.cpp',
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

install_headers(
  'AdsLib/AdsDef.h',
  'AdsLib/AdsDevice.h',
  'AdsLib/AdsException.h',
  'AdsLib/AdsFile.h',
  'AdsLib/AdsLib.h',
  'AdsLib/AdsNotification.h',
  'AdsLib/AdsNotificationOOI.h',
  'AdsLib/AdsVariable.h',
  'AdsLib/AmsConnection.h',
  'AdsLib/AmsHeader.h',
  'AdsLib/AmsPort.h',
  'AdsLib/AmsRouter.h',
  'AdsLib/ECatAccess.h',
  'AdsLib/Frame.h',
  'AdsLib/LicenseAccess.h',
  'AdsLib/Log.h',
  'AdsLib/MasterDcStatAccess.h',
  'AdsLib/NotificationDispatcher.h',
  'AdsLib/RTimeAccess.h',
  'AdsLib/RegistryAccess.h',
  'AdsLib/RingBuffer.h',
  'AdsLib/Router.h',
  'AdsLib/RouterAccess.h',
  'AdsLib/Semaphore.h',
  'AdsLib/Sockets.h',
  'AdsLib/SymbolAccess.h',
  'AdsLib/TwinCAT/AdsDef.h',
  'AdsLib/TwinCAT/AdsLib.h',
  'AdsLib/bhf/ParameterList.h',
  'AdsLib/bhf/StringToInteger.h',
  'AdsLib/bhf/WindowsQuirks.h',
  'AdsLib/standalone/AdsDef.h',
  'AdsLib/standalone/AdsLib.h',
  'AdsLib/wrap_endian.h',
  'AdsLib/wrap_registry.h',
  'AdsLib/wrap_socket.h',
  subdir: 'AdsLib'
)

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

install_libs = [ adslib ]

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

adstest_files = files([
  'AdsTest/main.cpp',
  'AdsTest/RegistryAccessTest.cpp',
])

adstest = executable('AdsTest',
  adstest_files,
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
    install: true,
  )
  install_libs += [ tcadslib ]

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

  tcadstest = executable('tcAdsTest',
    adstest_files,
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

pkg = import('pkgconfig')
pkg.generate(
  name: 'AdsLib',
  description: 'Static library for AdsLib',
  filebase: 'AdsLib',
  subdirs: 'AdsLib',
  libraries: install_libs,
  version: meson.project_version(),
)

