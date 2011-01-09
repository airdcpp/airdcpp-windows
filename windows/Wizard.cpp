#include "stdafx.h"

#include "../client/DCPlusPlus.h"
#include "../client/SettingsManager.h"

#include "Resource.h"

#include "Wizard.h"

//Todo Make Translateable
static const TCHAR rar[] = 
_T("Download Slot settings are set depending download speed\r\n")
_T("Upload Slots are set depending upload speed\r\n\r\n")
_T("Client profile Rar-Hub\r\n")
_T("Partial Upload slots will be set To: 1\r\n")
_T("Enable Segmented downloading will be set To: true\r\n")
_T("Manual number of Segments Enable will be set To: false\r\n")
_T("Manual number of Segments will be set To: 3\r\n")
_T("Min segments size will be set To: 1024\r\n")
_T("Expand Downloads in TransferView will be set To: false \r\n");

static const TCHAR publichub[] = 
_T("Download Slot settings are set depending download speed\r\n")
_T("Upload Slots are set depending upload speed\r\n\r\n")
_T("Client profile Public Hubs\r\n")
_T("Partial Upload slots will be set To: 2\r\n")
_T("Enable Segmented downloading will be set To: true\r\n")
_T("Manual number of Segments Enable will be set To: false\r\n")
_T("Manual number of Segments will be set To: 3\r\n")
_T("Min segments size will be set To: 1024\r\n")
_T("Expand Downloads in TransferView will be set To: false \r\n");

static const TCHAR nonsegment[] = 
_T("Download Slot settings are set depending download speed\r\n")
_T("Upload Slots are set depending upload speed\r\n\r\n")
_T("Client profile No Segments \r\n")
_T("Partial Upload slots will be set To: 0 \r\n")
//_T("Enable Segmented downloading will be set To: true\r\n") Maybe not show this for the users to avoid misunderstanding it
_T("Manual number of Segments will be set To: 1\r\n")
_T("Min segments size will be set To: largest value \r\n")
_T("Expand Downloads in TransferView will be set To: true \r\n");

