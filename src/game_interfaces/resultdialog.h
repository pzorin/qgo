#include <QDialog>

class QLabel;
class GameResult;
class BoardDispatch;

class ResultDialog : public QDialog
{
	Q_OBJECT
	public:
		ResultDialog(QWidget * parent, BoardDispatch * dis, unsigned int game_id, GameResult * gr);
		~ResultDialog();
		void recvRematchRequest(void);
		virtual void timerEvent(QTimerEvent*);
		//virtual QSize minimumSize(void) const { return QSize(300, 150); };
	public slots:
		void slot_okay(void);
	private:
		QLabel * mainlabel;
		QPushButton * okayButton;
		BoardDispatch * dispatch;
		int seconds;
		bool accepting;
};
