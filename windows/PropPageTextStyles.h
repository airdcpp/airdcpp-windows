#ifndef _PROP_PAGE_TEXT_STYLES_H_
#define _PROP_PAGE_TEXT_STYLES_H_

#include <atlcrack.h>
#include "PropPage.h"
#include "../client/ConnectionManager.h"
#include "ChatCtrl.h"
#include "../client/SettingsManager.h"


class PropPageTextStyles: public CPropertyPage<IDD_TEXT_STYLES>, public PropPage
{
public:
	PropPageTextStyles(SettingsManager *s) : PropPage(s) { 
		fg = 0;
		bg = 0;
		title = _tcsdup((TSTRING(SETTINGS_APPEARANCE) + _T('\\') + TSTRING(SETTINGS_TEXT_STYLES)).c_str());
		SetTitle(title);
		m_psp.dwFlags |= PSP_RTLREADING;
	};
	~PropPageTextStyles() {
		free(title);
	};

	BEGIN_MSG_MAP_EX(PropPageTextStyles)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, onCtlColor)	
		COMMAND_HANDLER(IDC_IMPORT, BN_CLICKED, onImport)
		COMMAND_HANDLER(IDC_EXPORT, BN_CLICKED, onExport)
		COMMAND_HANDLER(IDC_BACK_COLOR, BN_CLICKED, onEditBackColor)
		COMMAND_HANDLER(IDC_TEXT_COLOR, BN_CLICKED, onEditForeColor)
		COMMAND_HANDLER(IDC_TEXT_STYLE, BN_CLICKED, onEditTextStyle)
		COMMAND_HANDLER(IDC_DEFAULT_STYLES, BN_CLICKED, onDefaultStyles)
		COMMAND_HANDLER(IDC_ICONS_RESTORE, BN_CLICKED, onRestoreIcons)
		//COMMAND_HANDLER(IDC_SELWINCOLOR, BN_CLICKED, onEditBackground)
		//COMMAND_HANDLER(IDC_ERROR_COLOR, BN_CLICKED, onEditError)
		//COMMAND_HANDLER(IDC_ALTERNATE_COLOR, BN_CLICKED, onEditAlternate)
		COMMAND_ID_HANDLER(IDC_SELTEXT, onClickedText)

		COMMAND_HANDLER(IDC_TABCOLOR_LIST, LBN_SELCHANGE, onTabListChange)
		COMMAND_HANDLER(IDC_THEMES, CBN_SELENDOK , onTheme)
		COMMAND_HANDLER(IDC_RESET_TAB_COLOR, BN_CLICKED, onClickedResetTabColor)
		COMMAND_HANDLER(IDC_SELECT_TAB_COLOR, BN_CLICKED, onClientSelectTabColor)

		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
	END_MSG_MAP()

	LRESULT onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onCtlColor(UINT, WPARAM, LPARAM, BOOL&);
	LRESULT onImport(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onExport(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onEditBackColor(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onEditForeColor(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onEditTextStyle(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onDefaultStyles(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onClickedText(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	//LRESULT onEditBackground(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	//LRESULT onEditError(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	//LRESULT onEditAlternate(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onTabListChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onClickedResetTabColor(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onClientSelectTabColor(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onTheme(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT onRestoreIcons(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);

	void onResetColor(int i);

	void setForeColor(CEdit& cs, const COLORREF& cr) {
		HBRUSH hBr = CreateSolidBrush(cr);
		SetProp(cs.m_hWnd, _T("fillcolor"), hBr);
		cs.Invalidate();
		cs.RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_ERASENOW | RDW_UPDATENOW | RDW_FRAME);
	}


	// Common PropPage interface
	PROPSHEETPAGE *getPSP() { return (PROPSHEETPAGE *)*this; }
	void write();
	
	typedef std::map<string, string> themeMap;
	themeMap themes;
	void LoadTheme(const string& path);

private:
	void RefreshPreview();

	class TextStyleSettings: public CHARFORMAT2 {
	public:
		TextStyleSettings() /*: ChatCtrl()*/ { };
		~TextStyleSettings() { };

	void Init(PropPageTextStyles *pParent, SettingsManager *pSM, 
               LPCSTR sText, LPCSTR sPreviewText,
               SettingsManager::IntSetting iBack, SettingsManager::IntSetting iFore, 
               SettingsManager::IntSetting iBold, SettingsManager::IntSetting iItalic);
	void LoadSettings();
	void SaveSettings();
	void EditBackColor();
	void EditForeColor();
	void EditTextStyle();

	string m_sText;
	string m_sPreviewText;

	PropPageTextStyles *m_pParent;
	SettingsManager *settings;
	SettingsManager::IntSetting m_iBackColor;
	SettingsManager::IntSetting m_iForeColor;
	SettingsManager::IntSetting m_iBold;
	SettingsManager::IntSetting m_iItalic;
};

protected:
	static Item items[];
	static TextItem texts[];
	enum TextStyles {
		TS_GENERAL, TS_MYNICK, TS_MYMSG, TS_PRIVATE, TS_SYSTEM, TS_SERVER, TS_TIMESTAMP, TS_URL, TS_SHARE_DUPE, TS_QUEUE_DUPE, TS_FAVORITE, TS_OP, TS_NORM,
	TS_LAST };

	struct clrs{
		ResourceManager::Strings name;
		int setting;
		COLORREF value;
	};

	static clrs colours[];

	TCHAR* title;
	TextStyleSettings TextStyles[ TS_LAST ];
	CListBox m_lsbList;
	ChatCtrl m_Preview;
	LOGFONT m_Font;
	COLORREF m_BackColor;
	COLORREF m_ForeColor;
	COLORREF fg, bg;//, err, alt;


	CComboBox ctrlTabList;
	CComboBox ctrlTheme;
	CButton cmdResetTab;
	CButton cmdSetTabColor;
	CEdit ctrlTabExample;
	bool fontdirty;
	void PopulateThemes();

};

#endif // _PROP_PAGE_TEXT_STYLES_H_
