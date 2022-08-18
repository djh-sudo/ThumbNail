#pragma once
/*
* thumbnail is project to extract information from database(db)
* db file is location at
* C:\Users\%username%\AppData\Local\Microsoft\Windows\Explorer
* code is stolen from the following repository
* Also See
* https://github.com/thumbcacheviewer/thumbcacheviewer
*/

#include <io.h>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <Windows.h>
#include <wchar.h>


// Some magic identifiers
#define FILE_TYPE_BMP	"BM"
#define FILE_TYPE_JPEG	"\xFF\xD8\xFF\xE0"
#define FILE_TYPE_PNG	"\x89\x50\x4E\x47\x0D\x0A\x1A\x0A"
#define DB_MAGIC_ID     "CMMM"

/*
* database version in different Windows platform
* ignore version
* #define WINDOWS_8		    0x1A
* #define WINDOWS_8v2		0x1C
* #define WINDOWS_8v3		0x1E
* #define WINDOWS_8_1		0x1F
* #define WINDOWS_VISTA	    0x14
*/
#define WINDOWS_7		0x15
#define WINDOWS_10		0x20

typedef struct DATABASE_HEADER {
	/* Off[DEC] Description */
	/* 00 */    char magicIdentifier[4];
	/* 04 */    unsigned int version;
	/* 08 */    unsigned int type;
	/* totally 12 bytes */
} DB_HEADER, *PDB_HEADER;


// Found in WINDOWS_VISTA/7/8 databases.
typedef struct DATABASE_HEADER_ENTRY_INFO_7 {
	/* Off[DEC] Description */
	/* 00 */    unsigned int firstCacheEntry;
	/* 04 */    unsigned int availableCacheEntry;
	/* 08 */    unsigned int numberCacheEntries;
	/* totally 12 bytes */
} DB_HEADER_ENTRY_7, * PDB_HEADER_ENTRY_7;


// Found in WINDOWS_8v3/8_1/10 databases.
typedef struct DATABASE_HEADER_ENTRY_INFO_10 {
	unsigned int unknown;
	unsigned int firstCacheEntry;
	unsigned int availableCacheEntry;
} DB_HEADER_ENTRY_10, * PDB_HEADER_ENTRY_10;


// Window 7 Thumb cache entry.
typedef struct DATABASE_CACHE_ENTRY_7{
	char magicIdentifier[4];
	unsigned int dwCacheEntry;
	long long entryHash;
	unsigned int dwFilename;
	unsigned int dwPadding;
	unsigned int dwData;
	unsigned int unknown;
	long long dataChecksum;
	long long headerChecksum;
} DB_CACHE_ENTRY_7, * PDB_CACHE_ENTRY_7;


// Window 8+ Thumb cache entry.
typedef struct DATABASE_CACHE_ENTRY_8P{
	char magicIdentifier[4];
	unsigned int dwCacheEntry;
	long long entryHash;
	unsigned int dwFilename;
	unsigned int dwPadding;
	unsigned int dwData;
	unsigned int width;
	unsigned int height;
	unsigned int unknown;
	long long dataChecksum;
	long long headerChecksum;
} DB_CACHE_ENTRY_8P, * PDB_CACHE_ENTRY_8P;

/*
* Wrapper the Output Information
*/
class Wrapper {

public:
	void SetCacheSize(unsigned int cacheSize) {
		m_dwCache = cacheSize;
	}
	
	void SetDataSize(unsigned int dataSize) {
		m_dwData = dataSize;
	}

	void SetEntryHash(const char * hash) {
		memcpy(m_entryHash, hash, 19);
	}

	Wrapper() {
		memset(m_entryHash, 0, 19);
		memset(m_dataChecksum, 0 ,19);
		memset(m_headerChecksum, 0, 19);
		m_dwCache = 0;
		m_dwData = 0;
	}

	Wrapper(unsigned int cacheSize,
		    unsigned int dataSize,
		    const char * hash,
		    const char * checksum,
		    const char * headersum,
		    const wchar_t * fileName,
		    unsigned int dwFileName){
		
		m_dwCache = cacheSize;
		m_dwData = dataSize;
		memcpy(m_entryHash, hash, 19);
		memcpy(m_dataChecksum, checksum, 19);
		memcpy(m_headerChecksum, headersum, 19);
		m_fileName = std::wstring(fileName, dwFileName);
	}

