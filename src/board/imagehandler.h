/*
* imagehandler.h
*/

#ifndef IMAGEDATA_H
#define IMAGEDATA_H


#include "../defines.h"
#include <cmath>

#include <QtGui>

typedef struct WhiteDesc_struct  {
	double cosTheta, sinTheta;
	double stripeWidth, xAdd;
	double stripeMul, zMul;
} WhiteDesc;


class ImageHandler
{
public:
	ImageHandler();
	~ImageHandler();
	
	void init(int size, bool isDisplay = FALSE);
	void rescale(int size);
	static QPixmap* getBoardPixmap(QString );
	static QPixmap* getTablePixmap(QString );
	QList<QPixmap> *getStonePixmaps() const { return stonePixmaps; }
	QList<QPixmap> *getGhostPixmaps() const { return ghostPixmaps; }
	static QList<QPixmap> * getAlternateGhostPixmaps() { return altGhostPixmaps; }
	void ghostImage(QImage *img);

	void icopy(int *im, QImage &qim, int w, int h);
	void decideAppearance(WhiteDesc *desc, int size);
	double getStripe(WhiteDesc &white, double bright, double z, int x, int y);
	void paintBlackStone (QImage &bi, int d, int stone_render);
	void paintShadowStone (QImage &si, int d);
	void paintWhiteStone (QImage &wi, int d, int stone_render);

protected:
	void scaleBoardPixmap(QPixmap *pix, int size);
	
private:
	bool isDisplayBoard;
	QList<QPixmap> *stonePixmaps, *ghostPixmaps;
	static QList<QPixmap> *altGhostPixmaps;
	static QPixmap *tablePixmap;
	static QPixmap *woodPixmap1;
	static int classCounter;
};

#endif
