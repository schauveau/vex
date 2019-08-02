
# generate input video usng FFMpeg Mandelbrot filter
gen_input1_video = custom_target('input1_video',
    output  : 'input1.mp4',
    command : [ 'ffmpeg',
                '-loglevel', 'warning',
                '-y', '-hide_banner',
                '-f', 'lavfi',
                '-i', 'mandelbrot=start_scale=0.01',
                '-vf', 'gblur=1',
                '-t', '10',
                '@OUTPUT@'
              ] , 
    build_by_default : get_option('gen-input-videos')
  )

# Another generated Mandelbrot input video using FFMpeg Mandelbrot filter
gen_input2_video = custom_target('input2_video',
    output  : 'input2.webm',
    command : [ 'ffmpeg',
                '-loglevel', 'warning',
                '-y', '-hide_banner',
                '-f', 'lavfi',
                '-i', 'mandelbrot',
                '-t', '10',
                '@OUTPUT@'
              ] ,
    build_by_default : get_option('gen-input-videos')
   )

# Generate input video using ffmpe 'testsrc2' filter
gen_input3_video = custom_target('input3_video',
    output  : 'input3.mkv',
    command : [ 'ffmpeg',
                '-loglevel', 'warning',
                '-y', '-hide_banner',
                '-f', 'lavfi',
                '-i', 'testsrc2=size=1024x768:rate=24',
                '-t', '10',
                '@OUTPUT@'
              ] ,
    build_by_default : get_option('gen-input-videos')
   )

# Additional options passed to all tests
test_cpp_args = [ '-DSRC_DIR="@0@"'.format(meson.source_root()),
                  '-DBUILD_DIR="@0@"'.format(meson.build_root())
                ] 

all_tests = {
          'vex-demo1':
            {
              'src': [ 'vex-demo1.cc' ],
              'dep': [ vex , fontconfig ] ,
              'cpp_args': [ ]
            },
          'test-timestamp':
            {
              'src': [ 'test-timestamp.cc' ],
              'dep': [ vex ],
              'cpp_args': [ ]
            },
          'test-video-writer':
            {
              'src': [ 'test-video-writer.cc' ],
              'dep': [ vex ],
              'cpp_args': [ ]
            },      
          'test-sdl':
            {
              'src': [ 'test-sdl.cc' ],
              'dep': [ vex , sdl2 ],
              'cpp_args': [ '-Dfoobar' ]
            }            
        }


foreach name, info : all_tests
  ok = true
  foreach d : info.get('dep')
    if not d.found()
      ok = false
    endif
  endforeach
  if ok    
    executable(name,
               sources : info.get('src'),
               dependencies: info.get('dep'),
               build_by_default: get_option('tests'),
               cpp_args : test_cpp_args + info.get('cpp_args'),
               install : true, 
#               install_rpath : '/opt/foobar/lib'
              )          
  else
    warning('Ignoring test ' + name+ ' because of missing dependencies')
  endif
endforeach