	virtual ~Wrapper() = default;

private:
	unsigned int m_dwCache;
	unsigned int m_dwData;
	char m_entryHash[19];
	char m_dataChecksum[19];
	char m_headerChecksum[19];
	std::wstring m_fileName;
};


class ThumbNail {

public:

	bool Init(std::wstring path) {
		bool status = true;
		m_dbPath = path;
		return status;
	}

	void CreateOutputDir() {
		if (GetFileAttributesW(m_outputPath) == INVALID_FILE_ATTRIBUTES) {
			CreateDirectoryW(m_outputPath, NULL);
		}
		SetCurrentDirectory(m_outputPath);
		GetCurrentDirectory(MAX_PATH, m_outputPath);
		return;
	}

	bool Parser() {
		bool status = false;
		HANDLE hFile = INVALID_HANDLE_VALUE;

		do {
			hFile = CreateFileW(m_dbPath.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hFile == INVALID_HANDLE_VALUE) {
				break;
			}
			DWORD dwRead = 0;
			DB_HEADER dbHeader = { 0 };
			unsigned int fileOffset = 0;

			status = ReadFile(hFile, &dbHeader, sizeof(DB_HEADER), &dwRead, NULL);
			if (status == false) {
				break;
			}
			if (memcmp(dbHeader.magicIdentifier, DB_MAGIC_ID, 4) != 0 || dwRead != sizeof(DB_HEADER)) {
				status = false;
				break;
			}
			if (dbHeader.version != WINDOWS_7 && dbHeader.version != WINDOWS_10) {
				status = false;
				break;
			}
			// Create output directory
			CreateOutputDir();

			unsigned int firstCacheEntry = 0;
			unsigned int availableCcheEntry = 0;
			unsigned int dwCacheEntries = 0;

			if (dbHeader.version == WINDOWS_10) {
				DB_HEADER_ENTRY_10 dbEntry = { 0 };
				status = ReadFile(hFile, &dbEntry, sizeof(DB_HEADER_ENTRY_10), &dwRead, NULL);
				if (status == false || dwRead != sizeof(DB_HEADER_ENTRY_10)) {
					break;
				}
				firstCacheEntry = dbEntry.firstCacheEntry;
				availableCcheEntry = dbEntry.availableCacheEntry;
				status = SubParserWin10(hFile);
			}
			else {
				DB_HEADER_ENTRY_7 dbEntry = { 0 };
				status = ReadFile(hFile, &dbEntry, sizeof(DB_HEADER_ENTRY_7), &dwRead, NULL);
				if (status == false || dwRead != sizeof(DB_HEADER_ENTRY_7)) {
					break;
				}
				firstCacheEntry = dbEntry.firstCacheEntry;
				availableCcheEntry = dbEntry.availableCacheEntry;
				dwCacheEntries = dbEntry.numberCacheEntries;
			}

		} while (false);
		if (hFile != INVALID_HANDLE_VALUE) {
			CloseHandle(hFile);
		}
		return status;
	}

