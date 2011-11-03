
#include "stdafx.h"
#include "../client/DCPlusPlus.h"
#include "../client/SettingsManager.h"
#include "../client/SimpleXML.h"

#include "Resource.h"
#include "PropPageTextStyles.h"
#include "WinUtil.h"
#include "OperaColorsPage.h"
#include "PropertiesDlg.h"

PropPage::TextItem PropPageTextStyles::texts[] = {
	{ IDC_AVAILABLE_STYLES, ResourceManager::SETCZDC_STYLES },
	{ IDC_BACK_COLOR, ResourceManager::SETCZDC_BACK_COLOR },
	{ IDC_TEXT_COLOR, ResourceManager::SETCZDC_TEXT_COLOR },
	{ IDC_TEXT_STYLE, ResourceManager::SETCZDC_TEXT_STYLE },
	{ IDC_DEFAULT_STYLES, ResourceManager::SETCZDC_DEFAULT_STYLE },
	{ IDC_BLACK_AND_WHITE, ResourceManager::SETCZDC_BLACK_WHITE },
	{ IDC_CZDC_PREVIEW, ResourceManager::SETCZDC_PREVIEW },
	{ IDC_SELTEXT, ResourceManager::SETTINGS_SELECT_TEXT_FACE },
	{ IDC_RESET_TAB_COLOR, ResourceManager::SETTINGS_RESET },
	{ IDC_SELECT_TAB_COLOR, ResourceManager::SETTINGS_SELECT_COLOR },
	{ IDC_STYLES, ResourceManager::SETTINGS_TEXT_STYLES },
	{ IDC_CHATCOLORS, ResourceManager::SETTINGS_COLORS },
	{ IDC_BLACK, ResourceManager::BLACK_THEME },
	{ IDC_IMPORT, ResourceManager::IMPORT_THEME },
	{ IDC_EXPORT, ResourceManager::EXPORT_THEME },
	{ IDC_UNDERLINE_LINKS, ResourceManager::PROPPAGE_UNDERLINE_LINKS },
	{ IDC_UNDERLINE_DUPES, ResourceManager::PROPPAGE_UNDERLINE_DUPES },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
}; 


PropPageTextStyles::clrs PropPageTextStyles::colours[] = {
	{ResourceManager::SETTINGS_SELECT_WINDOW_COLOR,	SettingsManager::BACKGROUND_COLOR, 0},
	{ResourceManager::SETTINGS_COLOR_ALTERNATE,	SettingsManager::SEARCH_ALTERNATE_COLOUR, 0},
	{ResourceManager::SETCZDC_ERROR_COLOR,	SettingsManager::ERROR_COLOR, 0},
	{ResourceManager::PROGRESS_BACK,	SettingsManager::PROGRESS_BACK_COLOR, 0},
	{ResourceManager::PROGRESS_COMPRESS,	SettingsManager::PROGRESS_COMPRESS_COLOR, 0},
	{ResourceManager::PROGRESS_SEGMENT,	SettingsManager::PROGRESS_SEGMENT_COLOR, 0},
	{ResourceManager::PROGRESS_RUNNING,	SettingsManager::COLOR_RUNNING, 0},
	{ResourceManager::PROGRESS_DOWNLOADED,	SettingsManager::COLOR_DOWNLOADED, 0},
	{ResourceManager::PROGRESS_DONE,	SettingsManager::COLOR_DONE, 0},
};

PropPage::Item PropPageTextStyles::items[] = {	 
         { IDC_UNDERLINE_LINKS, SettingsManager::UNDERLINE_LINKS, PropPage::T_BOOL },
		 { IDC_UNDERLINE_DUPES, SettingsManager::UNDERLINE_DUPES, PropPage::T_BOOL },
         { 0, 0, PropPage::T_END }	 
 };

