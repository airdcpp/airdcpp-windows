#include "stdafx.h"

#include "../client/DCPlusPlus.h"
#include "../client/SettingsManager.h"
#include "../client/HighlightManager.h"
#include "Resource.h"
#include "WinUtil.h"
#include "Wizard.h"


static const TCHAR rar[] = 
_T("Client profile Rar-Hub\r\n")
_T("This will disable segment downloading by setting the minimum segment size!\r\n")
_T("(safer way to disable segments )\r\n")
_T("Grouped Downloads will be automatically Expanded.\r\n")
_T("Files bigger than 600 MB and File types not belonging to original release won't be shared.\r\n")
_T("(these can be changed from the AirSharing page)\r\n");


LRESULT WizardDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
			
	//Set current nick setting
	nickline.Attach(GetDlgItem(IDC_NICK));
	SetDlgItemText(IDC_NICK, Text::toT(SETTING(NICK)).c_str());

	ctrlDownload.Attach(GetDlgItem(IDC_DOWN_SPEED));
	ctrlUpload.Attach(GetDlgItem(IDC_CONNECTION));
	ctrlLanguage.Attach(GetDlgItem(IDC_LANGUAGE));

	CUpDownCtrl spin;
	spin.Attach(GetDlgItem(IDC_DOWN_SPEED_SPIN));
	spin.SetRange32(0, 100000);
	spin.Detach();

	CUpDownCtrl spin3;
	spin3.Attach(GetDlgItem(IDC_DOWNLOAD_S_SPIN));
	spin3.SetRange32(0, 999);
	spin3.Detach();

	CUpDownCtrl spin4;
	spin4.Attach(GetDlgItem(IDC_UPLOAD_S_SPIN));
	spin4.SetRange32(1, 100);
	spin4.Detach();

	CUpDownCtrl spin5;
	spin5.Attach(GetDlgItem(IDC_UP_SPEED_SPIN));
	spin5.SetRange32(1, 100);
	spin5.Detach();

	CUpDownCtrl spin6;
	spin6.Attach(GetDlgItem(IDC_AUTO_SLOTS_SPIN));
	spin6.SetRange32(1, 100);
	spin6.Detach();

		
	Images.CreateFromImage(WinUtil::getIconPath(_T("flags.bmp")).c_str(), 24, 20, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED | LR_LOADFROMFILE);
	ctrlLanguage.SetImageList(Images);
		
	int count = 0;
	int img = 0;
		
	for(StringIter i = SettingsManager::Languages.begin(); i != SettingsManager::Languages.end(); ++i){
		COMBOBOXEXITEM cbli =  {CBEIF_TEXT|CBEIF_IMAGE|CBEIF_SELECTEDIMAGE};
		CString str = Text::toT(*i).c_str();
		cbli.iItem =  count;
		cbli.pszText = (LPTSTR)(LPCTSTR) str;
		cbli.cchTextMax = str.GetLength();
		cbli.iImage = img;
		cbli.iSelectedImage = img;
		ctrlLanguage.InsertItem(&cbli);
		count = count++;
		img = img++;
		//ctrlLanguage.AddString(Text::toT(*i).c_str());
	}

	ctrlLanguage.SetCurSel(SETTING(LANGUAGE_SWITCH));


	download = SETTING(DOWNLOAD_SPEED);
	bool found=false;

	//add the Download speed strings, Using the same list as upload	
	for(StringIter i = SettingsManager::connectionSpeeds.begin(); i != SettingsManager::connectionSpeeds.end(); ++i) {
		if (Util::toDouble(download) < Util::toDouble(*i) && !found) {
			ctrlDownload.AddString(Text::toT(SETTING(DOWNLOAD_SPEED)).c_str());
			found=true;
		} else if (download == (*i)) {
			found=true;
		}
		ctrlDownload.AddString(Text::toT(*i).c_str());
	}

	//set current Download speed setting
	ctrlDownload.SetCurSel(ctrlDownload.FindString(0, Text::toT(SETTING(DOWNLOAD_SPEED)).c_str()));


	upload = SETTING(UPLOAD_SPEED);
	found=false;
	//add the upload speed strings	
	for(StringIter i = SettingsManager::connectionSpeeds.begin(); i != SettingsManager::connectionSpeeds.end(); ++i) {
		if (Util::toDouble(upload) < Util::toDouble(*i) && !found) {
			ctrlUpload.AddString(Text::toT(SETTING(UPLOAD_SPEED)).c_str());
			found=true;
		} else if (upload == (*i)) {
			found=true;
		}
		ctrlUpload.AddString(Text::toT(*i).c_str());
	}

	//set current upload speed setting
	ctrlUpload.SetCurSel(ctrlUpload.FindString(0, Text::toT(upload).c_str()));


	//Set current values
	SetDlgItemText(IDC_MAX_DOWNLOAD_SP, Text::toT(Util::toString(Util::getSpeedLimit(true))).c_str());
	SetDlgItemText(IDC_DOWNLOAD_SLOTS, Text::toT(Util::toString(Util::getSlots(true))).c_str());
	SetDlgItemText(IDC_UPLOAD_SLOTS, Text::toT(Util::toString(Util::getSlots(false))).c_str());
	SetDlgItemText(IDC_MAX_UPLOAD_SP, Text::toT(Util::toString(Util::getSpeedLimit(false))).c_str());
	SetDlgItemText(IDC_MAX_AUTO_OPENED, Text::toT(Util::toString(Util::getMaxAutoOpened())).c_str());

	switch(SETTING(SETTINGS_PROFILE)) {
		case SettingsManager::PROFILE_PUBLIC: CheckDlgButton(IDC_PUBLIC, BST_CHECKED); break;
		case SettingsManager::PROFILE_RAR: CheckDlgButton(IDC_RAR, BST_CHECKED); break;
		case SettingsManager::PROFILE_PRIVATE: CheckDlgButton(IDC_PRIVATE_HUB, BST_CHECKED); break;
		default: CheckDlgButton(IDC_PUBLIC, BST_CHECKED); break;
	}

	if (SETTING(DL_AUTODETECT))
		CheckDlgButton(IDC_DL_AUTODETECT_WIZ, BST_CHECKED);
	else
		CheckDlgButton(IDC_DL_AUTODETECT_WIZ, BST_UNCHECKED);

	if (SETTING(UL_AUTODETECT))
		CheckDlgButton(IDC_UL_AUTODETECT_WIZ, BST_CHECKED);
	else
		CheckDlgButton(IDC_UL_AUTODETECT_WIZ, BST_UNCHECKED);

	CenterWindow(GetParent());
	fixcontrols();
	return TRUE;
}

