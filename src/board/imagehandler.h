/***************************************************************************
 *   Copyright (C) 2009 by The qGo Project                                 *
 *                                                                         *
 *   This file is part of qGo.   					   *
 *                                                                         *
 *   qGo is free software: you can redistribute it and/or modify           *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 *   or write to the Free Software Foundation, Inc.,                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


#ifndef IMAGEDATA_H
#define IMAGEDATA_H

#include <QtCore>
#include <QPixmap>

/* DONTREDRAWSTONES lets Qt do the scaling rather than redrawing them
 * to scale.  I'm not convinced it improves the speed all that much
 * but I do know that it removes the border on the white stones in 2d
 * mode so until that's fixed or until another solution is found, I'll
 * comment this out */

//#define DONTREDRAWSTONES
struct WhiteDesc {
	double cosTheta, sinTheta;
	double stripeWidth, xAdd;
	double stripeMul, zMul;
};

class ImageHandler
{
public:
	ImageHandler();
	~ImageHandler();
	
    void setDisplay(bool isDisplay = false);
    void setSquareSize(int size);
	static QPixmap *getBoardPixmap(QString ) ;
	static QPixmap *getTablePixmap(QString ) ;
#ifdef DONTREDRAWSTONES
	QList<QPixmap> *getStonePixmaps() const { return stonePixmapsScaled; }
	QList<QPixmap> *getSmallStonePixmaps() const { return smallStonePixmapsScaled; }
	QList<QPixmap> *getGhostPixmaps() const { return ghostPixmapsScaled; }
#else
	QList<QPixmap> *getStonePixmaps() const { return stonePixmaps; }
	QList<QPixmap> *getSmallStonePixmaps() const { return smallStonePixmaps; }
	QList<QPixmap> *getGhostPixmaps() const { return ghostPixmaps; }
#endif //DONTREDRAWSTONES
	static QList<QPixmap> * getAlternateGhostPixmaps() { return altGhostPixmaps; }
	void ghostImage(QImage *img);

	void icopy(int *im, QImage &qim, int w, int h);
	void decideAppearance(WhiteDesc *desc, int size);
	double getStripe(WhiteDesc &white, double bright, double z, int x, int y);
	

protected:
	void scaleBoardPixmap(QPixmap *pix, int size);
	
private:

#ifdef DONTREDRAWSTONES
	void generateStonePixmaps(int size);
	QList<QPixmap> *stonePixmapsScaled, *ghostPixmapsScaled, *smallStonePixmapsScaled;
#endif //DONTREDRAWSTONES
	void paintBlackStone (QImage &bi, int d, int stone_render);
	void paintShadowStone (QImage &si, int d);
	void paintWhiteStone (QImage &wi, int d, int stone_render);
	int * painting_buffer;	

	bool isDisplayBoard;
	QList<QPixmap> *stonePixmaps, *ghostPixmaps, *smallStonePixmaps;
	static QList<QPixmap> *altGhostPixmaps;
	static QPixmap *tablePixmap;
	static QPixmap *woodPixmap1;
	static int classCounter;
};

#endif
