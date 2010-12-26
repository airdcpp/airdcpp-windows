/* 
* Copyright (C) 2003-2005 Pär Björklund, per.bjorklund@gmail.com
*
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
#include "../client/DCPlusPlus.h"
#include "Resource.h"

#include "../client/SettingsManager.h"
#include "../client/ColorSettings.h"
#include "../client/HighlightManager.h"
#include "../client/StringTokenizer.h"
#include "../client/version.h"

#include "FulHighlightDialog.h"
#include "WinUtil.h"

PropPage::TextItem FulHighlightDialog::texts[] = {
	{ IDC_TEXT_STYLES,	 ResourceManager::HIGHLIGHT_TEXT_STYLES		},
	{ IDC_MATCH_OPTIONS, ResourceManager::HIGHLIGHT_MATCH_OPTIONS	},
	{ IDC_ACTIONS,		 ResourceManager::HIGHLIGHT_ACTIONS			},
	{ IDC_TEXT_TO_MATCH, ResourceManager::HIGHLIGHT_TEXT_TO_MATCH	},
	{ IDC_ST_MATCH_TYPE, ResourceManager::SETTINGS_ST_MATCH_TYPE	},
	{ IDC_BGCOLOR,		 ResourceManager::SETTINGS_BTN_BGCOLOR		},
	{ IDC_FGCOLOR,		 ResourceManager::SETTINGS_BTN_TEXTCOLOR	},
	{ IDC_SELECT_SOUND,  ResourceManager::SETTINGS_SELECT_SOUND		},
	{ IDC_BOLD,			 ResourceManager::BOLD						},
	{ IDC_ITALIC,		 ResourceManager::ITALIC					},
	{ IDC_UNDERLINE,	 ResourceManager::UNDERLINE					},
	{ IDC_STRIKEOUT,	 ResourceManager::STRIKEOUT					},
	{ IDC_POPUP,		 ResourceManager::SETTINGS_POPUP			},
	{ IDC_SOUND,		 ResourceManager::SETTINGS_PLAY_SOUND		},
	{ IDC_INCLUDENICK,	 ResourceManager::SETTINGS_INCLUDE_NICK		},
	{ IDC_WHOLELINE,	 ResourceManager::SETTINGS_WHOLE_LINE		},
	{ IDC_CASESENSITIVE, ResourceManager::CASE_SENSITIVE	},
	{ IDC_WHOLEWORD,	 ResourceManager::SETTINGS_ENTIRE_WORD		},
	{ IDC_TABCOLOR,		 ResourceManager::SETTINGS_TAB_COLOR		},
	{ IDC_LASTLOG,		 ResourceManager::SETTINGS_LASTLOG			},
	{ IDOK,				 ResourceManager::OK						},
	{ IDCANCEL,			 ResourceManager::CANCEL					},
	{ 0,				 ResourceManager::SETTINGS_AUTO_AWAY		}
};

LRESULT FulHighlightDialog::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	if (texts != NULL) {
		for(int i = 0; texts[i].itemID != 0; i++) {
			::SetDlgItemText(m_hWnd, texts[i].itemID,
				CTSTRING_I(texts[i].translatedString));
		}
	}

	//initalize colors
	bgColor = WinUtil::bgColor;
	fgColor = WinUtil::textColor;

	//initalize ComboBox
	ctrlMatchType.Attach(GetDlgItem(IDC_MATCHTYPE));

	//add alternatives
	StringTokenizer<tstring> s(Text::toT(STRING(HIGHLIGHT_MATCH_TYPES)), _T(','));
	TStringList l = s.getTokens();
	for(TStringIter i = l.begin(); i != l.end(); ++i)
		ctrlMatchType.AddString((*i).c_str());

	ctrlMatchType.SetCurSel(1);

	CenterWindow(WinUtil::mainWnd);
	SetWindowText(CTSTRING(HIGHLIGHT_DIALOG_TITLE));

	initControls();

	return TRUE;
}

LRESULT FulHighlightDialog::onOk(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/){
	getValues();

	if(cs.getMatch().empty()){
		MessageBox(CTSTRING(ADD_EMPTY), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_OK | MB_ICONEXCLAMATION);
		return TRUE;
	}

	if(cs.getMatch().find(_T("$Re:")) == 0) {
		string str1 = "^$";
		string str2 = (Text::fromT(cs.getMatch())).substr(4);
		try {
			boost::regex reg(str1);
			if(boost::regex_search(str2.begin(), str2.end(), reg)){
				//MessageBox(CTSTRING(BAD_REGEXP), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_OK | MB_ICONEXCLAMATION);
				//return TRUE;
			}
			/*boost::regex reg(str1);
			if(boost::regex_search(str2.begin(), str2.end(), reg)){
				//Nothing? See below
			};*/
		} catch(...) {
			MessageBox(CTSTRING(BAD_REGEXP), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_OK | MB_ICONEXCLAMATION);
			return TRUE;
		}

/*		PME reg(cs.getMatch().substr(4));
		if(! reg.IsValid()){
			MessageBox(CTSTRING(BAD_REGEXP), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_OK | MB_ICONEXCLAMATION);
			return TRUE;
		}*/
	}

	EndDialog(IDOK);
	return 0;
}

