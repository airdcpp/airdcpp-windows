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

#if !defined(ABOUT_DLG_H)
#define ABOUT_DLG_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../client/HttpDownload.h"

using std::unique_ptr;

static const TCHAR thanks[] = 
_T("Dìkuji všem, kteøí mì ve vývoji podporovali. THX: Andyman (for muscle-arm logo), Atom, Blackrabbit, Catalin, ")
_T("Chmelic, Cinique, Corvik, Crakter, darwusch, Dreamveawer, FarCry, GargoyleMT, Gigadisk (for Czech translation), ")
_T("Ice, Jove, King Wenceslas, Lee, Legolas, Liny, MaynardK, Mlok, Naga, Patrol, popKorn, Pothead, ProLogic, Rm., ")
_T("Testament, Warrior, WereWiking, xAyiDe, XiND and ToH for very nice betatesting :-) and to all donators who support me: ")
_T("Kulmegil, Michal Seckar, k3dt, majki, mazeboy, Fino, Architect, Ujep, Jeepo, mlok, redsaq, anton009, Lee, share2all");
static const TCHAR Airthanks[] = 
_T("Thanks Zinden for Making me part of the project :D\r\n")
_T("Thanks to Myth for his work on the airdcpp.com site.\r\n")
_T("Thanks Northwind for the toolbars for 1.0x versions.\r\n")
_T("Thanks to my GirlFriend to not getting too mad about all spent hours on PC ;).\r\n")
_T("Thanks to Yada, en_dator, Vimmer, Charlie, G-Spot, Alehk for Testing the client and helping to make it better.\r\n")
_T("Thanks also to all other Test users.\r\n")
_T("Thanks savone, spaljeni for the graphic works.\r\n")
_T("Thanks NT and Yada for the work on the code\r\n")
_T("Thanks to Translators: \r\n")
_T("xaozon, kryppy, B1ackBoX, shuttle, ICU2M8, en_dator, NT, Bl0m5t3r, Shuttle, LadyStardust, savone, aLti, MMWST, Lleexxii, What2Write, Kryppy, Toans, Kaas.\r\n");


class AboutDlg : public CDialogImpl<AboutDlg>
{

public:
	enum { IDD = IDD_ABOUTBOX };
	enum { WM_VERSIONDATA = WM_APP + 53 };

	AboutDlg() { }
	~AboutDlg() { }

	BEGIN_MSG_MAP(AboutDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_VERSIONDATA, onVersionData)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onVersionData(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

private:
	AboutDlg(const AboutDlg&) { dcassert(0); }
	unique_ptr<HttpDownload> dl;

	void completeDownload();
};

#endif // !defined(ABOUT_DLG_H)

/**
 * @file
 * $Id: AboutDlg.h 470 2010-01-02 23:23:39Z bigmuscle $
 */
