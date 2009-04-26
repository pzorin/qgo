#qmake file

#message($${CONFIG})
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
#Because I can't figureout how to turn console and exceptions off:
win32 {
    QMAKE_LFLAGS = -static -enable-stdcall-fixup -Wl,-enable-auto-import -Wl,-enable-runtime-pseudo-reloc -Wl,-subsystem,windows
    QMAKE_LFLAGS_CONSOLE =
    QMAKE_LFLAGS_EXCEPTIONS_ON =
    # also
    RC_FILE = qgo.rc
}
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
FORMS += gamedialog.ui \
	 mainwindow.ui \
	 talk_gui.ui \
	 board/boardwindow.ui \
	 board/gameinfo.ui \
	 network/createroomdialog.ui \
	 network/friendslistdialog.ui \
	 network/login.ui 

HEADERS += defines.h \
displayboard.h \
gamedata.h \
gamedialog.h \
listviews.h \
registry.h \
mainwindow.h \
mainwindow_settings.h \
talk.h \
audio/audio.h \
game_tree/group.h \
game_tree/matrix.h \
game_tree/move.h \
game_tree/tree.h \
board/board.h \
board/boardhandler.h \
board/boardwindow.h \
board/clockdisplay.h \
board/gatter.h \
board/imagehandler.h \
board/interfacehandler.h \
board/mark.h \
board/stone.h \
game_interfaces/countdialog.h \
game_interfaces/qgoboard.h \
game_interfaces/resultdialog.h \
game_interfaces/undoprompt.h \
gtp/qgtp.h \
network/boarddispatch.h \
network/codecwarndialog.h \
network/consoledispatch.h \
network/createroomdialog.h \
network/cyberoroconnection.h \
network/cyberoroprotocol.h \
network/dispatchregistries.h \
network/eweiqiconnection.h \
network/friendslistdialog.h \
network/gamedialogflags.h \
network/igsconnection.h \
network/lgs.h \
network/login.h \
network/matchinvitedialog.h \
network/messages.h \
network/networkconnection.h \
network/orosetphrasechat.h \
network/playergamelistings.h \
network/protocol.h \
network/quickconnection.h \
network/room.h \
network/roomregistries.h \
network/serverlistdialog.h \
network/serverliststorage.h \
network/setphrasepalette.h \
network/tomconnection.h \
network/tygemconnection.h \
network/tygemprotocol.h \
network/wing.h \
sgf/sgfparser.h

SOURCES += displayboard.cpp \
           gamedialog.cpp \
           listviews.cpp \
 	   main.cpp \
           mainwindow.cpp \
           mainwindow_server.cpp \
           mainwindow_settings.cpp \
 	   newline_pipe.h \
 	   talk.cpp \
           audio/audio.cpp \
           board/board.cpp \
           board/boardhandler.cpp \
           board/boardwindow.cpp \
           board/clockdisplay.cpp \
           board/gatter.cpp \
           board/imagehandler.cpp \
           board/interfacehandler.cpp \
           board/mark.cpp \
           board/stone.cpp \
	   game_interfaces/countdialog.cpp \
	   game_interfaces/qgoboard.cpp \
           game_interfaces/qgoboard_computer.cpp \
           game_interfaces/qgoboard_match.cpp \
	   game_interfaces/qgoboard_network.cpp \
           game_interfaces/qgoboard_observe.cpp \
           game_interfaces/qgoboard_review.cpp \
           game_interfaces/resultdialog.cpp \
           game_interfaces/undoprompt.cpp \
           game_tree/matrix.cpp \
           game_tree/move.cpp \
           game_tree/tree.cpp \
           game_tree/group.cpp \
           gtp/qgtp.cpp \
       	   network/boarddispatch.cpp \
	   network/codecwarndialog.cpp \
	   network/consoledispatch.cpp \
	   network/createroomdialog.cpp \
	   network/cyberoroconnection.cpp \
	   network/eweiqiconnection.cpp \
	   network/friendslistdialog.cpp \
	   network/igsconnection.cpp \
	   network/lgs.cpp \
 	   network/login.cpp \
	   network/matchinvitedialog.cpp \
	   network/messages.h \
	   network/networkconnection.cpp \
	   network/networkconnection.h \
	   network/orosetphrasechat.cpp \
 	   network/quickconnection.cpp \
 	   network/room.cpp \
	   network/serverlistdialog.cpp \
	   network/serverliststorage.cpp \
	   network/setphrasepalette.cpp \
	   network/tomconnection.cpp \
	   network/tygemconnection.cpp \
	   network/wing.cpp \
	   sgf/sgfparser.cpp

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
