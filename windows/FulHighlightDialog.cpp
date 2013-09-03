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
#include "Resource.h"

#include "../client/SettingsManager.h"
#include "../client/StringTokenizer.h"
#include "../client/version.h"

#include "HighlightManager.h"
#include "FulHighlightDialog.h"
#include "WinUtil.h"

ResourceManager::Strings FulHighlightDialog::UserColumnNames[] = { ResourceManager::NICK, ResourceManager::SHARED, ResourceManager::EXACT_SHARED, 
	ResourceManager::DESCRIPTION, ResourceManager::TAG, ResourceManager::SETCZDC_UPLOAD_SPEED, ResourceManager::SETCZDC_DOWNLOAD_SPEED, ResourceManager::IP_BARE, ResourceManager::EMAIL,
	ResourceManager::VERSION, ResourceManager::MODE, ResourceManager::HUBS, ResourceManager::SLOTS, ResourceManager::CID };

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
	{ IDC_WHOLELINE,	 ResourceManager::SETTINGS_WHOLE_LINE		},
	{ IDC_CASESENSITIVE, ResourceManager::CASE_SENSITIVE			},
	{ IDC_WHOLEWORD,	 ResourceManager::SETTINGS_ENTIRE_WORD		},
	{ IDC_HCONTEXT_TEXT, ResourceManager::HIGHLIGHT_CONTEXT			},
	{ IDOK,				 ResourceManager::OK						},
	{ IDCANCEL,			 ResourceManager::CANCEL					},
	{ IDC_MATCH_COL_TEXT,ResourceManager::MATCH_COLUMN				},
	{ IDC_TABCOLOR,		 ResourceManager::CHANGE_TAB_COLOR			},
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
	ctrlContext.Attach(GetDlgItem(IDC_HCONTEXT));
	ctrlText.Attach(GetDlgItem(IDC_HLTEXT));
	ctrlMatchCol.Attach(GetDlgItem(IDC_MATCH_COLUMN));

	//add alternatives
	StringTokenizer<tstring> s(Text::toT(STRING(HIGHLIGHT_MATCH_TYPES)), _T(','));
	TStringList l = s.getTokens();
	for(TStringIter i = l.begin(); i != l.end(); ++i)
		ctrlMatchType.AddString((*i).c_str());

	ctrlMatchType.SetCurSel(1);

	ctrlContext.AddString(CTSTRING(CONTEXT_CHAT));
	ctrlContext.AddString(CTSTRING(CONTEXT_NICKLIST));
	ctrlContext.AddString(CTSTRING(CONTEXT_FILELIST));

	CenterWindow(WinUtil::mainWnd);
	SetWindowText(CTSTRING(HIGHLIGHT_DIALOG_TITLE));

	initControls();
	fix();
	return TRUE;
}

