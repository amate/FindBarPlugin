#include "stdafx.h"
#include "plugin.h"
#include <fstream>
#include <string>
#include <vector>
#include <list>
#include <unordered_map>

#include "FindBarParent.h"
#include "Utility/CommonUtility.h"
#include "Utility/json.hpp"

using json = nlohmann::json;

#define IDS_MENU_TEXT 1
#define IDS_STATUSMESSAGE 2
#define IDI_ICON 101

////////////////////////////////////////
// global
HINSTANCE	g_hInst;

HHOOK	g_hKeybordHook;

HWND	g_activeEditorWindow;
std::list<FindBarParent>	g_findBarParentList;

std::vector<std::wstring>	g_tabFileNameList;
std::vector<std::wstring>	g_closedFileNameHistory;

FindBarParent* ActiveFindBarParent()
{
	for (auto it = g_findBarParentList.begin(); it != g_findBarParentList.end(); ++it) {
		auto& findBarParent = *it;
		if (findBarParent.GetEditorWindowHandle() == g_activeEditorWindow) {
			return &findBarParent;
		}
	}
	//ATLASSERT(FALSE);
	return nullptr;
}

// -----------------------------------------------------------------------------
// OnCommand
// プラグインを実行した時に呼び出されます
// パラメータ
//   hwnd: ウィンドウのハンドル

EXTERN_C void WINAPI OnCommand(HWND hwnd)
{
	//MessageBox(hwnd, L"Hello World!", L"基本的なサンプル", MB_OK);
	if (auto activeFindBar = ActiveFindBarParent()) {
		Editor_ToolBarShow(activeFindBar->GetEditorWindowHandle(), activeFindBar->GetToolBarID(), !activeFindBar->IsShowToolBar());
	}
}

// -----------------------------------------------------------------------------
// QueryStatus
//   プラグインが実行可能か、またはチェックされた状態かを調べます
// パラメータ
//   hwnd:      ウィンドウのハンドル
//   pbChecked: チェックの状態
// 戻り値
//   実行可能であればTrueを返します

EXTERN_C BOOL WINAPI QueryStatus(HWND hwnd, LPBOOL pbChecked)
{
	if (auto activeFindBar = ActiveFindBarParent()) {
		*pbChecked = activeFindBar->IsShowToolBar();
	}	
	return true;
}

// -----------------------------------------------------------------------------
// GetMenuTextID
//   メニューに表示するテキストのリソース識別子を取得します
// 戻り値
//   リソース識別子

EXTERN_C UINT WINAPI GetMenuTextID()
{
	return IDS_MENU_TEXT;
}

// -----------------------------------------------------------------------------
// GetStatusMessageID
//   ツールチップに表示するテキストのリソース識別子を取得します
// 戻り値
//   リソース識別子

EXTERN_C UINT WINAPI GetStatusMessageID()
{
	return IDS_STATUSMESSAGE;
}

// -----------------------------------------------------------------------------
// GetIconID
//   ツールバーに表示するアイコンのリソース識別子を取得します
// 戻り値
//   リソース識別子

EXTERN_C UINT WINAPI GetIconID()
{
	return IDI_ICON;
}



CWindow	FindTabBarFromCursorPos()
{
	POINT pt;
	::GetCursorPos(&pt);
	CWindow wndPt = ::WindowFromPoint(pt);
	if (!wndPt.IsWindow()) {
		return NULL;
	}

	auto funcIsTPageControlEx = [](CWindow wnd) -> bool {
		if (!wnd.IsWindow()) {
			return false;
		}
		WCHAR className[64] = L"";
		::GetClassName(wnd, className, 64);
		if (CString(L"TPageControlEx") == className) {
			return true;
		} else {
			return false;
		} 
	};

	if (!funcIsTPageControlEx(wndPt)) {
		wndPt = ::GetWindow(wndPt, GW_CHILD);
		if (!funcIsTPageControlEx(wndPt)) {
			wndPt = ::GetWindow(wndPt, GW_HWNDNEXT);
			if (!funcIsTPageControlEx(wndPt)) {
				return NULL;
			}
		}
	}
	return wndPt;
}

bool IsTabBarMClickRestoreTab()
{
	for (auto it = g_findBarParentList.begin(); it != g_findBarParentList.end(); ++it) {
		auto& findBarParent = *it;
		if (findBarParent.GetEditorWindowHandle() == g_activeEditorWindow) {
			return findBarParent.IsTabBarMClickRestoreTab();
			break;
		}
	}
	return false;
}


