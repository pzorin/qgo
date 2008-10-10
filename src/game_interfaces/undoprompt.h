#include <QDialog>

class QLabel;
class QSpinBox;
class QDialogButtonBox;

class UndoPrompt : public QDialog
{
	Q_OBJECT
	public:
		UndoPrompt(const QString * _name, bool multiple, int _moves);
		~UndoPrompt();
		virtual void timerEvent(QTimerEvent*);
	public slots:
		void slot_accept(void);
		void slot_decline(void);
	private:
		QLabel * mainlabel;
		QSpinBox * movesSpin;
		QPushButton * acceptButton;
		QPushButton * declineButton;	
		QDialogButtonBox * buttonBox;
		const QString * name;
		int moves;
};