LRESULT PropPageTextStyles::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);
	PropPage::read((HWND)*this, items);

	m_lsbList.Attach( GetDlgItem(IDC_TEXT_STYLES) );
	m_lsbList.ResetContent();
	m_Preview.Attach( GetDlgItem(IDC_PREVIEW) );

	WinUtil::decodeFont(Text::toT(SETTING(TEXT_FONT)), m_Font);
	m_BackColor = SETTING(BACKGROUND_COLOR);
	m_ForeColor = SETTING(TEXT_COLOR);

	fg = SETTING(TEXT_COLOR);
	bg = SETTING(BACKGROUND_COLOR);

	TextStyles[ TS_GENERAL ].Init( 
	this, settings, STRING(PROPPAGE_GENERAL_TEXT).c_str(), STRING(PROPPAGE_GENERAL_CHAT_TEXT).c_str(),
	SettingsManager::TEXT_GENERAL_BACK_COLOR, SettingsManager::TEXT_GENERAL_FORE_COLOR, 
	SettingsManager::TEXT_GENERAL_BOLD, SettingsManager::TEXT_GENERAL_ITALIC );

	TextStyles[ TS_MYNICK ].Init( 
	this, settings, STRING(PROPPAGE_MY_NICK).c_str(), STRING(PROPPAGE_MY_NICK).c_str(),
	SettingsManager::TEXT_MYNICK_BACK_COLOR, SettingsManager::TEXT_MYNICK_FORE_COLOR, 
	SettingsManager::TEXT_MYNICK_BOLD, SettingsManager::TEXT_MYNICK_ITALIC );

	TextStyles[ TS_MYMSG ].Init( 
	this, settings, STRING(PROPPAGE_MY_OWN_MSG).c_str(), STRING(PROPPAGE_MY_OWN_MSG).c_str(),
	SettingsManager::TEXT_MYOWN_BACK_COLOR, SettingsManager::TEXT_MYOWN_FORE_COLOR, 
	SettingsManager::TEXT_MYOWN_BOLD, SettingsManager::TEXT_MYOWN_ITALIC );

	TextStyles[ TS_PRIVATE ].Init( 
	this, settings, STRING(PROPPAGE_PRIVATE_MSG).c_str(), STRING(PROPPAGE_PRIVATE_MSG).c_str(),
	SettingsManager::TEXT_PRIVATE_BACK_COLOR, SettingsManager::TEXT_PRIVATE_FORE_COLOR, 
	SettingsManager::TEXT_PRIVATE_BOLD, SettingsManager::TEXT_PRIVATE_ITALIC );

	TextStyles[ TS_SYSTEM ].Init( 
	this, settings, STRING(PROPPAGE_SYSTEM_MSG).c_str(), STRING(PROPPAGE_SYSTEM_MSG).c_str(),
	SettingsManager::TEXT_SYSTEM_BACK_COLOR, SettingsManager::TEXT_SYSTEM_FORE_COLOR, 
	SettingsManager::TEXT_SYSTEM_BOLD, SettingsManager::TEXT_SYSTEM_ITALIC );

	TextStyles[ TS_SERVER ].Init( 
	this, settings, STRING(PROPPAGE_SERVER_MSG).c_str(), STRING(PROPPAGE_SERVER_MSG).c_str(),
	SettingsManager::TEXT_SERVER_BACK_COLOR, SettingsManager::TEXT_SERVER_FORE_COLOR, 
	SettingsManager::TEXT_SERVER_BOLD, SettingsManager::TEXT_SERVER_ITALIC );

	TextStyles[ TS_TIMESTAMP ].Init( 
	this, settings, STRING(PROPPAGE_TIMESTAMP).c_str(), STRING(PROPPAGE_STYLE_TIMESTAMP).c_str(),
	SettingsManager::TEXT_TIMESTAMP_BACK_COLOR, SettingsManager::TEXT_TIMESTAMP_FORE_COLOR, 
	SettingsManager::TEXT_TIMESTAMP_BOLD, SettingsManager::TEXT_TIMESTAMP_ITALIC );

	TextStyles[ TS_URL ].Init( 
	this, settings, (STRING(PROPPAGE_URL) + " (http, mailto, ...)").c_str(), STRING(PROPPAGE_URL).c_str(),
	SettingsManager::TEXT_URL_BACK_COLOR, SettingsManager::TEXT_URL_FORE_COLOR, 
	SettingsManager::TEXT_URL_BOLD, SettingsManager::TEXT_URL_ITALIC );

	TextStyles[ TS_SHARE_DUPE ].Init(
	this, settings, STRING(PROPPAGE_SHARE_DUPE_TEXT).c_str(), STRING(PROPPAGE_DUPE_MSG).c_str(),
	SettingsManager::TEXT_DUPE_BACK_COLOR, SettingsManager::DUPE_COLOR, 
	SettingsManager::TEXT_DUPE_BOLD, SettingsManager::TEXT_DUPE_ITALIC );

	TextStyles[ TS_QUEUE_DUPE ].Init(
	this, settings, STRING(PROPPAGE_QUEUE_DUPE_TEXT).c_str(), STRING(PROPPAGE_DUPE_MSG).c_str(),
	SettingsManager::TEXT_QUEUE_BACK_COLOR, SettingsManager::QUEUE_COLOR, 
	SettingsManager::TEXT_QUEUE_BOLD, SettingsManager::TEXT_QUEUE_ITALIC );

	TextStyles[ TS_FAVORITE ].Init( 
	this, settings, STRING(PROPPAGE_FAV_USER).c_str(), STRING(PROPPAGE_FAV_USER).c_str(),
	SettingsManager::TEXT_FAV_BACK_COLOR, SettingsManager::TEXT_FAV_FORE_COLOR, 
	SettingsManager::TEXT_FAV_BOLD, SettingsManager::TEXT_FAV_ITALIC );

	TextStyles[ TS_OP ].Init( 
	this, settings, STRING(PROPPAGE_OPERATOR).c_str(), STRING(PROPPAGE_OPERATOR).c_str(),
	SettingsManager::TEXT_OP_BACK_COLOR, SettingsManager::TEXT_OP_FORE_COLOR, 
	SettingsManager::TEXT_OP_BOLD, SettingsManager::TEXT_OP_ITALIC );

	TextStyles[ TS_NORM ].Init( 
	this, settings, STRING(PROPPAGE_NORM).c_str(), STRING(PROPPAGE_NORM).c_str(),
	SettingsManager::TEXT_NORM_BACK_COLOR, SettingsManager::TEXT_NORM_FORE_COLOR, 
	SettingsManager::TEXT_NORM_BOLD, SettingsManager::TEXT_NORM_ITALIC );

	TextStyles[ TS_LIST_HL ].Init(
	this, settings, STRING(PROPPAGE_LIST_HL).c_str(), STRING(PROPPAGE_LIST_HL_MSG).c_str(),
	SettingsManager::LIST_HL_BG_COLOR, SettingsManager::LIST_HL_COLOR, 
	SettingsManager::LIST_HL_BOLD, SettingsManager::LIST_HL_ITALIC );

	for ( int i = 0; i < TS_LAST; i++ ) {
		TextStyles[ i ].LoadSettings();
		_tcscpy(TextStyles[i].szFaceName, m_Font.lfFaceName );
		TextStyles[ i ].bCharSet = m_Font.lfCharSet;
		TextStyles[ i ].yHeight = m_Font.lfHeight;
		m_lsbList.AddString(Text::toT(TextStyles[i].m_sText).c_str() );
	}
	m_lsbList.SetCurSel( 0 );

	ctrlTabList.Attach(GetDlgItem(IDC_TABCOLOR_LIST));
	cmdResetTab.Attach(GetDlgItem(IDC_RESET_TAB_COLOR));
	cmdSetTabColor.Attach(GetDlgItem(IDC_SELECT_TAB_COLOR));
	ctrlTabExample.Attach(GetDlgItem(IDC_SAMPLE_TAB_COLOR));

	ctrlTabList.ResetContent();
	for(int i=0; i < sizeof(colours) / sizeof(clrs); i++){				
		ctrlTabList.AddString(Text::toT(ResourceManager::getInstance()->getString(colours[i].name)).c_str());
		onResetColor(i);
	}

	setForeColor(ctrlTabExample, GetSysColor(COLOR_3DFACE));	

	ctrlTabList.SetCurSel(0);
	BOOL bTmp;
	onTabListChange(0,0,0,bTmp);

	RefreshPreview();
	return TRUE;
}

