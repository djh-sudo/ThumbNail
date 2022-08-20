#pragma once
/*
* thumbnail is project to extract information from database(db)
* db file is located at
* C:\Users\%username%\AppData\Local\Microsoft\Windows\Explorer
* code is stolen from the following repository
* Also See
* https://github.com/thumbcacheviewer/thumbcacheviewer
* Warning !!! Code just test on Windows 10 / 11
*/

#include <io.h>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>
#include <Windows.h>
#include <wchar.h>
#include <ShlObj_core.h>


// Some magic identifiers
#define FILE_TYPE_BMP	"BM"
#define FILE_TYPE_JPEG	"\xFF\xD8\xFF\xE0"
#define FILE_TYPE_PNG	"\x89\x50\x4E\x47\x0D\x0A\x1A\x0A"
#define DB_MAGIC_ID     "CMMM"

/*
* database version in different Windows platform
* ignore version
* #define WINDOWS_8v2		0x1C
* #define WINDOWS_8v3		0x1E
* #define WINDOWS_8_1		0x1F
*/

#define WINDOWS_10		0x20
#define WINDOWS_7		0x15
#define WINDOWS_8		0x1A
#define WINDOWS_VISTA	0x14

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
	
	unsigned int GetOffset() const {
		return m_offset;
	}

	unsigned int GetDataSize() const {
		return m_dwData;
	}

	std::wstring GetFileName() const {
		return m_fileName;
	}

	explicit Wrapper() {
		m_dwData = 0;
		m_offset = -1;
		m_fileName = L"";
	}
	
	explicit Wrapper(unsigned int offset,
		    unsigned int dataSize,
		    const wchar_t * fileName,
		    unsigned int dwFileName){

		m_offset = offset;
		m_dwData = dataSize;
		m_fileName = std::wstring(fileName, dwFileName);
	}

	virtual ~Wrapper() = default;

private:
	unsigned int m_offset;
	unsigned int m_dwData;
	std::wstring m_fileName;
};


class ThumbNail {

public:

	bool Init(CONST LPCTSTR lpThumbFilePath) {
		m_count = 0;
		m_memoryBuffer = NULL;
		m_dwMemoryBuffer = 0;
		m_startCache = 0;
		memset(&m_dbHeader, 0, sizeof(m_dbHeader));
		if (lpThumbFilePath == NULL) {
			return false;
		}
		m_dbPath = std::wstring(lpThumbFilePath);
		bool status = false;
		do {
			m_hFile = ::CreateFileW(m_dbPath.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if (m_hFile == INVALID_HANDLE_VALUE) {
				break;
			}
			status = BaseParser();
			if (status == false) {
				break;
			}
			std::vector<Wrapper>obj;
			status = Statistics(obj);
		} while (false);
		return status;
	}

		m_count = 0;
		m_startCache = 0;
		m_dbPath = L"";
		bool status = false;
		memset(&m_dbHeader, 0, sizeof(m_dbHeader));
		do {
			if (lpThumbFileContent == NULL || size <= 0 || size > INT_MAX) {
				break;
			}
			m_memoryBuffer = (char*)lpThumbFileContent;
			m_dwMemoryBuffer = size;
			status = BaseParser();
			if (status == false) {
				break;
			}
			std::vector<Wrapper>obj;
			status = Statistics(obj);

		} while (false);
		return status;
	}

	void Uninit() {
		m_count = 0;
		m_startCache = 0;
		m_dbPath = L"";
		memset(&m_dbHeader, 0, sizeof(m_dbHeader));
		if (m_hFile != INVALID_HANDLE_VALUE) {
			::CloseHandle(m_hFile);
			m_hFile = INVALID_HANDLE_VALUE;
		}
		if (m_memoryBuffer != NULL) {
			m_memoryBuffer = NULL;
			m_dwMemoryBuffer = 0;
			/*
			* caller should free memory !
			*/
		}
	}

