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


class PlayerListing;
class MatchRequest;
class GameData;

class MatchNegotiationState
{
public:
	MatchNegotiationState(void) : state(MSNONE), player(0), opponent(QString()), game_number(0), negotiation_broken(false), match_request(0) {};
	
	bool broken(void) { return negotiation_broken; };
	PlayerListing * getPlayerListing(void) { return player; };
	unsigned short getGameId(void) { return game_number; };
	void setGameId(unsigned short id) { game_number = id; };
	void setCountingVerification(unsigned short v) { counting_verification = v; };
	unsigned short getCountingVerification(void) { return counting_verification; };
	QString & getOpponent(void) { return opponent; };
	void setPlayer(PlayerListing * p) { player = p; };

	bool newMatchAllowed(void);
	bool canEnterRematchAdjourned(void);
	bool inGame(void);
	bool isOurGame(unsigned short id);
	bool sentMatchInvite(void);
	bool sentMatchOfferPending(void);
	bool justCreatedRoom(void);
	bool waitingForRoomNumber(void);
	bool sentJoinRoom(void) { return waitingForMatchOffer(); };
	bool waitingForMatchOffer(void);
	bool startMatchAcceptable(void);
	bool isOngoingMatch(unsigned short id);
	bool isOngoingMatch(void);
	bool sentCreateRoom(void);
	bool sentMatchOffer(void);
	bool twoPasses(void);
	bool counting(void);
	bool sentDoneCounting(void);
	bool receivedDoneCounting(void);
	bool doneCounting(void);
	bool sentRematch(void);
	bool sentRematchAccept(void);
	bool sentAdjournResume(void);
	bool opponentDisconnected(void);
	bool opponentRejoining(void);
	void setupRematchAdjourned(unsigned short id, QString opponent_name);
	void reset(void);
	void sendMatchInvite(PlayerListing * p);
	void sendMatchAccept(PlayerListing *);
	void sendMatchOfferPending(void);
	void sendCreateRoom(void);
	void sendJoinRoom(unsigned short id);
	void createdRoom(unsigned short id);
	void offerMatchTerms(MatchRequest * mr);
	void modifyMatchTerms(MatchRequest * mr);
	void acceptMatchTerms(MatchRequest * mr);
	void startMatch(void);
	void incrementPasses(void);
	void enterScoreMode(void);
	void sendDoneCounting(void);
	void receiveDoneCounting(void);
	void setDoneCounting(void);
	void sendMatchModeRequest(void);
	void sendRematch(PlayerListing * p);
	void sendRematchAccept(void);
	void opponentDisconnect(void);
	void sendAdjournResume(void);
	void opponentRejoins(void);
	void opponentReconnect(void);
	void swapColors(void);
	bool verifyPlayer(PlayerListing * p);
	bool verifyMatchRequest(MatchRequest & mr);
	bool verifyGameData(GameData & g);
	bool verifyCountDoneMessage(unsigned short v);

private:
	enum MSSTATE { MSNONE, MSINVITE, MSACCEPTINVITE, MSSENTCREATEROOM, MSCREATEDROOM, MSJOINEDROOM,
		      MSMATCHOFFERPENDING,
		      MSMATCHOFFER, MSMATCHMODIFY, MSMATCHACCEPT,
		      MSSTARTMATCH, MSONGOINGMATCH,
		      MSONEPASS, MSTWOPASS, MSTHREEPASS,
		      MSCOUNTING, MSSENTDONECOUNTING, MSRECEIVEDDONECOUNTING, MSDONECOUNTING,
		      MSMATCHMODEREQUEST,
		      MSMATCHFINISHED,
		      MSSENTREMATCH, MSSENTREMATCHACCEPT, MSREMATCH,
		      MSOPPONENTDISCONNECT, MSOPPONENTREJOINING, MSSENTADJOURNRESUME, MSREMATCHADJOURNED } state;
	MSSTATE getState(void) { return state; };
	
	
	PlayerListing * player;
	QString opponent;
	unsigned short game_number;
	
	bool negotiation_broken;
	MatchRequest * match_request;
	unsigned short counting_verification;
};
