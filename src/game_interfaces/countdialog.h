#include <QDialog>

class QLabel;
class QDialogButtonBox;
class GameResult;
class BoardDispatch;
class BoardWindow;

class CountDialog : public QDialog
{
	Q_OBJECT
	public:
		CountDialog(BoardWindow * parent, BoardDispatch * dis, unsigned int game_id);
		~CountDialog();
		//virtual QSize minimumSize(void) const { return QSize(300, 150); };
		void recvRejectCount(void);
		void recvAcceptCount(void);
	public slots:
		void slot_accept(void);
		void slot_reject(void);
	private:
		QLabel * mainlabel;
		QPushButton * acceptButton;
		QPushButton * rejectButton;
		QDialogButtonBox * buttonBox;
		BoardDispatch * dispatch;
		BoardWindow * board;
		GameResult * result;
		bool oppAcceptsCount;
		bool oppRejectsCount;
		bool weAcceptCount;
		bool weRejectCount;
};