LRESULT CALLBACK GetMsgProc(
	_In_ int    nCode,
	_In_ WPARAM wParam,
	_In_ LPARAM lParam
)
{
	if (nCode == HC_ACTION) {
		if (wParam == PM_REMOVE) {
			auto pMsg = (LPMSG)lParam;
			HWND hwndEdit = NULL;
			if (auto activeFindBar = ActiveFindBarParent()) {
				hwndEdit = activeFindBar->GetEditWindowHandle();
			}
			if (pMsg->hwnd == hwndEdit) {
				if (pMsg->message == WM_CHAR) {
					::SendMessage(pMsg->hwnd, pMsg->message, pMsg->wParam, pMsg->lParam);
					//ATLTRACE(L"%x\n", pMsg->wParam);
					pMsg->message = WM_NULL;	// 文字入力がアクセラレータに奪われるのを防ぐ
					return 0;
				} else if (pMsg->message == WM_KEYDOWN) {
					//if ((VK_LEFT <= pMsg->wParam && pMsg->wParam <= VK_DOWN) || pMsg->wParam == VK_DELETE) {
						::TranslateMessage(pMsg);	// WM_CHARを発生させるために必要
						::SendMessage(pMsg->hwnd, pMsg->message, pMsg->wParam, pMsg->lParam);
						//ATLTRACE(L"%x\n", pMsg->wParam);
						pMsg->message = WM_NULL;	// 十字キー
						return 0;
					//}
				}
			} else if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN && ::GetKeyState(VK_SHIFT) < 0) {
				// 検索ダイアログで Shift+Enterを押したときに "前を検索"を実行するようにする
				enum { kSearchEditCtrlID = 0x3E9 };
				const int ctrlID = GetDlgCtrlID(pMsg->hwnd);
				if (ctrlID == kSearchEditCtrlID) {
					CWindow hwndParent = pMsg->hwnd;
					while (hwndParent = ::GetParent(hwndParent)) {
						if (hwndParent.GetStyle() & WS_CAPTION) {
							WCHAR title[64] = L"";
							hwndParent.GetWindowText(title, 64);
							if (CString(L"検索") == title) {
								Editor_ExecCommand(g_activeEditorWindow, MEID_SEARCH_PREV);
								return 0;
							}
							break;
						}
					}
				}
			} else if (pMsg->message == WM_MBUTTONDOWN) {
				// タブバーの空き領域をミドルクリックで、閉じたタブを復元する
				CTabCtrl tabCtrl = FindTabBarFromCursorPos();
				if (tabCtrl && IsTabBarMClickRestoreTab()) {
					POINT pt;
					::GetCursorPos(&pt);
					TCHITTESTINFO tcHittestInfo = {};
					tabCtrl.ScreenToClient(&pt);
					tcHittestInfo.pt = pt;
					int index = tabCtrl.HitTest(&tcHittestInfo);
					if (index == -1) {
						pMsg->message = WM_NULL;	//
						if (g_closedFileNameHistory.size() > 0) {
							WCHAR exePath[MAX_PATH] = L"";
							::GetModuleFileName(NULL, exePath, MAX_PATH);
							std::wstring commandLine = L" \"";
							commandLine += g_closedFileNameHistory.back();
							commandLine += L"\"";
							::ShellExecute(NULL, NULL, exePath, commandLine.c_str(), NULL, SW_NORMAL);
							ATLTRACE(L"最後に閉じたタブを開く: %s\n", g_closedFileNameHistory.back().c_str());
							g_closedFileNameHistory.erase(g_closedFileNameHistory.begin() + g_closedFileNameHistory.size() - 1);
						}
						return 0;
					}
				}		
			} else if (pMsg->message == WM_MBUTTONDBLCLK) {
				CTabCtrl tabCtrl = FindTabBarFromCursorPos();
				if (tabCtrl && IsTabBarMClickRestoreTab()) {
					pMsg->message = WM_NULL;	// 中ボタンダブルクリックを無効化する
					return 0;
				}
			}
		}
	}
	return ::CallNextHookEx(g_hKeybordHook, nCode, wParam, lParam);
}

void	UpdateCurrentTabFileName(HWND hwnd)
{
	int noname = 1;
	g_tabFileNameList.clear();
	int docCount = (int)Editor_Info(hwnd, MI_GET_DOC_COUNT, 0);
	for (int i = 0; i < docCount; ++i) {
		WCHAR docPath[MAX_PATH] = L"";
		Editor_DocInfo(hwnd, i, MI_GET_FILE_NAME, (LPARAM)docPath);
		if (docPath[0] == L'\0') {
			swprintf_s(docPath, L"*%d", noname);
			++noname;
		}
		g_tabFileNameList.push_back(docPath);
	}
}

