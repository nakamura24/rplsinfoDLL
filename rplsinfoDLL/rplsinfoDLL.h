// 以下の ifdef ブロックは DLL からのエクスポートを容易にするマクロを作成するための 
// 一般的な方法です。この DLL 内のすべてのファイルは、コマンド ラインで定義された RPLSINFODLL_EXPORTS
// シンボルを使用してコンパイルされます。このシンボルは、この DLL を使用するプロジェクトでは定義できません。
// ソースファイルがこのファイルを含んでいる他のプロジェクトは、 
// RPLSINFODLL_API 関数を DLL からインポートされたと見なすのに対し、この DLL は、このマクロで定義された
// シンボルをエクスポートされたと見なします。
#ifdef RPLSINFODLL_EXPORTS
#define RPLSINFODLL_API __declspec(dllexport)
#else
#define RPLSINFODLL_API __declspec(dllimport)
#endif

//// このクラスは rplsinfoDLL.dll からエクスポートされました。
//class RPLSINFODLL_API CrplsinfoDLL {
//public:
//	CrplsinfoDLL(void);
//	// TODO: メソッドをここに追加してください。
//};
//
//extern RPLSINFODLL_API int nrplsinfoDLL;
//
//RPLSINFODLL_API int fnrplsinfoDLL(void);

#ifdef  __cplusplus
extern "C" {
#endif

RPLSINFODLL_API LPCWSTR __stdcall version();
RPLSINFODLL_API int __stdcall rplsRead(LPCWSTR filename);
RPLSINFODLL_API int __stdcall recyear();
RPLSINFODLL_API int __stdcall recmonth();
RPLSINFODLL_API int __stdcall recday();
RPLSINFODLL_API int __stdcall rechour();
RPLSINFODLL_API int __stdcall recmin();
RPLSINFODLL_API int __stdcall recsec();
RPLSINFODLL_API int __stdcall durhour();
RPLSINFODLL_API int __stdcall durmin();
RPLSINFODLL_API int __stdcall dursec();
RPLSINFODLL_API int __stdcall rectimezone();
RPLSINFODLL_API int __stdcall makerid();
RPLSINFODLL_API int __stdcall modelcode();
RPLSINFODLL_API int __stdcall recsrc();
RPLSINFODLL_API int __stdcall chnum();
RPLSINFODLL_API int __stdcall chnamelen();
RPLSINFODLL_API int __stdcall pnamelen();
RPLSINFODLL_API int __stdcall pdetaillen();
RPLSINFODLL_API int __stdcall pextendlen();
RPLSINFODLL_API LPCWSTR __stdcall chname();
RPLSINFODLL_API LPCWSTR __stdcall pname();
RPLSINFODLL_API LPCWSTR __stdcall pdetail();
RPLSINFODLL_API LPCWSTR __stdcall pextend();
RPLSINFODLL_API int __stdcall genre(const int index);
RPLSINFODLL_API int __stdcall genretype();

RPLSINFODLL_API int __stdcall conv_from(const WCHAR *sbuf, const int total_length, const BOOL bCharSize);
RPLSINFODLL_API int __stdcall getconv(const int index);

#ifdef  __cplusplus
}
#endif
