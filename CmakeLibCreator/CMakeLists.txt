cmake_minimum_required(VERSION 3.1.0)

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

exampleMaster(${LIBRARY_NAME} ${LIB_PROFILE_DEFINE} use_qt deploy_qt "${QT_MODULES}" ${INSTALL_BIN_PATH})