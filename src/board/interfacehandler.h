/*
 * interfacehandler.h
 */

#ifndef INTERFACEHANDLER_H
#define INTERFACEHANDLER_H

#include "defines.h"
//#include "boardhandler.h"


//struct ButtonState;
class BoardWindow;
class GameData;

class InterfaceHandler
{
public:
	InterfaceHandler( BoardWindow * bw);
	~InterfaceHandler();
	void clearData();
	void toggleMode(GameMode gameMode);
//	void setEditMode();
//	void setMarkType(int m);
	void setMoveData(int n, bool black, int brothers, int sons, bool hasParent,
		bool hasPrev, bool hasNext, int lastX=-1, int lastY=-1);
	void updateCaption(GameData *gd);
	void displayComment(const QString &c);
	void setSliderMax(int n);
	void setCaptures(float black, float white);
	void clearComment();
//	void toggleToolbarButtons(bool state);
	void setTimes(const QString &btime, const QString &bstones, const QString &wtime, const QString &wstones);
	void setScore(int terrB, int capB, int terrW, int capW, float komi=0);
/*	void setTimes(bool, float, int);
	QString getComment();
	QString getComment2();
	void toggleSidebar(bool toggle);
	QString getTextLabelInput(QWidget *parent, const QString &oldText);
	void showEditGroup();
	void toggleMarks();
	const QString getStatusMarkText(MarkType t);
	void restoreToolbarButtons();

	void setClipboard(bool b);
	

*/
	BoardWindow *boardwindow;
	bool scored_flag;

private :
	QWidget *tabPlay, *tabEdit;

};

#endif