void WizardDlg::write() {
	//Think can keep the tchar bufs here when just have a few settings that needs it
	
	//set the language
	if(SETTING(LANGUAGE_SWITCH) != ctrlLanguage.GetCurSel()) {
		SettingsManager::getInstance()->set(SettingsManager::LANGUAGE_SWITCH, ctrlLanguage.GetCurSel());
		//To Make this a little cleaner
		setLang();
	}

	//for Nick
	TCHAR buf[64];
	GetDlgItemText(IDC_NICK, buf, sizeof(buf) + 1);
	string nick = Text::fromT(buf);
	if(nick != Util::emptyString)
		SettingsManager::getInstance()->set(SettingsManager::NICK, nick );
		

	//speed settings
	SettingsManager::getInstance()->set(SettingsManager::UPLOAD_SPEED, upload);
	SettingsManager::getInstance()->set(SettingsManager::DOWNLOAD_SPEED, download);
	

	//for max download speed
	TCHAR buf2[64];
	GetDlgItemText(IDC_MAX_DOWNLOAD_SP, buf2, sizeof(buf2) + 1);
	string downloadLimit = Text::fromT(buf2);
	SettingsManager::getInstance()->set(SettingsManager::MAX_DOWNLOAD_SPEED, downloadLimit);


	//upload auto grant limit
	TCHAR buf3[64];
	GetDlgItemText(IDC_MAX_AUTO_OPENED, buf3, sizeof(buf3) + 1);
	string uploadLimit = Text::fromT(buf3);
	SettingsManager::getInstance()->set(SettingsManager::AUTO_SLOTS, uploadLimit);


	//max auto opened
	TCHAR buf4[64];
	GetDlgItemText(IDC_MAX_UPLOAD_SP, buf4, sizeof(buf4) + 1);
	string autoOpened = Text::fromT(buf4);
	SettingsManager::getInstance()->set(SettingsManager::MIN_UPLOAD_SPEED, autoOpened);


	//Download Slots
	TCHAR buf5[64];
	GetDlgItemText(IDC_DOWNLOAD_SLOTS, buf5, sizeof(buf5) + 1);
	SettingsManager::getInstance()->set(SettingsManager::DOWNLOAD_SLOTS, Text::fromT(buf5));
			
	//Upload Slots
	TCHAR buf6[64];
	GetDlgItemText(IDC_UPLOAD_SLOTS, buf6, sizeof(buf6) + 1);
	SettingsManager::getInstance()->set(SettingsManager::SLOTS, Text::fromT(buf6));


	if(IsDlgButtonChecked(IDC_PUBLIC)){
		Util::setProfile(0);
	} else if(IsDlgButtonChecked(IDC_RAR)) {
		Util::setProfile(1, IsDlgButtonChecked(IDC_WIZARD_SKIPLIST) ? true : false);
	} else if(IsDlgButtonChecked(IDC_PRIVATE_HUB)){
		Util::setProfile(2);
	}


	if(IsDlgButtonChecked(IDC_DL_AUTODETECT_WIZ)){
		SettingsManager::getInstance()->set(SettingsManager::DL_AUTODETECT, true);
	} else {
		SettingsManager::getInstance()->set(SettingsManager::DL_AUTODETECT, false);
	}


	if(IsDlgButtonChecked(IDC_UL_AUTODETECT_WIZ)){
		SettingsManager::getInstance()->set(SettingsManager::UL_AUTODETECT, true);
	} else {
		SettingsManager::getInstance()->set(SettingsManager::UL_AUTODETECT, false);
	}

	//if (IsDlgButtonChecked(IDC_CHECK1)) {
		//SettingsManager::getInstance()->set(SettingsManager::WIZARD_RUN_NEW, true);
	//}
	SettingsManager::getInstance()->save();
}
	
