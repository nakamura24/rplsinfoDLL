// rplsinfo.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <new.h>
#include <locale.h>
#include <wchar.h>
#include "rplsinfo.h"
#include "proginfo.h"
#include "convtounicode.h"

#include <conio.h>


// 定数など

#define		MAXFLAGNUM				256
#define		FILE_INVALID			0
#define		FILE_RPLS				1
#define		FILE_188TS				188
#define		FILE_192TS				192
#define		DEFAULTTSFILEPOS		50

#define		NAMESTRING				L"\nrplsinfo version 1.3 "

#ifdef _WIN64
	#define		NAMESTRING2				L"(64bit)\n"
#else
	#define		NAMESTRING2				L"(32bit)\n"
#endif


// 構造体宣言

typedef struct {
	int		argSrc;
	int		argDest;
	int		separator;
	int		flags[MAXFLAGNUM + 1];
	BOOL	bNoControl;
	BOOL	bNoComma;
	BOOL	bDQuot;
	BOOL	bItemName;
	BOOL	bDisplay;
	BOOL	bCharSize;
	int		tsfilepos;
} CopyParams;


// プロトタイプ宣言

void	initCopyParams(CopyParams*);
BOOL	parseCopyParams(int, _TCHAR*[], CopyParams*);
int		rplsTsCheck(HANDLE);
BOOL	rplsMakerCheck(unsigned char*, int);
void	readRplsProgInfo(HANDLE, ProgInfo*, BOOL);
void	outputProgInfo(HANDLE, ProgInfo*, CopyParams*);
int		putGenreStr(WCHAR*, int, int*, int*);
int		getRecSrcIndex(int);
int		putRecSrcStr(WCHAR*, int, int);
int		convforcsv(WCHAR*, int, WCHAR*, int, BOOL, BOOL, BOOL, BOOL);


// メイン

