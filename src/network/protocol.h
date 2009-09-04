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


#ifndef PROTOCOL_H
#define PROTOCOL_H

class ProtocolPacket
{
	public:
		ProtocolPacket() {};

};

/* This is a huge pain in the ass and probably just
 * a waste of time.  Its like, if I don't want to allocate
 * the records and copy the data to them because that's a
 * waste, then its like I have to override an accessor
 * function.  So then I guess its like a RecordShell that
 * returns a record offset by the index * sizeof the record.
 * and then maybe a type case */

class PacketRecord
{
	public:
		virtual unsigned int size() = 0;	
};
 
class RecordShell
{
	public:
		RecordShell(class PacketRecord & p, void * d) : records(p), data(d) {};
		class PacketRecord & operator[](int i) { return *(PacketRecord *)((char *)data + (i * records.size())); };	
	private:
		class PacketRecord & records;
		void * data;
};

class ZeroPaddedString
{
	public:
		ZeroPaddedString(int s, char * d) : size(s), data(d) {};
		unsigned char operator[] (int index) { };
		void operator=(char *) {};
	private:
		unsigned int size;
		char * data;
};

//test
class OROPlayerListPacket : public ProtocolPacket
{
	public:
		OROPlayerListPacket(char * p) : data(p), playerRecord(pr, p) { };
		void * data;
		unsigned short unknown(void) { return *(unsigned short * )data; };
		unsigned char playerRecords(void) { return (unsigned char )((char *)data)[2]; };
		/* Need to pass it data pointer, but then offset by RecordShell ?
 		 * or RecordShell does all offsets, data */
		class PlayerRecord : public PacketRecord
		{
			public:
				PlayerRecord(void) : name(10) {};
				unsigned int size(void) { return 0x28; };
				ZeroPaddedString name;
				unsigned short id(void) { return (unsigned short)*((char *)this + 0x0a); };
				unsigned char specialIdByte(void) { return (unsigned char)*((char *)this + 0xc); };
				unsigned char rankByte(void) { return (unsigned char)*((char *)this + 0xd); };
				unsigned char unknown2;
				unsigned char countryCode;
				unsigned char inviteByte;
				unsigned char unknown3;
				unsigned short rankScore;
				unsigned short wins;
				unsigned short losses;
				unsigned short unknown4;
				unsigned short unknown5;
		} pr;
		const class RecordShell playerRecord;	
};

#endif //PROTOCOL_H
