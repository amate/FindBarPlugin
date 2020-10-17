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
// �v���O�C�������s�������ɌĂяo����܂�
// �p�����[�^
//   hwnd: �E�B���h�E�̃n���h��

EXTERN_C void WINAPI OnCommand(HWND hwnd)
{
	//MessageBox(hwnd, L"Hello World!", L"��{�I�ȃT���v��", MB_OK);
	if (auto activeFindBar = ActiveFindBarParent()) {
		Editor_ToolBarShow(activeFindBar->GetEditorWindowHandle(), activeFindBar->GetToolBarID(), !activeFindBar->IsShowToolBar());
	}
}

// -----------------------------------------------------------------------------
// QueryStatus
//   �v���O�C�������s�\���A�܂��̓`�F�b�N���ꂽ��Ԃ��𒲂ׂ܂�
// �p�����[�^
//   hwnd:      �E�B���h�E�̃n���h��
//   pbChecked: �`�F�b�N�̏��
// �߂�l
//   ���s�\�ł����True��Ԃ��܂�

EXTERN_C BOOL WINAPI QueryStatus(HWND hwnd, LPBOOL pbChecked)
{
	if (auto activeFindBar = ActiveFindBarParent()) {
		*pbChecked = activeFindBar->IsShowToolBar();
	}	
	return true;
}

// -----------------------------------------------------------------------------
// GetMenuTextID
//   ���j���[�ɕ\������e�L�X�g�̃��\�[�X���ʎq���擾���܂�
// �߂�l
//   ���\�[�X���ʎq

EXTERN_C UINT WINAPI GetMenuTextID()
{
	return IDS_MENU_TEXT;
}

// -----------------------------------------------------------------------------
// GetStatusMessageID
//   �c�[���`�b�v�ɕ\������e�L�X�g�̃��\�[�X���ʎq���擾���܂�
// �߂�l
//   ���\�[�X���ʎq

EXTERN_C UINT WINAPI GetStatusMessageID()
{
	return IDS_STATUSMESSAGE;
}

// -----------------------------------------------------------------------------
// GetIconID
//   �c�[���o�[�ɕ\������A�C�R���̃��\�[�X���ʎq���擾���܂�
// �߂�l
//   ���\�[�X���ʎq

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
					pMsg->message = WM_NULL;	// �������͂��A�N�Z�����[�^�ɒD����̂�h��
					return 0;
				} else if (pMsg->message == WM_KEYDOWN) {
					//if ((VK_LEFT <= pMsg->wParam && pMsg->wParam <= VK_DOWN) || pMsg->wParam == VK_DELETE) {
						::TranslateMessage(pMsg);	// WM_CHAR�𔭐������邽�߂ɕK�v
						::SendMessage(pMsg->hwnd, pMsg->message, pMsg->wParam, pMsg->lParam);
						//ATLTRACE(L"%x\n", pMsg->wParam);
						pMsg->message = WM_NULL;	// �\���L�[
						return 0;
					//}
				}
			} else if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN && ::GetKeyState(VK_SHIFT) < 0) {
				// �����_�C�A���O�� Shift+Enter���������Ƃ��� "�O������"�����s����悤�ɂ���
				enum { kSearchEditCtrlID = 0x3E9 };
				const int ctrlID = GetDlgCtrlID(pMsg->hwnd);
				if (ctrlID == kSearchEditCtrlID) {
					CWindow hwndParent = pMsg->hwnd;
					while (hwndParent = ::GetParent(hwndParent)) {
						if (hwndParent.GetStyle() & WS_CAPTION) {
							WCHAR title[64] = L"";
							hwndParent.GetWindowText(title, 64);
							if (CString(L"����") == title) {
								Editor_ExecCommand(g_activeEditorWindow, MEID_SEARCH_PREV);
								return 0;
							}
							break;
						}
					}
				}
			} else if (pMsg->message == WM_MBUTTONDOWN) {
				// �^�u�o�[�̋󂫗̈���~�h���N���b�N�ŁA�����^�u�𕜌�����
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
							ATLTRACE(L"�Ō�ɕ����^�u���J��: %s\n", g_closedFileNameHistory.back().c_str());
							g_closedFileNameHistory.erase(g_closedFileNameHistory.begin() + g_closedFileNameHistory.size() - 1);
						}
						return 0;
					}
				}		
			} else if (pMsg->message == WM_MBUTTONDBLCLK) {
				CTabCtrl tabCtrl = FindTabBarFromCursorPos();
				if (tabCtrl && IsTabBarMClickRestoreTab()) {
					pMsg->message = WM_NULL;	// ���{�^���_�u���N���b�N�𖳌�������
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
		// ���肪�����ꂽ�ꍇ
		for (const auto& pair : mapNameIndex) {
			if (pair.first.front() != L'*') {
				return pair.second;	// * �t���͖�������
			}
		}
		return -1;
	}
	return mapNameIndex.begin()->second;
}

