/* 
 * Copyright (C) 2001-2006 Jacek Sieka, arnetheduck on gmail point com
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
#include "WinUtil.h"

#include "FavHubProperties.h"

#include "../client/FavoriteManager.h"
#include "../client/ResourceManager.h"

LRESULT FavHubProperties::OnInitDialog(UINT, WPARAM, LPARAM, BOOL&)
{
	// Translate dialog
	SetWindowText(CTSTRING(FAVORITE_HUB_PROPERTIES));
	SetDlgItemText(IDOK, CTSTRING(OK));
	SetDlgItemText(IDCANCEL, CTSTRING(CANCEL));
	SetDlgItemText(IDC_FH_HUB, CTSTRING(HUB));
	SetDlgItemText(IDC_FH_IDENT, CTSTRING(FAVORITE_HUB_IDENTITY));
	SetDlgItemText(IDC_FH_NAME, CTSTRING(HUB_NAME));
	SetDlgItemText(IDC_FH_ADDRESS, CTSTRING(HUB_ADDRESS));
	SetDlgItemText(IDC_FH_HUB_DESC, CTSTRING(DESCRIPTION));
	SetDlgItemText(IDC_FH_NICK, CTSTRING(NICK));
	SetDlgItemText(IDC_FH_PASSWORD, CTSTRING(PASSWORD));
	SetDlgItemText(IDC_FH_USER_DESC, CTSTRING(DESCRIPTION));
	SetDlgItemText(IDC_FH_CONN, CTSTRING(FAVORITE_HUB_CONNECTION));
	SetDlgItemText(IDC_DEFAULT, CTSTRING(DEFAULT));
	SetDlgItemText(IDC_ACTIVE, CTSTRING(SETTINGS_DIRECT));
	SetDlgItemText(IDC_PASSIVE, CTSTRING(SETTINGS_FIREWALL_PASSIVE));
	SetDlgItemText(IDC_STEALTH, CTSTRING(STEALTH_MODE));
	SetDlgItemText(IDC_FAV_NO_PM, CTSTRING(FAV_NO_PM));
	SetDlgItemText(IDC_SHOW_JOIN, CTSTRING(FAV_SHOW_JOIN));
	SetDlgItemText(IDC_HIDE_SHARE, CTSTRING(HIDE_SHARE));
	SetDlgItemText(IDC_FAV_SEARCH_INTERVAL, CTSTRING(MINIMUM_SEARCH_INTERVAL));
	SetDlgItemText(IDC_FAVGROUP, CTSTRING(GROUP));
	SetDlgItemText(IDC_LOGMAINCHAT, CTSTRING(FAV_LOG_CHAT));

	// Fill in values
	SetDlgItemText(IDC_HUBNAME, Text::toT(entry->getName()).c_str());
	SetDlgItemText(IDC_HUBDESCR, Text::toT(entry->getDescription()).c_str());
	SetDlgItemText(IDC_HUBADDR, Text::toT(entry->getServer()).c_str());
	SetDlgItemText(IDC_HUBNICK, Text::toT(entry->getNick(false)).c_str());
	SetDlgItemText(IDC_HUBPASS, Text::toT(entry->getPassword()).c_str());
	SetDlgItemText(IDC_HUBUSERDESCR, Text::toT(entry->getUserDescription()).c_str());
	CheckDlgButton(IDC_STEALTH, entry->getStealth() ? BST_CHECKED : BST_UNCHECKED);
	SetDlgItemText(IDC_SERVER, Text::toT(entry->getIP()).c_str());
	CheckDlgButton(IDC_HIDE_SHARE, entry->getHideShare() ? BST_CHECKED : BST_UNCHECKED); //Hide Share Mod
	CheckDlgButton(IDC_FAV_NO_PM, entry->getFavNoPM() ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_SHOW_JOIN, entry->getHubShowJoins() ? BST_CHECKED : BST_UNCHECKED); //hub show joins
	SetDlgItemText(IDC_FAV_SEARCH_INTERVAL_BOX, Util::toStringW(entry->getSearchInterval()).c_str());
	CheckDlgButton(IDC_LOGMAINCHAT, entry->getHubLogMainchat() ? BST_CHECKED : BST_UNCHECKED);

	CComboBox combo;
	combo.Attach(GetDlgItem(IDC_FAVGROUP_BOX));
	combo.AddString(_T("---"));
	combo.SetCurSel(0);

	const FavHubGroups& favHubGroups = FavoriteManager::getInstance()->getFavHubGroups();
	for(FavHubGroups::const_iterator i = favHubGroups.begin(); i != favHubGroups.end(); ++i) {
		const string& name = i->first;
		int pos = combo.AddString(Text::toT(name).c_str());
		
		if(name == entry->getGroup())
			combo.SetCurSel(pos);
	}

	combo.Detach();

	// TODO: add more encoding into wxWidgets version, this is enough now
	// FIXME: following names are Windows only!
	combo.Attach(GetDlgItem(IDC_ENCODING));
	combo.AddString(_T("System default"));
	combo.AddString(_T("English_United Kingdom.1252"));
	combo.AddString(_T("Czech_Czech Republic.1250"));
	combo.AddString(_T("Russian_Russia.1251"));
	combo.AddString(Text::toT(Text::utf8).c_str());

	if(strnicmp("adc://", entry->getServer().c_str(), 6) == 0 || strnicmp("adcs://", entry->getServer().c_str(), 7) == 0)
	{
		combo.SetCurSel(4); // select UTF-8 for ADC hubs
		combo.EnableWindow(false);
	}
	else
	{
		if(entry->getEncoding().empty())
			combo.SetCurSel(0);
		else
			combo.SetWindowText(Text::toT(entry->getEncoding()).c_str());
	}

	combo.Detach();

	if(entry->getMode() == 0)
		CheckRadioButton(IDC_ACTIVE, IDC_DEFAULT, IDC_DEFAULT);
	else if(entry->getMode() == 1)
		CheckRadioButton(IDC_ACTIVE, IDC_DEFAULT, IDC_ACTIVE);
	else if(entry->getMode() == 2)
		CheckRadioButton(IDC_ACTIVE, IDC_DEFAULT, IDC_PASSIVE);

	CEdit tmp;
	tmp.Attach(GetDlgItem(IDC_HUBNAME));
	tmp.SetFocus();
	tmp.SetSel(0,-1);
	tmp.Detach();
	tmp.Attach(GetDlgItem(IDC_HUBNICK));
	tmp.LimitText(35);
	tmp.Detach();
	tmp.Attach(GetDlgItem(IDC_HUBUSERDESCR));
	tmp.LimitText(50);
	tmp.Detach();
	tmp.Attach(GetDlgItem(IDC_HUBPASS));
	tmp.SetPasswordChar('*');
	tmp.Detach();
	
	CUpDownCtrl updown;
	updown.Attach(GetDlgItem(IDC_FAV_SEARCH_INTERVAL_SPIN));
	updown.SetRange32(10, 9999);
	updown.Detach();
	
	CenterWindow(GetParent());
	return FALSE;
}

LRESULT FavHubProperties::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if(wID == IDOK)
	{
		TCHAR buf[512];
		GetDlgItemText(IDC_HUBADDR, buf, 256);
		if(buf[0] == '\0') {
			MessageBox(CTSTRING(INCOMPLETE_FAV_HUB), _T(""), MB_ICONWARNING | MB_OK);
			return 0;
		}
		entry->setServer(Text::fromT(buf));
		GetDlgItemText(IDC_HUBNAME, buf, 256);
		entry->setName(Text::fromT(buf));
		GetDlgItemText(IDC_HUBDESCR, buf, 256);
		entry->setDescription(Text::fromT(buf));
		GetDlgItemText(IDC_HUBNICK, buf, 256);
		entry->setNick(Text::fromT(buf));
		GetDlgItemText(IDC_HUBPASS, buf, 256);
		entry->setPassword(Text::fromT(buf));
		GetDlgItemText(IDC_HUBUSERDESCR, buf, 256);
		entry->setUserDescription(Text::fromT(buf));
		entry->setStealth(IsDlgButtonChecked(IDC_STEALTH) == 1);
		GetDlgItemText(IDC_SERVER, buf, 512);
		entry->setIP(Text::fromT(buf));
		entry->setHideShare(IsDlgButtonChecked(IDC_HIDE_SHARE) == 1); // Hide Share Mod
		entry->setFavNoPM(IsDlgButtonChecked(IDC_FAV_NO_PM) == 1);
		entry->setHubShowJoins(IsDlgButtonChecked(IDC_SHOW_JOIN) == 1); //show joins
		entry->setHubLogMainchat(IsDlgButtonChecked(IDC_LOGMAINCHAT) == 1);
		GetDlgItemText(IDC_FAV_SEARCH_INTERVAL_BOX, buf, 512);
		entry->setSearchInterval(Util::toUInt32(Text::fromT(buf)));
		
		GetDlgItemText(IDC_ENCODING, buf, 512);
		if(_tcschr(buf, _T('.')) == NULL && _tcscmp(buf, Text::toT(Text::utf8).c_str()) != 0 && _tcscmp(buf, _T("System default")) != 0)
		{
			MessageBox(_T("Invalid encoding!"), _T(""), MB_ICONWARNING | MB_OK);
			return 0;
		}
		entry->setEncoding(Text::fromT(buf));
		
		CComboBox combo;
		combo.Attach(GetDlgItem(IDC_FAV_DLG_GROUP));
	
		if(combo.GetCurSel() == 0)
		{
			entry->setGroup(Util::emptyString);
		}
		else
		{
			tstring text(combo.GetWindowTextLength() + 1, _T('\0'));
			combo.GetWindowText(&text[0], text.size());
			text.resize(text.size()-1);
			entry->setGroup(Text::fromT(text));
		}
		combo.Detach();

		int	ct = -1;
		if(IsDlgButtonChecked(IDC_DEFAULT))
			ct = 0;
		else if(IsDlgButtonChecked(IDC_ACTIVE))
			ct = 1;
		else if(IsDlgButtonChecked(IDC_PASSIVE))
			ct = 2;

		entry->setMode(ct);

		FavoriteManager::getInstance()->save();
	}
	EndDialog(wID);
	return 0;
}

LRESULT FavHubProperties::OnTextChanged(WORD /*wNotifyCode*/, WORD wID, HWND hWndCtl, BOOL& /*bHandled*/)
{
	TCHAR buf[256];

	GetDlgItemText(wID, buf, 256);
	tstring old = buf;

	// Strip '$', '|' and ' ' from text
	TCHAR *b = buf, *f = buf, c;
	while( (c = *b++) != 0 )
	{
		if(c != '$' && c != '|' && (wID == IDC_HUBUSERDESCR || c != ' ') && ( (wID != IDC_HUBNICK) || (c != '<' && c != '>')) )
			*f++ = c;
	}

	*f = '\0';

	if(old != buf)
	{
		// Something changed; update window text without changing cursor pos
		CEdit tmp;
		tmp.Attach(hWndCtl);
		int start, end;
		tmp.GetSel(start, end);
		tmp.SetWindowText(buf);
		if(start > 0) start--;
		if(end > 0) end--;
		tmp.SetSel(start, end);
		tmp.Detach();
	}

	return TRUE;
}

/**
 * @file
 * $Id: FavHubProperties.cpp 470 2010-01-02 23:23:39Z bigmuscle $
 */
