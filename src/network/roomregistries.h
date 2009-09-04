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


#include "../registry.h"

class PlayerListModel;
class GamesListModel;

class PlayerListingRegistry : public Registry <QString, PlayerListing>
{
	public:
		PlayerListingRegistry(PlayerListModel * model) : playerListModel(model) {};
		PlayerListing * getPlayerFromNotNickName(const QString & notnickname);
	private:
		virtual void initEntry(PlayerListing * l);
		virtual void onErase(PlayerListing * l);		
		PlayerListModel * playerListModel;
};

class PlayerListingIDRegistry : public Registry <unsigned int, PlayerListing>
{
	public:
		PlayerListingIDRegistry(PlayerListModel * model) : playerListModel(model) {};
		PlayerListing * getPlayerFromName(const QString & name);
	private:
		virtual void initEntry(PlayerListing * l);
		virtual void onErase(PlayerListing * l);		
		PlayerListModel * playerListModel;
};

class GameListingRegistry : public Registry <unsigned int, GameListing>
{
	public:
		GameListingRegistry(GamesListModel * model) : gamesListModel(model) {};
	private:
		virtual void initEntry(GameListing * l);
		virtual void onErase(GameListing * l);		
		GamesListModel * gamesListModel;
};
