#include "stdafx.h"
#include "FindBarParent.h"
#include <memory>

#include "plugin.h"
#include "Utility/CommonUtility.h"
#include "Utility/CodeConvert.h"
#include "Utility/json.hpp"

using json = nlohmann::json;


FindBarParent::FindBarParent() : m_editorWindow(NULL), m_edit(this, 1), m_cmbListbox(this, 2), m_toolBar(this, 3)
{
}

void FindBarParent::SubclassWindow(HWND hwnd, HWND editorWindow, UINT toolbarID)
{
	__super::SubclassWindow(hwnd);

	m_editorWindow = editorWindow;
	m_rebar = ::GetParent(hwnd);
	m_toolbarID = toolbarID;

	// 設定読み込み
	_LoadSettings();

	/* 検索バー作成 */
	m_cmbFindBar.Create(hwnd, CRect(0, 0, m_settings.nSearchEditWidth, 400), NULL,
		WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWN | CBS_AUTOHSCROLL, 0, IDC_CMB_FINDBAR);
	m_cmbFindBar.SetFont(AtlGetDefaultGuiFont());

	COMBOBOXINFO comboBoxInfo = { sizeof(comboBoxInfo) };
	m_cmbFindBar.GetComboBoxInfo(&comboBoxInfo);
	m_edit.SubclassWindow(comboBoxInfo.hwndItem);
	m_cmbListbox.SubclassWindow(comboBoxInfo.hwndList);

	// 検索履歴追加
	_LoadSearchHistory();

	/* ツールバー作成 */
	m_toolBar.Create(hwnd, CRect(m_settings.nSearchEditWidth, 0, 800, 28), nullptr, WS_CHILD | WS_VISIBLE | TBSTYLE_LIST | CCS_NODIVIDER | CCS_NOPARENTALIGN);
	m_toolBar.SetButtonStructSize();
	m_toolBar.AddSeparator();

	TBBUTTON tbBtn = {};
	tbBtn.iBitmap = I_IMAGENONE;
	tbBtn.fsState = TBSTATE_ENABLED;
	tbBtn.fsStyle = BTNS_CHECK | BTNS_SHOWTEXT | BTNS_AUTOSIZE;

	tbBtn.idCommand = ID_FIND_REG_EXP;
	tbBtn.iString = (INT_PTR)L"正規表現";
	m_toolBar.AddButton(&tbBtn);
	m_toolBar.CheckButton(ID_FIND_REG_EXP, m_settings.bFindRegExp);

	tbBtn.idCommand = ID_FIND_ONLY_WORD;
	tbBtn.iString = (INT_PTR)L"単語のみ";
	m_toolBar.AddButton(&tbBtn);
	m_toolBar.CheckButton(ID_FIND_ONLY_WORD, m_settings.bFindOnlyWord);

	tbBtn.idCommand = ID_FIND_CASE;
	tbBtn.iString = (INT_PTR)L"大文字/小文字を区別";
	m_toolBar.AddButton(&tbBtn);
	m_toolBar.CheckButton(ID_FIND_CASE, m_settings.bFindCase);

}

void FindBarParent::UnsubclassWindow()
{
	if (!IsWindow()) {
		return;
	}

	_SaveSearchHistory();

	_SaveSettings();

	m_edit.UnsubclassWindow();
	m_cmbListbox.UnsubclassWindow();

	m_cmbFindBar.DestroyWindow();
	m_toolBar.DestroyWindow();

	__super::UnsubclassWindow();

	// Mery(x64) Version 2.6.7 で落ちるのでコメントアウト
	//Editor_ToolBarClose(m_editorWindow, m_toolbarID);
}