void PropPageTextStyles::write()
{
	PropPage::write((HWND)*this, items);

	tstring f = WinUtil::encodeFont(m_Font);
	settings->set(SettingsManager::TEXT_FONT, Text::fromT(f));

	m_BackColor = TextStyles[ TS_GENERAL ].crBackColor;
	m_ForeColor = TextStyles[ TS_GENERAL ].crTextColor;

	settings->set(SettingsManager::TEXT_COLOR, (int)fg);
	settings->set(SettingsManager::BACKGROUND_COLOR, (int)bg);
	
	for(int i=1; i < sizeof(colours) / sizeof(clrs); i++){
		settings->set((SettingsManager::IntSetting)colours[i].setting, (int)colours[i].value);
	}

	for ( int i = 0; i < TS_LAST; i++ ) {
		TextStyles[ i ].SaveSettings();
	}
	WinUtil::initColors();
}

LRESULT PropPageTextStyles::onEditBackColor(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	int iNdx = m_lsbList.GetCurSel();
	TextStyles[ iNdx ].EditBackColor();
	RefreshPreview();
	return TRUE;
}

LRESULT PropPageTextStyles::onEditForeColor(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	int iNdx = m_lsbList.GetCurSel();
	TextStyles[ iNdx ].EditForeColor();
	RefreshPreview();
	return TRUE;
}

LRESULT PropPageTextStyles::onEditTextStyle(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	int iNdx = m_lsbList.GetCurSel();
	TextStyles[ iNdx ].EditTextStyle();

	_tcscpy( m_Font.lfFaceName, TextStyles[ iNdx ].szFaceName );
	m_Font.lfCharSet = TextStyles[ iNdx ].bCharSet;
	m_Font.lfHeight = TextStyles[ iNdx ].yHeight;

	if ( iNdx == TS_GENERAL ) {
		if ( ( TextStyles[ iNdx ].dwEffects & CFE_ITALIC ) == CFE_ITALIC )
			m_Font.lfItalic = true;
		if ( ( TextStyles[ iNdx ].dwEffects & CFE_BOLD ) == CFE_BOLD )
			m_Font.lfWeight = FW_BOLD;
	}

	for ( int i = 0; i < TS_LAST; i++ ) {
		_tcscpy( TextStyles[ iNdx ].szFaceName, m_Font.lfFaceName );
		TextStyles[ i ].bCharSet = m_Font.lfCharSet;
		TextStyles[ i ].yHeight = m_Font.lfHeight;
// TODO		m_Preview.AppendText(_T("My nick"), _T("12:34 "), Text::toT(TextStyles[i].m_sPreviewText).c_str(), TextStyles[i]);
	}

	RefreshPreview();
	return TRUE;
}

void PropPageTextStyles::RefreshPreview() {
	m_Preview.SetBackgroundColor( bg );

	CHARFORMAT2 old = WinUtil::m_TextStyleMyNick;
	CHARFORMAT2 old2 = WinUtil::m_TextStyleTimestamp;
	
	WinUtil::m_TextStyleMyNick = TextStyles[ TS_MYNICK ];
	WinUtil::m_TextStyleTimestamp = TextStyles[ TS_TIMESTAMP ];
	m_Preview.SetWindowText(_T(""));

	string sText;
	Identity id = Identity(NULL, 0);
	for ( int i = 0; i < TS_LAST; i++ ) {
		m_Preview.AppendText(id, _T("My nick"), _T("[12:34] "), Text::toT(TextStyles[i].m_sPreviewText) + _T('\n'), TextStyles[i], false);
	}
	m_Preview.InvalidateRect( NULL );

	WinUtil::m_TextStyleMyNick = old;
	WinUtil::m_TextStyleTimestamp = old2;
}

LRESULT PropPageTextStyles::onDefaultStyles(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	bg = RGB(255, 255, 255);
	fg = RGB(0,0,0);
	::GetObject((HFONT)GetStockObject(DEFAULT_GUI_FONT), sizeof(m_Font), &m_Font);
	TextStyles[ TS_GENERAL ].crBackColor = RGB(255, 255, 255);
	TextStyles[ TS_GENERAL ].crTextColor = RGB(0,0,0);
	TextStyles[ TS_GENERAL ].dwEffects = 0;

	TextStyles[ TS_MYNICK ].crBackColor = RGB(255,255,255);
	TextStyles[ TS_MYNICK ].crTextColor = RGB(0,180,0);
	TextStyles[ TS_MYNICK ].dwEffects = CFE_BOLD;

	TextStyles[ TS_MYMSG ].crBackColor = RGB(255, 255, 255);
	TextStyles[ TS_MYMSG ].crTextColor = RGB(0,0,0);
	TextStyles[ TS_MYMSG ].dwEffects = 0;

	TextStyles[ TS_PRIVATE ].crBackColor = RGB(255, 255, 255);
	TextStyles[ TS_PRIVATE ].crTextColor = RGB(0,0,0);
	TextStyles[ TS_PRIVATE ].dwEffects = 0;

	TextStyles[ TS_SYSTEM ].crBackColor = RGB(255, 255, 255);
	TextStyles[ TS_SYSTEM ].crTextColor = RGB(255, 102, 0);
	TextStyles[ TS_SYSTEM ].dwEffects = CFE_BOLD | CFE_ITALIC;

	TextStyles[ TS_SERVER ].crBackColor = RGB(255, 255, 255);
	TextStyles[ TS_SERVER ].crTextColor = RGB(255, 153, 204);
	TextStyles[ TS_SERVER ].dwEffects = CFE_BOLD;

	TextStyles[ TS_TIMESTAMP ].crBackColor = RGB(255, 255, 255);
	TextStyles[ TS_TIMESTAMP ].crTextColor = RGB(255,0,0);
	TextStyles[ TS_TIMESTAMP ].dwEffects = 0;

	TextStyles[ TS_URL ].crBackColor = RGB(255, 255, 255);
	TextStyles[ TS_URL ].crTextColor = RGB(0,0,255);
	TextStyles[ TS_URL ].dwEffects = 0;

	TextStyles[ TS_SHARE_DUPE ].crBackColor = RGB(255, 255, 255);
	TextStyles[ TS_SHARE_DUPE ].crTextColor = RGB(255,128,255);
	TextStyles[ TS_SHARE_DUPE ].dwEffects = 0;

	TextStyles[ TS_QUEUE_DUPE ].crBackColor = RGB(255, 255, 255);
	TextStyles[ TS_QUEUE_DUPE ].crTextColor = RGB(255,200,0);
	TextStyles[ TS_QUEUE_DUPE ].dwEffects = 0;

	TextStyles[ TS_LIST_HL ].crBackColor = RGB(255, 255, 255);
	TextStyles[ TS_LIST_HL ].crTextColor = RGB(255,189,202);
	TextStyles[ TS_LIST_HL ].dwEffects = 0;

	TextStyles[ TS_FAVORITE ].crBackColor = RGB(255, 255, 255);
	TextStyles[ TS_FAVORITE ].crTextColor = RGB(0,0,0);
	TextStyles[ TS_FAVORITE ].dwEffects = CFE_BOLD | CFE_ITALIC;

	TextStyles[ TS_OP ].crBackColor = RGB(255, 255, 255);
	TextStyles[ TS_OP ].crTextColor = RGB(0,0,0);
	TextStyles[ TS_OP ].dwEffects = CFE_BOLD;

	TextStyles[ TS_NORM ].crBackColor = RGB(255, 255, 255);
	TextStyles[ TS_NORM ].crTextColor = RGB(0,0,0);
	TextStyles[ TS_NORM ].dwEffects = CFE_BOLD;

	
	SettingsManager::getInstance()->set(SettingsManager::NORMAL_COLOUR, (int)RGB(0,0,0));
	SettingsManager::getInstance()->set(SettingsManager::FAVORITE_COLOR, (int)RGB(51,51,255));
	SettingsManager::getInstance()->set(SettingsManager::OP_COLOR, (int)RGB(0,0,205));
	SettingsManager::getInstance()->set(SettingsManager::RESERVED_SLOT_COLOR, (int)RGB(0,51,0));
	SettingsManager::getInstance()->set(SettingsManager::IGNORED_COLOR, (int)RGB(192,192,192));
	SettingsManager::getInstance()->set(SettingsManager::PASIVE_COLOR, (int)RGB(132,132,132));
	PropertiesDlg::needUpdate = true;

	RefreshPreview();
	return TRUE;
}