	bool SaveAs(const unsigned int offset, const unsigned int size, CONST LPCTSTR lpFilePath) {
		// save file content
		DWORD dwWrite = 0;
		bool status = false;
		std::unique_ptr<char[]> buffer(new char[size]);
		if (buffer == NULL) {
			return false;
		}
		memset(buffer.get(), 0, size);
		// get content from file or memory 
		status = GetContent(offset, buffer.get(), size);
		if (status == false) {
			return false;
		}
		
		HANDLE hFileSave = CreateFile(lpFilePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFileSave == INVALID_HANDLE_VALUE) {
			return false;
		}
		
		WriteFile(hFileSave, buffer.get(), size, &dwWrite, NULL);
		CloseHandle(hFileSave);
		hFileSave = INVALID_HANDLE_VALUE;
		return true;
	}

	unsigned int GetCount() const {
		return m_count;
	}

	bool GetItemList(std::vector<Wrapper>& items) {
		return Statistics(items, true);
	}

		ThumbNail obj;
		std::vector<Wrapper>items;
		bool status = false;
		do {
			status = obj.Init(lpThumbFileContent, size);
			if (status == false) {
				break;
			}
			status = obj.CreateOutputDir(lpOutputDirectory);
			if (status == false) {
				break;
			}
			status = obj.GetItemList(items);
			if (status == false) {
				break;
			}
			std::wstring savePath = std::wstring(lpOutputDirectory) + L"\\";
			for (auto& it : items) {
				obj.SaveAs(it.GetOffset(), it.GetDataSize(), (savePath + it.GetFileName()).c_str());
			}
			obj.Uninit();
		} while (false);
		return;
	}

	static void Extract(CONST LPCTSTR lpThumbFilePath, CONST LPCTSTR lpOutputDirectory) {
		ThumbNail obj;
		std::vector<Wrapper>items;
		bool status = false;
		do {
			status = obj.Init(lpThumbFilePath);
			if (status == false) {
				break;
			}
			status = obj.CreateOutputDir(lpOutputDirectory);
			if (status == false) {
				break;
			}
			status = obj.GetItemList(items);
			if (status == false) {
				break;
			}
			std::wstring savePath = std::wstring(lpOutputDirectory) + L"\\";
			for (auto& it : items) {
				obj.SaveAs(it.GetOffset(), it.GetDataSize(), (savePath + it.GetFileName()).c_str());
			}
			obj.Uninit();
		} while (false);
		return;
	}

	explicit ThumbNail() {
		m_hFile = INVALID_HANDLE_VALUE;
		m_memoryBuffer = NULL;
		m_dwMemoryBuffer = 0;
		m_count = 0;
		m_startCache = 0;
		m_dbPath = L"";
	}

	virtual ~ThumbNail() {
		Uninit();
	}

