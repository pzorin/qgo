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


#include "audio/audio.h"
#include <QDir>
#include <defines.h>
#include <QtMultimedia/QMediaPlayer>

Sound::Sound(const QString &filename, QObject *parent)
    : QObject(parent)
{
    player = new QMediaPlayer;
    if (QDir().exists(SOUND_PATH_PREFIX + filename))
        player->setMedia(QMediaContent(QUrl::fromLocalFile(QDir().absoluteFilePath(SOUND_PATH_PREFIX + filename))));
}

Sound::~Sound()
{
    delete player;
}

void Sound::play(void)
{
    player->play();
}
