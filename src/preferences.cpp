#include "preferences.h"
#include "ui_preferences.h"
#include "defines.h"
#include <QMessageBox>
#include <QFileDialog>

Preferences::Preferences(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Preferences)
{
    ui->setupUi(this);
    loadSettings();

    QPushButton * button;
    button =  ui->buttonBox->button(QDialogButtonBox::Reset);
    if (button)
        connect(button, &QPushButton::clicked, this, &Preferences::loadSettings);
    button = ui->buttonBox->button(QDialogButtonBox::Ok);
    if (button)
        connect(button, &QPushButton::clicked, this, &Preferences::saveSettings);

    connect( ui->gobanPathButton,  &QPushButton::clicked, this,  &Preferences::slot_getGobanPath);
    connect( ui->tablePathButton,  &QPushButton::clicked, this,  &Preferences::slot_getTablePath);

    engineTableModel = new EngineTableModel(this);
    ui->engineTableView->setModel(engineTableModel);
    ui->engineTableView->setColumnWidth ( EngineTableModel::ENGINE_DEFAULT, 60 );
    ui->engineTableView->setColumnWidth ( EngineTableModel::ENGINE_PATH, 300 );
    connect(ui->engineAddButton, &QPushButton::pressed, this, &Preferences::addEngine);
    connect(ui->engineRemoveButton, &QPushButton::pressed, this, &Preferences::removeEngine);
}

Preferences::~Preferences()
{
    delete ui;
}


EngineTableModel::EngineTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{
    loadEngines();
}

EngineTableModel::~EngineTableModel()
{
    saveEngines();
}

int EngineTableModel::rowCount(const QModelIndex &) const
{
    return engines.size();
}

int EngineTableModel::columnCount(const QModelIndex &) const
{
    return ENGINE_NUMITEMS;
}

QModelIndex EngineTableModel::index ( int row, int column, const QModelIndex &) const
{
    return createIndex(row,column);
}

QVariant EngineTableModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    switch (index.column())
    {
    case ENGINE_DEFAULT:
        if (role == Qt::CheckStateRole)
            return (index.row() == selected_engine ? Qt::Checked : Qt::Unchecked);
        break;
    case ENGINE_PATH:
        if ((role == Qt::DisplayRole) || (role == Qt::EditRole))
            return engines[index.row()].path;
        break;
    case ENGINE_ARGUMENTS:
        if ((role == Qt::DisplayRole) || (role == Qt::EditRole))
            return engines[index.row()].arguments;
        break;
    }
    return QVariant();
}

QVariant EngineTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal && role == Qt::DisplayRole)
        switch(section)
        {
        case ENGINE_DEFAULT:
            return QVariant(tr("Default"));
        case ENGINE_PATH:
            return QVariant(tr("Path"));
        case ENGINE_ARGUMENTS:
            return QVariant(tr("Arguments"));
        }
    return QVariant();
}

Qt::ItemFlags EngineTableModel::flags(const QModelIndex & index) const
{
    if (index.column() == ENGINE_DEFAULT)
        return Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable;
    else
        return Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsSelectable;
}

bool EngineTableModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
    int row = index.row();
    int column = index.column();
    if ((row < 0) || (row >= engines.size()) || ((role != Qt::EditRole) && (role != Qt::CheckStateRole)))
        return false;

    switch (column)
    {
    case ENGINE_DEFAULT:
        if (role == Qt::CheckStateRole)
        {
            selected_engine = (value.toBool() ? row : 0);
            saveEngines();
            emit dataChanged(QAbstractTableModel::index(0,(int)ENGINE_DEFAULT),
                             QAbstractTableModel::index(engines.size()-1,(int)ENGINE_DEFAULT));
            return true;
        }
        else
            return false;
    case ENGINE_PATH:
        engines[row].path = value.toString();
        break;
    case ENGINE_ARGUMENTS:
        engines[row].arguments = value.toString();
        break;
    default:
        return false;
    }
    saveEngines();
    emit dataChanged(index,index);
    return true;
}

bool EngineTableModel::removeRows(int row, int count, const QModelIndex &)
{
    beginRemoveRows(QModelIndex(),row,row+count-1);
    for(;count>0;count--)
        engines.removeAt(row);
    endRemoveRows();
    if (selected_engine >= engines.size())
        selected_engine = 0;
    saveEngines();
    return true;
}


void EngineTableModel::addEngine(Engine e)
{
    int row = engines.size();
    beginInsertRows(QModelIndex(),row,row);
    engines.append(e);
    saveEngines();
    endInsertRows();
}

void EngineTableModel::loadEngines(void)
{
    QSettings settings;
    int size = settings.beginReadArray("ENGINES");
    for (int i = 0; i < size; ++i)
    {
        settings.setArrayIndex(i);
        engines.append(Engine(settings.value("path").toString(),settings.value("args").toString()));
    }
    settings.endArray();
    if (engines.isEmpty())
        engines.append(Engine(QString(DEFAULT_ENGINE_PATH)+QString(DEFAULT_ENGINE),QString(DEFAULT_ENGINE_OPTIONS)));
    selected_engine = settings.value("DEFAULT_ENGINE").toInt();
    if ((selected_engine < 0) || (selected_engine >= engines.size()))
        selected_engine = 0;
}