LRESULT PropPageTextStyles::onBlackAndWhite(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	bg = RGB(255,255,255);
	fg = RGB(0,0,0);
	TextStyles[ TS_GENERAL ].crBackColor = RGB(255,255,255);
	TextStyles[ TS_GENERAL ].crTextColor = RGB(37,60,121);
	TextStyles[ TS_GENERAL ].dwEffects = 0;

	TextStyles[ TS_MYNICK ].crBackColor = RGB(255,255,255);
	TextStyles[ TS_MYNICK ].crTextColor = RGB(37,60,121);
	TextStyles[ TS_MYNICK ].dwEffects = 0;

	TextStyles[ TS_MYMSG ].crBackColor = RGB(255,255,255);
	TextStyles[ TS_MYMSG ].crTextColor = RGB(37,60,121);
	TextStyles[ TS_MYMSG ].dwEffects = 0;

	TextStyles[ TS_PRIVATE ].crBackColor = RGB(255,255,255);
	TextStyles[ TS_PRIVATE ].crTextColor = RGB(37,60,121);
	TextStyles[ TS_PRIVATE ].dwEffects = 0;

	TextStyles[ TS_SYSTEM ].crBackColor = RGB(255,255,255);
	TextStyles[ TS_SYSTEM ].crTextColor = RGB(37,60,121);
	TextStyles[ TS_SYSTEM ].dwEffects = 0;

	TextStyles[ TS_SERVER ].crBackColor = RGB(255,255,255);
	TextStyles[ TS_SERVER ].crTextColor = RGB(37,60,121);
	TextStyles[ TS_SERVER ].dwEffects = 0;

	TextStyles[ TS_TIMESTAMP ].crBackColor = RGB(255,255,255);
	TextStyles[ TS_TIMESTAMP ].crTextColor = RGB(37,60,121);
	TextStyles[ TS_TIMESTAMP ].dwEffects = 0;

	TextStyles[ TS_URL ].crBackColor = RGB(255,255,255);
	TextStyles[ TS_URL ].crTextColor = RGB(37,60,121);
	TextStyles[ TS_URL ].dwEffects = 0;

	TextStyles[ TS_SHARE_DUPE ].crBackColor = RGB(255, 255, 255);
	TextStyles[ TS_SHARE_DUPE ].crTextColor = RGB(255,128,255);
	TextStyles[ TS_SHARE_DUPE ].dwEffects = 0;

	TextStyles[ TS_QUEUE_DUPE ].crBackColor = RGB(255, 255, 255);
	TextStyles[ TS_QUEUE_DUPE ].crTextColor = RGB(255,200,0);
	TextStyles[ TS_QUEUE_DUPE ].dwEffects = 0;

	TextStyles[ TS_LIST_HL ].crBackColor = RGB(255, 255, 255);
	TextStyles[ TS_LIST_HL ].crTextColor = RGB(255,189,202);
	TextStyles[ TS_LIST_HL ].dwEffects = 0;

	TextStyles[ TS_FAVORITE ].crBackColor = RGB(255,255,255);
	TextStyles[ TS_FAVORITE ].crTextColor = RGB(37,60,121);
	TextStyles[ TS_FAVORITE ].dwEffects = 0;

	TextStyles[ TS_OP ].crBackColor = RGB(255,255,255);
	TextStyles[ TS_OP ].crTextColor = RGB(37,60,121);
	TextStyles[ TS_OP ].dwEffects = 0;

	TextStyles[ TS_NORM ].crBackColor = RGB(255,255,255);
	TextStyles[ TS_NORM ].crTextColor = RGB(37,60,121);
	TextStyles[ TS_NORM ].dwEffects = 0;

		
	SettingsManager::getInstance()->set(SettingsManager::NORMAL_COLOUR, (int)RGB(0,0,0));
	SettingsManager::getInstance()->set(SettingsManager::FAVORITE_COLOR, (int)RGB(51,51,255));
	SettingsManager::getInstance()->set(SettingsManager::OP_COLOR, (int)RGB(0,0,205));
	SettingsManager::getInstance()->set(SettingsManager::RESERVED_SLOT_COLOR, (int)RGB(0,51,0));
	SettingsManager::getInstance()->set(SettingsManager::IGNORED_COLOR, (int)RGB(192,192,192));
	SettingsManager::getInstance()->set(SettingsManager::PASIVE_COLOR, (int)RGB(132,132,132));
	PropertiesDlg::needUpdate = true;
	RefreshPreview();
	return TRUE;
}

