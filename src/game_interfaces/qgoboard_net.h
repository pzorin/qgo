#ifndef QGOBOARD_NET_H
#define QGOBOARD_NET_H

#include "defines.h"
#include "qgoboard.h"

class NetworkConnection;
class BoardDispatch;

class qGoBoardNetworkInterface : public qGoBoard
{
    Q_OBJECT
public:
    virtual ~qGoBoardNetworkInterface() {}
public slots:
    virtual void slotUndoPressed();
    virtual void slotDonePressed();
    virtual void slotResignPressed();
    virtual void slotReviewPressed() {}		//should FIXME these two
    virtual void slotAdjournPressed() {}
    virtual void passRequest();
protected:
    qGoBoardNetworkInterface(BoardWindow *boardWindow, Tree * tree, GameData *gameData);
    virtual void sendMoveToInterface(StoneColor c,int x, int y);
    virtual void handleMove(MoveRecord * m);
    virtual void moveControl(QString & player) { controlling_player = player; }
    virtual void adjournGame(void);
    virtual void startGame(void) {}
    virtual void stopTime(void);
    virtual void onFirstMove(void) {}

    QString game_Id;
    bool dontsend;
    QString controlling_player;
    Move * reviewCurrent;
    virtual Move *doMove(StoneColor c, int x, int y);
    BoardDispatch *dispatch;
    NetworkConnection *connection;
};

class qGoBoardObserveInterface : public qGoBoardNetworkInterface
{
    Q_OBJECT

public:
    qGoBoardObserveInterface(BoardWindow *boardWindow, Tree * tree, GameData *gameData);
    ~qGoBoardObserveInterface() {}

    void setModified(bool)	{} //we don't modify an observed game

public slots:
    void slotUpdateComment() {}		//what is this ?!?!?
    virtual void slotUndoPressed(void){}
    virtual void slotDonePressed(void){}
    virtual void slotResignPressed(void){}
    virtual void slotAdjournPressed(void){}

signals:
    void signal_sendCommandFromBoard(const QString&, bool);

private:
//	bool doMove(StoneColor c, int x, int y);
    virtual void onFirstMove(void);



};

class qGoBoardMatchInterface : public qGoBoardNetworkInterface
{
    Q_OBJECT

public:
    qGoBoardMatchInterface(BoardWindow *boardWindow, Tree * tree, GameData *gameData);
    ~qGoBoardMatchInterface() {}

    void setModified(bool)	{} //we don't modify a match game
    void setTimerInfo(const QString&, const QString&, const QString&, const QString&);
    void enterScoreMode();
    void leaveScoreMode();
    void timerEvent(QTimerEvent*);
    virtual void requestAdjournDialog(void);
    virtual void requestCountDialog(void);
    virtual void requestMatchModeDialog(void);
    virtual void requestDrawDialog(void);
    virtual void recvRefuseAdjourn(void);
    virtual void recvRefuseCount(void);
    virtual void recvRefuseMatchMode(void);
    virtual void recvRefuseDraw(void);
public slots:
    void slotUpdateComment() {}
    virtual void slotReviewPressed();
    virtual void slotDrawPressed();
    virtual void slotCountPressed();
    virtual void slotAdjournPressed();

signals:
    void signal_sendCommandFromBoard(const QString&, bool);

private:
    void localMoveRequest(StoneColor c, int x, int y);
    void localMarkDeadRequest(int x, int y);
    virtual void startGame(void);
    virtual void onFirstMove(void);
//	bool warningSound;
//	int warningSecs;

};

class qGoBoardReviewInterface : public qGoBoardNetworkInterface
{
    Q_OBJECT

public:
    qGoBoardReviewInterface(BoardWindow *boardWindow, Tree * tree, GameData *gameData);
    ~qGoBoardReviewInterface() {}

//	void setModified(bool)	{} //we don't modify an  game
//	void setResult(QString res, QString xt_res);
//	void setTimerInfo(const QString&, const QString&, const QString&, const QString&);
    //void set_move(StoneColor sc, QString pt, QString mv_nr);
    void setNode(int move_nr, StoneColor c, int x, int y);

public slots:
    void slotUpdateComment() {}
//	void slotDonePressed();
    void slotUndoPressed() ;

signals:
    void signal_sendCommandFromBoard(const QString&, bool);

private:
    void localMoveRequest(StoneColor c, int x, int y);

};


#endif // QGOBOARD_NET_H
