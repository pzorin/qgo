# Fichier généré par le module QMake de KDevelop. 
# -------------------------------------------------- 
# Sous dossier relatif au dossier principal du projet : ./src
# Cible : une application??:  ../bin/qgo2

RESOURCES = application.qrc  \
board/board.qrc
QT = core gui
TARGET = ../bin/qgo2 
CONFIG += warn_on \
          qt \
          thread \
          qtestlib \
          debug \
          stl
TEMPLATE = app 
FORMS += mainwindow.ui \
board/boardwindow.ui
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
game_interfaces/qgoboard.h
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
           game_interfaces/qgoboard_computer.cpp
QT -= qt3support
DEPENDPATH += "board sgf game_tree game_interfaces gtp"
INCLUDEPATH += gtp \
game_interfaces \
game_tree \
board \
sgf