void EngineTableModel::saveEngines(void)
{
    QSettings settings;
    settings.beginWriteArray("ENGINES");
    for (int i = 0; i < engines.size(); ++i)
    {
        settings.setArrayIndex(i);
        settings.setValue("path", engines.at(i).path);
        settings.setValue("args", engines.at(i).arguments);
    }
    settings.endArray();
    settings.setValue("DEFAULT_ENGINE", selected_engine);
}

void Preferences::addEngine()
{
    QString fileName(QFileDialog::getOpenFileName(this, tr("Go engine"),
                                                  QString(DEFAULT_ENGINE_PATH)+QString(DEFAULT_ENGINE)));
    if (fileName.isEmpty())
        return;
    if (fileName.contains(QRegExp(QString(DEFAULT_ENGINE)+QString("$"))))
        engineTableModel->addEngine(Engine(fileName,DEFAULT_ENGINE_OPTIONS));
    else
        engineTableModel->addEngine(Engine(fileName,""));
}

void Preferences::removeEngine()
{
    QModelIndex index = ui->engineTableView->selectionModel()->currentIndex();
    if (index.isValid())
        engineTableModel->removeRow(index.row());
}

void Preferences::saveSettings()
{
    settings.setValue("MAIN_WINDOW_SIZE_X", width());
    settings.setValue("MAIN_WINDOW_SIZE_Y", height());
    settings.setValue("MAIN_WINDOW_POS_X", pos().x());
    settings.setValue("MAIN_WINDOW_POS_Y", pos().y());

    settings.setValue("LAST_PATH", currentWorkingDir);
//	settings.setValue("COMPUTER_PATH", ui->LineEdit_computer->text());
    settings.setValue("SKIN", ui->LineEdit_goban->text());
    settings.setValue("SKIN_TABLE", ui->LineEdit_table->text());

    settings.setValue("TIMER_INTERVAL", ui->timerComboBox->currentIndex());

    int i = 0;
    if ( ui->radioButtonStones_2D->isChecked())
        i=1;
    else if ( ui->radioButtonStones_3D->isChecked())
        i=2;
    settings.setValue("STONES_LOOK", i);

    if ( ui->terrCrossRB->isChecked())
        i=0;
    else
        i=1;
    settings.setValue("TERR_STONE_MARK", i);

    settings.setValue("SOUND_MYGAMES", ui->checkBox_myGamesSound->isChecked());
    settings.setValue("SOUND_OTHERGAMES", ui->checkBox_otherGamesSound->isChecked());

    if ( ui->komarkerCB->isChecked())
        i=1;
    else
        i=0;
    settings.setValue("KOMARKER", i);
    if ( ui->numberCurrentMoveCB->isChecked())
        i=1;
    else
        i=0;
    settings.setValue("NUMBER_CURRENT_MOVE", i);
#ifdef UNNECESSARY
    if ( ui->warnOnCloseEditedCB->isChecked())
        i=1;
    else
        i=0;

    settings.setValue("WARNONCLOSEEDITED", i);
    if ( ui->warnOnCloseEngineCB->isChecked())
        i=1;
    else
        i=0;
    settings.setValue("WARNONCLOSEENGINE", i);
#endif //UNNECESSARY
    if ( ui->simplePlayerNamesCB->isChecked())
        i=1;
    else
        i=0;
    settings.setValue("SIMPLEPLAYERNAMES", i);
    if ( ui->observeOutsideCB->isChecked())
        i=1;
    else
        i=0;
    settings.setValue("OBSERVEOUTSIDE", i);
    if ( ui->alternateListColorsCB->isChecked())
        i=1;
    else
        i=0;
    settings.setValue("ALTERNATELISTCOLORS", i);

    //settings.setValue("ACCOUNT", ui->connectionWidget->ui->serverComboBox->currentIndex());

    //server games default values
    settings.setValue("DEFAULT_KOMI",ui->komiSpinDefault->value() );
    settings.setValue("DEFAULT_SIZE",ui->boardSizeSpin->value() );
    settings.setValue("DEFAULT_TIME",ui->timeSpin->value() );
    settings.setValue("DEFAULT_BY",ui->BYSpin->value() );

    settings.setValue("NMATCH_BLACK", ui->checkBox_Nmatch_Black->isChecked());
    settings.setValue("NMATCH_WHITE", ui->checkBox_Nmatch_White->isChecked());
    settings.setValue("NMATCH_NIGIRI",ui->checkBox_Nmatch_Nigiri->isChecked());
    settings.setValue("NMATCH_MAIN_TIME", ui->timeSpin_Nmatch->value());
    settings.setValue("NMATCH_BYO_TIME", ui->BYSpin_Nmatch->value());
    settings.setValue("NMATCH_HANDICAP", ui->HandicapSpin_Nmatch->value());

    settings.setValue("AUTOSAVE", ui->CheckBox_autoSave->isChecked());
    settings.setValue("AUTOSAVE_PLAYED", ui->CheckBox_autoSave_Played->isChecked());


    //server byo yomi warning
    settings.setValue("BYO_SOUND_WARNING", ui->ByoSoundWarning->isChecked());
    settings.setValue("BYO_SEC_WARNING",ui->ByoSecWarning->value());

    //qDebug("password: %s\n", hostlist.at(0)->password().toLatin1().constData());
}

