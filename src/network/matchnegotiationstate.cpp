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


#include <QtCore>
#include "matchnegotiationstate.h"
#include "messages.h"
#include "gamedata.h"

bool MatchNegotiationState::newMatchAllowed(void)
{
	if(state == MSNONE)
		return true;
	return false;
}

//adjourned rematch
void MatchNegotiationState::setupRematchAdjourned(unsigned short id, QString opponent_name)
{
	game_number = id;
	opponent = opponent_name;
	state = MSREMATCHADJOURNED;
}

bool MatchNegotiationState::canEnterRematchAdjourned(void)
{
	if(state == MSREMATCHADJOURNED)
		return true;
	return false;
}

bool MatchNegotiationState::inGame(void)
{
	if(state == MSSTARTMATCH || state == MSONGOINGMATCH)
		return true;
	return false;
}

bool MatchNegotiationState::isOurGame(unsigned short id)
{
	if(id == 0)
		return false;
	if(id == game_number)
		return true;
	return false;
}

bool MatchNegotiationState::sentMatchInvite(void)
{
	if(state == MSINVITE)
		return true;
	return false;
}

bool MatchNegotiationState::sentMatchOfferPending(void)
{
	if(state == MSMATCHOFFERPENDING)
		return true;
	return false;
}

bool MatchNegotiationState::justCreatedRoom(void)
{
	if(state == MSCREATEDROOM)
		return true;
	return false;
}

bool MatchNegotiationState::waitingForRoomNumber(void)
{
	if(state == MSACCEPTINVITE)
		return true;
	return false;
}

bool MatchNegotiationState::waitingForMatchOffer(void)
{
	if(state == MSJOINEDROOM)
		return true;
	return false;
}

bool MatchNegotiationState::sentMatchOffer(void)
{
	if(state== MSMATCHOFFER || state == MSMATCHMODIFY)		//doublecheck MODIFY fixme
		return true;
	return false;
}

bool MatchNegotiationState::startMatchAcceptable(void)
{
	if(state == MSMATCHACCEPT || state == MSMATCHFINISHED || state == MSSENTADJOURNRESUME)
		return true;
	return false;
}	

bool MatchNegotiationState::isOngoingMatch(unsigned short id)
{
	if(!isOurGame(id))
		return false;
	if(state != MSONGOINGMATCH)
		return false;
	return true;
}

bool MatchNegotiationState::isOngoingMatch(void)
{
	if(state == MSONGOINGMATCH)
		return true;
	return false;
}

bool MatchNegotiationState::sentCreateRoom(void)
{
	if(state == MSSENTCREATEROOM)
		return true;
	return false;
}

bool MatchNegotiationState::twoPasses(void)
{
	if(state == MSTWOPASS)
		return true;
	return false;
}

bool MatchNegotiationState::sentDoneCounting(void)
{
	if(state == MSSENTDONECOUNTING)
		return true;
	return false;
}

bool MatchNegotiationState::receivedDoneCounting(void)
{
	if(state == MSRECEIVEDDONECOUNTING)
		return true;
	return false;
}

bool MatchNegotiationState::counting(void)
{
	if(state == MSCOUNTING)
		return true;
	return false;
}

bool MatchNegotiationState::doneCounting(void)
{
	if(state == MSDONECOUNTING)
		return true;
	return false;
}

bool MatchNegotiationState::sentRematch(void)
{
	if(state == MSSENTREMATCH)
		return true;
	return false;
}

bool MatchNegotiationState::sentRematchAccept(void)
{
	if(state == MSSENTREMATCHACCEPT)
		return true;
	return false;
}

bool MatchNegotiationState::sentAdjournResume(void)
{
	if(state == MSSENTADJOURNRESUME)
		return true;
	return false;
}

bool MatchNegotiationState::opponentDisconnected(void)
{
	if(state == MSOPPONENTDISCONNECT)
		return true;
	return false;
}

bool MatchNegotiationState::opponentRejoining(void)
{
	if(state == MSOPPONENTREJOINING)
		return true;
	return false;
}

void MatchNegotiationState::reset(void)
{
	player = 0;
	game_number = 0;
	opponent = QString();
	state = MSNONE;
}

void MatchNegotiationState::sendMatchInvite(PlayerListing * p)
{
	player = p;
	state = MSINVITE;
}

void MatchNegotiationState::sendMatchAccept(PlayerListing * p)
{
	player = p;
	state = MSACCEPTINVITE;
}