int _tmain(int argc, _TCHAR* argv[])
{
	_tsetlocale(LC_ALL, _T(""));


	// パラメータチェック

	if (argc == 1) {
		wprintf(NAMESTRING NAMESTRING2);
		exit(1);
	}

	CopyParams		param;
	initCopyParams(&param);

	if( !parseCopyParams(argc, argv, &param) ) exit(1);																	// パラメータ異常なら終了


	// 番組情報読み込み元ファイルを開く

	HANDLE	hReadFile = CreateFile(argv[param.argSrc], GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hReadFile == INVALID_HANDLE_VALUE) {
		wprintf(L"ファイル %s を開くのに失敗しました.\n", argv[param.argSrc]);
		exit(1);
	}

	int		sfiletype = rplsTsCheck(hReadFile);
	if(sfiletype == FILE_INVALID) {																						// 無効なファイルの場合
		wprintf(L"ファイル %s は有効なRPLSファイル，TSファイルではありません.\n", argv[param.argSrc]);
		CloseHandle(hReadFile);
		exit(1);
	}


	// 番組情報の読み込み

	ProgInfo	*proginfo;
	try {
		proginfo = new ProgInfo;
	}
	catch(...) {
		wprintf(L"アプリケーションメモリの確保に失敗しました.\n");
		exit(1);
	}
	memset(proginfo, 0, sizeof(ProgInfo)); 

	_wfullpath(proginfo->fullpath, argv[param.argSrc], _MAX_PATH);														// フルパス名取得
	_wsplitpath_s(proginfo->fullpath, NULL, 0, NULL, 0, proginfo->fname, _MAX_PATH, proginfo->fext, _MAX_PATH);			// ベースファイル名と拡張子

	if(sfiletype == FILE_RPLS) {
		readRplsProgInfo(hReadFile, proginfo, param.bCharSize);
	} else 	{
		BOOL bResult = readTsProgInfo(hReadFile, proginfo, sfiletype, param.tsfilepos, param.bCharSize);
		if(!bResult) {
			wprintf(L"TSファイル %s から有効な番組情報を検出できませんでした.\n", argv[param.argSrc]);
			CloseHandle(hReadFile);
			delete proginfo;
			exit(1);
		}
	}

	CloseHandle(hReadFile);


	// 必要なら出力ファイルを開く

	DWORD	dwNumWrite;
	HANDLE	hWriteFile = INVALID_HANDLE_VALUE;

	if(!param.bDisplay) {

		hWriteFile = CreateFile(argv[param.argDest], GENERIC_WRITE | GENERIC_READ, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if(hWriteFile == INVALID_HANDLE_VALUE) {
			wprintf(L"保存先ファイル %s を開けませんでした.\n", argv[param.argDest]);
			delete proginfo;
			exit(1);
		}

		LARGE_INTEGER	fsize;
		GetFileSizeEx(hWriteFile, &fsize);

		if(fsize.QuadPart == 0) {
			WCHAR	bom = 0xFEFF;
			WriteFile(hWriteFile, &bom, sizeof(bom), &dwNumWrite, NULL);									// ファイルが空ならBOM出力
		} else {
			SetFilePointerEx(hWriteFile, fsize, NULL, FILE_BEGIN);											// 空でないならファイルの最後尾に移動
		}

	}


	// オプション指定に応じて番組内容を出力

	outputProgInfo(hWriteFile, proginfo, &param);


	// 終了処理

	if(!param.bDisplay) CloseHandle(hWriteFile);
	delete proginfo;

	return 0;
}


void initCopyParams(CopyParams *param)
{
	param->argSrc			= 0;
	param->argDest			= 0;

	param->separator		= S_NORMAL;
	param->bNoControl		= TRUE;
	param->bNoComma			= TRUE;
	param->bDQuot			= FALSE;
	param->bItemName		= FALSE;

	param->bDisplay			= FALSE;
	param->bCharSize		= FALSE;

	param->tsfilepos		= DEFAULTTSFILEPOS;

	memset(param->flags, 0,sizeof(param->flags));

	return;
}


BOOL parseCopyParams(int argn, _TCHAR *args[], CopyParams *param)
{
	int		fnum		= 0;
	BOOL	bArgSkip	= FALSE;

	for(int	i = 1; i < argn; i++) {

		if( args[i][0] == L'-') {

			for(int	j = 1; j < (int)wcslen(args[i]); j++) {

				switch(args[i][j])
				{
					case L'f':
						param->flags[fnum++] = F_FileName;
						break;
					case L'u':
						param->flags[fnum++] = F_FileNameFullPath;
						break;
					case L'd':
						param->flags[fnum++] = F_RecDate;
						break;
					case L't':
						param->flags[fnum++] = F_RecTime;
						break;
					case L'p':
						param->flags[fnum++] = F_RecDuration;
						break;
					case L'z':
						param->flags[fnum++] = F_RecTimeZone;
						break;
					case L'a':
						param->flags[fnum++] = F_MakerID;
						break;
					case L'o':
						param->flags[fnum++] = F_ModelCode;
						break;
					case L's':
						param->flags[fnum++] = F_RecSrc;
						break;
					case L'c':
						param->flags[fnum++] = F_ChannelName;
						break;
					case L'n':
						param->flags[fnum++] = F_ChannelNum;
						break;
					case L'b':
						param->flags[fnum++] = F_ProgName;
						break;
					case L'i':
						param->flags[fnum++] = F_ProgDetail;
						break;
					case L'e':
						param->flags[fnum++] = F_ProgExtend;
						break;
					case L'g':
						param->flags[fnum++] = F_ProgGenre;
						break;
					case L'T':
						param->separator	= S_TAB;
						param->bNoControl	= TRUE;
						param->bNoComma		= FALSE;
						param->bDQuot		= FALSE;
						param->bItemName	= FALSE;
						break;
					case L'S':
						param->separator	= S_SPACE;
						param->bNoControl	= TRUE;
						param->bNoComma		= FALSE;
						param->bDQuot		= FALSE;
						param->bItemName	= FALSE;
						break;
					case L'C':
						param->separator	= S_CSV;
						param->bNoControl	= FALSE;
						param->bNoComma		= FALSE;
						param->bDQuot		= TRUE;
						param->bItemName	= FALSE;
						break;
					case L'N':
						param->separator	= S_NEWLINE;
						param->bNoControl	= FALSE;
						param->bNoComma		= FALSE;
						param->bDQuot		= FALSE;
						param->bItemName	= FALSE;
						break;
					case L'I':
						param->separator	= S_ITEMNAME;
						param->bNoControl	= FALSE;
						param->bNoComma		= FALSE;
						param->bDQuot		= FALSE;
						param->bItemName	= TRUE;
						break;
					case L'y':
						param->bCharSize = TRUE;
						break;
					case L'F':
						if( (i == (argn - 1)) || (_wtoi(args[i + 1]) < 0) || (_wtoi(args[i + 1]) > 99) ) {
							wprintf(L"-F オプションの引数が異常です.\n");
							return	FALSE;
						}
						param->tsfilepos = _wtoi(args[i + 1]);
						bArgSkip = TRUE;
						break;
					default:
						wprintf(L"無効なスイッチが指定されました.\n");
						return	FALSE;
				}

				if(fnum == MAXFLAGNUM) {
					wprintf(L"スイッチ指定が多すぎます.\n");
					return	FALSE;
				}
			}
		
		} else {

			if(param->argSrc == 0) {
				param->argSrc = i;
			} 
			else if ( param->argDest == 0) {
				param->argDest = i;
			}
			else {
				wprintf(L"パラメータが多すぎます.\n");
				return FALSE;
			}
		}

		if(bArgSkip){
			i++;
			bArgSkip = FALSE;
		}

	}

	if(param->argSrc == 0) {
		wprintf(L"パラメータが足りません.\n");
		return	FALSE;
	}

	if(param->argDest == 0)	param->bDisplay = TRUE;

	return	TRUE;
}


int rplsTsCheck(HANDLE hReadFile)
{
	DWORD			dwNumRead;
	
	unsigned char	buf[1024];
	memset(buf, 0, 1024);

	SetFilePointer(hReadFile, 0, NULL, FILE_BEGIN);
	ReadFile(hReadFile, buf, 1024, &dwNumRead, NULL);

	if( (buf[0] == 'P') && (buf[1] == 'L') && (buf[2] == 'S') && (buf[3] == 'T') ){
		return	FILE_RPLS;
	}
	else if( (buf[188 * 0] == 0x47) && (buf[188 * 1] == 0x47) && (buf[188 * 2] == 0x47) && (buf[188 * 3] == 0x47) ){
		return	FILE_188TS;
	}
	else if( (buf[192 * 0 + 4] == 0x47) && (buf[192 * 1 + 4] == 0x47) && (buf[192 * 2 + 4] == 0x47) && (buf[192 * 3 + 4] == 0x47) ){
		return	FILE_192TS;
	}

	return	FILE_INVALID;
}


BOOL rplsMakerCheck(unsigned char *buf, int idMaker)
{
	unsigned int	mpadr = (buf[ADR_MPDATA] << 24) + (buf[ADR_MPDATA + 1] << 16) + (buf[ADR_MPDATA + 2] << 8) + buf[ADR_MPDATA + 3];
	if(mpadr == 0) return FALSE;

	unsigned char	*mpdata = buf + mpadr;
	int		makerid = (mpdata[ADR_MPMAKERID] << 8) + mpdata[ADR_MPMAKERID + 1];

	if(makerid != idMaker) return FALSE;

	return TRUE;
}


void readRplsProgInfo(HANDLE hFile, ProgInfo *proginfo, BOOL bCharSize)
{
	int				fsize = GetFileSize(hFile, NULL);
	unsigned char	*buf;

	try {
		buf = new unsigned char[fsize];
	}
	catch(...) {
		wprintf(L"読み込みバッファメモリの確保に失敗しました.\n");
		exit(1);
	}

	DWORD		dwNumRead;

	SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
	ReadFile(hFile, buf, fsize, &dwNumRead, NULL);

	proginfo->bSonyRpls = rplsMakerCheck(buf, MAKERID_SONY);
	proginfo->bPanaRpls = rplsMakerCheck(buf, MAKERID_PANASONIC);

	unsigned char	*appinfo = buf + ADR_APPINFO;
	unsigned char	*mpdata = buf + (buf[ADR_MPDATA] << 24) + (buf[ADR_MPDATA + 1] << 16) + (buf[ADR_MPDATA + 2] << 8) + buf[ADR_MPDATA + 3];

	proginfo->recyear	= (appinfo[ADR_RECYEAR] >> 4) * 1000 + (appinfo[ADR_RECYEAR] & 0x0F) * 100 + (appinfo[ADR_RECYEAR + 1] >> 4) * 10 + (appinfo[ADR_RECYEAR + 1] & 0x0F);
	proginfo->recmonth	= (appinfo[ADR_RECMONTH] >> 4) * 10 + (appinfo[ADR_RECMONTH] & 0x0F);
	proginfo->recday	= (appinfo[ADR_RECDAY] >> 4) * 10 + (appinfo[ADR_RECDAY] & 0x0F);
	proginfo->rechour	= (appinfo[ADR_RECHOUR] >> 4) * 10 + (appinfo[ADR_RECHOUR] & 0x0F);
	proginfo->recmin	= (appinfo[ADR_RECMIN] >> 4) * 10 + (appinfo[ADR_RECMIN] & 0x0F);
	proginfo->recsec	= (appinfo[ADR_RECSEC] >> 4) * 10 + (appinfo[ADR_RECSEC] & 0x0F);

	proginfo->durhour	= (appinfo[ADR_DURHOUR] >> 4) * 10 + (appinfo[ADR_DURHOUR] & 0x0F);
	proginfo->durmin	= (appinfo[ADR_DURMIN] >> 4) * 10 + (appinfo[ADR_DURMIN] & 0x0F);
	proginfo->dursec	= (appinfo[ADR_DURSEC] >> 4) * 10 + (appinfo[ADR_DURSEC] & 0x0F);

	proginfo->rectimezone = appinfo[ADR_TIMEZONE];

	proginfo->makerid	= appinfo[ADR_MAKERID] * 256 + appinfo[ADR_MAKERID + 1];
	proginfo->modelcode	= appinfo[ADR_MODELCODE] * 256 + appinfo[ADR_MODELCODE + 1];

	if(proginfo->bPanaRpls) {
		proginfo->recsrc = getRecSrcIndex(mpdata[ADR_RECSRC_PANA] * 256 + mpdata[ADR_RECSRC_PANA + 1]);			// panaなら放送種別情報取得
	} else {
		proginfo->recsrc = -1;																					// 非panaなら放送種別情報無し
	}

	proginfo->chnum		= appinfo[ADR_CHANNELNUM] * 256 + appinfo[ADR_CHANNELNUM + 1];

	proginfo->chnamelen	= conv_to_unicode(proginfo->chname, CONVBUFSIZE, appinfo + ADR_CHANNELNAME + 1, (int)appinfo[ADR_CHANNELNAME], bCharSize);
	proginfo->pnamelen	= conv_to_unicode(proginfo->pname, CONVBUFSIZE, appinfo + ADR_PNAME + 1, (int)appinfo[ADR_PNAME], bCharSize);

	int		pdetaillen		= appinfo[ADR_PDETAIL] * 256 + appinfo[ADR_PDETAIL + 1];
	proginfo->pdetaillen	= conv_to_unicode(proginfo->pdetail, CONVBUFSIZE, appinfo + ADR_PDETAIL + 2, pdetaillen, bCharSize); 

	if(proginfo->bSonyRpls) {
		int		pextendlen		= mpdata[ADR_PEXTENDLEN] * 256 + mpdata[ADR_PEXTENDLEN + 1];
		proginfo->pextendlen	= conv_to_unicode(proginfo->pextend, CONVBUFSIZE, appinfo + ADR_PDETAIL + 2 + pdetaillen, pextendlen, bCharSize);
		for(int i = 0; i < 3; i++) {
			proginfo->genretype[i]	= mpdata[ADR_GENRE + i * 4 + 0];
			proginfo->genre[i]		= mpdata[ADR_GENRE + i * 4 + 1];
		}
	}

	if(proginfo->bPanaRpls) {
		switch(mpdata[ADR_GENRE_PANA])
		{
			case 0xD5:
				proginfo->genretype[2] = 0x01;
				proginfo->genre[2] = ((mpdata[ADR_GENRE_PANA + 1] & 0x0F) << 4) + (mpdata[ADR_GENRE_PANA + 1] >> 4);
			case 0xE5:
				proginfo->genretype[1] = 0x01;
				proginfo->genre[1] = ((mpdata[ADR_GENRE_PANA + 2] & 0x0F) << 4) + (mpdata[ADR_GENRE_PANA + 2] >> 4);
			case 0xE9:
				proginfo->genretype[0] = 0x01;
				proginfo->genre[0] = ((mpdata[ADR_GENRE_PANA + 3] & 0x0F) << 4) + (mpdata[ADR_GENRE_PANA + 3] >> 4);
				break;
			default:
				break;
		}
	}

	delete [] buf;

	return;
}


void outputProgInfo(HANDLE hFile, ProgInfo *proginfo, CopyParams *param)
{
	DWORD	dwNumWrite;
	WCHAR	sstr[CONVBUFSIZE];
	WCHAR	dstr[CONVBUFSIZE];

	int		i = 0;

	while(param->flags[i] != 0) {
	
		int		slen = 0;

		// flagsに応じて出力する項目をsstrにコピーする

		switch(param->flags[i])
		{
			case F_FileName:
				if(param->bItemName) slen = swprintf_s(sstr, CONVBUFSIZE, L"[ファイル名]\r\n");
				slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"%s%s", proginfo->fname, proginfo->fext);
				break;
			case F_FileNameFullPath:
				if(param->bItemName) slen = swprintf_s(sstr, CONVBUFSIZE, L"[フルパスファイル名]\r\n");
				slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, proginfo->fullpath);
				break;
			case F_RecDate:
				if(param->bItemName) slen = swprintf_s(sstr, CONVBUFSIZE, L"[録画日付]\r\n");
				slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"%.4d/%.2d/%.2d", proginfo->recyear, proginfo->recmonth, proginfo->recday);
				break;
			case F_RecTime:
				if(param->bItemName) slen = swprintf_s(sstr, CONVBUFSIZE, L"[録画時刻]\r\n");
				slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"%.2d:%.2d:%.2d", proginfo->rechour, proginfo->recmin, proginfo->recsec);
				break;
			case F_RecDuration:
				if(param->bItemName) slen = swprintf_s(sstr, CONVBUFSIZE, L"[録画期間]\r\n");
				slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"%.2d:%.2d:%.2d", proginfo->durhour, proginfo->durmin, proginfo->dursec);
				break;
			case F_RecTimeZone:
				if(param->bItemName) slen = swprintf_s(sstr, CONVBUFSIZE, L"[タイムゾーン]\r\n");
				slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"%d", proginfo->rectimezone);
				break;
			case F_MakerID:
				if(param->bItemName) slen = swprintf_s(sstr, CONVBUFSIZE, L"[メーカーID]\r\n");
				if(proginfo->makerid != -1) {
					slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"%d", proginfo->makerid);
				} else {
					slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"n/a");
				}
				break;
			case F_ModelCode:
				if(param->bItemName) slen = swprintf_s(sstr, CONVBUFSIZE, L"[メーカー機種コード]\r\n");
				if(proginfo->modelcode != -1) {
					slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"%d", proginfo->modelcode);
				} else {
					slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"n/a");
				}
				break;
			case F_RecSrc:
				if(param->bItemName) slen = swprintf_s(sstr, CONVBUFSIZE, L"[放送種別]\r\n");
				if(proginfo->recsrc == -1) {
					slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"n/a");
				} else {
					slen += putRecSrcStr(sstr + slen, CONVBUFSIZE - slen, proginfo->recsrc);
				}
				break;
			case F_ChannelNum:
				if(param->bItemName) slen = swprintf_s(sstr, CONVBUFSIZE, L"[チャンネル番号]\r\n");
				slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"%.3dch", proginfo->chnum);
				break;
			case F_ChannelName:
				if(param->bItemName) slen = swprintf_s(sstr, CONVBUFSIZE, L"[放送局名]\r\n");
				wmemcpy_s(sstr + slen, CONVBUFSIZE - slen, proginfo->chname, proginfo->chnamelen);
				slen += proginfo->chnamelen;
				break;
			case F_ProgName:
				if(param->bItemName) slen = swprintf_s(sstr, CONVBUFSIZE, L"[番組名]\r\n");
				wmemcpy_s(sstr + slen, CONVBUFSIZE - slen, proginfo->pname, proginfo->pnamelen);
				slen += proginfo->pnamelen;
				break;
			case F_ProgDetail:
				if(param->bItemName) slen = swprintf_s(sstr, CONVBUFSIZE, L"[番組内容]\r\n");
				wmemcpy_s(sstr + slen, CONVBUFSIZE - slen, proginfo->pdetail, proginfo->pdetaillen);
				slen += proginfo->pdetaillen;
				break;
			case F_ProgExtend:
				if(param->bItemName) slen = swprintf_s(sstr, CONVBUFSIZE, L"[内容詳細]\r\n");
				if(!proginfo->bSonyRpls){
					slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"n/a");
				} else {
					wmemcpy_s(sstr + slen, CONVBUFSIZE - slen, proginfo->pextend, proginfo->pextendlen);
					slen += proginfo->pextendlen;
				}
				break;
			case F_ProgGenre:
				if(param->bItemName) slen = swprintf_s(sstr, CONVBUFSIZE, L"[番組ジャンル]\r\n");
				if( (!proginfo->bSonyRpls && !proginfo->bPanaRpls) || (proginfo->genretype[0] != 0x01) ) {
					slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"n/a");
				} else {
					slen += putGenreStr(sstr + slen, CONVBUFSIZE - slen, proginfo->genretype, proginfo->genre);
				}
				break;
			default:
				slen = 0;
				break;
		}


		// 出力形式に応じてsstrを調整

		int	dlen =  convforcsv(dstr, CONVBUFSIZE - 4, sstr, slen, param->bNoControl, param->bNoComma, param->bDQuot, param->bDisplay);


		// セパレータに関する処理

		if(param->flags[i + 1] != 0)
		{
			switch(param->separator)
			{
				case S_NORMAL:
				case S_CSV:
					dstr[dlen++] = L',';								// COMMA
					break;
				case S_TAB:
					dstr[dlen++] = 0x0009;								// TAB
					break;
				case S_SPACE:
					dstr[dlen++] = L' ';								// SPACE
					break;
				case S_NEWLINE:
				case S_ITEMNAME:
					if(!param->bDisplay) dstr[dlen++] = 0x000D;			// 改行2回
					dstr[dlen++] = 0x000A;
					if(!param->bDisplay) dstr[dlen++] = 0x000D;
					dstr[dlen++] = 0x000A;
					break;
				default:
					break;
			}
		}
		else
		{
			if(!param->bDisplay) dstr[dlen++] = 0x000D;											// 全項目出力後の改行
			dstr[dlen++] = 0x000A;
			if( (param->separator == S_NEWLINE) || (param->separator == S_ITEMNAME) ) {			// セパレータがS_NEWLINE, S_ITEMNAMEの場合は１行余分に改行する
				if(!param->bDisplay) dstr[dlen++] = 0x000D;
				dstr[dlen++] = 0x000A;
			}
		}


		// データ出力

		if(!param->bDisplay){
			WriteFile(hFile, dstr, dlen * sizeof(WCHAR), &dwNumWrite, NULL);					// ディスク出力
		} else {
			dstr[dlen] = 0x0000;
			wprintf(L"%s", dstr);																// コンソール出力
		}

		i++;
	}

	return;
}