LRESULT FulHighlightDialog::onOk(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/){
	try{
		getValues();
	}catch(...) {
		MessageBox(CTSTRING(BAD_REGEXP), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_OK | MB_ICONEXCLAMATION);
		return TRUE;
	}

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
				//....
			}
		} catch(...) {
			MessageBox(CTSTRING(BAD_REGEXP), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_OK | MB_ICONEXCLAMATION);
			return TRUE;
		}

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
void FulHighlightDialog::fix() {

	if(ctrlContext.GetCurSel() == HighlightManager::CONTEXT_NICKLIST) {
		ctrlText.SetWindowText(CTSTRING(SETTINGS_INCLUDE_NICKLIST));
		BOOL use = 0;
		BOOL t;
		onClickedBox(0, IDC_HAS_BG_COLOR, NULL, t);
		onClickedBox(0, IDC_HAS_FG_COLOR, NULL, t);

		::EnableWindow(GetDlgItem(IDC_UNDERLINE),					use);
		::EnableWindow(GetDlgItem(IDC_ITALIC),					use);
		::EnableWindow(GetDlgItem(IDC_BOLD),					use);
		::EnableWindow(GetDlgItem(IDC_WHOLEWORD),					use);
		::EnableWindow(GetDlgItem(IDC_WHOLELINE),					use);
		::EnableWindow(GetDlgItem(IDC_POPUP),					use);
		::EnableWindow(GetDlgItem(IDC_HAS_BG_COLOR),					use);
		::EnableWindow(GetDlgItem(IDC_BGCOLOR),					use);
		::EnableWindow(GetDlgItem(IDC_MATCHTYPE),					use);
		::EnableWindow(GetDlgItem(IDC_SOUND),					use);
		::EnableWindow(GetDlgItem(IDC_STRIKEOUT),					use);
		::EnableWindow(GetDlgItem(IDC_TABCOLOR),					use);
		
		::EnableWindow(GetDlgItem(IDC_MATCH_COL_TEXT),					1);
		::EnableWindow(GetDlgItem(IDC_MATCH_COLUMN),					1);

	} else if(ctrlContext.GetCurSel() == HighlightManager::CONTEXT_FILELIST) {
		BOOL use = 0;
		::EnableWindow(GetDlgItem(IDC_UNDERLINE),					use);
		::EnableWindow(GetDlgItem(IDC_ITALIC),					use);
		::EnableWindow(GetDlgItem(IDC_BOLD),					use);
		::EnableWindow(GetDlgItem(IDC_WHOLEWORD),					use);
		::EnableWindow(GetDlgItem(IDC_WHOLELINE),					use);
		::EnableWindow(GetDlgItem(IDC_POPUP),					use);
		::EnableWindow(GetDlgItem(IDC_HAS_BG_COLOR),					1);
		::EnableWindow(GetDlgItem(IDC_MATCHTYPE),					use);
		::EnableWindow(GetDlgItem(IDC_SOUND),					use);
		::EnableWindow(GetDlgItem(IDC_STRIKEOUT),					use);
		::EnableWindow(GetDlgItem(IDC_MATCH_COL_TEXT),					use);
		::EnableWindow(GetDlgItem(IDC_MATCH_COLUMN),					use);
		::EnableWindow(GetDlgItem(IDC_TABCOLOR),					use);
		BOOL t;
		onClickedBox(0, IDC_HAS_BG_COLOR, NULL, t);
		onClickedBox(0, IDC_HAS_FG_COLOR, NULL, t);

		ctrlText.SetWindowText(CTSTRING(SETTINGS_CONTEXT_FILELIST));
	} else {
		BOOL use = 1;
		::EnableWindow(GetDlgItem(IDC_UNDERLINE),					use);
		::EnableWindow(GetDlgItem(IDC_ITALIC),					use);
		::EnableWindow(GetDlgItem(IDC_BOLD),					use);
		::EnableWindow(GetDlgItem(IDC_WHOLEWORD),					use);
		::EnableWindow(GetDlgItem(IDC_WHOLELINE),					use);
		::EnableWindow(GetDlgItem(IDC_POPUP),					use);
		::EnableWindow(GetDlgItem(IDC_HAS_BG_COLOR),					use);
		::EnableWindow(GetDlgItem(IDC_MATCHTYPE),					use);
		::EnableWindow(GetDlgItem(IDC_SOUND),					use);
		::EnableWindow(GetDlgItem(IDC_STRIKEOUT),					use);
		::EnableWindow(GetDlgItem(IDC_TABCOLOR),					use);
		::EnableWindow(GetDlgItem(IDC_MATCH_COL_TEXT),					0);
		::EnableWindow(GetDlgItem(IDC_MATCH_COLUMN),					0);

		BOOL t;
		onClickedBox(0, IDC_HAS_BG_COLOR, NULL, t);
		onClickedBox(0, IDC_HAS_FG_COLOR, NULL, t);
		onClickedBox(0, IDC_SOUND, NULL, t);

		ctrlText.SetWindowText(CTSTRING(HL_REGEXP));
	}
}
void FulHighlightDialog::populateMatchCombo() {

	for(int i = 0; i < 14; i++) 
		ctrlMatchCol.AddString(CTSTRING_I(UserColumnNames[i]));

	ctrlMatchCol.SetCurSel(cs.getMatchColumn());
}

LRESULT FulHighlightDialog::onApplyContext(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/){
	fix();
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
	cs.setWholeLine(	  IsDlgButtonChecked(IDC_WHOLELINE)		== BST_CHECKED );
	cs.setWholeWord(	  IsDlgButtonChecked(IDC_WHOLEWORD)		== BST_CHECKED );
	cs.setPopup(		  IsDlgButtonChecked(IDC_POPUP)			== BST_CHECKED );
	cs.setTab(			  IsDlgButtonChecked(IDC_TABCOLOR)		== BST_CHECKED );
	cs.setPlaySound(	  IsDlgButtonChecked(IDC_SOUND)			== BST_CHECKED );
	//cs.setLog(			  IsDlgButtonChecked(IDC_LASTLOG)		== BST_CHECKED );
	cs.setHasBgColor(	  IsDlgButtonChecked(IDC_HAS_BG_COLOR)	== BST_CHECKED );
	cs.setHasFgColor(	  IsDlgButtonChecked(IDC_HAS_FG_COLOR)	== BST_CHECKED );


	cs.setBgColor( bgColor );
	cs.setFgColor( fgColor );

	cs.setMatchType( ctrlMatchType.GetCurSel() );
	cs.setContext(ctrlContext.GetCurSel());
	
	if(ctrlContext.GetCurSel() == HighlightManager::CONTEXT_NICKLIST)
		cs.setMatchColumn(ctrlMatchCol.GetCurSel());
	else
		cs.setMatchColumn(0);

	cs.setSoundFile( soundFile );

	cs.setRegexp();

}


void FulHighlightDialog::initControls() {
	
	SetDlgItemText(IDC_STRING, cs.getMatch().c_str());
	ctrlMatchType.SetCurSel(cs.getMatchType());
	ctrlContext.SetCurSel(cs.getContext());
	populateMatchCombo();

	CheckDlgButton(IDC_BOLD			, cs.getBold()			 ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_ITALIC		, cs.getItalic()		 ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_UNDERLINE	, cs.getUnderline()	 ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_STRIKEOUT	, cs.getStrikeout()	 ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_WHOLELINE	, cs.getWholeLine()	 ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_CASESENSITIVE, cs.getCaseSensitive() ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_WHOLEWORD	, cs.getWholeWord()	 ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_POPUP		, cs.getPopup()		 ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_TABCOLOR		, cs.getTab()			 ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_SOUND		, cs.getPlaySound()	 ? BST_CHECKED : BST_UNCHECKED);
	//CheckDlgButton(IDC_LASTLOG		, cs.getLog()			 ? BST_CHECKED : BST_UNCHECKED);
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