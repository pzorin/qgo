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


#include <QtNetwork>
#include "igsconnection.h"

class WingConnection : public IGSConnection
{
	public:
        WingConnection(const ConnectionCredentials credentials);
		virtual QString getPlaceString(void);
		virtual void sendPlayersRequest(void);
		virtual void sendMatchRequest(class MatchRequest * mr);
		virtual void sendMove(unsigned int game_id, class MoveRecord * move);
		virtual void onReady(void);
		virtual void requestGameInfo(unsigned int game_id);
	private:
		virtual void handle_info(QString);
		virtual void handle_kibitz(QString);
};