LRESULT PropPageTextStyles::onBlackTheme(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	bg = 0;
	fg = 12632256;
	TextStyles[ TS_GENERAL ].crBackColor = 0;
	TextStyles[ TS_GENERAL ].crTextColor = 16431749;
	TextStyles[ TS_GENERAL ].dwEffects = 0;

	TextStyles[ TS_MYNICK ].crBackColor = 0;
	TextStyles[ TS_MYNICK ].crTextColor = 12615680;
	TextStyles[ TS_MYNICK ].dwEffects = 0;

	TextStyles[ TS_MYMSG ].crBackColor = 0;
	TextStyles[ TS_MYMSG ].crTextColor = 12615680;
	TextStyles[ TS_MYMSG ].dwEffects = 0;

	TextStyles[ TS_PRIVATE ].crBackColor = 0;
	TextStyles[ TS_PRIVATE ].crTextColor = 16431749;
	TextStyles[ TS_PRIVATE ].dwEffects = 0;

	TextStyles[ TS_SYSTEM ].crBackColor = 0;
	TextStyles[ TS_SYSTEM ].crTextColor = 12632256;
	TextStyles[ TS_SYSTEM ].dwEffects = CFE_BOLD | CFE_ITALIC;

	TextStyles[ TS_SERVER ].crBackColor = 0;
	TextStyles[ TS_SERVER ].crTextColor = 8454016;
	TextStyles[ TS_SERVER ].dwEffects = CFE_BOLD;

	TextStyles[ TS_TIMESTAMP ].crBackColor = 0;
	TextStyles[ TS_TIMESTAMP ].crTextColor = 16431749;
	TextStyles[ TS_TIMESTAMP ].dwEffects = 0;

	TextStyles[ TS_URL ].crBackColor = 0;
	TextStyles[ TS_URL ].crTextColor = RGB(248,248,255);
	TextStyles[ TS_URL ].dwEffects = 0;


	TextStyles[ TS_SHARE_DUPE ].crBackColor = 0;
	TextStyles[ TS_SHARE_DUPE ].crTextColor = RGB(255,128,255);
	TextStyles[ TS_SHARE_DUPE ].dwEffects = 0;

	TextStyles[ TS_QUEUE_DUPE ].crBackColor = 0;
	TextStyles[ TS_QUEUE_DUPE ].crTextColor = RGB(255,200,0);
	TextStyles[ TS_QUEUE_DUPE ].dwEffects = 0;

	TextStyles[ TS_LIST_HL ].crBackColor = 0;
	TextStyles[ TS_LIST_HL ].crTextColor = RGB(255,189,202);
	TextStyles[ TS_LIST_HL ].dwEffects = 0;

	TextStyles[ TS_FAVORITE ].crBackColor = 0;
	TextStyles[ TS_FAVORITE ].crTextColor = RGB(255,165,79);
	TextStyles[ TS_FAVORITE ].dwEffects = 0;

	TextStyles[ TS_OP ].crBackColor = 0;
	TextStyles[ TS_OP ].crTextColor = RGB(50,205,50);
	TextStyles[ TS_OP ].dwEffects = 0;

	TextStyles[ TS_NORM ].crBackColor = 0;
	TextStyles[ TS_NORM ].crTextColor = RGB(100,149,237);
	TextStyles[ TS_NORM ].dwEffects = 0;

	SettingsManager::getInstance()->set(SettingsManager::NORMAL_COLOUR, (int)RGB(255,255,255));
	SettingsManager::getInstance()->set(SettingsManager::FAVORITE_COLOR, (int)RGB(255,165,79));
	SettingsManager::getInstance()->set(SettingsManager::OP_COLOR, (int)RGB(50,205,50));
	SettingsManager::getInstance()->set(SettingsManager::RESERVED_SLOT_COLOR, (int)RGB(193,205,205));
	SettingsManager::getInstance()->set(SettingsManager::IGNORED_COLOR, (int)RGB(205,205,193));
	SettingsManager::getInstance()->set(SettingsManager::PASIVE_COLOR, (int)RGB(141,182,205));
	PropertiesDlg::needUpdate = true;
	RefreshPreview();
	return TRUE;
}


void PropPageTextStyles::TextStyleSettings::Init( 
	PropPageTextStyles *pParent, SettingsManager *pSM, 
	LPCSTR sText, LPCSTR sPreviewText,
	SettingsManager::IntSetting iBack, SettingsManager::IntSetting iFore, 
	SettingsManager::IntSetting iBold, SettingsManager::IntSetting iItalic) {

	cbSize = sizeof(CHARFORMAT2);
	dwMask = CFM_COLOR | CFM_BOLD | CFM_ITALIC | CFM_BACKCOLOR;
	dwReserved = 0;
  
	m_pParent = pParent;
	settings = pSM;      
	m_sText = sText;
	m_sPreviewText = sPreviewText;
	m_iBackColor = iBack;
	m_iForeColor = iFore;
	m_iBold = iBold;
	m_iItalic = iItalic;
}

void PropPageTextStyles::TextStyleSettings::LoadSettings() {
	dwEffects = 0;
	crBackColor = settings->get( m_iBackColor );
	crTextColor = settings->get( m_iForeColor );
	if ( settings->get( m_iBold ) ) dwEffects |= CFE_BOLD;
	if ( settings->get( m_iItalic) ) dwEffects |= CFE_ITALIC;
}

void PropPageTextStyles::TextStyleSettings::SaveSettings() {
	settings->set( m_iBackColor, (int) crBackColor);
	settings->set( m_iForeColor, (int) crTextColor);
	BOOL boBold = ( ( dwEffects & CFE_BOLD ) == CFE_BOLD );
	settings->set( m_iBold, (int) boBold);
	BOOL boItalic = ( ( dwEffects & CFE_ITALIC ) == CFE_ITALIC );
	settings->set( m_iItalic, (int) boItalic);
}

void PropPageTextStyles::TextStyleSettings::EditBackColor() {
	CColorDialog d( crBackColor, 0, *m_pParent );
	if (d.DoModal() == IDOK) {
		crBackColor = d.GetColor();
	}
}

