#pragma once

#include <string>
#include <atlctrls.h>
#include <atlcrack.h>

class FindBarParent : public CWindowImpl<FindBarParent>
{
public:
    enum {
        IDC_CMB_FINDBAR = 3001,

        WM_DELAYEDITCHANGE = WM_APP + 1,
        WM_DELAYCMBSELCHANGE = WM_APP + 2,
        WM_DELAYRESTOREREBAR = WM_APP + 3,

        ID_FIND_REG_EXP = 200,
        ID_FIND_ONLY_WORD = 201,
        ID_FIND_CASE = 202,

        ID_INCREMENTALSEARCH = 203,
        ID_FIND_AROUND      = 204,

        ID_TABBAR_MCLICK_RESTORETAB = 300,
        ID_FIX_TOOLBARPOSITION = 301,
    };

    FindBarParent();

    void    SubclassWindow(HWND hwnd, HWND editorWindow, UINT toolbarID);
    void    UnsubclassWindow();

    HWND    GetEditWindowHandle() const { return m_edit; }
    HWND    GetEditorWindowHandle() const { return m_editorWindow;  }
    bool    IsTabBarMClickRestoreTab() const { return m_settings.bTabBarMClickRestoreTab; }
    bool    IsShowToolBar() const { return m_settings.bShow;  }
    UINT    GetToolBarID() const { return m_toolbarID;  }

    void    ToggleShowHideToolBar() { m_settings.bShow = !m_settings.bShow; }

    BEGIN_MSG_MAP_EX(FindBarParent)
        //MSG_WM_LBUTTONDOWN(OnLButtonDown)
        MESSAGE_HANDLER_EX(WM_DELAYRESTOREREBAR, OnDelayRestoreRebar)
        COMMAND_HANDLER_EX(IDC_CMB_FINDBAR, CBN_EDITCHANGE, OnEditChange)
        MESSAGE_HANDLER_EX(WM_DELAYEDITCHANGE, OnDelayEditChange)
        COMMAND_HANDLER_EX(IDC_CMB_FINDBAR, CBN_SELENDOK, OnCmbSelChange)
        MESSAGE_HANDLER_EX(WM_DELAYCMBSELCHANGE, OnDelayCmbSelChange)
        MSG_WM_COMMAND(OnCommand)
        MSG_WM_RBUTTONDOWN(OnRButtonDown)
    ALT_MSG_MAP(1)
        MSG_WM_KEYDOWN(OnEditKeydown)
    ALT_MSG_MAP(2)
        MSG_WM_ERASEBKGND(OnCmbListboxErasebkgnd)
    ALT_MSG_MAP(3)
        MSG_WM_RBUTTONDOWN(OnRButtonDown)
    END_MSG_MAP()

    LRESULT OnDelayRestoreRebar(UINT uMsg, WPARAM wParam, LPARAM lParam);
    void OnEditChange(UINT uNotifyCode, int nID, CWindow wndCtl);
    LRESULT OnDelayEditChange(UINT uMsg, WPARAM wParam, LPARAM lParam);
    void OnCmbSelChange(UINT uNotifyCode, int nID, CWindow wndCtl);
    LRESULT OnDelayCmbSelChange(UINT uMsg, WPARAM wParam, LPARAM lParam);

    void OnCommand(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnRButtonDown(UINT nFlags, CPoint point);

    void OnEditKeydown(UINT nChar, UINT nRepCnt, UINT nFlags);

    BOOL OnCmbListboxErasebkgnd(CDCHandle dc);

private:
    void    _LoadSettings();
    void    _SaveSettings();

    void    _SearchComboText(bool bAddHistory = true);

    void    _LoadSearchHistory();
    void    _SaveSearchHistory();
    void    _AddSerchHistory(const std::wstring& text);

    std::wstring    m_lastSerchText;
    HWND        m_editorWindow;
    UINT        m_toolbarID;
    CReBarCtrl	m_rebar;
    CComboBox   m_cmbFindBar;
    CContainedWindowT<CEdit>		m_edit;
    CContainedWindow    m_cmbListbox;
    CContainedWindowT<CToolBarCtrl> m_toolBar;

    struct {
        // SearchBar
        int     nSearchEditWidth = 200;
        int     nMaxSearchHistory = 64;

        bool    bShow = true;

        bool    bFindRegExp = false;
        bool    bFindOnlyWord = false;
        bool    bFindCase = false;
        bool    bIncrementalSearch = true;
        bool    bFindAround = true;

        // Misc
        bool    bTabBarMClickRestoreTab = true;
        bool    bFixToolBarPosition = false;
    } m_settings;

};