LRESULT WizardDlg::onSpeedtest(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	WinUtil::openLink(_T("http://www.speedtest.net"));
	return 0;
}

LRESULT WizardDlg::onNext(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	write();
	EndDialog(wID);
	return 0;

}

LRESULT WizardDlg::OnDetect(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	fixcontrols();

	TCHAR buf1[64];
	GetDlgItemText(IDC_DOWN_SPEED, buf1, sizeof(buf1) +1);
	double valueDL = Util::toDouble(Text::fromT(buf1));
	setDownloadLimits(valueDL);

	TCHAR buf2[64];
	GetDlgItemText(IDC_CONNECTION, buf2, sizeof(buf2) +1);
	double valueUL = Util::toDouble(Text::fromT(buf2));
	setUploadLimits(valueUL);
	return 0;
}

LRESULT WizardDlg::OnDlgButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	fixcontrols();
	return 0;
}
LRESULT WizardDlg::OnDownSpeed(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	onSpeedChanged(wNotifyCode, wID, hWndCtl, bHandled);
	TCHAR buf2[64];
	switch(wNotifyCode) {
		case CBN_EDITCHANGE:
			GetDlgItemText(IDC_DOWN_SPEED, buf2, sizeof(buf2) +1);
			break;

		case CBN_SELENDOK:
			ctrlDownload.GetLBText(ctrlDownload.GetCurSel(), buf2);
			break;
	}

	download = Text::fromT(buf2);

	double value = Util::toDouble(download);

	setDownloadLimits(value);

	return 0;
}

