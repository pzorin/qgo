#include <QDialog>

class QLabel;
class QCheckBox;

class CodecWarnDialog : public QDialog
{
	Q_OBJECT
	public:
		CodecWarnDialog(const char * encoding);
		~CodecWarnDialog();
	private slots:
		void slot_okay();
	private:
		QLabel * textLabel;
		QPushButton * okayButton;
		QCheckBox * dontwarnCB;	
};
