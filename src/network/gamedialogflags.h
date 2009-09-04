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


#ifndef GAMEDIALOGFLAGS_H
#define GAMEDIALOGFLAGS_H

#define GDF_CANADIAN			0x00000001
#define GDF_BYOYOMI			0x00000002
#define GDF_TVASIA			0x00000004
#define GDF_STONES25_FIXED		0x00000008
#define GDF_FREE_RATED_CHOICE		0x00000010
#define GDF_RATED_SIZE_FIXED		0x00000020
#define GDF_RATED_HANDICAP_FIXED	0x00000040
#define GDF_RATED_NO_HANDICAP		0x00000080
#define GDF_NIGIRI_EVEN			0x00000100	// is this even needed?
#define GDF_ONLY_DISPUTE_TIME		0x00000200
#define GDF_BY_CAN_MAIN_MIN		0x00000400
#define GDF_HANDICAP1			0x00000800
#define GDF_CANADIAN300			0x00001000	//canadian time must be over 300
#define GDF_KOMI_FIXED6			0x00002000	//must be 6.5 komi or .5, etc

#endif //GAMEDIALOGFLAGS_H