void PropPageTextStyles::TextStyleSettings::EditForeColor() {
	CColorDialog d( crTextColor, 0, *m_pParent );
	if (d.DoModal() == IDOK) {
		crTextColor = d.GetColor();
	}
}

void PropPageTextStyles::TextStyleSettings::EditTextStyle() {
	LOGFONT font;
	WinUtil::decodeFont(Text::toT(SETTING(TEXT_FONT)), font );

	_tcscpy( font.lfFaceName, szFaceName );
	font.lfCharSet = bCharSet;
	font.lfHeight = yHeight;

	if ( dwEffects & CFE_BOLD ) 
		font.lfWeight = FW_BOLD;
	else
		font.lfWeight = FW_REGULAR;

	if ( dwEffects & CFE_ITALIC ) 
		font.lfItalic = true;
	else
		font.lfItalic = false;

	CFontDialog d( &font, CF_EFFECTS | CF_SCREENFONTS, NULL, *m_pParent );
	d.m_cf.rgbColors = crTextColor;
	if (d.DoModal() == IDOK) {
  	_tcscpy( szFaceName, font.lfFaceName );
	bCharSet = font.lfCharSet;
	yHeight = font.lfHeight;

	crTextColor = d.m_cf.rgbColors;
	if ( font.lfWeight == FW_BOLD )
		dwEffects |= CFE_BOLD;
	else
		dwEffects &= ~CFE_BOLD;

	if ( font.lfItalic )
		dwEffects |= CFE_ITALIC;
	else
		dwEffects &= ~CFE_ITALIC;
	}
}

LRESULT PropPageTextStyles::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	m_lsbList.Detach();
	m_Preview.Detach();
	ctrlTabList.Detach();
	cmdResetTab.Detach();
	cmdSetTabColor.Detach();
	ctrlTabExample.Detach();

	return 1;
}

static const TCHAR types[] = _T("DC++ Theme Files\0*.dctheme;\0DC++ Settings Files\0*.xml;\0All Files\0*.*\0");
static const TCHAR defExt[] = _T(".dctheme");

#define importData(x, y) \
		if(xml.findChild(x)) { SettingsManager::getInstance()->set(SettingsManager::y, xml.getChildData());} \
		xml.resetCurrentChild();

LRESULT PropPageTextStyles::onImport(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	tstring x = _T("");	
	if(WinUtil::browseFile(x, m_hWnd, false, x, types, defExt) == IDOK) {
		SimpleXML xml;
		xml.fromXML(File(Text::fromT(x), File::READ, File::OPEN).read());
		xml.resetCurrentChild();
		xml.stepIn();
		if(xml.findChild(("Settings"))) {
			xml.stepIn();

			importData("Font", TEXT_FONT);
			importData("BackgroundColor", BACKGROUND_COLOR);
			importData("TextColor", TEXT_COLOR);
			importData("DownloadBarColor", DOWNLOAD_BAR_COLOR);
			importData("UploadBarColor", UPLOAD_BAR_COLOR);
			importData("TextGeneralBackColor", TEXT_GENERAL_BACK_COLOR);
			importData("TextGeneralForeColor", TEXT_GENERAL_FORE_COLOR);
			importData("TextGeneralBold", TEXT_GENERAL_BOLD);
			importData("TextGeneralItalic", TEXT_GENERAL_ITALIC);
			importData("TextMyOwnBackColor", TEXT_MYOWN_BACK_COLOR);
			importData("TextMyOwnForeColor", TEXT_MYOWN_FORE_COLOR);
			importData("TextMyOwnBold", TEXT_MYOWN_BOLD);
			importData("TextMyOwnItalic", TEXT_MYOWN_ITALIC);
			importData("TextPrivateBackColor", TEXT_PRIVATE_BACK_COLOR);
			importData("TextPrivateForeColor", TEXT_PRIVATE_FORE_COLOR);
			importData("TextPrivateBold", TEXT_PRIVATE_BOLD);
			importData("TextPrivateItalic", TEXT_PRIVATE_ITALIC);
			importData("TextSystemBackColor", TEXT_SYSTEM_BACK_COLOR);
			importData("TextSystemForeColor", TEXT_SYSTEM_FORE_COLOR);
			importData("TextSystemBold", TEXT_SYSTEM_BOLD);
			importData("TextSystemItalic", TEXT_SYSTEM_ITALIC);
			importData("TextServerBackColor", TEXT_SERVER_BACK_COLOR);
			importData("TextServerForeColor", TEXT_SERVER_FORE_COLOR);
			importData("TextServerBold", TEXT_SERVER_BOLD);
			importData("TextServerItalic", TEXT_SERVER_ITALIC);
			importData("TextTimestampBackColor", TEXT_TIMESTAMP_BACK_COLOR);
			importData("TextTimestampForeColor", TEXT_TIMESTAMP_FORE_COLOR);
			importData("TextTimestampBold", TEXT_TIMESTAMP_BOLD);
			importData("TextTimestampItalic", TEXT_TIMESTAMP_ITALIC);
			importData("TextMyNickBackColor", TEXT_MYNICK_BACK_COLOR);
			importData("TextMyNickForeColor", TEXT_MYNICK_FORE_COLOR);
			importData("TextMyNickBold", TEXT_MYNICK_BOLD);
			importData("TextMyNickItalic", TEXT_MYNICK_ITALIC);
			importData("TextFavBackColor", TEXT_FAV_BACK_COLOR);
			importData("TextFavForeColor", TEXT_FAV_FORE_COLOR);
			importData("TextFavBold", TEXT_FAV_BOLD);
			importData("TextFavItalic", TEXT_FAV_ITALIC);
			importData("TextURLBackColor", TEXT_URL_BACK_COLOR);
			importData("TextURLForeColor", TEXT_URL_FORE_COLOR);
			importData("TextURLBold", TEXT_URL_BOLD);
			importData("TextURLItalic", TEXT_URL_ITALIC);
			importData("TextDupeBackColor", TEXT_DUPE_BACK_COLOR);
			importData("TextDupeColor", DUPE_COLOR);
			importData("TextDupeBold", TEXT_DUPE_BOLD);
			importData("TextDupeItalic", TEXT_DUPE_ITALIC);
			importData("ListHighlightBackColor", LIST_HL_BG_COLOR);
			importData("ListHighlightColor", LIST_HL_COLOR);
			importData("ListHighlightBold", LIST_HL_BOLD);
			importData("ListHighlightItalic", LIST_HL_ITALIC);
			importData("ProgressTextDown", PROGRESS_TEXT_COLOR_DOWN);
			importData("ProgressTextUp", PROGRESS_TEXT_COLOR_UP);
			importData("ErrorColor", ERROR_COLOR);
			importData("ProgressOverrideColors", PROGRESS_OVERRIDE_COLORS);
			importData("MenubarTwoColors", MENUBAR_TWO_COLORS);
			importData("MenubarLeftColor", MENUBAR_LEFT_COLOR);
			importData("MenubarRightColor", MENUBAR_RIGHT_COLOR);
			importData("MenubarBumped", MENUBAR_BUMPED);
			importData("Progress3DDepth", PROGRESS_3DDEPTH);
			importData("ProgressOverrideColors2", PROGRESS_OVERRIDE_COLORS2);
			importData("TextOPBackColor", TEXT_OP_BACK_COLOR);
			importData("TextOPForeColor", TEXT_OP_FORE_COLOR);
			importData("TextOPBold", TEXT_OP_BOLD);
			importData("TextOPItalic", TEXT_OP_ITALIC);
			importData("TextNormBackColor", TEXT_NORM_BACK_COLOR);
			importData("TextNormForeColor", TEXT_NORM_FORE_COLOR);
			importData("TextNormBold", TEXT_NORM_BOLD);
			importData("TextNormItalic", TEXT_NORM_ITALIC);
			importData("SearchAlternateColour", SEARCH_ALTERNATE_COLOUR);
			importData("ProgressBackColor", PROGRESS_BACK_COLOR);
			importData("ProgressCompressColor", PROGRESS_COMPRESS_COLOR);
			importData("ProgressSegmentColor", PROGRESS_SEGMENT_COLOR);
			importData("ColorDone", COLOR_DONE);
			importData("ColorDownloaded", COLOR_DOWNLOADED);
			importData("ColorRunning", COLOR_RUNNING);
			importData("ReservedSlotColor", RESERVED_SLOT_COLOR);
			importData("IgnoredColor", IGNORED_COLOR);
			importData("FavoriteColor", FAVORITE_COLOR);
			importData("NormalColour", NORMAL_COLOUR);
			importData("PasiveColor", PASIVE_COLOR);
			importData("OpColor", OP_COLOR);
			importData("ProgressbaroDCStyle", PROGRESSBAR_ODC_STYLE);
			importData("UnderlineLinks", UNDERLINE_LINKS);
			importData("UnderlineDupes", UNDERLINE_DUPES);
		}
		xml.resetCurrentChild();
		xml.stepOut();
	}

	SendMessage(WM_DESTROY,0,0);
	//SettingsManager::getInstance()->save();
	PropertiesDlg::needUpdate = true;
	SendMessage(WM_INITDIALOG,0,0);

//	RefreshPreview();
	return 0;
}