// -----------------------------------------------------------------------------
// OnEvents
//   �C�x���g�������������ɌĂяo����܂�
// �p�����[�^
//   hwnd:   �E�B���h�E�̃n���h��
//   nEvent: �C�x���g�̎��
//   lParam: ���b�Z�[�W���L�̒ǉ����
// ���l
//   EVENT_CREATE:             �G�f�B�^���N��������
//   EVENT_CLOSE:              �G�f�B�^���I��������
//   EVENT_CREATE_FRAME:       �t���[�����쐬���ꂽ��
//   EVENT_CLOSE_FRAME:        �t���[�����j�����ꂽ��
//   EVENT_SET_FOCUS:          �t�H�[�J�X���擾������
//   EVENT_KILL_FOCUS:         �t�H�[�J�X����������
//   EVENT_FILE_OPENED:        �t�@�C�����J������
//   EVENT_FILE_SAVED:         �t�@�C����ۑ�������
//   EVENT_MODIFIED:           �X�V�̏�Ԃ��ύX���ꂽ��
//   EVENT_CARET_MOVED:        �J�[�\�����ړ�������
//   EVENT_SCROLL:             �X�N���[�����ꂽ��
//   EVENT_SEL_CHANGED:        �I��͈͂��ύX���ꂽ��
//   EVENT_CHANGED:            �e�L�X�g���ύX���ꂽ��
//   EVENT_CHAR:               ���������͂��ꂽ��
//   EVENT_MODE_CHANGED:       �ҏW���[�h���ύX���ꂽ��
//   EVENT_DOC_SEL_CHANGED:    �A�N�e�B�u�ȕ������ύX���ꂽ��
//   EVENT_DOC_CLOSE:          �����������
//   EVENT_TAB_MOVED:          �^�u���ړ����ꂽ��
//   EVENT_CUSTOM_BAR_CLOSING: �J�X�^���o�[����悤�Ƃ�����
//   EVENT_CUSTOM_BAR_CLOSED:  �J�X�^���o�[�������
//   EVENT_TOOL_BAR_CLOSED:    �c�[���o�[�������
//   EVENT_TOOL_BAR_SHOW:      �c�[���o�[���\�����ꂽ��
//   EVENT_IDLE:               �A�C�h����

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
	// �Ō�ɕ����^�u�̃t�@�C���p�X��ǐՂ��邽��
	case EVENT_FILE_OPENED:
	{
		ATLTRACE(L"EVENT_FILE_OPENED\n");

		WCHAR docPath[MAX_PATH] = L"";
		Editor_Info(hwnd, MI_GET_FILE_NAME, (LPARAM)docPath);

		// �J���ꂽ�t�@�C���͗�������폜
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
		// �����ɒǉ�
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
//   �v���O�C���ɑ����郁�b�Z�[�W���������܂�
// �p�����[�^
//   hwnd:   �E�B���h�E�̃n���h��
//   nMsg:   ���b�Z�[�W
//   wParam: ���b�Z�[�W���L�̒ǉ����
//   lParam: ���b�Z�[�W���L�̒ǉ����
// �߂�l
//   ���b�Z�[�W�ɂ��قȂ�܂�

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

	case DLL_PROCESS_ATTACH: // DLL���v���Z�X�̃A�h���X��ԂɃ}�b�s���O���ꂽ�B
		g_hInst = hinstDll;
		break;

	case DLL_THREAD_ATTACH: // �X���b�h���쐬����悤�Ƃ��Ă���B
		break;

	case DLL_THREAD_DETACH: // �X���b�h���j������悤�Ƃ��Ă���B
		break;

	case DLL_PROCESS_DETACH: // DLL�̃}�b�s���O����������悤�Ƃ��Ă���B
		break;

	}

	return TRUE;
}