LRESULT FindBarParent::OnDelayRestoreRebar(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	std::ifstream settingStream((GetExeDirectory() / L"FindBarSettings.json").wstring(), std::ios::in | std::ios::binary);
	if (!settingStream) {
		return 0;
	}
	json jsonSettings;
	settingStream >> jsonSettings;

	const int bandCount = (int)m_rebar.GetBandCount();
	const int settingBandCount = (int)jsonSettings["RebarSettings"].size();
	if (bandCount != settingBandCount) {
		// 保存時のバンド数と一致しない
		ATLTRACE(L"保存時のバンド数と一致しない...");
		return 0;
	}

	struct BandInfo {
		UINT	bandID;
		int		bandWidth;
		bool	bandBreak;
	};
	// バンド設定読み込み
	std::vector<BandInfo> vecBandInfo;
	for (const json& bandSetting : jsonSettings["RebarSettings"]) {
		UINT bandID = bandSetting["BandID"].get<UINT>();
		int	bandWidth = bandSetting["BandWidth"].get<int>();
		bool bBreak = bandSetting["BandBreak"].get<bool>();

		vecBandInfo.emplace_back(BandInfo{ bandID, bandWidth, bBreak });
	}

	for (int i = 0; i < bandCount; ++i) {
		REBARBANDINFO rebarBandInfo = { sizeof(rebarBandInfo) };
		rebarBandInfo.fMask = RBBIM_ID | RBBIM_SIZE | RBBIM_STYLE;
		m_rebar.GetBandInfo(i, &rebarBandInfo);
		ATLTRACE(L"band ID: %d , Width: %d , Break: %d\n",
			rebarBandInfo.wID, rebarBandInfo.cx, (rebarBandInfo.fStyle & RBBS_BREAK) != 0);

		const auto& bandInfo = vecBandInfo[i];
		if (bandInfo.bandID != rebarBandInfo.wID) {
			// バンドIDが一致しなければ後ろから同じIDを探して移動させる
			bool found = false;
			for (int k = i + 1; k < bandCount; ++k) {
				REBARBANDINFO rebarBandInfo = { sizeof(rebarBandInfo) };
				rebarBandInfo.fMask = RBBIM_ID | RBBIM_SIZE | RBBIM_STYLE;
				m_rebar.GetBandInfo(k, &rebarBandInfo);
				ATLTRACE(L"\tband ID: %d , Width: %d , Break: %d\n",
					rebarBandInfo.wID, rebarBandInfo.cx, (rebarBandInfo.fStyle & RBBS_BREAK) != 0);
				if (bandInfo.bandID == rebarBandInfo.wID) {
					m_rebar.MoveBand(k, i);
					found = true;
					break;
				}
			}
			if (!found) {
				// 同じIDが見つからなかった...
				ATLASSERT(FALSE);
				return 0;
			}
		}

		m_rebar.GetBandInfo(i, &rebarBandInfo);
		ATLASSERT(bandInfo.bandID == rebarBandInfo.wID);
		ATLTRACE(L"band ID: %d , Width: %d , Break: %d\n",
			rebarBandInfo.wID, rebarBandInfo.cx, (rebarBandInfo.fStyle & RBBS_BREAK) != 0);

		rebarBandInfo.cx = bandInfo.bandWidth;
		if (bandInfo.bandBreak) {
			rebarBandInfo.fStyle |= RBBS_BREAK;
		} else {
			rebarBandInfo.fStyle &= ~RBBS_BREAK;
		}
		ATLTRACE(L"Set band ID: %d , Width: %d , Break: %d\n",
			rebarBandInfo.wID, rebarBandInfo.cx, (rebarBandInfo.fStyle & RBBS_BREAK) != 0);
		m_rebar.SetBandInfo(i, &rebarBandInfo);
	}
	return LRESULT();
}

void FindBarParent::OnEditChange(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	if (!m_settings.bIncrementalSearch) {
		return;
	}
	PostMessage(WM_DELAYEDITCHANGE);	 
}