	bool SubParserWin10(HANDLE hFile) {
		bool status = false;
		PDB_CACHE_ENTRY_8P dbCacheEntry = (PDB_CACHE_ENTRY_8P)new char[sizeof(DB_CACHE_ENTRY_8P)];
		if (dbCacheEntry == NULL) {
			return false;
		}

		unsigned int position = 24;
		unsigned int dwcacheEntry = 0;
		DWORD dwRead = 0;

		char entryHash[19] = { 0 };
		char dataChecksum[19] = { 0 };
		char headerChecksum[19] = { 0 };

		wchar_t fileName[MAX_PATH] = { 0 };
		unsigned int dwFileName = 0;

		unsigned int dwData = 0;
		unsigned int dwPadding = 0;
		unsigned int dwDataSize = 0;

		int count = 0;

		for (unsigned int i = 0; ; ++i) {
			memset(entryHash, 19, 0);
			memset(dataChecksum, 19, 0);
			memset(headerChecksum, 19, 0);

			dwFileName = 0;
			dwData = 0;
			dwDataSize = 0;

			position = SetFilePointer(hFile, position, NULL, FILE_BEGIN);

			if (position == INVALID_SET_FILE_POINTER) {
				break;
			}

			status = ReadFile(hFile, dbCacheEntry, sizeof(DB_CACHE_ENTRY_8P), &dwRead, NULL);
			if (status == false || dwRead != sizeof(DB_CACHE_ENTRY_8P)) {
				break;
			}

			status = (memcmp(dbCacheEntry->magicIdentifier, DB_MAGIC_ID, 4) == 0);
			if(status == false){
				break;
			}
			
			dwPadding = dbCacheEntry->dwPadding;
			if (SetFilePointer(hFile, dwPadding, 0, FILE_CURRENT) == INVALID_SET_FILE_POINTER) {
				break;
			}

			dwcacheEntry = dbCacheEntry->dwCacheEntry;
			position += dwcacheEntry;

			dwDataSize = dbCacheEntry->dwData;
			
			if (dwDataSize > 8) {
				// handle file name
				count++;
				dwFileName = dbCacheEntry->dwFilename;
				dwFileName = min(dwFileName, MAX_PATH);
				memset(fileName, 0, MAX_PATH);

				status = ReadFile(hFile, fileName, dwFileName, &dwRead, NULL);
				if (status == false || dwRead != dwFileName) {
					continue;
				}
				// save file content
				char* buffer = new char[dwDataSize];
				if (buffer == NULL) {
					break;
				}
				memset(buffer, 0, dwDataSize);
				status = ReadFile(hFile, buffer, dwDataSize, &dwRead, NULL);
				if (status == false || dwRead <= 0) {
					delete[] buffer;
					continue;
				}
				if (memcmp(buffer, FILE_TYPE_BMP, 2) == 0) {
					wmemcpy_s(fileName + (dwFileName >> 1), 4, L".bmp", 4);
				}
				else if (memcmp(buffer, FILE_TYPE_JPEG, 4) == 0) {
					wmemcpy_s(fileName + (dwFileName >> 1), 4, L".jpg", 4);
				}
				else if (memcmp(buffer, FILE_TYPE_PNG, 8) == 0) {
					wmemcpy_s(fileName + (dwFileName >> 1), 4, L".png", 4);
				}
				else {
					// other type ?
					delete[] buffer;
					continue;
				}
				TrimInvalidChar(fileName);
				HANDLE hFileSave = CreateFileW(fileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
				if (hFileSave == INVALID_HANDLE_VALUE) {
					delete[] buffer;
					continue;
				}
				WriteFile(hFileSave, buffer, dwDataSize, &dwRead, NULL);
				CloseHandle(hFileSave);

				sprintf_s(dataChecksum, 19, "0x%016llx", dbCacheEntry->dataChecksum);
				sprintf_s(headerChecksum, 19, "0x%016llx", dbCacheEntry->headerChecksum);
				sprintf_s(entryHash, 19, "0x%016llx", dbCacheEntry->entryHash);

				Wrapper obj(dwcacheEntry, dwDataSize, entryHash, dataChecksum, dataChecksum, fileName, dwFileName);
				m_data.push_back(obj);
				delete[] buffer;
			}
		}
		if (dbCacheEntry != NULL) {
			delete[] dbCacheEntry;
			dbCacheEntry = NULL;
			std::cout << count << std::endl;
		}
	}


	ThumbNail() {
		memset(m_outputPath, 0, MAX_PATH * sizeof(wchar_t));
	}

	virtual ~ThumbNail() = default;

private:
	std::wstring m_dbPath;
	wchar_t m_outputPath[MAX_PATH];
	std::vector<Wrapper>m_data;

	void TrimInvalidChar(wchar_t* fileName) {
		wchar_t* filename_ptr = fileName;
		while (filename_ptr != NULL && *filename_ptr != NULL)
		{
			if (*filename_ptr == L'\\' ||
				*filename_ptr == L'/' ||
				*filename_ptr == L':' ||
				*filename_ptr == L'*' ||
				*filename_ptr == L'?' ||
				*filename_ptr == L'\"' ||
				*filename_ptr == L'<' ||
				*filename_ptr == L'>' ||
				*filename_ptr == L'|'){
				*filename_ptr = L'_';
			}
			++filename_ptr;
		}
	}
};

