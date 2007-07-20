# Fichier généré par le module QMake de KDevelop. 
# -------------------------------------------------- 
# Sous dossier relatif au dossier principal du projet : ./src
# Cible : une application??:  ../bin/qgo2

RESOURCES = application.qrc  \
board/board.qrc
QT = core gui \
network
TARGET = ../bin/qgo2 
CONFIG += warn_on \
          qt \
          thread \
          qtestlib \
          debug \
          stl
TEMPLATE = app 
FORMS += mainwindow.ui \
board/boardwindow.ui \
talk_gui.ui \
server/gamedialog.ui
HEADERS += mainwindow.h \
board/boardwindow.h \
board/board.h \
defines.h \
sgf/sgfparser.h \
globals.h \
game_tree/matrix.h \
game_tree/move.h \
game_tree/tree.h \
game_tree/group.h \
board/imagehandler.h \
board/stone.h \
board/gatter.h \
board/boardhandler.h \
board/interfacehandler.h \
board/mark.h \
gtp/qgtp.h \
game_interfaces/qgoboard.h \
audio/audio.h \
mainwindow_settings.h \
server/igsconnection.h \
server/parser.h \
game_interfaces/qgo_interface.h \
board/clockdisplay.h \
talk.h \
server/gamedialog.h
SOURCES += main.cpp \
           mainwindow.cpp \
           board/boardwindow.cpp \
           board/board.cpp \
           sgf/sgfparser.cpp \
           game_tree/matrix.cpp \
           game_tree/move.cpp \
           game_tree/tree.cpp \
           game_tree/group.cpp \
           board/imagehandler.cpp \
           board/stone.cpp \
           board/gatter.cpp \
           board/boardhandler.cpp \
           board/interfacehandler.cpp \
           board/mark.cpp \
           gtp/qgtp.cpp \
           game_interfaces/qgoboard.cpp \
           game_interfaces/qgoboard_computer.cpp \
           audio/audio.cpp \
           mainwindow_settings.cpp \
           server/igsconnection.cpp \
           server/parser.cpp \
           mainwindow_server.cpp \
           game_interfaces/qgo_interface.cpp \
           game_interfaces/qgoboard_observe.cpp \
           board/clockdisplay.cpp \
           talk.cpp \
           server/gamedialog.cpp \
           game_interfaces/qgoboard_match.cpp
QT -= qt3support
DEPENDPATH += "board sgf game_tree game_interfaces gtp"
INCLUDEPATH += server \
audio \
gtp \
game_interfaces \
game_tree \
board \
sgf


macx{
    CONFIG += x86 ppc
    QMAKE_MAC_SDK = /Developer/SDKs/MacOSX10.4u.sdk
}

linux-g++{
    SOURCES += audio/alsa.cpp
    LIBS += -lasound
    CONFIG += debug
}