int putGenreStr(WCHAR *buf, int bufsize, int *genretype, int *genre)
{
	WCHAR	*str_genreL[] = {
		L"ニュース／報道",			L"スポーツ",	L"情報／ワイドショー",	L"ドラマ",
		L"音楽",					L"バラエティ",	L"映画",				L"アニメ／特撮",
		L"ドキュメンタリー／教養",	L"劇場／公演",	L"趣味／教育",			L"福祉",
		L"予備",					L"予備",		L"拡張",				L"その他"
	};

	WCHAR	*str_genreM[] = {
		L"定時・総合", L"天気", L"特集・ドキュメント", L"政治・国会", L"経済・市況", L"海外・国際", L"解説", L"討論・会談",
		L"報道特番", L"ローカル・地域", L"交通", L"-", L"-", L"-", L"-", L"その他",

		L"スポーツニュース", L"野球", L"サッカー", L"ゴルフ", L"その他の球技", L"相撲・格闘技", L"オリンピック・国際大会", L"マラソン・陸上・水泳",
		L"モータースポーツ", L"マリン・ウィンタースポーツ", L"競馬・公営競技", L"-", L"-", L"-", L"-", L"その他",

		L"芸能・ワイドショー", L"ファッション", L"暮らし・住まい", L"健康・医療", L"ショッピング・通販", L"グルメ・料理", L"イベント", L"番組紹介・お知らせ",
		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"その他",

		L"国内ドラマ", L"海外ドラマ", L"時代劇", L"-", L"-", L"-", L"-", L"-",
		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"その他",

		L"国内ロック・ポップス", L"海外ロック・ポップス", L"クラシック・オペラ", L"ジャズ・フュージョン", L"歌謡曲・演歌", L"ライブ・コンサート", L"ランキング・リクエスト", L"カラオケ・のど自慢",
		L"民謡・邦楽", L"童謡・キッズ", L"民族音楽・ワールドミュージック", L"-", L"-", L"-", L"-", L"その他",

		L"クイズ", L"ゲーム", L"トークバラエティ", L"お笑い・コメディ", L"音楽バラエティ", L"旅バラエティ", L"料理バラエティ", L"-",
		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"その他",

		L"洋画", L"邦画", L"アニメ", L"-", L"-", L"-", L"-", L"-",
		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"その他",

		L"国内アニメ", L"海外アニメ", L"特撮", L"-", L"-", L"-", L"-", L"-",
		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"その他",

		L"社会・時事", L"歴史・紀行", L"自然・動物・環境", L"宇宙・科学・医学", L"カルチャー・伝統芸能", L"文学・文芸", L"スポーツ", L"ドキュメンタリー全般",
		L"インタビュー・討論", L"-", L"-", L"-", L"-", L"-", L"-", L"その他",

		L"現代劇・新劇", L"ミュージカル", L"ダンス・バレエ", L"落語・演芸", L"歌舞伎・古典", L"-", L"-", L"-",
		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"その他",

		L"旅・釣り・アウトドア", L"園芸・ペット・手芸", L"音楽・美術・工芸", L"囲碁・将棋", L"麻雀・パチンコ", L"車・オートバイ", L"コンピュータ・ＴＶゲーム", L"会話・語学",
		L"幼児・小学生", L"中学生・高校生", L"大学生・受験", L"生涯教育・資格", L"教育問題", L"-", L"-", L"その他",

		L"高齢者", L"障害者", L"社会福祉", L"ボランティア", L"手話", L"文字（字幕）", L"音声解説", L"-",
		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"その他",

		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"-",
		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"-",

		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"-",
		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"-",

		L"BS/地上デジタル放送用番組付属情報", L"広帯域CSデジタル放送用拡張", L"衛星デジタル音声放送用拡張", L"サーバー型番組付属情報", L"-", L"-", L"-", L"-",
		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"-",

		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"-",
		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"その他"
	};

	int len;

	if(genretype[2] == 0x01) {
		len =  swprintf_s(buf, (size_t)bufsize, L"%s 〔%s〕　%s 〔%s〕　%s 〔%s〕", str_genreL[genre[0] >> 4], str_genreM[genre[0]], str_genreL[genre[1] >> 4], str_genreM[genre[1]], str_genreL[genre[2] >> 4], str_genreM[genre[2]]);
	} else if(genretype[1] == 0x01) {
		len =  swprintf_s(buf, (size_t)bufsize, L"%s 〔%s〕　%s 〔%s〕", str_genreL[genre[0] >> 4], str_genreM[genre[0]], str_genreL[genre[1] >> 4], str_genreM[genre[1]]);
	} else {
		len =  swprintf_s(buf, (size_t)bufsize, L"%s 〔%s〕", str_genreL[genre[0] >> 4],  str_genreM[genre[0]]);
	}

	return len;
}


