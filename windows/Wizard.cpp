#include "stdafx.h"

#include "../client/DCPlusPlus.h"
#include "../client/SettingsManager.h"
#include "../client/HighlightManager.h"
#include "Resource.h"
#include "WinUtil.h"
#include "Wizard.h"


static const TCHAR rar[] = 
_T("Client profile Rar-Hub\r\n")
_T("This will disable segment downloading by setting min segment size!!\r\n")
_T("( Safer Way to Disable segments )\r\n")
_T("Grouped Downloads will be automatically Expanded.\r\n")
_T("Partial Upload slots will be set To: 1\r\n");


static const TCHAR publichub[] = 
_T("Client profile Public Hubs\r\n")
_T("This Will Enable segmented Downloading\r\n")
_T("Min Segments size will be set to normal default value\r\n")
_T("Partial Upload slots will be set To: 2\r\n");

static const TCHAR privatehub[] = 
_T("Client profile Private Hub \r\n")
_T("SegmentDownloading will be Enabled\r\n")
_T("Min segments size will be set To: 2048 \r\n")
_T("Partial Upload slots will be set To: 2 \r\n");

LRESULT WizardDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
			
	//Set current nick setting
		nickline.Attach(GetDlgItem(IDC_NICK));
		SetDlgItemText(IDC_NICK, Text::toT(SETTING(NICK)).c_str());

	explain.Attach(GetDlgItem(IDC_EXPLAIN));
	ctrlDownload.Attach(GetDlgItem(IDC_DOWN_SPEED));
	ctrlUpload.Attach(GetDlgItem(IDC_CONNECTION));
	ctrlLanguage.Attach(GetDlgItem(IDC_LANGUAGE));

	CUpDownCtrl spin;
	spin.Attach(GetDlgItem(IDC_DOWN_SPEED_SPIN));
	spin.SetRange32(0, 100000);
	spin.Detach();

	CUpDownCtrl spin2;
	spin2.Attach(GetDlgItem(IDC_FILE_S_SPIN));
	spin2.SetRange32(0, 999);
	spin2.Detach();

	CUpDownCtrl spin3;
	spin3.Attach(GetDlgItem(IDC_DOWNLOAD_S_SPIN));
	spin3.SetRange32(0, 999);
	spin3.Detach();

	CUpDownCtrl spin4;
	spin4.Attach(GetDlgItem(IDC_UPLOAD_S_SPIN));
	spin4.SetRange32(1, 100);
	spin4.Detach();
		
		
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
		

		//add the Download speed strings, Using the same list as upload	
	for(StringIter i = SettingsManager::connectionSpeeds.begin(); i != SettingsManager::connectionSpeeds.end(); ++i)
		ctrlDownload.AddString(Text::toT(*i).c_str());
		//if we have a custom Download speed
	
	if(find(SettingsManager::connectionSpeeds.begin(), SettingsManager::connectionSpeeds.end(),
			SETTING(DOWNLOAD_SPEED)) == SettingsManager::connectionSpeeds.end()) {
		ctrlDownload.AddString(Text::toT(SETTING(DOWNLOAD_SPEED)).c_str());
	}
	//set current Download speed setting
	
	ctrlDownload.SetCurSel(ctrlDownload.FindString(0, Text::toT(SETTING(DOWNLOAD_SPEED)).c_str()));




	//add the upload speed strings	
	for(StringIter i = SettingsManager::connectionSpeeds.begin(); i != SettingsManager::connectionSpeeds.end(); ++i)
		ctrlUpload.AddString(Text::toT(*i).c_str());
		//if we have a custom upload speed
	if(find(SettingsManager::connectionSpeeds.begin(), SettingsManager::connectionSpeeds.end(),
			SETTING(UPLOAD_SPEED)) == SettingsManager::connectionSpeeds.end()) {
		ctrlUpload.AddString(Text::toT(SETTING(UPLOAD_SPEED)).c_str());
	}
	//set current upload speed setting
	ctrlUpload.SetCurSel(ctrlUpload.FindString(0, Text::toT(SETTING(UPLOAD_SPEED)).c_str()));


	//Set current values
	SetDlgItemText(IDC_MAX_DOWNLOAD_SP, Text::toT(Util::toString(SETTING(MAX_DOWNLOAD_SPEED))).c_str());
	SetDlgItemText(IDC_DOWNLOAD_SLOTS, Text::toT(Util::toString(SETTING(DOWNLOAD_SLOTS))).c_str());
	SetDlgItemText(IDC_UPLOAD_SLOTS, Text::toT(Util::toString(SETTING(SLOTS))).c_str());
	SetDlgItemText(IDC_FILE_SLOTS, Text::toT(Util::toString(SETTING(FILE_SLOTS))).c_str());


		switch(SETTING(SETTINGS_PROFILE)) {
		case SettingsManager::PROFILE_PUBLIC: CheckDlgButton(IDC_PUBLIC, BST_CHECKED); break;
		case SettingsManager::PROFILE_RAR: CheckDlgButton(IDC_RAR, BST_CHECKED); break;
		case SettingsManager::PROFILE_PRIVATE: CheckDlgButton(IDC_PRIVATE_HUB, BST_CHECKED); break;
		default: CheckDlgButton(IDC_PUBLIC, BST_CHECKED); break;
	}

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
	//end
		
		//speed settings
		SettingsManager::getInstance()->set(SettingsManager::UPLOAD_SPEED, upload);
		SettingsManager::getInstance()->set(SettingsManager::DOWNLOAD_SPEED, download);
	
			//for max download speed
			TCHAR buf2[64];
			GetDlgItemText(IDC_MAX_DOWNLOAD_SP, buf2, sizeof(buf2) + 1);
			string downloadspeed = Text::fromT(buf2);
			//int value = Util::toInt(downloadspeed); 
			SettingsManager::getInstance()->set(SettingsManager::MAX_DOWNLOAD_SPEED, downloadspeed);
			//end


			//File Slots
			TCHAR buf3[64];
			GetDlgItemText(IDC_FILE_SLOTS, buf3, sizeof(buf3) + 1);
			SettingsManager::getInstance()->set(SettingsManager::FILE_SLOTS, Text::fromT(buf3));
			//end

			//Download Slots
			TCHAR buf4[64];
			GetDlgItemText(IDC_DOWNLOAD_SLOTS, buf4, sizeof(buf4) + 1);
			SettingsManager::getInstance()->set(SettingsManager::DOWNLOAD_SLOTS, Text::fromT(buf4));
			//end
			
			//Upload Slots
			TCHAR buf5[64];
			GetDlgItemText(IDC_UPLOAD_SLOTS, buf5, sizeof(buf5) + 1);
			SettingsManager::getInstance()->set(SettingsManager::SLOTS, Text::fromT(buf5));
			//end

		/*Make settings depending selected client settings profile
			Note that if add a setting to one profile will need to add it to other profiles too*/
		if(IsDlgButtonChecked(IDC_PUBLIC)){
			SettingsManager::getInstance()->set(SettingsManager::EXTRA_PARTIAL_SLOTS, 2);
			SettingsManager::getInstance()->set(SettingsManager::MULTI_CHUNK, true);
			SettingsManager::getInstance()->set(SettingsManager::MIN_SEGMENT_SIZE, 1024);
			SettingsManager::getInstance()->set(SettingsManager::DOWNLOADS_EXPAND, false);
			//add more here

			SettingsManager::getInstance()->set(SettingsManager::SETTINGS_PROFILE, SettingsManager::PROFILE_PUBLIC);

		}else if(IsDlgButtonChecked(IDC_RAR)){
			SettingsManager::getInstance()->set(SettingsManager::EXTRA_PARTIAL_SLOTS, 1);
			SettingsManager::getInstance()->set(SettingsManager::MULTI_CHUNK, true);
			SettingsManager::getInstance()->set(SettingsManager::SEGMENTS_MANUAL, false);
			SettingsManager::getInstance()->set(SettingsManager::MIN_SEGMENT_SIZE, 10240000);
			SettingsManager::getInstance()->set(SettingsManager::DOWNLOADS_EXPAND, true);
			SettingsManager::getInstance()->set(SettingsManager::SKIPLIST_SHARE, "(.*(\\.(scn|asd|lnk|cmd|conf|dll|url|log|crc|dat|sfk|mxm|txt|message|iso|inf|sub|exe|img|bin|aac|mrg|tmp|xml|sup|ini|db|debug|pls|ac3|ape|par2|htm(l)?|bat|idx|srt|doc(x)?|ion|cue|b4s|bgl|cab|cat|bat)$))|((All-Files-CRC-OK|xCOMPLETEx|imdb.nfo|- Copy|(.*\\(\\d\\).*)).*$)");
			SettingsManager::getInstance()->set(SettingsManager::SHARE_SKIPLIST_USE_REGEXP, true);
			SettingsManager::getInstance()->set(SettingsManager::CHECK_SFV, true);
			SettingsManager::getInstance()->set(SettingsManager::CHECK_NFO, true);
			SettingsManager::getInstance()->set(SettingsManager::CHECK_EXTRA_SFV_NFO, true);
			SettingsManager::getInstance()->set(SettingsManager::CHECK_EXTRA_FILES, true);
			SettingsManager::getInstance()->set(SettingsManager::CHECK_DUPES, true);
			//add more here
			
			SettingsManager::getInstance()->set(SettingsManager::SETTINGS_PROFILE, SettingsManager::PROFILE_RAR);

		}else if(IsDlgButtonChecked(IDC_PRIVATE_HUB)){
			SettingsManager::getInstance()->set(SettingsManager::MULTI_CHUNK, true);
			SettingsManager::getInstance()->set(SettingsManager::EXTRA_PARTIAL_SLOTS, 2);
			SettingsManager::getInstance()->set(SettingsManager::SEGMENTS_MANUAL, false);
			SettingsManager::getInstance()->set(SettingsManager::MIN_SEGMENT_SIZE, 1024);
			SettingsManager::getInstance()->set(SettingsManager::DOWNLOADS_EXPAND, false);
			//add more here
			
			SettingsManager::getInstance()->set(SettingsManager::SETTINGS_PROFILE, SettingsManager::PROFILE_PRIVATE);
		}


			if(IsDlgButtonChecked(IDC_RELEASE_LINKS)){
				SettingsManager::getInstance()->set(SettingsManager::FORMAT_RELEASE, true);
			} else {
				SettingsManager::getInstance()->set(SettingsManager::FORMAT_RELEASE, false);
			}

		if(IsDlgButtonChecked(IDC_CHECK1)){
			SettingsManager::getInstance()->set(SettingsManager::WIZARD_RUN, true);
		}
		SettingsManager::getInstance()->save();
}
	


