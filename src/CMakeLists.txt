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

add_executable(012 012_rotate.cc)
target_link_libraries(012 util yuv)

add_executable(100 100_capture_mjpeg.cc)
target_link_libraries(100 util avdevice avfilter avformat avcodec swresample swscale avutil SDL2 m rt z pthread)

add_executable(101 101_capture_yuy2.cc)
target_link_libraries(101 util avdevice avfilter avformat avcodec swresample swscale avutil SDL2 m rt z pthread)

add_executable(102 102_jpg2yuv.cc)
target_link_libraries(102 util_ff avformat avcodec avfilter swresample swscale avutil m z rt pthread)

add_executable(103 103_yuv2jpg.cc)
target_link_libraries(103 util_ff util avformat avcodec avfilter swresample swscale avutil m z rt pthread)

add_executable(104 104_h264_to_yuv.cc)
target_link_libraries(104 util_ff avformat avcodec avfilter swresample swscale avutil m z rt pthread)

add_executable(105 105_yuv_to_h264.cc)
target_link_libraries(105 util_ff avformat avcodec avfilter swresample swscale avutil x264 m z rt pthread)

add_executable(106 106_mp4_speedup.c)
target_link_libraries(106 util_ff avformat avcodec avfilter swresample swscale avutil x264 m z rt pthread)

add_executable(107 107_extract_frame.cc)
target_link_libraries(107 util_ff util avformat avcodec avfilter swresample swscale avutil m z rt pthread)

# add_executable(108 108_yuv_to_mp4.cc)
# target_link_libraries(108 util_ff util avformat avcodec avfilter swresample swscale avutil m z rt pthread)

add_executable(109 109_mp4_to_h264.cc)
target_link_libraries(109 util_ff util avformat avcodec avfilter swresample swscale avutil m z rt pthread)