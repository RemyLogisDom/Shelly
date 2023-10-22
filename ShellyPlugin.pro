TEMPLATE        = lib
CONFIG         += plugin
QT             += widgets network
HEADERS         = ShellyPlugin.h qjsonmodel.h
SOURCES         = ShellyPlugin.cpp qjsonmodel.cpp
TARGET          = ShellyPlugin
FORMS += Shelly.ui

RESOURCES += \
    ShellyPlugin.qrc