#define exportData(x, y) \
	xml.addTag(x, SETTING(y)); \
	xml.addChildAttrib(type, curType);

LRESULT PropPageTextStyles::onExport(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	tstring x = _T("");	
	if(WinUtil::browseFile(x, m_hWnd, true, x, types, defExt) == IDOK) {
	SimpleXML xml;
	xml.addTag("DCPlusPlus");
	xml.stepIn();
	xml.addTag("Settings");
	xml.stepIn();

	string type("type"), curType("string");
	exportData("Font", TEXT_FONT);

	curType = "int";
	exportData("BackgroundColor", BACKGROUND_COLOR);
	exportData("TextColor", TEXT_COLOR);
	exportData("DownloadBarColor", DOWNLOAD_BAR_COLOR);
	exportData("UploadBarColor", UPLOAD_BAR_COLOR);
	exportData("TextGeneralBackColor", TEXT_GENERAL_BACK_COLOR);
	exportData("TextGeneralForeColor", TEXT_GENERAL_FORE_COLOR);
	exportData("TextGeneralBold", TEXT_GENERAL_BOLD);
	exportData("TextGeneralItalic", TEXT_GENERAL_ITALIC);
	exportData("TextMyOwnBackColor", TEXT_MYOWN_BACK_COLOR);
	exportData("TextMyOwnForeColor", TEXT_MYOWN_FORE_COLOR);
	exportData("TextMyOwnBold", TEXT_MYOWN_BOLD);
	exportData("TextMyOwnItalic", TEXT_MYOWN_ITALIC);
	exportData("TextPrivateBackColor", TEXT_PRIVATE_BACK_COLOR);
	exportData("TextPrivateForeColor", TEXT_PRIVATE_FORE_COLOR);
	exportData("TextPrivateBold", TEXT_PRIVATE_BOLD);
	exportData("TextPrivateItalic", TEXT_PRIVATE_ITALIC);
	exportData("TextSystemBackColor", TEXT_SYSTEM_BACK_COLOR);
	exportData("TextSystemForeColor", TEXT_SYSTEM_FORE_COLOR);
	exportData("TextSystemBold", TEXT_SYSTEM_BOLD);
	exportData("TextSystemItalic", TEXT_SYSTEM_ITALIC);
	exportData("TextServerBackColor", TEXT_SERVER_BACK_COLOR);
	exportData("TextServerForeColor", TEXT_SERVER_FORE_COLOR);
	exportData("TextServerBold", TEXT_SERVER_BOLD);
	exportData("TextServerItalic", TEXT_SERVER_ITALIC);
	exportData("TextTimestampBackColor", TEXT_TIMESTAMP_BACK_COLOR);
	exportData("TextTimestampForeColor", TEXT_TIMESTAMP_FORE_COLOR);
	exportData("TextTimestampBold", TEXT_TIMESTAMP_BOLD);
	exportData("TextTimestampItalic", TEXT_TIMESTAMP_ITALIC);
	exportData("TextMyNickBackColor", TEXT_MYNICK_BACK_COLOR);
	exportData("TextMyNickForeColor", TEXT_MYNICK_FORE_COLOR);
	exportData("TextMyNickBold", TEXT_MYNICK_BOLD);
	exportData("TextMyNickItalic", TEXT_MYNICK_ITALIC);
	exportData("TextFavBackColor", TEXT_FAV_BACK_COLOR);
	exportData("TextFavForeColor", TEXT_FAV_FORE_COLOR);
	exportData("TextFavBold", TEXT_FAV_BOLD);
	exportData("TextFavItalic", TEXT_FAV_ITALIC);
	exportData("TextURLBackColor", TEXT_URL_BACK_COLOR);
	exportData("TextURLForeColor", TEXT_URL_FORE_COLOR);
	exportData("TextURLBold", TEXT_URL_BOLD);
	exportData("TextURLItalic", TEXT_URL_ITALIC);
	exportData("ProgressTextDown", PROGRESS_TEXT_COLOR_DOWN);
	exportData("ProgressTextUp", PROGRESS_TEXT_COLOR_UP);
	exportData("ErrorColor", ERROR_COLOR);
	exportData("ProgressOverrideColors", PROGRESS_OVERRIDE_COLORS);
	exportData("MenubarTwoColors", MENUBAR_TWO_COLORS);
	exportData("MenubarLeftColor", MENUBAR_LEFT_COLOR);
	exportData("MenubarRightColor", MENUBAR_RIGHT_COLOR);
	exportData("MenubarBumped", MENUBAR_BUMPED);
	exportData("Progress3DDepth", PROGRESS_3DDEPTH);
	exportData("ProgressOverrideColors2", PROGRESS_OVERRIDE_COLORS2);
	exportData("TextOPBackColor", TEXT_OP_BACK_COLOR);
	exportData("TextOPForeColor", TEXT_OP_FORE_COLOR);
	exportData("TextOPBold", TEXT_OP_BOLD);
	exportData("TextOPItalic", TEXT_OP_ITALIC);
	exportData("TextNormBackColor", TEXT_NORM_BACK_COLOR);
	exportData("TextNormForeColor", TEXT_NORM_FORE_COLOR);
	exportData("TextNormBold", TEXT_NORM_BOLD);
	exportData("TextNormItalic", TEXT_NORM_ITALIC);
	exportData("SearchAlternateColour", SEARCH_ALTERNATE_COLOUR);
	exportData("ProgressBackColor", PROGRESS_BACK_COLOR);
	exportData("ProgressCompressColor", PROGRESS_COMPRESS_COLOR);
	exportData("ProgressSegmentColor", PROGRESS_SEGMENT_COLOR);
	exportData("ColorDone", COLOR_DONE);
	exportData("ColorDownloaded", COLOR_DOWNLOADED);
	exportData("ColorRunning", COLOR_RUNNING);
	exportData("ReservedSlotColor", RESERVED_SLOT_COLOR);
	exportData("IgnoredColor", IGNORED_COLOR);
	exportData("FavoriteColor", FAVORITE_COLOR);
	exportData("NormalColour", NORMAL_COLOUR);
	exportData("PasiveColor", PASIVE_COLOR);
	exportData("OpColor", OP_COLOR);
	exportData("ProgressbaroDCStyle", PROGRESSBAR_ODC_STYLE);
	exportData("UnderlineLinks", UNDERLINE_LINKS);
	exportData("UnderlineDupes", UNDERLINE_DUPES);
	exportData("TextDupeBackColor", TEXT_DUPE_BACK_COLOR);
	exportData("TextDupeColor", DUPE_COLOR);
	exportData("TextDupeBold", TEXT_DUPE_BOLD);
	exportData("TextDupeItalic", TEXT_DUPE_ITALIC);
	exportData("ListHighlightBackColor", LIST_HL_BG_COLOR);
	exportData("ListHighlightColor", LIST_HL_COLOR);
	exportData("ListHighlightBold", LIST_HL_BOLD);
	exportData("ListHighlightItalic", LIST_HL_ITALIC);
	
	try {
		File ff(Text::fromT(x) , File::WRITE, File::CREATE | File::TRUNCATE);
		BufferedOutputStream<false> f(&ff);
		f.write(SimpleXML::utf8Header);
		xml.toXML(&f);
		f.flush();
		ff.close();
	} catch(const FileException&) {
		// ...
	}

	}
	return 0;
}