LRESULT WizardDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		
	explain.Attach(GetDlgItem(IDC_EXPLAIN));
	ctrlDownload.Attach(GetDlgItem(IDC_DOWN_SPEED));
	ctrlUpload.Attach(GetDlgItem(IDC_CONNECTION));
	ctrlLanguage.Attach(GetDlgItem(IDC_LANGUAGE));


		for(StringIter i = SettingsManager::Languages.begin(); i != SettingsManager::Languages.end(); ++i)
		ctrlLanguage.AddString(Text::toT(*i).c_str());
		
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


	//Set current nick setting
		nickline.Attach(GetDlgItem(IDC_NICK));
		SetDlgItemText(IDC_NICK, Text::toT(SETTING(NICK)).c_str());

		switch(SETTING(SETTINGS_PROFILE)) {
		case SettingsManager::PROFILE_PUBLIC: CheckDlgButton(IDC_PUBLIC, BST_CHECKED); break;
		case SettingsManager::PROFILE_RAR: CheckDlgButton(IDC_RAR, BST_CHECKED); break;
		case SettingsManager::PROFILE_NONSEGMENT: CheckDlgButton(IDC_NON_SEGMENT, BST_CHECKED); break;
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


	TCHAR buf[64];
	GetDlgItemText(IDC_NICK, buf, sizeof(buf) + 1);
	string nick = Text::fromT(buf);
	if(nick != Util::emptyString)
		SettingsManager::getInstance()->set(SettingsManager::NICK, nick );

	TCHAR buf2[64];
	GetDlgItemText(IDC_DOWN_SPEED, buf2, sizeof(buf2) + 1);
	string download = Text::fromT(buf2);
	SettingsManager::getInstance()->set(SettingsManager::DOWNLOAD_SPEED, download);

		int value = Util::toInt(download); //compare as int?

		int speed = value *100; // * 100 is close enough?
		SettingsManager::getInstance()->set(SettingsManager::MAX_DOWNLOAD_SPEED, speed);

		/*Any Ideas for good Download Slot settings for each download speed*/
		if(value <= 1){
			SettingsManager::getInstance()->set(SettingsManager::FILE_SLOTS, 5);
			SettingsManager::getInstance()->set(SettingsManager::DOWNLOAD_SLOTS, 10);
			SettingsManager::getInstance()->set(SettingsManager::EXTRA_DOWNLOAD_SLOTS, 3);
            
		}else if(value > 1 && value <= 2) {
			  SettingsManager::getInstance()->set(SettingsManager::FILE_SLOTS, 10);
			SettingsManager::getInstance()->set(SettingsManager::DOWNLOAD_SLOTS, 20);
			SettingsManager::getInstance()->set(SettingsManager::EXTRA_DOWNLOAD_SLOTS, 4);
			
		}else if( value > 2 && value <= 4) {
			SettingsManager::getInstance()->set(SettingsManager::FILE_SLOTS, 12);
			SettingsManager::getInstance()->set(SettingsManager::DOWNLOAD_SLOTS, 25);
			SettingsManager::getInstance()->set(SettingsManager::EXTRA_DOWNLOAD_SLOTS, 4);

		}else if( value > 4 && value <= 6) {
			SettingsManager::getInstance()->set(SettingsManager::FILE_SLOTS, 15);
			SettingsManager::getInstance()->set(SettingsManager::DOWNLOAD_SLOTS, 28);
			SettingsManager::getInstance()->set(SettingsManager::EXTRA_DOWNLOAD_SLOTS, 4);

		 }else if( value > 6 && value <= 8) {
			SettingsManager::getInstance()->set(SettingsManager::FILE_SLOTS, 15);
			SettingsManager::getInstance()->set(SettingsManager::DOWNLOAD_SLOTS, 30);
			SettingsManager::getInstance()->set(SettingsManager::EXTRA_DOWNLOAD_SLOTS, 4);
		   
		}else if( value > 8 && value <= 10) { //Setting the same counts now, if we dont change it then dont need this much conditions
			SettingsManager::getInstance()->set(SettingsManager::FILE_SLOTS, 15);
			SettingsManager::getInstance()->set(SettingsManager::DOWNLOAD_SLOTS, 30);
			SettingsManager::getInstance()->set(SettingsManager::EXTRA_DOWNLOAD_SLOTS, 4);
		   
		}else if( value > 10 && value <= 20) {
			SettingsManager::getInstance()->set(SettingsManager::FILE_SLOTS, 20);
			SettingsManager::getInstance()->set(SettingsManager::DOWNLOAD_SLOTS, 30);
			SettingsManager::getInstance()->set(SettingsManager::EXTRA_DOWNLOAD_SLOTS, 4);
		   
		}else if( value > 50 && value <= 100) {
			SettingsManager::getInstance()->set(SettingsManager::FILE_SLOTS, 25);
			SettingsManager::getInstance()->set(SettingsManager::DOWNLOAD_SLOTS, 35);
			SettingsManager::getInstance()->set(SettingsManager::EXTRA_DOWNLOAD_SLOTS, 5);
		  
		}else if( value > 100) {
			SettingsManager::getInstance()->set(SettingsManager::FILE_SLOTS, 30);
			SettingsManager::getInstance()->set(SettingsManager::DOWNLOAD_SLOTS, 50);
			SettingsManager::getInstance()->set(SettingsManager::EXTRA_DOWNLOAD_SLOTS, 8);
		   }
	
		
	TCHAR buf3[64];
	GetDlgItemText(IDC_CONNECTION, buf3, sizeof(buf3) + 1);
	string connection = Text::fromT(buf3);
	SettingsManager::getInstance()->set(SettingsManager::UPLOAD_SPEED, connection);
		//using different int just to be safe
		int val = Util::toInt(connection); //compare as int?

		/*These are just out of the head, Will be more spesific when this is finished*/
		if(val <= 1){
			SettingsManager::getInstance()->set(SettingsManager::SLOTS, 2);
            
		}else if(val > 1 && val <= 2) {
			   SettingsManager::getInstance()->set(SettingsManager::SLOTS, 3);
			
		}else if( val > 2 && val <= 4) {
			SettingsManager::getInstance()->set(SettingsManager::SLOTS, 4);

		}else if( val > 4 && val <= 6) {
			SettingsManager::getInstance()->set(SettingsManager::SLOTS, 5);

		 }else if( val > 6 && val <= 8) {
			SettingsManager::getInstance()->set(SettingsManager::SLOTS, 6);
		   
		}else if( val > 8 && val <= 10) {
			SettingsManager::getInstance()->set(SettingsManager::SLOTS, 8);
		   
		}else if( val > 10 && val <= 20) {
			SettingsManager::getInstance()->set(SettingsManager::SLOTS, 15);
		   
		}else if( val > 50 && val <= 100) {
			SettingsManager::getInstance()->set(SettingsManager::SLOTS, 25);
		  
		}else if( val > 100) {
			SettingsManager::getInstance()->set(SettingsManager::SLOTS, 50);
		   }
		
	

		/*Make settings depending depending selected client settings profile
			Note that if add a setting to one profile will need to add it to other profiles too*/
		if(IsDlgButtonChecked(IDC_PUBLIC)){
			SettingsManager::getInstance()->set(SettingsManager::EXTRA_PARTIAL_SLOTS, 2);
			SettingsManager::getInstance()->set(SettingsManager::MULTI_CHUNK, true);
			SettingsManager::getInstance()->set(SettingsManager::NUMBER_OF_SEGMENTS, 3);
			SettingsManager::getInstance()->set(SettingsManager::SEGMENTS_MANUAL, false);
			SettingsManager::getInstance()->set(SettingsManager::MIN_SEGMENT_SIZE, 1024);
			SettingsManager::getInstance()->set(SettingsManager::DOWNLOADS_EXPAND, false);
			//add more here

			SettingsManager::getInstance()->set(SettingsManager::SETTINGS_PROFILE, SettingsManager::PROFILE_PUBLIC);

		}else if(IsDlgButtonChecked(IDC_RAR)){
			SettingsManager::getInstance()->set(SettingsManager::EXTRA_PARTIAL_SLOTS, 1);
			SettingsManager::getInstance()->set(SettingsManager::MULTI_CHUNK, true);
			SettingsManager::getInstance()->set(SettingsManager::NUMBER_OF_SEGMENTS, 3);
			SettingsManager::getInstance()->set(SettingsManager::SEGMENTS_MANUAL, false);
			SettingsManager::getInstance()->set(SettingsManager::MIN_SEGMENT_SIZE, 1024);
			SettingsManager::getInstance()->set(SettingsManager::DOWNLOADS_EXPAND, false);
			//add more here
			
			SettingsManager::getInstance()->set(SettingsManager::SETTINGS_PROFILE, SettingsManager::PROFILE_RAR);

		}else if(IsDlgButtonChecked(IDC_NON_SEGMENT)){
			SettingsManager::getInstance()->set(SettingsManager::MULTI_CHUNK, true);
			SettingsManager::getInstance()->set(SettingsManager::EXTRA_PARTIAL_SLOTS, 0);
			SettingsManager::getInstance()->set(SettingsManager::NUMBER_OF_SEGMENTS, 1);
			SettingsManager::getInstance()->set(SettingsManager::SEGMENTS_MANUAL, true);
			SettingsManager::getInstance()->set(SettingsManager::MIN_SEGMENT_SIZE, 2147483647);
			SettingsManager::getInstance()->set(SettingsManager::DOWNLOADS_EXPAND, true);
			//add more here
			
			SettingsManager::getInstance()->set(SettingsManager::SETTINGS_PROFILE, SettingsManager::PROFILE_NONSEGMENT);
		}

		if(IsDlgButtonChecked(IDC_CHECK1)){
			SettingsManager::getInstance()->set(SettingsManager::WIZARD_RUN, true);
		}
		
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
void WizardDlg::fixcontrols() {
	if(IsDlgButtonChecked(IDC_PUBLIC)){
		CheckDlgButton(IDC_RAR, BST_UNCHECKED);
		CheckDlgButton(IDC_NON_SEGMENT, BST_UNCHECKED);
		explain.SetWindowText(publichub);
	}
	if(IsDlgButtonChecked(IDC_RAR)){
		CheckDlgButton(IDC_NON_SEGMENT, BST_UNCHECKED);
		CheckDlgButton(IDC_PUBLIC, BST_UNCHECKED);
		explain.SetWindowText(rar);
	}
	if(IsDlgButtonChecked(IDC_NON_SEGMENT)){
		CheckDlgButton(IDC_RAR, BST_UNCHECKED);
		CheckDlgButton(IDC_PUBLIC, BST_UNCHECKED);
		explain.SetWindowText(nonsegment);
	}

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

	}

}