LRESULT WizardDlg::OnUploadSpeed(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	onSpeedChanged(wNotifyCode, wID, hWndCtl, bHandled);
	TCHAR buf2[64];
	switch(wNotifyCode) {
		case CBN_EDITCHANGE:
			GetDlgItemText(IDC_CONNECTION, buf2, sizeof(buf2) +1);
			break;
		case CBN_SELENDOK:
			ctrlUpload.GetLBText(ctrlUpload.GetCurSel(), buf2);
			break;

	}

	upload = Text::fromT(buf2);
	double value = Util::toDouble(upload);
	setUploadLimits(value);

	return 0;
}

LRESULT WizardDlg::onSpeedChanged(WORD /*wNotifyCode*/, WORD wID, HWND hWndCtl, BOOL& /*bHandled*/)
{
	tstring speed;
	speed.resize(1024);
	speed.resize(GetDlgItemText(wID, &speed[0], 1024));
	if (!speed.empty()) {
		boost::wregex reg;
		if(speed[speed.size() -1] == '.')
			reg.assign(_T("(\\d+\\.)"));
		else
			reg.assign(_T("(\\d+(\\.\\d+)?)"));
		if (!regex_match(speed, reg)) {
			CComboBox tmp;
			tmp.Attach(hWndCtl);
			DWORD dwSel;
			if ((dwSel = tmp.GetEditSel()) != CB_ERR) {
				tstring::iterator it = speed.begin() +  HIWORD(dwSel)-1;
				speed.erase(it);
				tmp.SetEditSel(0,-1);
				tmp.SetWindowText(speed.c_str());
				tmp.SetEditSel(HIWORD(dwSel)-1, HIWORD(dwSel)-1);
				tmp.Detach();
			}
		}
	}
	return TRUE;
}

LRESULT WizardDlg::onCtlColor(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	if((HWND) lParam == GetDlgItem(IDC_WIZARD_NOTETOP)) {
		HDC hDC = (HDC)wParam;
		::SetTextColor(hDC, RGB(251,69,69));
		::SetBkMode(hDC, TRANSPARENT);
		return (LRESULT)GetStockObject(HOLLOW_BRUSH);
	}
	/*
	BOOL dl = IsDlgButtonChecked(IDC_DL_AUTODETECT_WIZ) != BST_CHECKED;
	BOOL ul = IsDlgButtonChecked(IDC_UL_AUTODETECT_WIZ) != BST_CHECKED;
	HDC hDC = (HDC)wParam;

	if((HWND) lParam == GetDlgItem(IDC_MAX_AUTO_WIZ) || (HWND) lParam == GetDlgItem(IDC_OPEN_EXTRA_WIZ) || (HWND) lParam == GetDlgItem(IDC_UPLOAD_SLOTS_WIZ)) {
		if (ul)
			::SetTextColor(hDC, RGB(0,0,0));
		else
			::SetTextColor(hDC, RGB(100,100,100));

		::SetBkMode(hDC, TRANSPARENT);
		return (LRESULT)GetStockObject(HOLLOW_BRUSH);
	}


	if(((HWND) lParam == GetDlgItem(IDC_MAX_DL_WIZ) || (HWND) lParam == GetDlgItem(IDC_MAX_DL_SPEED_WIZ))) {
		if (dl)
			::SetTextColor(hDC, RGB(0,0,0));
		else
			::SetTextColor(hDC, RGB(100,100,100));

		::SetBkMode(hDC, TRANSPARENT);
		return (LRESULT)GetStockObject(HOLLOW_BRUSH);
	}
	*/
	return FALSE;
}

