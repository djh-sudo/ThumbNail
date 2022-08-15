#include "iostream"
#include "format.h"

int main() {
	ThumbNail obj;
	obj.Init(L"C:/Users/Administrator/AppData/Local/Microsoft/Windows/Explorer/thumbcache_32.db");
	obj.Parser();
	return 0;
}