void MatchNegotiationState::sendMatchOfferPending(void)
{
	state = MSMATCHOFFERPENDING;
}

void MatchNegotiationState::sendCreateRoom(void)
{
	state = MSSENTCREATEROOM;
}

void MatchNegotiationState::createdRoom(unsigned short id)
{
	game_number = id;
	state = MSCREATEDROOM;
}

void MatchNegotiationState::sendJoinRoom(unsigned short id)
{
	game_number = id;
	state = MSJOINEDROOM;
}

void MatchNegotiationState::offerMatchTerms(MatchRequest * mr)
{
	state = MSMATCHOFFER;
	match_request = new MatchRequest(*mr);
}

void MatchNegotiationState::modifyMatchTerms(MatchRequest * mr)
{
	state = MSMATCHMODIFY;
	match_request = new MatchRequest(*mr);
}

void MatchNegotiationState::acceptMatchTerms(MatchRequest * mr)
{
	state = MSMATCHACCEPT;
	match_request = new MatchRequest(*mr);
}

void MatchNegotiationState::startMatch(void)
{
	state = MSONGOINGMATCH;
}

void MatchNegotiationState::incrementPasses(void)
{
	if(state == MSONGOINGMATCH)
		state = MSONEPASS;
	else if(state == MSONEPASS)
		state = MSTWOPASS;
	else if(state == MSTWOPASS)
		state = MSTHREEPASS;
	//error
}

void MatchNegotiationState::enterScoreMode(void)
{
	state = MSCOUNTING;
}

void MatchNegotiationState::sendDoneCounting(void)
{
	state = MSSENTDONECOUNTING;
}

void MatchNegotiationState::receiveDoneCounting(void)
{
	state = MSRECEIVEDDONECOUNTING;
}

void MatchNegotiationState::setDoneCounting(void)
{
	state = MSDONECOUNTING;
}

void MatchNegotiationState::sendMatchModeRequest(void)
{
	state = MSMATCHMODEREQUEST;
}

void MatchNegotiationState::sendRematch(PlayerListing * p)
{
	state = MSSENTREMATCH;
	player = p;
}

void MatchNegotiationState::sendRematchAccept(void)
{
	state = MSSENTREMATCHACCEPT;
}

void MatchNegotiationState::opponentDisconnect(void)
{
	state = MSOPPONENTDISCONNECT;
}

void MatchNegotiationState::sendAdjournResume(void)
{
	state = MSSENTADJOURNRESUME;
}

void MatchNegotiationState::opponentRejoins(void)
{
	state = MSOPPONENTREJOINING;
}

void MatchNegotiationState::opponentReconnect(void)
{
	state = MSONGOINGMATCH;		//FIXME
}

void MatchNegotiationState::swapColors(void)
{
	if(match_request->color_request == MatchRequest::WHITE)
		match_request->color_request = MatchRequest::BLACK;
	else if(match_request->color_request == MatchRequest::BLACK)
		match_request->color_request = MatchRequest::WHITE;
}

bool MatchNegotiationState::verifyPlayer(PlayerListing * p)
{
	if(p == player)
		return true;
	return false;
}

bool MatchNegotiationState::verifyMatchRequest(MatchRequest & mr)
{
	if(!match_request)
		return false;		//shouldn't be called here
	if(match_request->stones_periods != mr.stones_periods ||
		match_request->periodtime != mr.periodtime ||
		match_request->maintime != mr.maintime ||
		match_request->komi != mr.komi ||
		match_request->timeSystem != mr.timeSystem ||
		match_request->handicap != mr.handicap ||
		match_request->color_request != mr.color_request)
		return false;
	return true;
}

bool MatchNegotiationState::verifyGameData(GameData & g)
{
	if(!match_request)
		return false;		//shouldn't be called here
	if(match_request->stones_periods != g.stones_periods ||
		match_request->periodtime != g.periodtime ||
		match_request->maintime != g.maintime ||
		match_request->komi != g.komi ||
		match_request->timeSystem != g.timeSystem ||
		(match_request->handicap != g.handicap && (match_request->handicap != 1 || (g.handicap != 0 && g.handicap != 1))))/* ||
		match_request->color_request != g.color_request)*/	//FIXME
		return false;
	return true;
}

bool MatchNegotiationState::verifyCountDoneMessage(unsigned short v)
{
	if(v == counting_verification)
		return true;
	return false;
}