void WizardDlg::fixcontrols() {
	if(IsDlgButtonChecked(IDC_PUBLIC)){
		CheckDlgButton(IDC_RAR, BST_UNCHECKED);
		CheckDlgButton(IDC_PRIVATE_HUB, BST_UNCHECKED);
		::EnableWindow(GetDlgItem(IDC_WIZARD_SKIPLIST),	0);
	}
	if(IsDlgButtonChecked(IDC_RAR)){
		CheckDlgButton(IDC_PRIVATE_HUB, BST_UNCHECKED);
		CheckDlgButton(IDC_PUBLIC, BST_UNCHECKED);
		::EnableWindow(GetDlgItem(IDC_WIZARD_SKIPLIST),	1);
	}
	if(IsDlgButtonChecked(IDC_PRIVATE_HUB)){
		CheckDlgButton(IDC_RAR, BST_UNCHECKED);
		CheckDlgButton(IDC_PUBLIC, BST_UNCHECKED);
		::EnableWindow(GetDlgItem(IDC_WIZARD_SKIPLIST),	0);
	}

	BOOL dl = IsDlgButtonChecked(IDC_DL_AUTODETECT_WIZ) != BST_CHECKED;
	::EnableWindow(GetDlgItem(IDC_DOWNLOAD_SLOTS),				dl);
	::EnableWindow(GetDlgItem(IDC_MAX_DOWNLOAD_SP),				dl);
	::EnableWindow(GetDlgItem(IDC_MAX_DL_WIZ),				dl);
	::EnableWindow(GetDlgItem(IDC_MAX_DL_SPEED_WIZ),				dl);

	BOOL ul = IsDlgButtonChecked(IDC_UL_AUTODETECT_WIZ) != BST_CHECKED;
	::EnableWindow(GetDlgItem(IDC_UPLOAD_SLOTS),				ul);
	::EnableWindow(GetDlgItem(IDC_MAX_UPLOAD_SP),				ul);
	::EnableWindow(GetDlgItem(IDC_MAX_AUTO_OPENED),				ul);
	::EnableWindow(GetDlgItem(IDC_MAX_AUTO_WIZ),				ul);
	::EnableWindow(GetDlgItem(IDC_OPEN_EXTRA_WIZ),				ul);
	::EnableWindow(GetDlgItem(IDC_UPLOAD_SLOTS_WIZ),			ul);


	TCHAR buf[64];
	GetDlgItemText(IDC_CONNECTION, buf, sizeof(buf) +1);
	double uploadvalue = Util::toDouble(Text::fromT(buf));
	setUploadLimits(uploadvalue);

	TCHAR buf2[64];
	GetDlgItemText(IDC_DOWN_SPEED, buf2, sizeof(buf2) +1);
	double downloadvalue = Util::toDouble(Text::fromT(buf2));
	setDownloadLimits(downloadvalue);
	
}

