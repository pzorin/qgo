#ifndef NEWGAMEDIALOG_H
#define NEWGAMEDIALOG_H

#include <QDialog>
class MainWindow;

namespace Ui {
class NewGameDialog;
}

class NewGameDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NewGameDialog(MainWindow *parent);
    ~NewGameDialog();

public slots:
    void slot_newFile_HandicapChange(int);
    virtual void accept();

private:
    Ui::NewGameDialog *ui;
    MainWindow *mw;
    int handicap;
};

#endif // NEWGAMEDIALOG_H
