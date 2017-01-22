#include <cassert>
#include "ChunkFile.h"
#include "exceptions.h"
using namespace std;

void ChunkWriter::beginChunk(ChunkType type)
{
	m_curDepth++;
	m_chunks[m_curDepth].offset   = SetFilePointer(m_file, 0, NULL, FILE_CURRENT);
	m_chunks[m_curDepth].hdr.type = type;
	m_chunks[m_curDepth].hdr.size = 0;
	if (m_curDepth > 0)
	{
		// Set 'container' bit in parent chunk
		m_chunks[m_curDepth-1].hdr.size |= 0x80000000;
	}

	// Write dummy header
	CHUNKHDR hdr = {0,0};
	DWORD written;
	if (!WriteFile(m_file, &hdr, sizeof(CHUNKHDR), &written, NULL) || written != sizeof(CHUNKHDR))
	{
		throw WriteException();
	}
}

void ChunkWriter::beginMiniChunk(ChunkType type)
{
	assert(m_curDepth >= 0);
	assert(type <= 0xFF);
	if (m_miniChunk.offset != -1)
	{
		endMiniChunk();
	}

	m_miniChunk.offset   = SetFilePointer(m_file, 0, NULL, FILE_CURRENT);
	m_miniChunk.hdr.type = (uint8_t)type;
	m_miniChunk.hdr.size = 0;
	
	// Write dummy header
	MINICHUNKHDR hdr = {0, 0};
	DWORD written;
	if (!WriteFile(m_file, &hdr, sizeof(MINICHUNKHDR), &written, NULL) || written != sizeof(MINICHUNKHDR))
	{
		throw WriteException();
	}
}

void ChunkWriter::endMiniChunk()
{
	assert(m_curDepth >= 0);
	assert(m_miniChunk.offset != -1);

	// Ending mini-chunk
	long pos  = SetFilePointer(m_file, 0, NULL, FILE_CURRENT);
	long size = pos - (m_miniChunk.offset + sizeof(MINICHUNKHDR));
	assert(size <= 0xFF);

	m_miniChunk.hdr.size = (uint8_t)size;
	SetFilePointer(m_file, m_miniChunk.offset, NULL, FILE_BEGIN);
	DWORD written;
	if (!WriteFile(m_file, &m_miniChunk.hdr, sizeof(MINICHUNKHDR), &written, NULL) || written != sizeof(MINICHUNKHDR))
	{
		throw WriteException();
	}
	SetFilePointer(m_file, pos, NULL, FILE_BEGIN);
	m_miniChunk.offset = -1;
}

void ChunkWriter::endChunk()
{
	assert(m_curDepth >= 0);

	if (m_miniChunk.offset != -1)
	{
		endMiniChunk();
	}

	// Ending normal chunk
	long pos  = SetFilePointer(m_file, 0, NULL, FILE_CURRENT);
	long size = pos - (m_chunks[m_curDepth].offset + sizeof(CHUNKHDR));

	m_chunks[m_curDepth].hdr.size = (m_chunks[m_curDepth].hdr.size & 0x80000000) | (size & ~0x80000000);
	SetFilePointer(m_file, m_chunks[m_curDepth].offset, NULL, FILE_BEGIN);
	DWORD written;
	if (!WriteFile(m_file, &m_chunks[m_curDepth].hdr, sizeof(CHUNKHDR), &written, NULL) || written != sizeof(CHUNKHDR))
	{
		throw WriteException();
	}
	SetFilePointer(m_file, pos, NULL, FILE_BEGIN);

	m_curDepth--;
}

void ChunkWriter::write(const void* buffer, long size)
{
	assert(m_curDepth >= 0);
	DWORD written;
	if (!WriteFile(m_file, buffer, size, &written, NULL) || written != size)
	{
		throw WriteException();
	}
}

void ChunkWriter::writeString(const std::string& str)
{
	write(str.c_str(), (int)str.length() + 1);
}

void ChunkWriter::writeFloat(float value)
{
	write(&value, sizeof(value));
}

void ChunkWriter::writeByte(unsigned char value)
{
	uint8_t leValue = value;
	write(&leValue, sizeof(leValue));
}

void ChunkWriter::writeShort(unsigned short value)
{
	uint16_t leValue = htoles(value);
	write(&leValue, sizeof(leValue));
}

void ChunkWriter::writeInteger(unsigned long value)
{
	uint32_t leValue = htolel(value);
	write(&leValue, sizeof(leValue));
}

ChunkWriter::ChunkWriter(HANDLE file)
{
	m_file     = file;
	m_curDepth = -1;
	m_miniChunk.offset = -1;
}