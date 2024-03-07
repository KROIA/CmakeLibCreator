cmake_minimum_required(VERSION 3.1.0)


include(FetchContent)

set(DEBUG_POSTFIX_STR "-d")


FetchContent_Declare(
    EASY_PROFILER
    GIT_REPOSITORY https://github.com/yse/easy_profiler.git
    GIT_TAG        develop
)

set(EASY_PROFILER_NO_SAMPLES True)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build easy_profiler as static library.")
FetchContent_MakeAvailable(EASY_PROFILER)


# Add this library to the specific profiles of this project
list(APPEND DEPENDENCIES_FOR_SHARED_LIB )
list(APPEND DEPENDENCIES_FOR_STATIC_LIB )
list(APPEND DEPENDENCIES_FOR_STATIC_PROFILE_LIB easy_profiler) # only use for static profiling profile

# Deploy the Profiler GUI
if(QT_ENABLE AND QT_DEPLOY)
    DEPLOY_QT(profiler_gui ${INSTALL_BIN_PATH})
endif()


#set(EASY_PROFILER_NO_GUI False)
set_target_properties(easy_profiler PROPERTIES CMAKE_RUNTIME_OUTPUT_DIRECTORY ${RUNTIME_OUTPUT_DIRECTORY})
set_target_properties(easy_profiler PROPERTIES CMAKE_LIBRARY_OUTPUT_DIRECTORY ${RUNTIME_OUTPUT_DIRECTORY})
set_target_properties(easy_profiler PROPERTIES CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${RUNTIME_OUTPUT_DIRECTORY})
set_target_properties(easy_profiler PROPERTIES DEBUG_POSTFIX ${DEBUG_POSTFIX_STR})
target_compile_definitions(easy_profiler PUBLIC EASY_PROFILER_STATIC)