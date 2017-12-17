#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <QDialog>
#include <QSettings>
#include <QAbstractTableModel>

namespace Ui {
class Preferences;
}

class Engine
{
public:
    Engine(QString p, QString a) :
        path(p), arguments(a) {}
    QString path;
    QString arguments;
};

class EngineTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum EngineTableColumn { ENGINE_DEFAULT, ENGINE_PATH, ENGINE_ARGUMENTS, ENGINE_NUMITEMS };

    EngineTableModel(QObject * parent = 0);
    ~EngineTableModel();
    int rowCount(const QModelIndex & parent = QModelIndex()) const;
    int columnCount(const QModelIndex & parent = QModelIndex()) const;
    QModelIndex index ( int row, int column, const QModelIndex & parent = QModelIndex() ) const;
    QVariant data(const QModelIndex & index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    Qt::ItemFlags flags(const QModelIndex & index) const;
    bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole);
    bool removeRows(int row, int count, const QModelIndex &parent);

    void addEngine(Engine e);
    void loadEngines(void);
    void saveEngines(void);
public:
    QList <Engine> engines;
    int selected_engine;
};

class Preferences : public QDialog
{
    Q_OBJECT

public:
    explicit Preferences(QWidget *parent = 0);
    ~Preferences();

    void addEngine();
    void removeEngine();
    void saveSettings();
    void loadSettings();
    void slot_getGobanPath();
    void slot_getTablePath();

private:
    Ui::Preferences *ui;
    QSettings settings;
    EngineTableModel * engineTableModel;
    QString currentWorkingDir;
};

#endif // PREFERENCES_H
