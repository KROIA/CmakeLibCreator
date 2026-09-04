##
## This file creates a new target exe with the given parameters
## Override any settings if needed.
## If any setting is not overriden, the default value from the library will be used.
##

## USER_SECTION_START 1

## The UnitTest GUI driver lives in the UnitTest library but is behind UNITTEST_GUI_ENABLED, so the
## library build compiles it to nothing. A consuming target has to define the macro and compile the
## one .cpp in itself. FetchContent exports its source dir inside the dep() function scope only, so
## fall back to the known cache layout when the variable did not survive.
if(DEFINED unittest_SOURCE_DIR)
    set(UNITTEST_SOURCE_DIR "${unittest_SOURCE_DIR}")
else()
    set(UNITTEST_SOURCE_DIR "${FETCHCONTENT_BASE_DIR}/unittest-src")
endif()
set(GUI_INC_DIR "${UNITTEST_SOURCE_DIR}/core/inc/gui")
set(GUI_SRC_DIR "${UNITTEST_SOURCE_DIR}/core/src/gui")
include_directories(${GUI_INC_DIR})
list(APPEND ADDITONAL_SOURCES "${GUI_SRC_DIR}/UnitTest_Gui.cpp")
add_definitions(-DUNITTEST_GUI_ENABLED)

## USER_SECTION_END

## The dialog under test is a QWidget, so this target needs the widget toolkit.
set(QT_MODULES
    Core
    Gui
    Widgets
)

## USER_SECTION_START 2

## USER_SECTION_END

set(QT_ENABLE ON)
set(QT_DEPLOY ON)

## USER_SECTION_START 3

## USER_SECTION_END

## A unittest target will use an additional library
list(APPEND ADDITIONAL_LIBRARIES UnitTest_static)

## USER_SECTION_START 4

## USER_SECTION_END

## Do not change the first 2 parameters
##             Do not change      Do not change
##                 V                  V
exampleMaster(${LIBRARY_NAME} ${LIB_PROFILE_DEFINE} ${QT_ENABLE} ${QT_DEPLOY} "${QT_MODULES}" "${ADDITONAL_SOURCES}" "${ADDITIONAL_LIBRARIES}" "${INSTALL_BIN_PATH}")

## USER_SECTION_START 5

## USER_SECTION_END
