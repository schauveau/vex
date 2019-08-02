project('vex',
	'cpp',
        version: '0.1.0',
	license: 'MIT',
	meson_version: '>=0.47.0',
	default_options: [
		'cpp_std=c++17',
                'buildtype=debug',
                'warning_level=1',                
		'werror=false',
	],
)



cpp = meson.get_compiler('cpp')


version_arr = meson.project_version().split('.')
libvex_major_version = version_arr[0]
libvex_minor_version = version_arr[1]
libvex_micro_version = version_arr[2]
libvex_version = '@0@.@1@.@2@'.format(libvex_major_version,
                                      libvex_minor_version,
                                      libvex_micro_version)

prefix  = get_option('prefix')
datadir = get_option('datadir')

vex_include_dirs = include_directories(['.'])

# Disable a few annoying warnings (mostly because of blend2d)
add_project_arguments(cpp.get_supported_arguments([
   '-Wno-deprecated-declarations',
   '-Wno-unused-parameter',
   '-Wno-unused-variable',
   '-Wno-class-memaccess'
  ]),
  language: 'cpp'
)
               
# FFMpeg libraries

libavutil   = dependency('libavutil')
libavcodec  = dependency('libavcodec')
libavformat = dependency('libavformat')
libswscale  = dependency('libswscale')

# Optional dependencies used by some of the test programs 

sdl2 = dependency('sdl2')
fontconfig = dependency('fontconfig')

# Other libraries
# threads = dependency('threads')
# pulse = dependency('libpulse-simple')


#
# Provide a way to find blend2d if it is not properly packaged.
# 
blend2d_dir = get_option('blend2d_dir')
if blend2d_dir != ''
  lib_blend2d = cpp.find_library('blend2d',  dirs : [ join_paths(blend2d_dir,'lib') ] )
  inc_blend2d = include_directories(join_paths(blend2d_dir,'include'))
  blend2d = declare_dependency(
    include_directories : [ inc_blend2d ] ,
    dependencies : [ lib_blend2d ]
  )
else
  blend2d = dependency('blend2d')
endif

vex_cpp_args = []
if get_option('enable_abc')
  vex_cpp_args += ['-DENABLE_VEX_ABC=1']
endif

subdir('vex')
subdir('scripts')
subdir('tests')

pkgconfig = import('pkgconfig')
pkgconfig.generate(
        libraries: libvex,
        version: meson.project_version(),
        filebase: meson.project_name(),
        name: meson.project_name(),
        description: 'Vex video editing library',
)


