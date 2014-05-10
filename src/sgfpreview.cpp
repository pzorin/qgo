#include "sgfpreview.h"
#include "ui_sgfpreview.h"
#include "displayboard.h"
#include "sgfparser.h"
#include "gamedata.h"
#include "defines.h"
#include "mainwindow.h"
#include "boardwindow.h"

SGFPreview::SGFPreview(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SGFPreview)
{
    MW_SGFparser = new SGFParser(NULL);
    GameLoaded = NULL;
    ui->setupUi(this);
}

SGFPreview::~SGFPreview()
{
    if (GameLoaded != NULL)
    {
        delete GameLoaded;
        GameLoaded = NULL;
    }
    delete ui;
    delete MW_SGFparser;
}

void SGFPreview::clearData()
{
    if (GameLoaded != NULL)
    {
        delete GameLoaded;
        GameLoaded = NULL;
    }
    ui->displayBoard->clearData();

    ui->File_WhitePlayer->setText("");
    ui->File_BlackPlayer->setText("");
    ui->File_Date->setText("");
    ui->File_Handicap->setText("");
    ui->File_Result->setText("");
    ui->File_Komi->setText("");
    ui->File_Size->setText("");
}

void SGFPreview::setPath(QString path)
{
    //if (QFileInfo("path").isFile() == false)
    //    return;
    clearData();

    //fileLoaded = path.toLatin1().constData();
    SGFloaded = MW_SGFparser->loadFile(path);
    if (SGFloaded == NULL)
    {
        emit isValidSGF(false);
        return;
    }

    GameLoaded = MW_SGFparser->initGame(SGFloaded, path);
    if (GameLoaded == NULL)
    {
        emit isValidSGF(false);
        return;
    }

    GameLoaded->gameMode = modeEdit;

        QString komi, hcp, sz;
        komi.setNum(GameLoaded->komi);
        hcp.setNum(GameLoaded->handicap);
        sz.setNum(GameLoaded->board_size);

        ui->File_WhitePlayer->setText(GameLoaded->white_name);
        ui->File_BlackPlayer->setText(GameLoaded->black_name);
        ui->File_Date->setText(GameLoaded->date);
        ui->File_Handicap->setText(hcp);
        ui->File_Result->setText(GameLoaded->result);
        ui->File_Komi->setText(komi);
        ui->File_Size->setText(sz);

        DisplayBoard* board = ui->displayBoard;
        board->clearData();
        if (board->getSize() != GameLoaded->board_size)
            board->init(GameLoaded->board_size);

        board->displayHandicap(GameLoaded->handicap);

        QString s = SGFloaded.trimmed();
        int end_main = s.indexOf(")(");
        if (end_main == -1)
            end_main = s.size();
        int a_offset = QChar::fromLatin1('a').unicode() - 1 ;
        int cursor = 0;
        int x,y;
        int nb_displayed = 20;
        QString coords;

        cursor = s.indexOf(";B[");

        while ((cursor >0) && (cursor < end_main) && (nb_displayed--) )
        {
            x = s.at(cursor+3).unicode() - a_offset;
            y = s.at(cursor+4).unicode() - a_offset;
            board->updateStone(stoneBlack,x,y);
            cursor = s.indexOf(";B[",cursor +1);

        }

        cursor = s.indexOf(";W[");
        nb_displayed = 20;

        while ( (cursor >0) &&  (cursor < end_main) && (nb_displayed--) )
        {
            x = s.at(cursor+3).unicode() - a_offset;
            y = s.at(cursor+4).unicode() - a_offset;
            board->updateStone(stoneWhite,x,y);
            cursor = s.indexOf(";W[",cursor +1);

        }
    emit isValidSGF(true);
}

// Maybe handle computer games separately.
// Here is the old code that was supposed to do that.
/*void MainWindow::slot_loadComputerFile(const QModelIndex & topLeft, const QModelIndex & bottomRight )
{
    QVariant v = topLeft.data(QDirModel::FilePathRole);

    if (!selectFile(topLeft))
    {
        ui.button_loadComputerGame->setDisabled(true);
    } else {
        GameLoaded->gameMode = modeComputer;
        ui.button_loadComputerGame->setEnabled(true);

        QString komi, hcp, sz;
        komi.setNum(GameLoaded->komi);
        hcp.setNum(GameLoaded->handicap);
        sz.setNum(GameLoaded->board_size);

        ui.File_WhitePlayer->setText(GameLoaded->white_name);
        ui.File_BlackPlayer->setText(GameLoaded->black_name);
        ui.File_Date->setText(GameLoaded->date);
        ui.File_Handicap->setText(hcp);
        ui.File_Result->setText(GameLoaded->result);
        ui.File_Komi->setText(komi);
        ui.File_Size->setText(sz);

        displayGame(ui.displayBoard);
    }
}*/