LRESULT FindBarParent::OnDelayEditChange(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
#if 1
	// 検索文字列を１文字ずつ消していった場合、次に検索が移ってしまうのを防ぐ処理
	bool bEditSelected = Editor_GetSelType(m_editorWindow) == SEL_TYPE_CHAR;
	if (bEditSelected) {
		POINT ptSelStart, ptSelEnd;
		Editor_GetSelStart(m_editorWindow, POS_LOGICAL, &ptSelStart);
		Editor_GetSelEnd(m_editorWindow, POS_LOGICAL, &ptSelEnd);
		// テキストを右から左へ選択した場合と、左から右に選択した場合に、ptSelStartとptSelEndの数値が変わってしまうのを考慮する
		POINT ptNewCaret;
		if (ptSelStart.y == ptSelEnd.y) {
			ptNewCaret.y = ptSelStart.y;
			ptNewCaret.x = min(ptSelStart.x, ptSelEnd.x);
		} else if (ptSelStart.y < ptSelEnd.y) {
			ptNewCaret = ptSelStart;
		} else {
			ptNewCaret = ptSelEnd;
		}
		Editor_SetCaretPos(m_editorWindow, POS_LOGICAL, &ptNewCaret);
	}
#endif

	int length = m_edit.GetWindowTextLengthW();
	if (length == 0) {
		m_lastSerchText.clear();
		Editor_SetSelLength(m_editorWindow, 0);
		Editor_ExecCommand(m_editorWindow, MEID_SEARCH_ERASE_FIND_HIGHLIGHT);
		return 0;
	}

	auto text = std::make_unique<WCHAR[]>(length + 1);
	m_edit.GetWindowTextW(text.get(), length + 1);
	if (m_lastSerchText == text.get()) {
		return 0;
	}
	m_lastSerchText = text.get();

	UINT flags = FLAG_FIND_NEXT;
	if (m_settings.bFindAround) {
		flags |= FLAG_FIND_AROUND;
	}
	if (m_settings.bFindRegExp) {
		flags |= FLAG_FIND_REG_EX;
	}
	if (m_settings.bFindOnlyWord) {
		flags |= FLAG_FIND_WHOLE_WORD;
	}
	if (m_settings.bFindCase) {
		flags |= FLAG_FIND_MATCH_CASE;
	}
	BOOL ret = Editor_Find(m_editorWindow, flags, text.get());
	
	return 0;
}

void FindBarParent::OnCmbSelChange(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	PostMessage(WM_DELAYCMBSELCHANGE);
}

LRESULT FindBarParent::OnDelayCmbSelChange(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	_SearchComboText(false);
	return 0;
}

void FindBarParent::OnCommand(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	switch (nID) {
	case ID_FIND_REG_EXP:
		m_settings.bFindRegExp = m_toolBar.IsButtonChecked(ID_FIND_REG_EXP) != 0;
		break;
	case ID_FIND_ONLY_WORD:
		m_settings.bFindOnlyWord = m_toolBar.IsButtonChecked(ID_FIND_ONLY_WORD) != 0;
		break;
	case ID_FIND_CASE:
		m_settings.bFindCase = m_toolBar.IsButtonChecked(ID_FIND_CASE) != 0;
		break;

	default:
		break;
	}
}

void FindBarParent::OnRButtonDown(UINT /*nFlags*/, CPoint point)
{
	CMenu menu;
	menu.CreatePopupMenu();

	UINT nFlags = 0;

	nFlags = m_settings.bIncrementalSearch ? MF_CHECKED : MF_STRING;
	menu.AppendMenu(nFlags, (UINT_PTR)ID_INCREMENTALSEARCH, L"インクリメンタルサーチ");
	nFlags = m_settings.bFindAround ? MF_CHECKED : MF_STRING;
	menu.AppendMenu(nFlags, (UINT_PTR)ID_FIND_AROUND, L"文末まで検索したら文頭に移動する");

	menu.AppendMenu(MF_SEPARATOR);

	nFlags = m_settings.bTabBarMClickRestoreTab ? MF_CHECKED : MF_STRING;
	menu.AppendMenu(nFlags, (UINT_PTR)ID_TABBAR_MCLICK_RESTORETAB, L"タブバーの空き領域をミドルクリックで、閉じたタブを復元する");
	
	menu.AppendMenu(MF_SEPARATOR);
	
	nFlags = m_settings.bFixToolBarPosition ? MF_CHECKED : MF_STRING;
	menu.AppendMenu(nFlags, (UINT_PTR)ID_FIX_TOOLBARPOSITION, L"独自にツールバーの位置を記録/復元する");


	POINT pt;
	::GetCursorPos(&pt);
	BOOL ret = menu.TrackPopupMenu(TPM_RETURNCMD, pt.x, pt.y, m_hWnd);
	switch (ret) {
	case ID_INCREMENTALSEARCH:
		m_settings.bIncrementalSearch = !m_settings.bIncrementalSearch;
		break;

	case ID_FIND_AROUND:
		m_settings.bFindAround = !m_settings.bFindAround;
		break;

	case ID_TABBAR_MCLICK_RESTORETAB:
		m_settings.bTabBarMClickRestoreTab = !m_settings.bTabBarMClickRestoreTab;
		break;

	case ID_FIX_TOOLBARPOSITION:
		m_settings.bFixToolBarPosition = !m_settings.bFixToolBarPosition;
		break;

	default:
		break;
	}
}

