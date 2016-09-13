/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "stdafx.h"
#include <airdcpp/HighlightManager.h>
#include <airdcpp/SettingsManager.h>
#include <airdcpp/SimpleXML.h>
#include <airdcpp/version.h>

#include "BrowseDlg.h"
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
	{ IDC_CZDC_PREVIEW, ResourceManager::PREVIEW },
	{ IDC_SELTEXT, ResourceManager::SETTINGS_SELECT_TEXT_FACE },
	{ IDC_RESET_TAB_COLOR, ResourceManager::SETTINGS_RESET },
	{ IDC_SELECT_TAB_COLOR, ResourceManager::SETTINGS_SELECT_COLOR },
	{ IDC_STYLES, ResourceManager::COLORS_AND_TEXT_STYLES },
	{ IDC_CHATCOLORS, ResourceManager::SETTINGS_COLORS },
	{ IDC_IMPORT, ResourceManager::IMPORT_THEME },
	{ IDC_EXPORT, ResourceManager::EXPORT_THEME },
	{ IDC_UNDERLINE_LINKS, ResourceManager::PROPPAGE_UNDERLINE_LINKS },
	{ IDC_UNDERLINE_DUPES, ResourceManager::PROPPAGE_UNDERLINE_DUPES },
	{ IDC_THEME_TEXT, ResourceManager::THEME_TEXT },
	{ IDC_ICONS_RESTORE, ResourceManager::ICONS_DEFAULT },
	{ 0, ResourceManager::LAST }
}; 

