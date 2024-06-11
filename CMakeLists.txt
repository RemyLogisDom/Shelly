cmake_minimum_required(VERSION 3.16)
project(ShellyPlugin VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)

find_package(QT NAMES Qt5 Qt6 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Gui Mqtt Network Widgets)

qt_standard_project_setup()

qt_add_plugin(ShellyPlugin)
target_sources(ShellyPlugin PRIVATE
    Shelly.ui
    ShellyPlugin.cpp ShellyPlugin.h
    qjsonmodel.cpp qjsonmodel.h
    simplecrypt.cpp simplecrypt.h
)
target_link_libraries(ShellyPlugin PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Mqtt
    Qt::Network
    Qt::Widgets
)


# Resources:
set(ShellyPlugin_resource_files
    "images/lock.png"
    "images/unlock.png"
)

qt_add_resources(ShellyPlugin "ShellyPlugin"
    PREFIX
        "/"
    FILES
        ${ShellyPlugin_resource_files}
)

install(TARGETS ShellyPlugin
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    FRAMEWORK DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
