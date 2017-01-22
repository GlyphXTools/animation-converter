#include <cassert>
#include "ChunkFile.h"
#include "exceptions.h"
using namespace std;

ChunkType ChunkReader::nextMini()
{
	assert(m_curDepth >= 0);
	assert(m_size >= 0);

	if (m_miniSize >= 0)
	{
		// We're in a mini chunk, so skip it
		skip();
	}

	if (SetFilePointer(m_file, 0, NULL, FILE_CURRENT) == m_offsets[m_curDepth])
	{
		// We're at the end of the current chunk, move up one
		m_curDepth--;
		m_size     = -1;
		m_position =  0;
		return -1;
	}

	MINICHUNKHDR hdr;
	DWORD read;
	if (!ReadFile(m_file, &hdr, sizeof(MINICHUNKHDR), &read, NULL) || read != sizeof(MINICHUNKHDR))
	{
		throw ReadException();
	}

	m_miniSize   = letohl(hdr.size);
	m_miniOffset = SetFilePointer(m_file, 0, NULL, FILE_CURRENT) + m_miniSize;
	m_position   = 0;

	return letohl(hdr.type);
}

ChunkType ChunkReader::next()
{
	assert(m_curDepth >= 0);

	if (m_size >= 0)
	{
		// We're in a data chunk, so skip it
		skip();
	}
	
	if (SetFilePointer(m_file, 0, NULL, FILE_CURRENT) == m_offsets[m_curDepth])
	{
		// We're at the end of the current chunk, move up one
		m_curDepth--;
		m_size     = -1;
		m_position =  0;
		return -1;
	}

	CHUNKHDR hdr;
	DWORD read;
	if (!ReadFile(m_file, &hdr, sizeof(CHUNKHDR), &read, NULL) || read != sizeof(CHUNKHDR))
	{
		throw ReadException();
	}

	unsigned long size = letohl(hdr.size);
	m_offsets[ ++m_curDepth ] = SetFilePointer(m_file, 0, NULL, FILE_CURRENT) + (size & 0x7FFFFFFF);
	m_size     = (~size & 0x80000000) ? size : -1;
	m_miniSize = -1;
	m_position = 0;

	return letohl(hdr.type);
}

void ChunkReader::skip()
{
	if (m_miniSize >= 0)
	{
		SetFilePointer(m_file, m_miniOffset, NULL, FILE_BEGIN);
	}
	else
	{
		SetFilePointer(m_file, m_offsets[m_curDepth--], NULL, FILE_BEGIN);
	}
}

long ChunkReader::size()
{
	return (m_miniSize >= 0) ? m_miniSize : m_size;
}

string ChunkReader::readString()
{
	string str;
	char* data = new char[ size() ];
	try
	{
		read(data, size());
	}
	catch (...)
	{
		delete[] data;
		throw;
	}
	str = data;
	delete[] data;
	return str;
}

float ChunkReader::readFloat()
{
	float value;
	if (size() != sizeof(value)) throw ReadException();
	read(&value, sizeof(value));
	return value;
}

unsigned char ChunkReader::readByte()
{
	uint8_t value;
	if (size() != sizeof(value)) throw ReadException();
	read(&value, sizeof(value));
	return value;
}

unsigned short ChunkReader::readShort()
{
	uint16_t value;
	if (size() != sizeof(value)) throw ReadException();
	read(&value, sizeof(value));
	return letohs(value);
}

unsigned long ChunkReader::readInteger()
{
	uint32_t value;
	if (size() != sizeof(value)) throw ReadException();
	read(&value, sizeof(value));
	return letohl(value);
}

long ChunkReader::read(void* buffer, long size, bool check)
{
	if (m_size >= 0)
	{
		DWORD read = 0;
		BOOL  ok   = ReadFile(m_file, buffer, min(m_position + size, this->size()) - m_position, &read, NULL);
		m_position += read;
		if (check && (!ok || read != size))
		{
			throw ReadException();
		}
		return size;
	}
	throw ReadException();
}

ChunkReader::ChunkReader(HANDLE file)
{
	m_file       = file;
	m_offsets[0] = GetFileSize(m_file, NULL);
	m_curDepth   = 0;
	m_size       = -1;
	m_miniSize   = -1;
}
