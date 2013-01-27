// rplsinfoDLL.cpp : DLL アプリケーション用にエクスポートされる関数を定義します。
//

#include "stdafx.h"
#include "rplsinfoDLL.h"
#include "rplsinfo.h"
#include "resource.h"


//// これは、エクスポートされた変数の例です。
//RPLSINFODLL_API int nrplsinfoDLL=0;
//
//// これは、エクスポートされた関数の例です。
//RPLSINFODLL_API int fnrplsinfoDLL(void)
//{
//	return 42;
//}
//
//// これは、エクスポートされたクラスのコンストラクターです。
//// クラス定義に関しては rplsinfoDLL.h を参照してください。
//CrplsinfoDLL::CrplsinfoDLL()
//{
//	return;
//}


//// Version取得関数
wchar_t * getVer();

//// DLL用main改変関数
int rplsinfo(LPCWSTR filename, ProgInfo *proginfo);

ProgInfo proginfo;

RPLSINFODLL_API int __stdcall rplsRead(LPCWSTR filename)
{
	memset(&proginfo,'\0',sizeof(proginfo));
	return rplsinfo(filename, &proginfo);
}

RPLSINFODLL_API LPCWSTR __stdcall version()
{
	static wchar_t _version[256];
	wmemset(_version,'\0',sizeof(_version));
	wcscpy_s(_version, getVer());
	wcscat_s(_version, L"(DLL Ver." VER_STR_PRODUCTVERSION L" " VER_STR_LEGALCOPYRIGHT L")");
	return _version;
}

RPLSINFODLL_API int __stdcall recyear() { return proginfo.recyear; }
RPLSINFODLL_API int __stdcall recmonth() { return proginfo.recmonth; }
RPLSINFODLL_API int __stdcall recday() { return proginfo.recday; }
RPLSINFODLL_API int __stdcall rechour() { return proginfo.rechour; }
RPLSINFODLL_API int __stdcall recmin() { return proginfo.recmin; }
RPLSINFODLL_API int __stdcall recsec() { return proginfo.recsec; }
RPLSINFODLL_API int __stdcall durhour() { return proginfo.durhour; }
RPLSINFODLL_API int __stdcall durmin() { return proginfo.durmin; }
RPLSINFODLL_API int __stdcall dursec() { return proginfo.dursec; }
RPLSINFODLL_API int __stdcall rectimezone() { return proginfo.rectimezone; }
RPLSINFODLL_API int __stdcall makerid() { return proginfo.makerid; }
RPLSINFODLL_API int __stdcall modelcode() { return proginfo.modelcode; }
RPLSINFODLL_API int __stdcall recsrc() { return proginfo.recsrc; }
RPLSINFODLL_API int __stdcall chnum() { return proginfo.chnum; }
RPLSINFODLL_API int __stdcall chnamelen() { return proginfo.chnamelen; }
RPLSINFODLL_API int __stdcall pnamelen() { return proginfo.pnamelen; }
RPLSINFODLL_API int __stdcall pdetaillen() { return proginfo.pdetaillen; }
RPLSINFODLL_API int __stdcall pextendlen() { return proginfo.pextendlen; }
RPLSINFODLL_API LPCWSTR __stdcall chname() { return proginfo.chname; }
RPLSINFODLL_API LPCWSTR __stdcall pname() { return proginfo.pname; }
RPLSINFODLL_API LPCWSTR __stdcall pdetail() { return proginfo.pdetail; }
RPLSINFODLL_API LPCWSTR __stdcall pextend() { return proginfo.pextend; }
RPLSINFODLL_API int __stdcall genre(const int index) { return proginfo.genre[index]; }
RPLSINFODLL_API int __stdcall genretype() 
{
	if(proginfo.genretype[2] == 0x01)
		return 3;
	else	if(proginfo.genretype[1] == 0x01)
		return 2;
	else	if(proginfo.genretype[0] == 0x01)
		return 1;
	return 0;
}

//// conv_from_unicode
int conv_from_unicode(unsigned char *dbuf, const int maxbufsize, const WCHAR *sbuf, const int total_length, const BOOL bCharSize);

unsigned char dbuf[CONVBUFSIZE];

RPLSINFODLL_API int __stdcall conv_from(const WCHAR *sbuf, const int total_length, const BOOL bCharSize) 
{
	memset(dbuf,'\0',sizeof(dbuf));
	return conv_from_unicode(dbuf, CONVBUFSIZE, sbuf, total_length, bCharSize);
}
RPLSINFODLL_API int __stdcall getconv(const int index) { return dbuf[index]; }
