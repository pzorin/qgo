#include <QToolBar>

class QAction;
class CyberOroConnection;
class QPushButton;

class SetPhrasePalette : public QToolBar
{
	Q_OBJECT
	public:
		SetPhrasePalette(CyberOroConnection *);
		~SetPhrasePalette();
	public slots:
		void slot_triggered(QAction *);
	private:
		QPushButton * button0;
		QPushButton * button1;
		QPushButton * button2;
		QPushButton * button3;
		QPushButton * button4;
		QPushButton * button5;
		QPushButton * button6;
		QPushButton * button7;
		QPushButton * button8;
		QPushButton * button9;
		std::vector<QAction *> actions;
		CyberOroConnection * connection;
};