LRESULT FulHighlightDialog::onCancel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/){
	EndDialog(IDCANCEL);
	return 0;
}

LRESULT FulHighlightDialog::onFgColor(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/){
	CColorDialog dlg(fgColor, CC_FULLOPEN);
	if(dlg.DoModal() == IDOK) {
		fgColor = dlg.GetColor();
	}
	return 0;
}

LRESULT FulHighlightDialog::onBgColor(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/){
	CColorDialog dlg(bgColor, CC_FULLOPEN);
	if(dlg.DoModal() == IDOK) {
		bgColor = dlg.GetColor();
	}
	return 0;
}

LRESULT FulHighlightDialog::onSelSound(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	CFileDialog dlg(TRUE);
	dlg.m_ofn.lpstrFilter = _T("Wave Files\0*.wav\0\0");
	if(dlg.DoModal() == IDOK){
		soundFile = dlg.m_ofn.lpstrFile;
	}

	return 0;
}

FulHighlightDialog::~FulHighlightDialog() {
	ctrlMatchType.Detach();
}

void FulHighlightDialog::getValues(){
	TCHAR buf[1000];
	GetDlgItemText(IDC_STRING, buf, 1000);
	cs.setMatch(buf);

	cs.setBold(	  IsDlgButtonChecked(IDC_BOLD)		== BST_CHECKED );
	cs.setItalic(	  IsDlgButtonChecked(IDC_ITALIC)	== BST_CHECKED );
	cs.setUnderline( IsDlgButtonChecked(IDC_UNDERLINE)	== BST_CHECKED );
	cs.setStrikeout( IsDlgButtonChecked(IDC_STRIKEOUT) == BST_CHECKED );

	cs.setCaseSensitive( IsDlgButtonChecked(IDC_CASESENSITIVE) == BST_CHECKED );
	cs.setIncludeNick(	  IsDlgButtonChecked(IDC_INCLUDENICK)	== BST_CHECKED );
	cs.setWholeLine(	  IsDlgButtonChecked(IDC_WHOLELINE)		== BST_CHECKED );
	cs.setWholeWord(	  IsDlgButtonChecked(IDC_WHOLEWORD)		== BST_CHECKED );
	cs.setPopup(		  IsDlgButtonChecked(IDC_POPUP)			== BST_CHECKED );
	cs.setTab(			  IsDlgButtonChecked(IDC_TABCOLOR)		== BST_CHECKED );
	cs.setPlaySound(	  IsDlgButtonChecked(IDC_SOUND)			== BST_CHECKED );
	cs.setLog(			  IsDlgButtonChecked(IDC_LASTLOG)		== BST_CHECKED );
	cs.setHasBgColor(	  IsDlgButtonChecked(IDC_HAS_BG_COLOR)	== BST_CHECKED );
	cs.setHasFgColor(	  IsDlgButtonChecked(IDC_HAS_FG_COLOR)	== BST_CHECKED );


	cs.setBgColor( bgColor );
	cs.setFgColor( fgColor );

	cs.setMatchType( ctrlMatchType.GetCurSel() );

	cs.setSoundFile( soundFile );

}


void FulHighlightDialog::initControls() {
	
	SetDlgItemText(IDC_STRING, cs.getMatch().c_str());
	ctrlMatchType.SetCurSel(cs.getMatchType());

	CheckDlgButton(IDC_BOLD			, cs.getBold()			 ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_ITALIC		, cs.getItalic()		 ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_UNDERLINE	, cs.getUnderline()	 ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_STRIKEOUT	, cs.getStrikeout()	 ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_INCLUDENICK	, cs.getIncludeNick()	 ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_WHOLELINE	, cs.getWholeLine()	 ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_CASESENSITIVE, cs.getCaseSensitive() ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_WHOLEWORD	, cs.getWholeWord()	 ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_POPUP		, cs.getPopup()		 ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_TABCOLOR		, cs.getTab()			 ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_SOUND		, cs.getPlaySound()	 ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_LASTLOG		, cs.getLog()			 ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_HAS_BG_COLOR , cs.getHasBgColor()	 ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_HAS_FG_COLOR , cs.getHasFgColor()	 ? BST_CHECKED : BST_UNCHECKED);

	if(cs.getHasBgColor())
		bgColor = cs.getBgColor();
	if(cs.getHasFgColor())
		fgColor = cs.getFgColor();

	if(cs.getPlaySound())
		soundFile = cs.getSoundFile();

	BOOL t;
	onClickedBox(0, IDC_HAS_BG_COLOR, NULL, t);
	onClickedBox(0, IDC_HAS_FG_COLOR, NULL, t);
	onClickedBox(0, IDC_SOUND, NULL, t);
}

LRESULT FulHighlightDialog::onClickedBox(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/){
	int button = 0;
	switch(wID) {
	case IDC_SOUND:		   button = IDC_SELECT_SOUND; break;
	case IDC_HAS_BG_COLOR: button = IDC_BGCOLOR;	  break;
	case IDC_HAS_FG_COLOR: button = IDC_FGCOLOR;	  break;
	}

	bool enabled = IsDlgButtonChecked(wID) == BST_CHECKED;
	ctrlButton.Attach(GetDlgItem(button));
	ctrlButton.EnableWindow(enabled);
	ctrlButton.Detach();

	return 0;
}