#include <QDialog>

class QLabel;
class QCheckBox;
class QDialogButtonBox;

class MatchInviteDialog : public QDialog
{
	Q_OBJECT
	public:
		MatchInviteDialog(QString name, QString rank, bool canRefuseFuture = false);
		~MatchInviteDialog();
		virtual void timerEvent(QTimerEvent*);
	public slots:
		void slot_accept(void);
		void slot_decline(void);
		void slot_refuseFutureCB(bool b);
	private:
		QLabel * namelabel, * dialoglabel, * timelabel;
		QPushButton * acceptButton;
		QPushButton * declineButton;
		QCheckBox * refuseFutureCB;
		QDialogButtonBox * buttonBox;
		int seconds;
};
