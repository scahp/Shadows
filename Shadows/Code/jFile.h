#ifndef __FILE_H__
#define __FILE_H__

struct FileType
{
	enum Enum
	{
		BINARY = 0,
		TEXT,
	};
};

struct ReadWriteType
{
	enum Enum
	{
		READ = 0,			// READ ONLY, MUST EXIST FILE
		WRITE,				// WRITE ONLY, IF THE FILE EXIST, DELETE ALL CONTENTS, IF THE FILE NOT EXIST CREATE FILE.
		APPEND,				// WRITE ONLY END OF THE FILE, IF THE FILE NOT EXIST CREATE FILE.
		READ_UPDATE,		// READ / WRITE, IF THE FILE NOT EXIST, NULL WILL RETURN
		WRITE_UPDATE,		// READ / WRITE, IF THE FILE EXIST, DELETE ALL CONTENTS, IF THE FILE NOT EXIST, FILE WILL CREATE.
		APPEND_UPDATE,		// READ / WRITE, WRITE END OF THE FILE, READ CAN ANY WHERE. IF THE FILE NOT EXIST, FILE WILL CREATE 
	};
};

class jFile
{
public:
	typedef char ELEMENT_TYPE;
	typedef std::vector<ELEMENT_TYPE> FILE_BUFFER;

	static uint64 GetFileTimeStamp(const char* filename);

	jFile() : m_fp(nullptr) {}
	~jFile();

	bool OpenFile(const char* szFileName, FileType::Enum fileType = FileType::BINARY, ReadWriteType::Enum readWriteType = ReadWriteType::READ);
	size_t ReadFileToBuffer(bool appendToEndofBuffer = true, unsigned int index = 0, unsigned int count = 0);
	void CloseFile();
	const char* GetBuffer(unsigned int index = 0, unsigned int count = 0) const;
	bool GetBuffer(FILE_BUFFER& outBuffer, const char* startToken, const char* endToken);
	bool IsBufferEmpty() const { return m_buffer.empty(); }

private:
	FILE* m_fp;
	std::vector<ELEMENT_TYPE> m_buffer;
};

#endif // __FILE_H__

