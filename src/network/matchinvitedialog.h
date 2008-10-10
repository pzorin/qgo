#include <QDialog>

class QLabel;
class QDialogButtonBox;

class MatchInviteDialog : public QDialog
{
	Q_OBJECT
	public:
		MatchInviteDialog(QString name, QString rank);
		~MatchInviteDialog();
		virtual void timerEvent(QTimerEvent*);
	public slots:
		void slot_accept(void);
		void slot_decline(void);
	private:
		QLabel * namelabel, * dialoglabel, * timelabel;
		QPushButton * acceptButton;
		QPushButton * declineButton;	
		QDialogButtonBox * buttonBox;
		int seconds;
};
