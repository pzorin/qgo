/*
 * interfacehandler.h
 */

#ifndef INTERFACEHANDLER_H
#define INTERFACEHANDLER_H

#include "../defines.h"
//#include "boardwindow.h"
//#include "boardhandler.h"


//struct ButtonState;
class BoardWindow;

class InterfaceHandler
{
public:
	InterfaceHandler( BoardWindow * bw);
	~InterfaceHandler();
	void clearData();
//	GameMode toggleMode();
//	void setEditMode();
//	void setMarkType(int m);
	void setMoveData(int n, bool black, int brothers, int sons, bool hasParent,
		bool hasPrev, bool hasNext, int lastX=-1, int lastY=-1);
/*	void setCaptures(float black, float white, bool scored=false);
	void setTimes(const QString &btime, const QString &bstones, const QString &wtime, const QString &wstones);
	void setTimes(bool, float, int);
	void clearComment();
	void displayComment(const QString &c);
	QString getComment();
	QString getComment2();
	void toggleSidebar(bool toggle);
	QString getTextLabelInput(QWidget *parent, const QString &oldText);
	void showEditGroup();
	void toggleMarks();
	const QString getStatusMarkText(MarkType t);
	void disableToolbarButtons();
	void restoreToolbarButtons();
	void setScore(int terrB, int capB, int terrW, int capW, float komi=0);
	void setClipboard(bool b);
	void setSliderMax(int n);

	QLabel *moveNumLabel, *turnLabel, *varLabel, *capturesBlack, *capturesWhite;
	QAction *navBackward,  *navForward, *navFirst, *navLast, *navNextVar, *navIntersection, //SL added eb 11
		*navPrevVar, *navStartVar, *navNextBranch, *navMainBranch, *navNthMove, *navAutoplay,
		*editCut, *editPaste, *editPasteBrother, *editDelete,
		*navEmptyBranch, *navCloneNode, *navSwapVariations, *navPrevComment, *navNextComment,
 		*fileImportASCII, *fileImportASCIIClipB, *fileImportSgfClipB, *fileNew, *fileNewBoard, *fileOpen ;
	QTextEdit *commentEdit;
	QLineEdit *commentEdit2;
//	EditTools *editTools;
	NormalTools *normalTools;
//	TeachTools *teachTools;
	ScoreTools *scoreTools;
	QFrame *toolsFrame;
	Board *board;
	QLabel *statusMode, *statusTurn, *statusMark, *statusNav;
	QPushButton  *scoreButton, *passButton, *undoButton, *resignButton, 
		*adjournButton, *refreshButton;
	ButtonState *buttonState;
	QSlider *slider;
*/
//	MainWidget *mainWidget;
	BoardWindow *boardwindow;
	bool scored_flag;

};

#endif
