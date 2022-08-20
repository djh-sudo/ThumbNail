#include <cstdlib>
#include <assert.h>
#include <iostream>
#include "format.h"

/*
* thumb cache file is located:
* C:\\Users\\Administrator\\AppData\\Local\\Microsoft\\Windows\\Explorer\\thumbcache_*.db
* code is steal from the following repository
* https://github.com/thumbcacheviewer/thumbcacheviewer/tree/master/thumbcache_viewer_cmd
*/

using namespace std;

/*
* test demo 1 [static interface]:
* 
	ThumbNail::Extract(L"C:/Users/Administrator/AppData/Local/Microsoft/Windows/Explorer/thumbcache_256.db", L"C:\\results");
*
* This will save picture at C:\results
*/

/*
* test demo 2 [static interface]:
* 
 	FILE* fp = fopen("C:/Users/Administrator/AppData/Local/Microsoft/Windows/Explorer/thumbcache_256.db", "rb");
	if (fp == NULL) {
		return -1;
	}
	char* buf = NULL;
	int bufSize = 0;
	assert(fseek(fp, 0, SEEK_END) == 0);
	bufSize = ftell(fp);
	buf = new char[bufSize + 1];
	assert(buf);
	fseek(fp, 0, SEEK_SET);
	fread(buf, bufSize, 1, fp);
	// load memory

	ThumbNail::Extract(buf, bufSize, L"C:\\1\\2");

	// free memory
	if (buf != NULL) {
		delete[] buf;
		buf = NULL;
	}
*
* Warning!!! please use `\\`, not use `/`
* Mention!!! Support multi-level directory
*/
int main() {

	ThumbNail::Extract(L"C:/Users/Administrator/AppData/Local/Microsoft/Windows/Explorer/thumbcache_256.db", L"C:\\results\\");

	system("pause");
	return 0;
}