void WizardDlg::setLang() {

	if(SETTING(LANGUAGE_SWITCH) == 0) {
		SettingsManager::getInstance()->set(SettingsManager::LANGUAGE_FILE, Util::emptyString);
	} else if(SETTING(LANGUAGE_SWITCH) == 1) {
		SettingsManager::getInstance()->set(SettingsManager::LANGUAGE_FILE, (Util::getPath(Util::PATH_GLOBAL_CONFIG) + "Language//Swedish_for_AirDc.xml"));
	} else if(SETTING(LANGUAGE_SWITCH) == 2) {
		SettingsManager::getInstance()->set(SettingsManager::LANGUAGE_FILE, (Util::getPath(Util::PATH_GLOBAL_CONFIG) + "Language//Finnish_for_AirDc.xml"));
	} else if(SETTING(LANGUAGE_SWITCH) == 3) {
		SettingsManager::getInstance()->set(SettingsManager::LANGUAGE_FILE, (Util::getPath(Util::PATH_GLOBAL_CONFIG) + "Language//Italian_for_AirDc.xml"));
	} else if(SETTING(LANGUAGE_SWITCH) == 4) {
		SettingsManager::getInstance()->set(SettingsManager::LANGUAGE_FILE, (Util::getPath(Util::PATH_GLOBAL_CONFIG) + "Language//Hungarian_for_AirDc.xml"));
	} else if(SETTING(LANGUAGE_SWITCH) == 5) {
		SettingsManager::getInstance()->set(SettingsManager::LANGUAGE_FILE, (Util::getPath(Util::PATH_GLOBAL_CONFIG) + "Language//Romanian_for_AirDc.xml"));
	} else if(SETTING(LANGUAGE_SWITCH) == 6) {
		SettingsManager::getInstance()->set(SettingsManager::LANGUAGE_FILE, (Util::getPath(Util::PATH_GLOBAL_CONFIG) + "Language//Danish_for_AirDc.xml"));
	} else if(SETTING(LANGUAGE_SWITCH) == 7) {
		SettingsManager::getInstance()->set(SettingsManager::LANGUAGE_FILE, (Util::getPath(Util::PATH_GLOBAL_CONFIG) + "Language//Norwegian_for_AirDc.xml"));
	} else if(SETTING(LANGUAGE_SWITCH) == 8) {
		SettingsManager::getInstance()->set(SettingsManager::LANGUAGE_FILE, (Util::getPath(Util::PATH_GLOBAL_CONFIG) + "Language//Port_Br_for_AirDc.xml"));
    } else if(SETTING(LANGUAGE_SWITCH) == 9) {
		SettingsManager::getInstance()->set(SettingsManager::LANGUAGE_FILE, (Util::getPath(Util::PATH_GLOBAL_CONFIG) + "Language//Polish_for_AirDc.xml"));
	} else if(SETTING(LANGUAGE_SWITCH) == 10) {
		SettingsManager::getInstance()->set(SettingsManager::LANGUAGE_FILE, (Util::getPath(Util::PATH_GLOBAL_CONFIG) + "Language//French_for_AirDc.xml"));
	} else if(SETTING(LANGUAGE_SWITCH) == 11) {
		SettingsManager::getInstance()->set(SettingsManager::LANGUAGE_FILE, (Util::getPath(Util::PATH_GLOBAL_CONFIG) + "Language//Dutch_for_AirDc.xml"));
	} else if(SETTING(LANGUAGE_SWITCH) == 12) {
		SettingsManager::getInstance()->set(SettingsManager::LANGUAGE_FILE, (Util::getPath(Util::PATH_GLOBAL_CONFIG) + "Language//Russian_for_AirDc.xml"));
	} else if(SETTING(LANGUAGE_SWITCH) == 13) {
		SettingsManager::getInstance()->set(SettingsManager::LANGUAGE_FILE, (Util::getPath(Util::PATH_GLOBAL_CONFIG) + "Language//German_for_AirDc.xml"));
	}

}
void WizardDlg::setDownloadLimits(double value) {

	if (IsDlgButtonChecked(IDC_DL_AUTODETECT_WIZ)) {
		int dlSlots=Util::getSlots(true, value, IsDlgButtonChecked(IDC_RAR) ? true : false);
		SetDlgItemText(IDC_DOWNLOAD_SLOTS, Util::toStringW(dlSlots).c_str());
	
		int dlLimit=Util::getSpeedLimit(true, value);
		SetDlgItemText(IDC_MAX_DOWNLOAD_SP, Util::toStringW(dlLimit).c_str());
	}
}

void WizardDlg::setUploadLimits(double value) {

	if (IsDlgButtonChecked(IDC_UL_AUTODETECT_WIZ)) {

		int ulSlots=Util::getSlots(false, value, IsDlgButtonChecked(IDC_RAR) ? true : false);
		SetDlgItemText(IDC_UPLOAD_SLOTS, Util::toStringW(ulSlots).c_str());
	
		int ulLimit=Util::getSpeedLimit(false, value);
		SetDlgItemText(IDC_MAX_UPLOAD_SP, Util::toStringW(ulLimit).c_str());

		int autoOpened=Util::getMaxAutoOpened(value);
		SetDlgItemText(IDC_MAX_AUTO_OPENED, Util::toStringW(autoOpened).c_str());
	}
}