int getRecSrcIndex(int num)
{
	int		recsrc_table[] = {		// 順序はputRecSrcStrと対応する必要あり
		0x5444,						// TD	地上デジタル
		0x4244,						// BD	BSデジタル
		0x4331,						// C1	CSデジタル1
		0x4332,						// C2	CSデジタル2
		0x694C,						// iL	i.LINK(TS)入力
		0x4D56,						// MV	AVCHD
		0x534B,						// SK	スカパー(DLNA)
		0x4456,						// DV	DV入力
		0x5441,						// TA	地上アナログ
		0x4E4C,						// NL	ライン入力
	};

	// 小さなテーブルなので順次探索で

	for(int i = 0; i < (sizeof(recsrc_table) / sizeof(int)); i++) {
		if(num == recsrc_table[i]) return i;
	}

	return	(sizeof(recsrc_table) / sizeof(int));			// 不明なrecsrc
}


int putRecSrcStr(WCHAR *buf, int bufsize, int index)
{
	WCHAR	*str_recsrc[] = {
		L"地上デジタル",
		L"BSデジタル",
		L"CSデジタル1",
		L"CSデジタル2",
		L"i.LINK(TS)",
		L"AVCHD",
		L"スカパー(DLNA)",
		L"DV入力",
		L"地上アナログ",
		L"ライン入力",
		L"unknown",
	};

	int	len = 0;

	if( (index >= 0) && (index < (sizeof(str_recsrc) / sizeof(WCHAR*))) ) {
		len = swprintf_s(buf, (size_t)bufsize, L"%s", str_recsrc[index]);
	}

	return len;
}


