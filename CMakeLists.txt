cmake_minimum_required(VERSION 3.10)

# set the project name and version
project(rtsp_server VERSION 1.0)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# copy config file
file(COPY "${PROJECT_SOURCE_DIR}/vlog.conf" DESTINATION "${PROJECT_BINARY_DIR}")

# configure a header file to pass the version number only
configure_file(rtsp_server.h.in rtsp_server.h)

# vlog lib
list(APPEND EXTRA_INCLUDES "${PROJECT_SOURCE_DIR}/third_party/vlog/include")
list(APPEND EXTRA_LIBS "${PROJECT_SOURCE_DIR}/third_party/vlog/lib")

# ffmpeg lib
list(APPEND EXTRA_INCLUDES "${PROJECT_SOURCE_DIR}/third_party/ffmpeg/include")
list(APPEND EXTRA_LIBS "${PROJECT_SOURCE_DIR}/third_party/ffmpeg/lib")

# md5
list(APPEND EXTRA_INCLUDES "${PROJECT_SOURCE_DIR}/third_party/md5/include")

# search all src code
aux_source_directory("${PROJECT_SOURCE_DIR}/src" DIR_MAIN_SRCS)
aux_source_directory("${PROJECT_SOURCE_DIR}/src/net" DIR_NET_SRCS)
aux_source_directory("${PROJECT_SOURCE_DIR}/src/xop" DIR_XOP_SRCS)

add_executable(rtsp_server ${DIR_DEV_SRCS} ${DIR_NET_SRCS} ${DIR_XOP_SRCS} ${DIR_MAIN_SRCS})

target_include_directories(rtsp_server PUBLIC "${PROJECT_BINARY_DIR}")
target_include_directories(rtsp_server PUBLIC "${PROJECT_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}/src/net" "${PROJECT_SOURCE_DIR}/src/xop")
target_include_directories(rtsp_server PUBLIC "${EXTRA_INCLUDES}")
target_link_directories(rtsp_server PUBLIC "${EXTRA_LIBS}")
target_link_libraries(rtsp_server PUBLIC vlog avdevice avfilter postproc avformat avcodec rt dl Xext X11 asound SDL z rt swresample swscale avutil m z swresample pthread)

install(TARGETS rtsp_server DESTINATION bin)
install(DIRECTORY "${PROJECT_SOURCE_DIR}/third_party/vlog/lib" DESTINATION .)
install(DIRECTORY "${PROJECT_SOURCE_DIR}/third_party/ffmpeg/lib" DESTINATION .)