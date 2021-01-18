#include <pch.h>
#include "jFile.h"

uint64 jFile::GetFileTimeStamp(const char* filename)
{
	JASSERT(filename);

	WIN32_FILE_ATTRIBUTE_DATA attributes;
	GetFileAttributesExA(filename, GetFileExInfoStandard, &attributes);
	return attributes.ftLastWriteTime.dwLowDateTime | static_cast<uint64>(attributes.ftLastWriteTime.dwHighDateTime) << 32;
}

jFile::~jFile()
{
	CloseFile();
}

bool jFile::OpenFile(const char* szFileName, FileType::Enum fileType /*= FileType::BINARY*/, ReadWriteType::Enum readWriteType /*= ReadWriteType::READ*/)
{
	std::string option;
	switch (readWriteType)
	{
	case ReadWriteType::READ:
		option += "r";
		break;
	case ReadWriteType::WRITE:
		option += "w";
		break;
	case ReadWriteType::APPEND:
		option += "a";
		break;
	case ReadWriteType::READ_UPDATE:
		option += "r+";
		break;
	case ReadWriteType::WRITE_UPDATE:
		option += "w+";
		break;
	case ReadWriteType::APPEND_UPDATE:
		option += "a+";
		break;
	}

	switch(fileType)
	{
	case FileType::BINARY:
		option += "b";
		break;
	case FileType::TEXT:
		option += "t";
		break;
	}

	CloseFile();

	fopen_s(&m_fp, szFileName, option.c_str());

	return (nullptr != m_fp);
}

size_t jFile::ReadFileToBuffer(bool appendToEndofBuffer/* = true*/, unsigned int index/* = 0*/, unsigned int count/* = 0*/)
{
	size_t readSize = 0;
	if (m_fp)
	{
		unsigned int writeStartIndex = 0;
		if (0 < count)
		{
			if (appendToEndofBuffer)
			{
				writeStartIndex = static_cast<unsigned int>(m_buffer.size() - 1);
				m_buffer.resize(m_buffer.size() + count + 1);
			}
			else
			{
				m_buffer.resize(count + 1);
			}
		}
		else
		{
			fseek(m_fp, 0, SEEK_END);   // non-portable
			count = ftell(m_fp);

			m_buffer.clear();
			m_buffer.resize(count + 1);
		}
		fseek(m_fp, index, SEEK_SET);

		readSize += fread(&m_buffer[writeStartIndex], sizeof(ELEMENT_TYPE), count, m_fp);
		m_buffer.resize(readSize + 1);
		m_buffer[readSize] = 0;
	}

	return readSize;
}

void jFile::CloseFile()
{
	if (m_fp)
	{
		fclose(m_fp);
		m_fp = NULL;
	}
}

const char* jFile::GetBuffer(unsigned int index /*= 0*/, unsigned int count /*= 0*/) const
{
	if (m_buffer.empty())
	{
		return nullptr;
	}

	return &m_buffer[0];
}

bool jFile::GetBuffer(FILE_BUFFER& outBuffer, const char* startToken, const char* endToken)
{
	if (m_buffer.empty())
	{
		return false;
	}

	char* startPos = strstr(&m_buffer[0], startToken);

	if (startPos)
	{
		size_t startTokenLength = strlen(startToken);
		startPos += startTokenLength;

		char* endPos = strstr(&m_buffer[0], endToken);
		size_t count = 0;
		if (endPos)
		{
			count = endPos - startPos;
		}
		else
		{
			count = &m_buffer[m_buffer.size() - 1] - startPos;
		}

		if (0 < count)
		{
			outBuffer.resize(count + 1);
			memcpy(&outBuffer[0], startPos, count * sizeof(ELEMENT_TYPE));
			outBuffer[count] = 0;

			return true;
		}
	}

	return false;
}
