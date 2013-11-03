#ifndef SGFPREVIEW_H
#define SGFPREVIEW_H

#include <QWidget>

class SGFParser;
class GameData;

namespace Ui {
class SGFPreview;
}

class SGFPreview : public QWidget
{
    Q_OBJECT
    
public:
    explicit SGFPreview(QWidget *parent = 0);
    ~SGFPreview();
    void clearData();

public slots:
    void setPath(QString path);

signals:
    void isValidSGF(bool);
    // This class should decide if the path contains a valid SGF file.
    // As far as I can tell, this is not checked at the moment.
    // It is implicitly assumed that the file is valid SGF.
    
private:
    Ui::SGFPreview *ui;
    GameData * GameLoaded;
    SGFParser * MW_SGFparser;
    QString SGFloaded;
};

#endif // SGFPREVIEW_H