void FindBarParent::OnEditKeydown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	SetMsgHandled(FALSE);
	if (nChar == VK_RETURN) {
		_SearchComboText();
	}
}

// コンボボックスのリストボックスの背景が真っ黒になっているのを修正
BOOL FindBarParent::OnCmbListboxErasebkgnd(CDCHandle dc)
{
	CRect rc;
	m_cmbListbox.GetClientRect(&rc);
	dc.FillSolidRect(&rc, RGB(0xFF, 0xFF, 0xFF));
	return 0;
}

void FindBarParent::_LoadSettings()
{
	std::ifstream settingStream((GetExeDirectory() / L"FindBarSettings.json").wstring(), std::ios::in | std::ios::binary);
	if (!settingStream) {
		return;
	}
	json jsonSettings;
	settingStream >> jsonSettings;

	const json& jsonSearchBar = jsonSettings["SearchBar"];
	if (jsonSearchBar.is_object()) {
		m_settings.nSearchEditWidth = jsonSearchBar.value<int>("SearchEditWidth", m_settings.nSearchEditWidth);
		m_settings.nMaxSearchHistory = jsonSearchBar.value<int>("MaxSearchHistory", m_settings.nMaxSearchHistory);

		m_settings.bShow = jsonSearchBar.value<bool>("Show", m_settings.bShow);

		m_settings.bFindRegExp = jsonSearchBar.value<bool>("FindRegExp", m_settings.bFindRegExp);
		m_settings.bFindOnlyWord = jsonSearchBar.value<bool>("FindOnlyWord", m_settings.bFindOnlyWord);
		m_settings.bFindCase = jsonSearchBar.value<bool>("FindCase", m_settings.bFindCase);
		m_settings.bIncrementalSearch = jsonSearchBar.value<bool>("IncrementalSearch", m_settings.bIncrementalSearch);
		m_settings.bFindAround = jsonSearchBar.value<bool>("FindAround", m_settings.bFindAround);
	}
	const json& jsonMisc = jsonSettings["Misc"];
	if (jsonMisc.is_object()) {
		m_settings.bTabBarMClickRestoreTab = jsonMisc.value<bool>("TabBarMClickRestoreTab", m_settings.bTabBarMClickRestoreTab);
		m_settings.bFixToolBarPosition = jsonMisc.value<bool>("FixToolBarPosition", m_settings.bFixToolBarPosition);
	}

	// バンド位置復元
	if (m_settings.bShow) {
		m_settings.bShow = false;	// コールバックで有効にされるので一時的に無効にしておく
		Editor_ToolBarShow(m_editorWindow, m_toolbarID, TRUE);	// 最初に表示すること！！！

		if (m_settings.bFixToolBarPosition) {
			PostMessage(WM_DELAYRESTOREREBAR);
		}
	}

}

void FindBarParent::_SaveSettings()
{
	json jsonSettings;

	std::ifstream fs((GetExeDirectory() / L"FindBarSettings.json").wstring(), std::ios::in | std::ios::binary);
	if (fs) {
		fs >> jsonSettings;
		fs.close();
	}

	jsonSettings["SearchBar"]["Show"] = m_settings.bShow;

	jsonSettings["SearchBar"]["FindRegExp"] = m_settings.bFindRegExp;
	jsonSettings["SearchBar"]["FindOnlyWord"] = m_settings.bFindOnlyWord;
	jsonSettings["SearchBar"]["FindCase"] = m_settings.bFindCase;
	jsonSettings["SearchBar"]["IncrementalSearch"] = m_settings.bIncrementalSearch;
	jsonSettings["SearchBar"]["FindAround"] = m_settings.bFindAround;

	jsonSettings["Misc"]["TabBarMClickRestoreTab"] = m_settings.bTabBarMClickRestoreTab;
	jsonSettings["Misc"]["FixToolBarPosition"] = m_settings.bFixToolBarPosition;

	// バンド位置の設定保存
	json jsonRebarSettings;

	UINT bandCount = m_rebar.GetBandCount();
	for (UINT i = 0; i < bandCount; ++i) {
		REBARBANDINFO rebarBandInfo = { sizeof(rebarBandInfo) };
		rebarBandInfo.fMask = RBBIM_ID | RBBIM_SIZE | RBBIM_STYLE;
		m_rebar.GetBandInfo(static_cast<int>(i), &rebarBandInfo);
		ATLTRACE(L"band ID: %d , Width: %d , Break: %d\n",
			rebarBandInfo.wID, rebarBandInfo.cx, (rebarBandInfo.fStyle & RBBS_BREAK) != 0);

		bool bBreak = (rebarBandInfo.fStyle & RBBS_BREAK) != 0;
		bool bHide = (rebarBandInfo.fStyle & RBBS_HIDDEN) != 0;
		
		jsonRebarSettings.push_back(json{
			{"BandID", rebarBandInfo.wID },
			{"BandWidth", rebarBandInfo.cx },
			{"BandBreak", bBreak }
			});
	}
	jsonSettings["RebarSettings"] = jsonRebarSettings;

	std::ofstream settingStream((GetExeDirectory() / L"FindBarSettings.json").wstring(), std::ios::out | std::ios::binary);
	ATLASSERT(settingStream);
	settingStream << jsonSettings.dump(4);
}

