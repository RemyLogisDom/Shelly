TEMPLATE        = lib
CONFIG         += plugin
QT             += widgets network mqtt
HEADERS         = ShellyPlugin.h qjsonmodel.h simplecrypt.h
SOURCES         = ShellyPlugin.cpp qjsonmodel.cpp simplecrypt.cpp
TARGET          = ShellyPlugin
FORMS += Shelly.ui

RESOURCES += \
    ShellyPlugin.qrc

