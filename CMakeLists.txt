cmake_minimum_required(VERSION 3.20)

## INFO
## Template version: 1.0.0
##
## This is a template for a CMake project.
## It is designed to be used with the Visual Studio
## IDE and the Qt framework.
##
## ********************************************************************************
##  !!! IMPORTANT !!!
##  DO NOT CHANGE/REMOVE THE COMMENTS MARKED WITH <...>
##  THEY ARE USED BY THE SCRIPTS TO AUTOMATICALLY 
##  CONFIGURE THE PROJECT FILES.
##  YOU CAN ADD YOUR OWN COMMENTS, BUT DO NOT REMOVE
##  THE EXISTING ONES.
## ********************************************************************************
##
## --------------------------------------------------------------------------------
## -------------------- Project settings ------------------------------------------
## --------------------------------------------------------------------------------

# Name of the library
set(LIBRARY_NAME CmakeLibCreator)                   # <LIBRARY_NAME>

# Define is only active if the library is built
set(LIB_DEFINE CMAKE_LIB_CREATOR_LIB)                # <LIB_DEFINE>

# macro to enable profiling
set(LIB_PROFILE_DEFINE CLC_PROFILING)               # <LIB_PROFILE_DEFINE>  


# Enable/disable QT modules
set(QT_ENABLE ON)	                            # <QT_ENABLE> 
# Enable/disable QT deployment
set(QT_DEPLOY ON)	                            # <QT_DEPLOY> 

# Needed QT modules
set(QT_MODULES                                  # <QT_MODULES> 
    Core
    Widgets
    # Add any other required modules here
)

# Library file name settings
set(DEBUG_POSTFIX_STR  "-d")                    # <DEBUG_POSTFIX_STR> 
set(STATIC_POSTFIX_STR "-s")                    # <STATIC_POSTFIX_STR> 
set(PROFILING_POSTFIX_STR "-p")                 # <PROFILING_POSTFIX_STR> 

# Langauge settings
set(CMAKE_CXX_STANDARD 23)                      # <CMAKE_CXX_STANDARD> 
set(CMAKE_CXX_STANDARD_REQUIRED ON)             # <CMAKE_CXX_STANDARD_REQUIRED> 

# --------------------------------------------------------------------------------
# -------------------- Build settings --------------------------------------------
# --------------------------------------------------------------------------------

set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR})   # Default install path
set(RELATIVE_BUILD_FOLDER build)                # Relative path to the build folder
set(RELATIVE_INSTALL_FOLDER lib)                # Relative path to the install folder

# --------------------------------------------------------------------------------
# --------------------------------------------------------------------------------
# --------------------------------------------------------------------------------

project(${LIBRARY_NAME})
 
# Set output paths
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/${RELATIVE_BUILD_FOLDER}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/${RELATIVE_BUILD_FOLDER}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/${RELATIVE_BUILD_FOLDER}")

set(INSTALL_LIB_PATH "${CMAKE_INSTALL_PREFIX}/${RELATIVE_INSTALL_FOLDER}")
set(INSTALL_BIN_PATH "${CMAKE_INSTALL_PREFIX}/bin")
# create bin folder

link_directories(${CMAKE_LIBRARY_OUTPUT_DIRECTORY})


include(cmake/QtLocator.cmake)
include(cmake/utility.cmake)
include(cmake/dependencies.cmake)
include(cmake/ExampleMaster.cmake)


add_subdirectory(core)
add_subdirectory(CmakeLibCreator)     
#add_subdirectory(examples)     
#add_subdirectory(unitTests)

