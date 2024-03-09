## description: simple ribbon widget for QT applications

cmake_minimum_required(VERSION 3.1.0)


include(FetchContent)


FetchContent_Declare(
    RibbonWidget
    GIT_REPOSITORY https://github.com/KROIA/RibbonWidget.git
    GIT_TAG        main
)

set(RIBBONWIDGET_NO_EXAMPLE True)
set(RIBBONWIDGET_NO_UNITTESTS True)
FetchContent_MakeAvailable(RibbonWidget)


# Add this library to the specific profiles of this project
list(APPEND DEPENDENCIES_FOR_SHARED_LIB RibbonWidget_static)
list(APPEND DEPENDENCIES_FOR_STATIC_LIB RibbonWidget_static)
list(APPEND DEPENDENCIES_FOR_STATIC_PROFILE_LIB RibbonWidget_static_profile) # only use for static profiling profile