int		CloseTabIndex(HWND hwnd)
{
	std::unordered_map<std::wstring, int> mapNameIndex;
	const int size = (int)g_tabFileNameList.size();
	for (int i = 0; i < size; ++i) {
		int index = i;
		if (g_tabFileNameList[i].front() == L'*') {
			index = -1;
		}
		mapNameIndex.emplace(g_tabFileNameList[i], index);
	}

	int noname = 1;
	int docCount = (int)Editor_Info(hwnd, MI_GET_DOC_COUNT, 0);
	for (int i = 0; i < docCount; ++i) {
		WCHAR docPath[MAX_PATH] = L"";
		Editor_DocInfo(hwnd, i, MI_GET_FILE_NAME, (LPARAM)docPath);
		if (docPath[0] == L'\0') {
			swprintf_s(docPath, L"*%d", noname);
			++noname;
		}
		mapNameIndex.erase(docPath);
	}

	//ATLASSERT(mapNameIndex.size() == 1);
	if (mapNameIndex.size() != 1) {
		int delCount = size - docCount;
		ATLTRACE(L"\tdelCount: %d\n", delCount);
		// 無題が消された場合
		for (const auto& pair : mapNameIndex) {
			if (pair.first.front() != L'*') {
				return pair.second;	// * 付きは無視する
			}
		}
		return -1;
	}
	return mapNameIndex.begin()->second;
}

// -----------------------------------------------------------------------------
// OnEvents
//   イベントが発生した時に呼び出されます
// パラメータ
//   hwnd:   ウィンドウのハンドル
//   nEvent: イベントの種類
//   lParam: メッセージ特有の追加情報
// 備考
//   EVENT_CREATE:             エディタを起動した時
//   EVENT_CLOSE:              エディタを終了した時
//   EVENT_CREATE_FRAME:       フレームを作成された時
//   EVENT_CLOSE_FRAME:        フレームが破棄された時
//   EVENT_SET_FOCUS:          フォーカスを取得した時
//   EVENT_KILL_FOCUS:         フォーカスを失った時
//   EVENT_FILE_OPENED:        ファイルを開いた時
//   EVENT_FILE_SAVED:         ファイルを保存した時
//   EVENT_MODIFIED:           更新の状態が変更された時
//   EVENT_CARET_MOVED:        カーソルが移動した時
//   EVENT_SCROLL:             スクロールされた時
//   EVENT_SEL_CHANGED:        選択範囲が変更された時
//   EVENT_CHANGED:            テキストが変更された時
//   EVENT_CHAR:               文字が入力された時
//   EVENT_MODE_CHANGED:       編集モードが変更された時
//   EVENT_DOC_SEL_CHANGED:    アクティブな文書が変更された時
//   EVENT_DOC_CLOSE:          文書を閉じた時
//   EVENT_TAB_MOVED:          タブが移動された時
//   EVENT_CUSTOM_BAR_CLOSING: カスタムバーを閉じようとした時
//   EVENT_CUSTOM_BAR_CLOSED:  カスタムバーを閉じた時
//   EVENT_TOOL_BAR_CLOSED:    ツールバーを閉じた時
//   EVENT_TOOL_BAR_SHOW:      ツールバーが表示された時
//   EVENT_IDLE:               アイドル時

