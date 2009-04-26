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