void PropPageTextStyles::onResetColor(int i){
	colours[i].value = SettingsManager::getInstance()->get((SettingsManager::IntSetting)colours[i].setting, true);
	setForeColor(ctrlTabExample, colours[i].value);
}

LRESULT PropPageTextStyles::onTabListChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/){
	COLORREF color = colours[ctrlTabList.GetCurSel()].value;
	setForeColor(ctrlTabExample, color);
	RefreshPreview();
	return S_OK;
}

LRESULT PropPageTextStyles::onClickedResetTabColor(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/){
	onResetColor(ctrlTabList.GetCurSel());
	return S_OK;
}

LRESULT PropPageTextStyles::onClientSelectTabColor(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/){
	CColorDialog d(colours[ctrlTabList.GetCurSel()].value, 0, *this);
	if (d.DoModal() == IDOK) {
		colours[ctrlTabList.GetCurSel()].value = d.GetColor();
		switch(ctrlTabList.GetCurSel()) {
			case 0: bg = d.GetColor(); break;
		}
		setForeColor(ctrlTabExample, d.GetColor());
		RefreshPreview();
	}
	return S_OK;
}

LRESULT PropPageTextStyles::onClickedText(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	LOGFONT tmp = m_Font;
	CFontDialog d(&tmp, CF_EFFECTS | CF_SCREENFONTS, NULL, *this);
	d.m_cf.rgbColors = fg;
	if(d.DoModal() == IDOK)
	{
		m_Font = tmp;
		fg = d.GetColor();
	}
	RefreshPreview();
	return TRUE;
}

LRESULT PropPageTextStyles::onCtlColor(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	HWND hWnd = (HWND)lParam;
	if(hWnd == ctrlTabExample.m_hWnd) {
		::SetBkMode((HDC)wParam, TRANSPARENT);
		HANDLE h = GetProp(hWnd, _T("fillcolor"));
		if (h != NULL) {
			return (LRESULT)h;
		}
		return TRUE;
	} else {
		return FALSE;
	}
}