LRESULT WizardDlg::onNext(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	write();
	EndDialog(wID);
	return 0;

}
LRESULT WizardDlg::OnDlgButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{

	fixcontrols();
	return 0;
}
LRESULT WizardDlg::OnDownSpeed(WORD wNotifyCode, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	TCHAR buf2[64];
	//need to do it like this because we have support for custom speeds, do we really need that?
	switch(wNotifyCode) {

	case CBN_EDITCHANGE:
	GetDlgItemText(IDC_DOWN_SPEED, buf2, sizeof(buf2) +1);
	break;

	case CBN_SELENDOK:
	ctrlDownload.GetLBText(ctrlDownload.GetCurSel(), buf2);
	break;
	}

	download = Text::fromT(buf2);

		double value = Util::toDouble(download); //compare as int?
		
		int speed = value *100; // * 100 is close enough?
		SetDlgItemText(IDC_MAX_DOWNLOAD_SP, Text::toT(Util::toString(speed)).c_str());

		setDownloadSlots(value);

		return 0;
}
LRESULT WizardDlg::OnUploadSpeed(WORD wNotifyCode, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	TCHAR buf2[64];
	//need to do it like this because we have support for custom speeds, do we really need that?
	switch(wNotifyCode) {
	case CBN_EDITCHANGE:
	GetDlgItemText(IDC_CONNECTION, buf2, sizeof(buf2) +1);
	break;
	case CBN_SELENDOK:
	ctrlUpload.GetLBText(ctrlUpload.GetCurSel(), buf2);
	break;

	}	
		upload = Text::fromT(buf2);
		int value = Util::toInt(upload); //compare as int?
		setUploadSlots(value);

		return 0;
}

