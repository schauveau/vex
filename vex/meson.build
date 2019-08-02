
#
# Generate config.h
#

config_cfg = configuration_data()

config_cfg.set('prefix', prefix)
config_cfg.set('datadir', join_paths(prefix, datadir))
config_cfg.set('VEX_MAJOR_VERSION', libvex_major_version)
config_cfg.set('VEX_MINOR_VERSION', libvex_minor_version )
config_cfg.set('VEX_MICRO_VERSION', libvex_micro_version)
config_cfg.set('VEX_VERSION', libvex_version)

config_h = configure_file(
  input: 'config.h.in',
  output: 'config.h',
  configuration: config_cfg
)


libvex_sources = [
  'Timestamp.cc',
  'VideoReader.cc',
  'VideoWriter.cc',
  'VideoPlayer.cc',
  'TextBox.cc'
] 

libvex_headers = [
  'FFMpegCommon.h',
  'vex.h',
  'Timestamp.h',
  'VideoReader.h',
  'VideoWriter.h',
  config_h
]

libvex_headers_priv = [
  # add here the include files used to build but not installed.
] 

install_headers(libvex_headers, subdir : 'vex')


libvex_so_version = '@0@.@1@.@2@'.format(libvex_major_version,
                                         libvex_minor_version,
                                         libvex_micro_version)
vex_dependencies = [
  libavutil,
  libavcodec,
  libavformat,
  libswscale,
  blend2d,
  fontconfig
]

libvex = library(
  'vex',
  sources : libvex_sources +
            libvex_headers +
            libvex_headers_priv,
  version : libvex_so_version,
  cpp_args : vex_cpp_args,
  include_directories : vex_include_dirs,
  install : true,
  dependencies : vex_dependencies 
)

vex = declare_dependency(
  link_with: libvex,
  dependencies: vex_dependencies,
  include_directories: vex_include_dirs,
)