private:
	void TrimInvalidChar(wchar_t* fileName) {
		wchar_t* filename_ptr = fileName;
		while (filename_ptr != NULL && *filename_ptr != NULL)
		{
			if (*filename_ptr == L'\\' ||
				*filename_ptr == L'/'  ||
				*filename_ptr == L':'  ||
				*filename_ptr == L'*'  ||
				*filename_ptr == L'?'  ||
				*filename_ptr == L'\"' ||
				*filename_ptr == L'<'  ||
				*filename_ptr == L'>'  ||
				*filename_ptr == L'|'){
				*filename_ptr = L'_';
			}
			++filename_ptr;
		}
	}
	
	bool GetContent(DWORD offset, char *content, DWORD dwContent) {
		if (m_hFile != INVALID_HANDLE_VALUE) {
			BOOL status = FALSE;
			DWORD dwRead = 0;
			DWORD position = ::SetFilePointer(m_hFile, offset, NULL, FILE_BEGIN);
			if (position == INVALID_SET_FILE_POINTER) {
				return false;
			}
			status = ::ReadFile(m_hFile, content, dwContent, &dwRead, NULL);
			if (status && dwRead == dwContent) {
				return true;
			}
			else return false;
		}
		else if (m_memoryBuffer != NULL && m_dwMemoryBuffer != 0) {
			if (offset + dwContent < m_dwMemoryBuffer) {
				memcpy(content, m_memoryBuffer + offset, dwContent);
				return true;
			}
			else return false;
		}
		else return false;
	}

	bool GetAndCatExtension(const char *buffer, wchar_t *fileName, DWORD dwFileName) {
		if (buffer == NULL || fileName == NULL || dwFileName <= 0) {
			return false;
		}
		TrimInvalidChar(fileName);
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
			return false;
		}
		return true;
	}

	bool StatisticsWin10(std::vector<Wrapper>& items, bool needSave = false) {
		bool status = false;
		PDB_CACHE_ENTRY_8P dbCacheEntry = NULL;
		dbCacheEntry = (PDB_CACHE_ENTRY_8P)new char[sizeof(DB_CACHE_ENTRY_8P)];
		if (dbCacheEntry == NULL) {
			return false;
		}
		memset(dbCacheEntry, 0, sizeof(DB_CACHE_ENTRY_8P));
		unsigned int position = m_startCache;
		for (int i = 0;; ++i) {
			status = GetContent(position, (char *)dbCacheEntry, sizeof(DB_CACHE_ENTRY_8P));
			if (status == false) {
				delete[] dbCacheEntry;
				dbCacheEntry = NULL;
				break;
			}
			status = (memcmp(dbCacheEntry->magicIdentifier, DB_MAGIC_ID, 4) == 0);
			if (status == false) {
				break;
			}
			if (dbCacheEntry->dwData > 8) {
				
				if (needSave) {
					wchar_t fileName[MAX_PATH] = { 0 };
					unsigned int dwFileName = dbCacheEntry->dwFilename;
					unsigned int dwDataSize = dbCacheEntry->dwData;
					unsigned int dwPadding = dbCacheEntry->dwPadding;
					unsigned int subPosition = position + dwPadding + sizeof(DB_CACHE_ENTRY_8P);

					status = GetContent(subPosition, (char *)fileName, dwFileName);
					if (status == false) {
						break;
					}
					char buffer[9] = { 0 };
					status = GetContent(subPosition + dwFileName, buffer, 8);
					if (status == false) {
						break;
					}
					status = GetAndCatExtension(buffer, fileName, dwFileName);
					if (status == false) {
						break;
					}
					Wrapper item(subPosition + dwFileName, dwDataSize, fileName, dwFileName);
					items.emplace_back(item);
				}
				else {
					m_count++;
				}
				
				position += dbCacheEntry->dwCacheEntry;
				memset(dbCacheEntry, 0, sizeof(DB_CACHE_ENTRY_8P));
				continue;
			}
			position += dbCacheEntry->dwCacheEntry;
			
		}

		if (dbCacheEntry != NULL) {
			delete[] dbCacheEntry;
			dbCacheEntry = NULL;
		}
		return true;
	}
	
	bool StatisticsWin7(std::vector<Wrapper>& items, bool needSave = false) {
		bool status = false;
		PDB_CACHE_ENTRY_7 dbCacheEntry = NULL;
		dbCacheEntry = (PDB_CACHE_ENTRY_7)new char[sizeof(PDB_CACHE_ENTRY_7)];
		if (dbCacheEntry == NULL) {
			return false;
		}
		memset(dbCacheEntry, 0, sizeof(PDB_CACHE_ENTRY_7));
		unsigned int position = m_startCache;
		for (int i = 0;; ++i) {
			status = GetContent(position, (char*)dbCacheEntry, sizeof(PDB_CACHE_ENTRY_7));
			if (status == false) {
				delete[] dbCacheEntry;
				dbCacheEntry = NULL;
				break;
			}
			status = (memcmp(dbCacheEntry->magicIdentifier, DB_MAGIC_ID, 4) == 0);
			if (status == false) {
				break;
			}
			if (dbCacheEntry->dwData > 8) {
				if (needSave) {
					wchar_t fileName[MAX_PATH] = { 0 };
					unsigned int dwFileName = dbCacheEntry->dwFilename;
					unsigned int dwDataSize = dbCacheEntry->dwData;
					unsigned int dwPadding = dbCacheEntry->dwPadding;
					unsigned int subPosition = position + dwPadding + sizeof(PDB_CACHE_ENTRY_7);

					status = GetContent(subPosition, (char*)fileName, dwFileName);
					if (status == false) {
						break;
					}
					char buffer[9] = { 0 };
					status = GetContent(subPosition + dwFileName, buffer, 8);
					if (status == false) {
						break;
					}
					status = GetAndCatExtension(buffer, fileName, dwFileName);
					if (status == false) {
						break;
					}
					Wrapper item(subPosition + dwFileName, dwDataSize, fileName, dwFileName);
					items.push_back(item);
				}
				else {
					m_count++;
				}

				position += dbCacheEntry->dwCacheEntry;
				memset(dbCacheEntry, 0, sizeof(PDB_CACHE_ENTRY_7));
				continue;
			}
			position += dbCacheEntry->dwCacheEntry;

		}

		if (dbCacheEntry != NULL) {
			delete[] dbCacheEntry;
			dbCacheEntry = NULL;
		}
		return true;
	}

	bool Statistics(std::vector<Wrapper>& items, bool needSave = false) {
		if (m_dbHeader.version == WINDOWS_10) {
			return StatisticsWin10(items, needSave);
		}
		else {
			return StatisticsWin7(items, needSave);
		}
	}

	bool CreateOutputDir(CONST LPCTSTR outputPath) {
		if (::GetFileAttributesW(outputPath) == INVALID_FILE_ATTRIBUTES) {
			return ::SHCreateDirectoryExW(NULL, outputPath, NULL) == ERROR_SUCCESS;
		}
		/*
		* `SetCurrentDirectory` is not safe in multi-thread scenario
		* if some thread use this API, all threads in this process will be affected!
		* Also See
		* https://docs.microsoft.com/zh-cn/windows/win32/api/winbase/nf-winbase-setcurrentdirectory
		*/
		// SetCurrentDirectory(outputPath);
		return true;
	}

	bool BaseParser() {
		bool status = false;
		do {
			unsigned int fileOffset = 0;

			status = GetContent(fileOffset, (char*)&m_dbHeader, sizeof(DB_HEADER));
			if (status == false) {
				break;
			}
			if (memcmp(m_dbHeader.magicIdentifier, DB_MAGIC_ID, 4) != 0) {
				status = false;
				break;
			}
			if (m_dbHeader.version != WINDOWS_10 &&
				m_dbHeader.version != WINDOWS_7 &&
				m_dbHeader.version != WINDOWS_8 &&
				m_dbHeader.version != WINDOWS_VISTA) {
				status = false;
				break;
			}
			fileOffset += sizeof(DB_HEADER);

			if (m_dbHeader.version == WINDOWS_10) {
				DB_HEADER_ENTRY_10 dbEntry = { 0 };
				status = GetContent(fileOffset, (char*)&dbEntry, sizeof(DB_HEADER_ENTRY_10));
				if (status == false) {
					break;
				}
				m_startCache = dbEntry.firstCacheEntry;
			}
			else {
				DB_HEADER_ENTRY_7 dbEntry = { 0 };
				status = GetContent(fileOffset, (char*)&dbEntry, sizeof(DB_HEADER_ENTRY_7));
				if (status == false) {
					break;
				}
				m_startCache = dbEntry.firstCacheEntry;
			}
		} while (false);

		return status;
	}

private:
	HANDLE m_hFile;

	char* m_memoryBuffer;
	unsigned int m_dwMemoryBuffer;

	std::wstring m_dbPath;
	unsigned int m_count;
	int m_startCache;
	DB_HEADER m_dbHeader;
};
