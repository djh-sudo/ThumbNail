#include "iostream"
#include "format.h"

/*
* thumb cache file is located:
* C:\\Users\\Administrator\\AppData\\Local\\Microsoft\\Windows\\Explorer\\thumbcache_*.db
* code is steal from the following repository
* https://github.com/thumbcacheviewer/thumbcacheviewer/tree/master/thumbcache_viewer_cmd
*/


int main() {
	ThumbNail obj;
	obj.Init(L"C:/Users/Administrator/AppData/Local/Microsoft/Windows/Explorer/thumbcache_256.db");
	obj.Parser();
	return 0;
}