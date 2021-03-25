add_executable(001 001_remove_yuv_color.cc)
target_link_libraries(001 util)

add_executable(002 002_picture_in_picture.cc)
target_link_libraries(002 util)

add_executable(003 003_merge.cc)
target_link_libraries(003 util)

add_executable(004 004_rgb2yuv.cc)
target_link_libraries(004 util)

add_executable(010 010_capture.cc)
target_link_libraries(010 util SDL2 pthread)

add_executable(100 100_capture_mjpeg.cc)
target_link_libraries(100 avdevice avfilter avformat avcodec swresample swscale avutil SDL2 m rt z pthread util)

add_executable(101 101_capture_yuy2.cc)
target_link_libraries(101 avdevice avfilter avformat avcodec swresample swscale avutil SDL2 m rt z pthread util)

add_executable(102 102_jpg2yuv.cc)
target_link_libraries(102 avformat avcodec avfilter swresample swscale avutil m z rt pthread)

add_executable(103 103_h264_to_yuv.cc)
target_link_libraries(103 avformat avcodec avfilter swresample swscale avutil m z rt pthread)