void FindBarParent::_SearchComboText(bool bAddHistory)
{
	int length = m_edit.GetWindowTextLengthW();
	if (length == 0) {
		Editor_ExecCommand(m_editorWindow, MEID_SEARCH_ERASE_FIND_HIGHLIGHT);
		return;
	}

	auto text = std::make_unique<WCHAR[]>(length + 1);
	m_edit.GetWindowTextW(text.get(), length + 1);

	if (bAddHistory) {
		_AddSerchHistory(text.get());
	}

	bool bShift = GetKeyState(VK_SHIFT) < 0;
	UINT flags = 0;
	if (!bShift) {
		flags |= FLAG_FIND_NEXT;
	}
	if (m_settings.bFindAround) {
		flags |= FLAG_FIND_AROUND;
	}
	if (m_settings.bFindRegExp) {
		flags |= FLAG_FIND_REG_EX;
	}
	if (m_settings.bFindOnlyWord) {
		flags |= FLAG_FIND_WHOLE_WORD;
	}
	if (m_settings.bFindCase) {
		flags |= FLAG_FIND_MATCH_CASE;
	}
	BOOL ret = Editor_Find(m_editorWindow, flags, text.get());
}

void FindBarParent::_LoadSearchHistory()
{
	std::ifstream fs((GetExeDirectory() / L"FindBarSearchHistory.json").wstring(), std::ios::in | std::ios::binary);
	if (!fs) {
		return;
	}
	json jsonHistory;
	jsonHistory << fs;
	for (const json& historyItem : jsonHistory["SearchHistory"]) {
		m_cmbFindBar.AddString(CodeConvert::UTF16fromUTF8(historyItem.get<std::string>()).c_str());
	}
}

void FindBarParent::_SaveSearchHistory()
{
	json jsonHistory;
	json& jsonSearchHistory = jsonHistory["SearchHistory"];
	const int count = m_cmbFindBar.GetCount();
	for (int i = 0; i < count; ++i) {
		CString historyText;
		m_cmbFindBar.GetLBText(i, historyText);
		jsonSearchHistory.push_back(CodeConvert::UTF8fromUTF16((LPCWSTR)historyText));
	}

	std::ofstream fs((GetExeDirectory() / L"FindBarSearchHistory.json").wstring(), std::ios::out | std::ios::binary);
	ATLASSERT(fs);
	fs << jsonHistory.dump(4);
}

void FindBarParent::_AddSerchHistory(const std::wstring& text)
{
	m_cmbFindBar.InsertString(0, text.c_str());
	m_cmbFindBar.SetCurSel(0);

	const int count = m_cmbFindBar.GetCount();
	for (int i = 1; i < count; ++i) {
		CString historyText;
		m_cmbFindBar.GetLBText(i, historyText);
		if (historyText == text.c_str()) {
			m_cmbFindBar.DeleteString(i);
			return;
		}
	}

	while (m_settings.nMaxSearchHistory < m_cmbFindBar.GetCount()) {
		m_cmbFindBar.DeleteString(m_cmbFindBar.GetCount() - 1);
	}
}
