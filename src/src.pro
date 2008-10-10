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
          stl \
          debug
TEMPLATE = app 
OBJECTS_DIR = objects
TRANSLATIONS = translations/qgo2_cz.ts \
	    	translations/qgo2_de.ts \
		translations/qgo2_dk.ts \
		translations/qgo2_fr.ts \
		translations/qgo2_it.ts \
		translations/qgo2_la.ts \
		translations/qgo2_nl.ts \
		translations/qgo2_pt.ts \
		translations/qgo2_ru.ts \
		translations/qgo2_tr.ts \
		translations/qgo2_zh_cn.ts \
		translations/qgo2_zh.ts
FORMS += mainwindow.ui \
board/boardwindow.ui \
talk_gui.ui \
gamedialog.ui \
board/gameinfo.ui \
network/createroomdialog.ui
HEADERS += mainwindow.h \
board/boardwindow.h \
board/board.h \
defines.h \
gamedata.h \
sgf/sgfparser.h \
#globals.h \
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
room.h \
listviews.h \
network/playergamelistings.h \
network/networkconnection.h \
network/networkdispatch.h \
network/messages.h \
network/msghandler.h \
network/roomdispatch.h\
network/boarddispatch.h \
network/consoledispatch.h \
network/gamedialogdispatch.h \
network/talkdispatch.h \
#network/parser.h \
roomregistries.h \
network/dispatchregistries.h \
registry.h \
network/igsconnection.h \
network/wing.h \
network/lgs.h \
network/cyberoroconnection.h \
game_interfaces/undoprompt.h \
game_interfaces/resultdialog.h \
board/clockdisplay.h \
talk.h \
gamedialog.h \
displayboard.h \
 network/serverlistdialog.h \
 network/matchinvitedialog.h \
 network/gamedialogflags.h \
 network/orosetphrasechat.h \
 network/setphrasepalette.h \
 network/createroomdialog.h \
 network/cyberoroprotocolcodes.h

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
	   game_interfaces/undoprompt.cpp \
	   game_interfaces/resultdialog.cpp \
           audio/audio.cpp \
           mainwindow_settings.cpp \
	   network/networkconnection.cpp \
	   network/networkdispatch.cpp \
 	   network/msghandler.cpp \
	   network/igsconnection.cpp \
	   network/wing.cpp \
	   network/lgs.cpp \
	   network/cyberoroconnection.cpp \
	   network/messages.h \
	   network/networkconnection.h \
	   network/roomdispatch.cpp \
	   network/boarddispatch.cpp \
	   network/gamedialogdispatch.cpp \
	   network/talkdispatch.cpp \
	   network/consoledispatch.cpp \
	   newline_pipe.h \
	   room.cpp \
           mainwindow_server.cpp \
	   listviews.cpp \
           board/clockdisplay.cpp \
           talk.cpp \
           gamedialog.cpp \
           displayboard.cpp \
	   game_interfaces/qgoboard_network.cpp \
           game_interfaces/qgoboard_observe.cpp \
           game_interfaces/qgoboard_match.cpp \
           game_interfaces/qgoboard_review.cpp \
 	   network/serverlistdialog.cpp \
 	   network/matchinvitedialog.cpp \
 	   network/orosetphrasechat.cpp \
 	   network/setphrasepalette.cpp \ 
	   network/createroomdialog.cpp

QT -= qt3support
DEPENDPATH += """"""board sgf game_tree game_interfaces gtp""""""
macx {
    CONFIG += x86 ppc
    QMAKE_MAC_SDK = /Developer/SDKs/MacOSX10.4u.sdk
}

linux-g++ {
    SOURCES += audio/alsa.cpp
    HEADERS += audio/alsa.h

    LIBS += -lasound

}
INCLUDEPATH += server \
network \
audio \
gtp \
game_interfaces \
game_tree \
board \
sgf
