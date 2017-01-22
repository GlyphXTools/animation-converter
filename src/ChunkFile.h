#ifndef CHUNKFILE_H
#define CHUNKFILE_H

#include <string>

#include "types.h"

typedef long ChunkType;

#pragma pack(1)
struct CHUNKHDR
{
	uint32_t type;
	uint32_t size;
};

struct MINICHUNKHDR
{
	uint8_t type;
	uint8_t size;
};
#pragma pack()

class ChunkReader
{
	static const int MAX_CHUNK_DEPTH = 256;

	HANDLE m_file;
	long   m_position;
	long   m_size;
	long   m_offsets[ MAX_CHUNK_DEPTH ];
	long   m_miniSize;
	long   m_miniOffset;
	int    m_curDepth;

public:
	ChunkType   next();
	ChunkType   nextMini();
	void        skip();
	long        size();
	long        read(void* buffer, long size, bool check = true);

	float			readFloat();
	unsigned char   readByte();
	unsigned short	readShort();
	unsigned long	readInteger();
	std::string		readString();

	ChunkReader(HANDLE file);
};

class ChunkWriter
{
	static const int MAX_CHUNK_DEPTH = 256;
	
	template <typename HDRTYPE>
	struct ChunkInfo
	{
		HDRTYPE       hdr;
		unsigned long offset;
	};

	HANDLE                  m_file;
	ChunkInfo<CHUNKHDR>     m_chunks[ MAX_CHUNK_DEPTH ];
	ChunkInfo<MINICHUNKHDR> m_miniChunk;
	int                     m_curDepth;

	void endMiniChunk();

public:
	void beginChunk(ChunkType type);
	void beginMiniChunk(ChunkType type);
	void endChunk();
	void write(const void* buffer, long size);

	void writeFloat(float value);
	void writeByte(unsigned char value);
	void writeShort(unsigned short value);
	void writeInteger(unsigned long value);
	void writeString(const std::string& str);

	ChunkWriter(HANDLE file);
};
#endif