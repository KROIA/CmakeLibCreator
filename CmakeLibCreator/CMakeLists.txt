
# Find the necessary QT modules
set(QT_MODULES
    Core
    Widgets
    Gui
    # Network
    # Add any other required modules here
)

set(use_qt ON)
set(deploy_qt ON)
set(APP_ICON "${CMAKE_CURRENT_SOURCE_DIR}/AppIcon.rc")
list(APPEND ADDITONAL_SOURCES ${APP_ICON})

list(APPEND ADDITIONAL_LIBRARIES ) 

## USER_SECTION_START 1

## USER_SECTION_END

exampleMaster(${LIBRARY_NAME} ${LIB_PROFILE_DEFINE} use_qt deploy_qt "${QT_MODULES}" "${ADDITONAL_SOURCES}" "${ADDITIONAL_LIBRARIES}" ${INSTALL_BIN_PATH})

## USER_SECTION_START 2

## USER_SECTION_END