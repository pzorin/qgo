#include "newgamedialog.h"
#include "ui_newgamedialog.h"

#include <QSettings>
#include "mainwindow.h"
#include "boardwindow.h"

NewGameDialog::NewGameDialog(MainWindow *parent) :
    QDialog(parent),
    ui(new Ui::NewGameDialog)
{
    mw = parent;
    ui->setupUi(this);

    QSettings settings;
    QVariant var;
    //SGF edition tab default values
    if((var = settings.value("EDIT_SIZE")) == QVariant())
        var = 19;
    ui->newFile_Size->setValue(var.toInt());
    handicap = settings.value("EDIT_HANDICAP").toInt();
    ui->newFile_Handicap->setValue(handicap);
    if((var = settings.value("EDIT_KOMI")) == QVariant())
        var = 5.5;
    ui->newFile_Komi->setValue(var.toDouble());

    ui->computerPlaysWhite->setChecked(settings.value("COMPUTER_PLAYS_WHITE").toBool());
    ui->computerPlaysBlack->setChecked(settings.value("COMPUTER_PLAYS_BLACK").toBool());

    connect(ui->newFile_Handicap, SIGNAL(valueChanged(int)), this, SLOT(slot_newFile_HandicapChange(int)));
}

NewGameDialog::~NewGameDialog()
{
    QSettings settings;
    //SGF edition tab default values
    settings.setValue("EDIT_SIZE",ui->newFile_Size->value());
    settings.setValue("EDIT_HANDICAP",ui->newFile_Handicap->value());
    settings.setValue("EDIT_KOMI",ui->newFile_Komi->value());

    settings.setValue("COMPUTER_PLAYS_WHITE", ui->computerPlaysWhite->isChecked());
    settings.setValue("COMPUTER_PLAYS_BLACK", ui->computerPlaysBlack->isChecked());
    delete ui;
}

void NewGameDialog::slot_newFile_HandicapChange(int a)
{
    if(a == 1)
    {
        handicap = (handicap < 1 ? 2 : 0);
        ui->newFile_Handicap->setValue(handicap);
    } else
        handicap = a;
}

void NewGameDialog::accept()
{
    GameData *gd = new GameData();
    gd->gameMode = modeLocal;

    gd->board_size = ui->newFile_Size->value();
    gd->handicap = ui->newFile_Handicap->value();
    gd->black_name = ui->newFile_BlackPlayer->text();
    gd->white_name = ui->newFile_WhitePlayer->text();
    gd->komi = ui->newFile_Komi->value();

    BoardWindow * bw = new BoardWindow(gd, !(ui->computerPlaysBlack->isChecked()),
                                       !(ui->computerPlaysWhite->isChecked()));
    if(bw)
        mw->addBoardWindow(bw);

    QDialog::accept();
}