int convforcsv(WCHAR *dbuf, int bufsize, WCHAR *sbuf, int slen, BOOL bNoControl, BOOL bNoComma, BOOL bDQuot, BOOL bDisplay)
{
	int dst = 0;

	if( bDQuot && (dst < bufsize) ) dbuf[dst++] = 0x0022;		//  「"」						// CSV用出力なら項目の前後を「"」で囲む

	for(int	src = 0; src < slen; src++)
	{
		WCHAR	s = sbuf[src];
		BOOL	bOut = TRUE;

		if( bNoControl && (s < L' ') ) bOut = FALSE;											// 制御コードは出力しない
		if( bNoComma && (s == L',') ) bOut = FALSE;												// コンマは出力しない
		if( bDisplay && (s == 0x000D) ) bOut = FALSE;											// コンソール出力の場合は改行コードの0x000Dは省略する

		if( bDQuot && (s == 0x0022) && (dst < bufsize) ) dbuf[dst++] = 0x0022;					// CSV用出力なら「"」の前に「"」でエスケープ
		if( bOut && (dst < bufsize) ) dbuf[dst++] = s;											// 出力
	}

	if( bDQuot && (dst < bufsize) ) dbuf[dst++] = 0x0022;		//  「"」

	if(dst < bufsize) dbuf[dst] = 0x0000;

	return dst;
}