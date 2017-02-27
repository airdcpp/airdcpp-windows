/*
* Copyright (C) 2011-2017 AirDC++ Project
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
#include <commctrl.h>
#include "stdafx.h"
#include "Resource.h"
#include "WinUtil.h"
#include "AutosearchSearchTimesPage.h"

#define ATTACH(id, var) var.Attach(GetDlgItem(id))
#define setMinMax(x, y, z) \
	updown.Attach(GetDlgItem(x)); \
	updown.SetRange32(y, z); \
	updown.Detach();

AutosearchSearchTimesPage::AutosearchSearchTimesPage(AutoSearchItemSettings& aSettings, const string& aName) : options(aSettings), name(aName), loading(true) {}

AutosearchSearchTimesPage::~AutosearchSearchTimesPage() { }

LRESULT AutosearchSearchTimesPage::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {

	::SetWindowText(GetDlgItem(IDC_CUSTOM_SEARCH_TIMES), CTSTRING(CUSTOM_SEARCH_TIMES));
	::SetWindowText(GetDlgItem(IDC_MON), CTSTRING(MONDAY));
	::SetWindowText(GetDlgItem(IDC_TUE), CTSTRING(TUESDAY));
	::SetWindowText(GetDlgItem(IDC_WED), CTSTRING(WEDNESDAY));
	::SetWindowText(GetDlgItem(IDC_THU), CTSTRING(THURSDAY));
	::SetWindowText(GetDlgItem(IDC_FRI), CTSTRING(FRIDAY));
	::SetWindowText(GetDlgItem(IDC_SAT), CTSTRING(SATURDAY));
	::SetWindowText(GetDlgItem(IDC_SUN), CTSTRING(SUNDAY));
	::SetWindowText(GetDlgItem(IDC_SAT), CTSTRING(SATURDAY));
	::SetWindowText(GetDlgItem(IDC_SUN), CTSTRING(SUNDAY));
	::SetWindowText(GetDlgItem(IDC_START_TIME), CTSTRING(START_TIME));
	::SetWindowText(GetDlgItem(IDC_END_TIME), CTSTRING(END_TIME));
	::SetWindowText(GetDlgItem(IDC_SEARCH_TIMES_LABEL), CTSTRING(SEARCH_TIMES));
	::SetWindowText(GetDlgItem(IDC_SEARCH_INT_LABEL), CTSTRING(AUTOSEARCH_MINIMUM_SEARCH_INTERVAL));

	CheckDlgButton(IDC_MON, options.searchDays[1]);
	CheckDlgButton(IDC_TUE, options.searchDays[2]);
	CheckDlgButton(IDC_WED, options.searchDays[3]);
	CheckDlgButton(IDC_THU, options.searchDays[4]);
	CheckDlgButton(IDC_FRI, options.searchDays[5]);
	CheckDlgButton(IDC_SAT, options.searchDays[6]);
	CheckDlgButton(IDC_SUN, options.searchDays[0]);

	ATTACH(IDC_SEARCH_START, ctrlSearchStart);
	ATTACH(IDC_SEARCH_END, ctrlSearchEnd);
	ctrlSearchStart.SetFormat(_T("HH:mm"));
	ctrlSearchEnd.SetFormat(_T("HH:mm"));

	if (!((int8_t(options.searchDays.count()) == 7) && options.startTime.minute == 0 && options.startTime.hour == 0 && options.endTime.minute == 59 && options.endTime.hour == 23)) {
		CheckDlgButton(IDC_CUSTOM_SEARCH_TIMES, true);
	}
	else {
		CheckDlgButton(IDC_CUSTOM_SEARCH_TIMES, false);
	}

	SYSTEMTIME s;
	GetSystemTime(&s);
	s.wHour = options.startTime.hour;
	s.wMinute = options.startTime.minute;
	ctrlSearchStart.SetSystemTime(0, &s);

	SYSTEMTIME e;
	GetSystemTime(&e);
	e.wHour = options.endTime.hour;
	e.wMinute = options.endTime.minute;
	ctrlSearchEnd.SetSystemTime(0, &e);

	fixControls();
	loading = false; //loading done.
	return TRUE;
}


bool AutosearchSearchTimesPage::write() {

	bool useDefaultTimes = IsDlgButtonChecked(IDC_CUSTOM_SEARCH_TIMES) != BST_CHECKED;
	options.searchDays[1] = (IsDlgButtonChecked(IDC_MON) == BST_CHECKED || useDefaultTimes);
	options.searchDays[2] = (IsDlgButtonChecked(IDC_TUE) == BST_CHECKED || useDefaultTimes);
	options.searchDays[3] = (IsDlgButtonChecked(IDC_WED) == BST_CHECKED || useDefaultTimes);
	options.searchDays[4] = (IsDlgButtonChecked(IDC_THU) == BST_CHECKED || useDefaultTimes);
	options.searchDays[5] = (IsDlgButtonChecked(IDC_FRI) == BST_CHECKED || useDefaultTimes);
	options.searchDays[6] = (IsDlgButtonChecked(IDC_SAT) == BST_CHECKED || useDefaultTimes);
	options.searchDays[0] = (IsDlgButtonChecked(IDC_SUN) == BST_CHECKED || useDefaultTimes);

	if (useDefaultTimes) {
		options.startTime.hour = 0;
		options.startTime.minute = 0;
		options.endTime.hour = 23;
		options.endTime.minute = 59;
	}
	else {
		SYSTEMTIME s;
		ctrlSearchStart.GetSystemTime(&s);
		options.startTime.hour = s.wHour;
		options.startTime.minute = s.wMinute;

		SYSTEMTIME e;
		ctrlSearchEnd.GetSystemTime(&e);
		options.endTime.hour = e.wHour;
		options.endTime.minute = e.wMinute;

		if (options.endTime.hour < options.startTime.hour || (options.endTime.hour == options.startTime.hour && options.endTime.minute <= options.startTime.minute)) {
			MessageBox(CTSTRING(AS_END_GREATER));
			return false;
		}
	}

	return true;
}

LRESULT AutosearchSearchTimesPage::onCheckTimes(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	fixControls();
	return 0;
}

void AutosearchSearchTimesPage::fixControls() {

	BOOL useCustomTimes = (IsDlgButtonChecked(IDC_CUSTOM_SEARCH_TIMES) == BST_CHECKED);
	::EnableWindow(GetDlgItem(IDC_MON), useCustomTimes);
	::EnableWindow(GetDlgItem(IDC_TUE), useCustomTimes);
	::EnableWindow(GetDlgItem(IDC_WED), useCustomTimes);
	::EnableWindow(GetDlgItem(IDC_THU), useCustomTimes);
	::EnableWindow(GetDlgItem(IDC_FRI), useCustomTimes);
	::EnableWindow(GetDlgItem(IDC_SAT), useCustomTimes);
	::EnableWindow(GetDlgItem(IDC_SUN), useCustomTimes);

	::EnableWindow(GetDlgItem(IDC_START_TIME), useCustomTimes);
	::EnableWindow(GetDlgItem(IDC_SEARCH_START), useCustomTimes);
	::EnableWindow(GetDlgItem(IDC_END_TIME), useCustomTimes);
	::EnableWindow(GetDlgItem(IDC_SEARCH_END), useCustomTimes);
}