void WizardDlg::fixcontrols() {
	if(IsDlgButtonChecked(IDC_PUBLIC)){
		CheckDlgButton(IDC_RAR, BST_UNCHECKED);
		CheckDlgButton(IDC_PRIVATE_HUB, BST_UNCHECKED);
		CheckDlgButton(IDC_RELEASE_LINKS, BST_UNCHECKED);
		explain.SetWindowText(publichub);
	}
	if(IsDlgButtonChecked(IDC_RAR)){
		CheckDlgButton(IDC_PRIVATE_HUB, BST_UNCHECKED);
		CheckDlgButton(IDC_PUBLIC, BST_UNCHECKED);
		CheckDlgButton(IDC_RELEASE_LINKS, BST_CHECKED);
		explain.SetWindowText(rar);
	}
	if(IsDlgButtonChecked(IDC_PRIVATE_HUB)){
		CheckDlgButton(IDC_RAR, BST_UNCHECKED);
		CheckDlgButton(IDC_PUBLIC, BST_UNCHECKED);
		CheckDlgButton(IDC_RELEASE_LINKS, BST_UNCHECKED);
		explain.SetWindowText(privatehub);
	}

	TCHAR buf[64];
	GetDlgItemText(IDC_CONNECTION, buf, sizeof(buf) +1);
	int uploadvalue = Util::toInt(Text::fromT(buf));
	setUploadSlots(uploadvalue);

	TCHAR buf2[64];
	GetDlgItemText(IDC_DOWN_SPEED, buf2, sizeof(buf2) +1);
	int downloadvalue = Util::toInt(Text::fromT(buf2));
	setDownloadSlots(downloadvalue);
	
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
void WizardDlg::setDownloadSlots(int value) {
	
		/*Any Ideas for good Download Slot settings for each download speed*/
		if(!IsDlgButtonChecked(IDC_RAR)) {
			
			//non-rar hubs!
		if(value <= 1) {
			SetDlgItemText(IDC_FILE_SLOTS, _T("5"));
			SetDlgItemText(IDC_DOWNLOAD_SLOTS, _T("10"));

		} else if(value > 1 && value <= 2) {
			SetDlgItemText(IDC_FILE_SLOTS, _T("10"));
			SetDlgItemText(IDC_DOWNLOAD_SLOTS, _T("15"));
		
		} else if( value > 2 && value <= 4) {
			SetDlgItemText(IDC_FILE_SLOTS, _T("11"));
			SetDlgItemText(IDC_DOWNLOAD_SLOTS, _T("15"));

		} else if( value > 4 && value <= 6) {
			SetDlgItemText(IDC_FILE_SLOTS, _T("15"));
			SetDlgItemText(IDC_DOWNLOAD_SLOTS, _T("20"));

		 } else if( value > 6 && value <= 10) {
			SetDlgItemText(IDC_FILE_SLOTS, _T("16"));
			SetDlgItemText(IDC_DOWNLOAD_SLOTS, _T("30"));
		   
		} else if( value > 10 && value <= 20) {
			SetDlgItemText(IDC_FILE_SLOTS, _T("20"));
			SetDlgItemText(IDC_DOWNLOAD_SLOTS, _T("32"));
		   
		} else if( value > 50 && value <= 100) {
			SetDlgItemText(IDC_FILE_SLOTS, _T("25"));
			SetDlgItemText(IDC_DOWNLOAD_SLOTS, _T("35"));
		  
		} else if( value > 100) {
			SetDlgItemText(IDC_FILE_SLOTS, _T("30"));
			SetDlgItemText(IDC_DOWNLOAD_SLOTS, _T("50"));
		   }
		//non-rar hubs end

		} else {
		if (value <= 1) {
			SetDlgItemText(IDC_FILE_SLOTS, _T("3"));
			SetDlgItemText(IDC_DOWNLOAD_SLOTS, _T("8"));

		} else if(value > 1 && value <= 2) {
			SetDlgItemText(IDC_FILE_SLOTS, _T("4"));
			SetDlgItemText(IDC_DOWNLOAD_SLOTS, _T("8"));
		
		} else if( value > 2 && value <= 4) {
			SetDlgItemText(IDC_FILE_SLOTS, _T("5"));
			SetDlgItemText(IDC_DOWNLOAD_SLOTS, _T("10"));

		} else if( value > 4 && value <= 6) {
			SetDlgItemText(IDC_FILE_SLOTS, _T("5"));
			SetDlgItemText(IDC_DOWNLOAD_SLOTS, _T("12"));

		 } else if( value > 6 && value <= 10) {
			SetDlgItemText(IDC_FILE_SLOTS, _T("6"));
			SetDlgItemText(IDC_DOWNLOAD_SLOTS, _T("15"));
		   
		} else if( value > 10 && value <= 20) {
			SetDlgItemText(IDC_FILE_SLOTS, _T("10"));
			SetDlgItemText(IDC_DOWNLOAD_SLOTS, _T("15"));
		   
		} else if( value > 50 && value <= 100) {
			SetDlgItemText(IDC_FILE_SLOTS, _T("30"));
			SetDlgItemText(IDC_DOWNLOAD_SLOTS, _T("40"));
		  
		} else if( value > 100) {
			SetDlgItemText(IDC_FILE_SLOTS, _T("50"));
			SetDlgItemText(IDC_DOWNLOAD_SLOTS, _T("70"));
		   }
		}
}

void WizardDlg::setUploadSlots(int value) {
		/*Good Upload slot counts per upload speed??*/

		if(!IsDlgButtonChecked(IDC_RAR)) {
			if (value <= 1) {
				SetDlgItemText(IDC_UPLOAD_SLOTS, _T("2"));

			} else if (value > 1 && value <= 2) {
				SetDlgItemText(IDC_UPLOAD_SLOTS, _T("3"));
		
			} else if (value > 2 && value <= 4) {
				SetDlgItemText(IDC_UPLOAD_SLOTS, _T("4"));

			} else if( value > 4 && value <= 6) {
				SetDlgItemText(IDC_UPLOAD_SLOTS, _T("5"));

			} else if( value > 6 && value <= 8) {
				SetDlgItemText(IDC_UPLOAD_SLOTS, _T("6"));

			} else if( value > 8 && value <= 10) {
				SetDlgItemText(IDC_UPLOAD_SLOTS, _T("8"));
		   
			} else if( value > 10 && value <= 20) {
				SetDlgItemText(IDC_UPLOAD_SLOTS, _T("12"));
		   
			} else if( value > 50 && value <= 100) {
				SetDlgItemText(IDC_UPLOAD_SLOTS, _T("15"));
		  
			} else if( value > 100) {
				SetDlgItemText(IDC_UPLOAD_SLOTS, _T("30"));
			}			

		} else {

			//For RAR hubs
			if (value <= 1) {
				SetDlgItemText(IDC_UPLOAD_SLOTS, _T("2"));

			} else if(value > 1 && value <= 2) {
				SetDlgItemText(IDC_UPLOAD_SLOTS, _T("2"));
		
			} else if( value > 2 && value <= 4) {
				SetDlgItemText(IDC_UPLOAD_SLOTS, _T("3"));

			} else if( value > 4 && value <= 6) {
				SetDlgItemText(IDC_UPLOAD_SLOTS, _T("3"));

			} else if( value > 6 && value <= 8) {
				SetDlgItemText(IDC_UPLOAD_SLOTS, _T("4"));

			} else if( value > 8 && value <= 10) {
				SetDlgItemText(IDC_UPLOAD_SLOTS, _T("5"));
		   
			} else if( value > 10 && value <= 20) {
				SetDlgItemText(IDC_UPLOAD_SLOTS, _T("6"));
		   
			} else if( value > 50 && value <= 100) {
				SetDlgItemText(IDC_UPLOAD_SLOTS, _T("9"));
		  
			} else if( value > 100) {
				SetDlgItemText(IDC_UPLOAD_SLOTS, _T("12"));
			}
			//RAR hubs end
		}
}