EXTERN_C void WINAPI OnEvents(HWND hwnd, UINT nEvent, LPARAM lParam)
{
	switch (nEvent)
	{
	case EVENT_CREATE:
	{
		g_hKeybordHook = ::SetWindowsHookEx(WH_GETMESSAGE, GetMsgProc, NULL, ::GetCurrentThreadId());
	}
	break;

	case EVENT_CLOSE:
	{
		::UnhookWindowsHookEx(g_hKeybordHook);
		g_hKeybordHook = NULL;
	}
	break;

	case EVENT_SET_FOCUS:
	{
		ATLTRACE(L"EVENT_SET_FOCUS: [%x]\n", hwnd);
		g_activeEditorWindow = hwnd;
	}
	break;
#if 1
	case EVENT_CREATE_FRAME:
	{
		ATLTRACE(L"EVENT_CREATE_FRAME\n");
		g_activeEditorWindow = hwnd;

		TOOLBAR_INFO toolbarInfo = { sizeof(toolbarInfo) };
		toolbarInfo.pszTitle = L"Find";
		//toolbarInfo.bVisible = TRUE;
		UINT toolbarID = Editor_ToolBarOpen(hwnd, &toolbarInfo);

		g_findBarParentList.emplace_back();
		g_findBarParentList.back().SubclassWindow(toolbarInfo.hwndToolBar, hwnd, toolbarID);
	}
	break;

	case EVENT_TOOL_BAR_CLOSED:
	{
		ATLTRACE(L"EVENT_TOOL_BAR_CLOSED\n");
		for (auto it = g_findBarParentList.begin(); it != g_findBarParentList.end(); ++it) {
			auto& findBarParent = *it;
			if (findBarParent.GetEditorWindowHandle() == hwnd) {
				findBarParent.UnsubclassWindow();
				g_findBarParentList.erase(it);
				//Editor_ToolBarClose(hwnd, s_toolbarID);
				ATLTRACE(L"\tg_findBarParentList.erase(it);\n");
				break;
			}
		}

	}
	break;

	case EVENT_TOOL_BAR_SHOW:
	{
		ATLTRACE(L"EVENT_TOOL_BAR_SHOW\n");
		if (auto activeFindBar = ActiveFindBarParent()) {
			activeFindBar->ToggleShowHideToolBar();
		}
	}
	break;
#endif
	// ==============================================================
	// 最後に閉じたタブのファイルパスを追跡するため
	case EVENT_FILE_OPENED:
	{
		ATLTRACE(L"EVENT_FILE_OPENED\n");

		WCHAR docPath[MAX_PATH] = L"";
		Editor_Info(hwnd, MI_GET_FILE_NAME, (LPARAM)docPath);

		// 開かれたファイルは履歴から削除
		g_closedFileNameHistory.erase(std::remove(g_closedFileNameHistory.begin(), g_closedFileNameHistory.end(), std::wstring(docPath)), g_closedFileNameHistory.end());

		UpdateCurrentTabFileName(hwnd);
	}
	break;

	case EVENT_FILE_SAVED:
	case EVENT_TAB_MOVED:
	{
		ATLTRACE(L"EVENT_FILE_SAVED or EVENT_TAB_MOVED\n");

		UpdateCurrentTabFileName(hwnd);
	}
	break;

	case EVENT_DOC_CLOSE:
	{
		ATLTRACE(L"EVENT_DOC_CLOSE\n");

		const size_t closeDocIndex = CloseTabIndex(hwnd);
		if (closeDocIndex < 0 || g_tabFileNameList.size() <= closeDocIndex) {
			//ATLASSERT(FALSE);
			ATLTRACE(L"\tcloseDocIndex == -1\n");
			UpdateCurrentTabFileName(hwnd);
			break;
		}
		// 履歴に追加
		g_closedFileNameHistory.push_back(g_tabFileNameList[closeDocIndex]);
		ATLTRACE(L"\tAdd closedFileNameHistory: %s\n", g_tabFileNameList[closeDocIndex].c_str());
		UpdateCurrentTabFileName(hwnd);
	}
	break;
	// ==============================================================

	default:
		break;
	}
}

// -----------------------------------------------------------------------------
// PluginProc
//   プラグインに送られるメッセージを処理します
// パラメータ
//   hwnd:   ウィンドウのハンドル
//   nMsg:   メッセージ
//   wParam: メッセージ特有の追加情報
//   lParam: メッセージ特有の追加情報
// 戻り値
//   メッセージにより異なります

EXTERN_C LRESULT WINAPI PluginProc(HWND hwnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	wchar_t szName[] = L"FindBarPlugin";
	wchar_t szVersion[] = L"Version 1.0";
	switch(nMsg)
	{
	case MP_GET_NAME:
		if(lParam != 0)
			lstrcpynW(LPWSTR(lParam), szName, wParam);
		return wcslen(szName);
	case MP_GET_VERSION:
		if(lParam != 0)
			lstrcpynW(LPWSTR(lParam), szVersion, wParam);
		return wcslen(szVersion);


	case MP_PRE_TRANSLATE_MSG:
	{
		return 0;
	}
	default:
		break;
	}

	return	0;
}

BOOL WINAPI DllMain(HINSTANCE hinstDll, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason) {

	case DLL_PROCESS_ATTACH: // DLLがプロセスのアドレス空間にマッピングされた。
		g_hInst = hinstDll;
		break;

	case DLL_THREAD_ATTACH: // スレッドが作成されようとしている。
		break;

	case DLL_THREAD_DETACH: // スレッドが破棄されようとしている。
		break;

	case DLL_PROCESS_DETACH: // DLLのマッピングが解除されようとしている。
		break;

	}

	return TRUE;
}