/*
 * loads the parameters from the QSettings into the 2 lats tabs
 */
void Preferences::loadSettings()
{
    QSettings settings;
    QVariant var;

    if((var = settings.value("LAST_PATH")) == QVariant())
        currentWorkingDir = QString();
    else
        currentWorkingDir = var.toString();

    ui->radioButtonStones_real->setChecked(true);
    ui->radioButtonStones_2D->setChecked((settings.value("STONES_LOOK")==1));
    ui->radioButtonStones_3D->setChecked((settings.value("STONES_LOOK")==2));

    ui->checkBox_myGamesSound->setChecked(settings.value("SOUND_MYGAMES", QVariant(true)).toBool());
    ui->checkBox_otherGamesSound->setChecked(settings.value("SOUND_OTHERGAMES", QVariant(true)).toBool());

    ui->LineEdit_goban->setText(settings.value("SKIN").toString());
    ui->LineEdit_table->setText(settings.value("SKIN_TABLE").toString());

    ui->timerComboBox->setCurrentIndex(settings.value("TIMER_INTERVAL").toInt());
    ui->komarkerCB->setChecked(settings.value("KOMARKER", QVariant(true)).toBool());
    ui->numberCurrentMoveCB->setChecked(settings.value("NUMBER_CURRENT_MOVE", QVariant(false)).toBool());
    if(settings.value("TERR_STONE_MARK").toBool())
        ui->terrStoneRB->setChecked(true);
    else
        ui->terrCrossRB->setChecked(true);

    ui->simplePlayerNamesCB->setChecked((settings.value("SIMPLEPLAYERNAMES") == 1));
    ui->observeOutsideCB->setChecked((settings.value("OBSERVEOUTSIDE") == 1));
    ui->alternateListColorsCB->setChecked(settings.value("ALTERNATELISTCOLORS",QVariant(false)).toBool());

    ui->komiSpinDefault->setValue(settings.value("DEFAULT_KOMI", QVariant(5.5)).toInt());
    ui->boardSizeSpin->setValue(settings.value("DEFAULT_SIZE", QVariant(19)).toInt());
    ui->timeSpin->setValue(settings.value("DEFAULT_TIME").toInt());
    ui->BYSpin->setValue(settings.value("DEFAULT_BY").toInt());

    ui->checkBox_Nmatch_Black->setChecked(settings.value("NMATCH_BLACK", QVariant(true)).toBool());
    ui->checkBox_Nmatch_White->setChecked(settings.value("NMATCH_WHITE", QVariant(true)).toBool());
    ui->checkBox_Nmatch_Nigiri->setChecked(settings.value("NMATCH_NIGIRI", QVariant(true)).toBool());
    ui->HandicapSpin_Nmatch->setValue(settings.value("NMATCH_HANDICAP", QVariant(8)).toInt());
    ui->timeSpin_Nmatch->setValue(settings.value("NMATCH_MAIN_TIME", QVariant(99)).toInt());
    ui->BYSpin_Nmatch->setValue(settings.value("NMATCH_BYO_TIME", QVariant(60)).toInt());

    ui->CheckBox_autoSave->setChecked(settings.value("AUTOSAVE").toBool());
    ui->CheckBox_autoSave_Played->setChecked(settings.value("AUTOSAVE_PLAYED").toBool());

    //server byo yomi warning
    ui->ByoSoundWarning->setChecked(settings.value("BYO_SOUND_WARNING").toBool());
    ui->ByoSecWarning->setValue(settings.value("BYO_SEC_WARNING").toInt());

    preferences.fill();
}

void Preferences::slot_getGobanPath()
{
    QString fileName(QFileDialog::getOpenFileName(this, tr("Goban picture"), "",
        tr("All Files (*)"), new QString(""), QFileDialog::DontUseNativeDialog));
    if (fileName.isEmpty())
        return;

    ui->LineEdit_goban->setText(fileName);
}

void Preferences::slot_getTablePath()
{
    QString fileName(QFileDialog::getOpenFileName(this, tr("Table picture"), "",
        tr("All Files (*)"), new QString(""), QFileDialog::DontUseNativeDialog));
    if (fileName.isEmpty())
        return;

    ui->LineEdit_table->setText(fileName);
}
