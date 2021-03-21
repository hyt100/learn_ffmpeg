add_executable(001 001_remove_yuv_color.cc)
target_link_libraries(001 util)

add_executable(002 002_picture_in_picture.cc)
target_link_libraries(002 util)

add_executable(003 003_merge.cc)
target_link_libraries(003 util)

add_executable(010 010_capture.cc)
target_link_libraries(010 util SDL2 pthread)

add_executable(101 101_jpg2yuv.cc)
target_link_libraries(101 avformat avcodec avfilter swresample swscale avutil m z rt pthread)