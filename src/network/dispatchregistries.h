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


#include "registry.h"

class PlayerListing;

class BoardDispatchRegistry : public Registry <unsigned int, BoardDispatch>
{
	public:
		BoardDispatchRegistry(NetworkConnection * c) : _c(c) {};
		std::map<unsigned int, BoardDispatch *> * getRegistryStorage(void);
	private:
		virtual BoardDispatch * getNewEntry(unsigned int game_id);
		//virtual void initEntry(BoardDispatch *);
		virtual void onErase(BoardDispatch *);
		NetworkConnection * _c;
};

class GameDialogRegistry : public Registry <const PlayerListing *, GameDialog>
{
	public:
		GameDialogRegistry(NetworkConnection * c) : _c(c) {};
	private:
		virtual GameDialog * getNewEntry(const PlayerListing * opponent);
		//virtual void initEntry(GameDialog *);
		virtual void onErase(GameDialog *);
		NetworkConnection * _c;
};

class TalkRegistry : public Registry <PlayerListing *, Talk>
{
	public:
		TalkRegistry(NetworkConnection * c) : _c(c) {};
	private:
		virtual Talk * getNewEntry(PlayerListing * opponent);
		//virtual void initEntry(Talk *);
		virtual void onErase(Talk *);
		NetworkConnection * _c;
};