PropPageTextStyles::clrs PropPageTextStyles::colours[] = {
	{ResourceManager::SETTINGS_SELECT_WINDOW_COLOR,	SettingsManager::BACKGROUND_COLOR, 0},
	{ResourceManager::SETTINGS_COLOR_ALTERNATE,	SettingsManager::SEARCH_ALTERNATE_COLOUR, 0},
	{ResourceManager::SETCZDC_ERROR_COLOR,	SettingsManager::ERROR_COLOR, 0},
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
	
	SettingsManager::getInstance()->addListener(this);

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

	TextStyles[ TS_DUPE_SHARE ].Init(
	this, settings, STRING(PROPPAGE_DUPE_SHARE_TEXT).c_str(), STRING(PROPPAGE_DUPE_MSG).c_str(),
	SettingsManager::TEXT_DUPE_BACK_COLOR, SettingsManager::DUPE_COLOR, 
	SettingsManager::TEXT_DUPE_BOLD, SettingsManager::TEXT_DUPE_ITALIC );

	TextStyles[ TS_DUPE_QUEUE ].Init(
	this, settings, STRING(PROPPAGE_DUPE_QUEUE_TEXT).c_str(), STRING(PROPPAGE_DUPE_MSG).c_str(),
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
	ctrlTheme.Attach(GetDlgItem(IDC_THEMES));
	
	PopulateThemes();

	ctrlTabList.ResetContent();
	for(int i=0; i < sizeof(colours) / sizeof(clrs); i++){				
		ctrlTabList.AddString(Text::toT(ResourceManager::getInstance()->getString(colours[i].name)).c_str());
		onResetColor(i);
	}

	setForeColor(ctrlTabExample, GetSysColor(COLOR_3DFACE));	

	ctrlTabList.SetCurSel(0);
	BOOL bTmp;
	onTabListChange(0,0,0,bTmp);
	fontdirty = false;

	RefreshPreview();

	return TRUE;
}

void PropPageTextStyles::PopulateThemes() {
	ctrlTheme.ResetContent();
	
	if(themes.empty()) {
		string path = Util::getPath(Util::PATH_THEMES);
		for(FileFindIter i(path, "*.dctheme"); i != FileFindIter(); ++i) {
			string filepath = path + i->getFileName();
			themes.emplace(i->getFileName(), filepath);
		}
	}
	
	ctrlTheme.AddString(CTSTRING(SELECT_THEME));
	
	for(themeMap::const_iterator m = themes.begin(); m != themes.end(); ++m) {
		ctrlTheme.AddString(Text::toT(m->first).c_str());
	}

	ctrlTheme.SetCurSel(0);
}
LRESULT PropPageTextStyles::onTheme(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	string sel;
	TCHAR buf[128];
	ctrlTheme.GetLBText(ctrlTheme.GetCurSel(), buf);

	sel = Text::fromT(buf);
	themeMap::iterator i = themes.find(sel);
	if( i != themes.end() )
		LoadTheme(i->second);

	return 0;
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
	if(fontdirty) {
		WinUtil::setFonts();
		::SendMessage(WinUtil::mainWnd, IDC_SET_FONTS, 0, 0);
	}

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
	fontdirty = true;
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

	//string sText;
	Identity id = Identity(NULL, 0);
	for ( int i = 0; i < TS_LAST; i++ ) {
		m_Preview.AppendChat(id, _T("My nick"), _T("[12:34] "), Text::toT(TextStyles[i].m_sPreviewText) + _T('\n'), TextStyles[i], false);
	}
	m_Preview.InvalidateRect( NULL );

	WinUtil::m_TextStyleMyNick = old;
	WinUtil::m_TextStyleTimestamp = old2;
}

void PropPageTextStyles::TextStyleSettings::Init( 
	PropPageTextStyles *pParent, SettingsManager *pSM, 
	LPCSTR sText, LPCSTR sPreviewText,
	SettingsManager::IntSetting iBack, SettingsManager::IntSetting iFore, 
	SettingsManager::BoolSetting iBold, SettingsManager::BoolSetting iItalic) {

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
	bool boBold = ( ( dwEffects & CFE_BOLD ) == CFE_BOLD );
	settings->set( m_iBold, boBold);
	bool boItalic = ( ( dwEffects & CFE_ITALIC ) == CFE_ITALIC );
	settings->set( m_iItalic, boItalic);
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
	ctrlTheme.Detach();
	SettingsManager::getInstance()->removeListener(this);

	return 1;
}

static const COMDLG_FILTERSPEC types [] = {
	{ _T("DC++ Theme Files"), _T("*.dctheme") },
	{ _T("DC++ Settings Files"), _T("*.xml") },
	{ _T("All Files"), _T("*.*") }
};

//static const TCHAR defExt[] = _T(".dctheme");

#define importData(x, y) \
		if(xml.findChild(x)) { SettingsManager::getInstance()->set(SettingsManager::y, xml.getChildData());} \
		xml.resetCurrentChild();


void PropPageTextStyles::LoadTheme(const string& path, bool silent/* = false*/) {

	//backup the old theme for cancelling
	if(initial){
		SaveTheme(Util::getPath(Util::PATH_THEMES) + "backup.dctheme", true);
		initial = false;
	}
		
	SimpleXML xml;
	try {
		xml.fromXML(File(path, File::READ, File::OPEN, File::BUFFER_SEQUENTIAL, false).read());
	} catch(...) {
		return;
	}
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
		importData("ProgressSegmentColor", PROGRESS_SEGMENT_COLOR);
		importData("ColorDone", COLOR_DONE);
		importData("ReservedSlotColor", RESERVED_SLOT_COLOR);
		importData("IgnoredColor", IGNORED_COLOR);
		importData("FavoriteColor", FAVORITE_COLOR);
		importData("NormalColour", NORMAL_COLOUR);
		importData("PasiveColor", PASIVE_COLOR);
		importData("OpColor", OP_COLOR);
		importData("ProgressbaroDCStyle", PROGRESSBAR_ODC_STYLE);
		importData("UnderlineLinks", UNDERLINE_LINKS);
		importData("UnderlineDupes", UNDERLINE_DUPES);
		importData("TextQueueBackColor", TEXT_QUEUE_BACK_COLOR);
		importData("QueueColor", QUEUE_COLOR);
		importData("TextQueueBold", TEXT_QUEUE_BOLD);
		importData("TextQueueItalic", TEXT_QUEUE_ITALIC);
		importData("UnderlineQueue", UNDERLINE_QUEUE);

		//tabs
		importData("tabactivebg", TAB_ACTIVE_BG);
		importData("TabActiveText", TAB_ACTIVE_TEXT);
		importData("TabActiveBorder", TAB_ACTIVE_BORDER);
		importData("TabInactiveBg", TAB_INACTIVE_BG);
		importData("TabInactiveBgDisconnected", TAB_INACTIVE_BG_DISCONNECTED);
		importData("TabInactiveText", TAB_INACTIVE_TEXT);
		importData("TabInactiveBorder", TAB_INACTIVE_BORDER);
		importData("TabInactiveBgNotify", TAB_INACTIVE_BG_NOTIFY);
		importData("TabDirtyBlend", TAB_DIRTY_BLEND);
		importData("BlendTabs", BLEND_TABS);
		importData("TabSize", TAB_SIZE);

	}
	xml.stepOut();

	if(xml.findChild("Icons")) {
		if (silent || MessageBox(CTSTRING(ICONS_IN_THEME), Text::toT(shortVersionString).c_str(), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES) {
			xml.stepIn();
			importData("IconPath", ICON_PATH);
			xml.resetCurrentChild();
			xml.stepOut();
		}
	}
	xml.resetCurrentChild();
	if(xml.findChild("Highlights")) {
		if (silent || MessageBox(CTSTRING(HIGHLIGHTS_IN_THEME), Text::toT(shortVersionString).c_str(), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES) {
			HighlightManager::getInstance()->clearList();
			HighlightManager::getInstance()->load(xml);
		}
	}
	xml.resetCurrentChild();
	
	if(!silent)
		SettingsManager::getInstance()->reloadPages();
	
	fontdirty = true;

}
LRESULT PropPageTextStyles::onRestoreIcons(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/){
	if (MessageBox(CTSTRING(ICONS_RESTORE), Text::toT(shortVersionString).c_str(), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES) {
		SettingsManager::getInstance()->set(SettingsManager::ICON_PATH, "");
	}
	return 0;
}
LRESULT PropPageTextStyles::onImport(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	tstring x;

	BrowseDlg dlg(m_hWnd, BrowseDlg::TYPE_SETTINGS_RESOURCES, BrowseDlg::DIALOG_OPEN_FILE);
	dlg.setTypes(3, types);
	if (dlg.show(x)) {
		LoadTheme(Text::fromT(x));
	}
	return 0;
}

#define exportData(x, y) \
	xml.addTag(x, SETTING(y)); \
	xml.addChildAttrib(type, curType);

void PropPageTextStyles::SaveTheme(const string& path, bool backup) {
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
	exportData("ProgressSegmentColor", PROGRESS_SEGMENT_COLOR);
	exportData("ColorDone", COLOR_DONE);
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
	exportData("TextQueueBackColor", TEXT_QUEUE_BACK_COLOR);
	exportData("QueueColor", QUEUE_COLOR);
	exportData("TextQueueBold", TEXT_QUEUE_BOLD);
	exportData("TextQueueItalic", TEXT_QUEUE_ITALIC);
	exportData("UnderlineQueue", UNDERLINE_QUEUE);
	//tabs
	exportData("tabactivebg", TAB_ACTIVE_BG);
	exportData("TabActiveText", TAB_ACTIVE_TEXT);
	exportData("TabActiveBorder", TAB_ACTIVE_BORDER);
	exportData("TabInactiveBg", TAB_INACTIVE_BG);
	exportData("TabInactiveBgDisconnected", TAB_INACTIVE_BG_DISCONNECTED);
	exportData("TabInactiveText", TAB_INACTIVE_TEXT);
	exportData("TabInactiveBorder", TAB_INACTIVE_BORDER);
	exportData("TabInactiveBgNotify", TAB_INACTIVE_BG_NOTIFY);
	exportData("TabDirtyBlend", TAB_DIRTY_BLEND);
	exportData("BlendTabs", BLEND_TABS);
	exportData("TabSize", TAB_SIZE);
	
	xml.stepOut();
		
	/*
	Don't manually export icon stuff, user might have absolute path for toolbars. Icon packs can be included by editing the .dctheme example.
	*/
	if(backup) {
		curType = "string";
		xml.addTag("Icons");
		xml.stepIn();
		exportData("IconPath", ICON_PATH);
		xml.stepOut();
	}
	

	if(!HighlightManager::getInstance()->emptyList())
		HighlightManager::getInstance()->save(xml);

	try {
		File ff(path, File::WRITE, File::CREATE | File::TRUNCATE);
		BufferedOutputStream<false> f(&ff);
		f.write(SimpleXML::utf8Header);
		xml.toXML(&f);
	} catch(const FileException&) {
		// ...
	}
	
}

LRESULT PropPageTextStyles::onExport(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	tstring x;

	BrowseDlg dlg(m_hWnd, BrowseDlg::TYPE_SETTINGS_RESOURCES, BrowseDlg::DIALOG_SAVE_FILE);
	dlg.setTypes(3, types);
	if (dlg.show(x)) {
		SaveTheme(Text::fromT(x), false);
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
		fontdirty = true;
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
