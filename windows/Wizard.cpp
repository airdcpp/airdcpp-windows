#include "stdafx.h"

#include "../client/DCPlusPlus.h"
#include "../client/SettingsManager.h"

#include "Resource.h"

#include "Wizard.h"

//Todo Make Translateable
static const TCHAR rar[] = 
_T("Client profile Rar-Hub\r\n")
_T("Partial Upload slots will be set To: 1\r\n")
_T("Enable Segmented downloading will be set To: true\r\n")
_T("Manual number of Segments Enable will be set To: false\r\n")
_T("Manual number of Segments will be set To: 3\r\n")
_T("Min segments size will be set To: 1024\r\n")
_T("Expand Downloads in TransferView will be set To: false \r\n");

static const TCHAR publichub[] = 
_T("Client profile Public Hubs\r\n")
_T("Partial Upload slots will be set To: 2\r\n")
_T("Enable Segmented downloading will be set To: true\r\n")
_T("Manual number of Segments Enable will be set To: false\r\n")
_T("Manual number of Segments will be set To: 3\r\n")
_T("Min segments size will be set To: 1024\r\n")
_T("Expand Downloads in TransferView will be set To: false \r\n");

static const TCHAR nonsegment[] = 
_T("Client profile No Segments \r\n")
_T("Partial Upload slots will be set To: 0 \r\n")
//_T("Enable Segmented downloading will be set To: true\r\n") Maybe not show this for the users to avoid misunderstanding it
_T("Manual number of Segments will be set To: 1\r\n")
_T("Min segments size will be set To: largest value \r\n")
_T("Expand Downloads in TransferView will be set To: true \r\n");

LRESULT WizardDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		
	explain.Attach(GetDlgItem(IDC_EXPLAIN));

	ctrlConnection.Attach(GetDlgItem(IDC_CONNECTION));

	//add the upload speed strings	
	for(StringIter i = SettingsManager::connectionSpeeds.begin(); i != SettingsManager::connectionSpeeds.end(); ++i)
		ctrlConnection.AddString(Text::toT(*i).c_str());
	
	//set current upload speed setting
	ctrlConnection.SetCurSel(ctrlConnection.FindString(0, Text::toT(SETTING(UPLOAD_SPEED)).c_str()));

	//if we have a custom upload speed
	if(find(SettingsManager::connectionSpeeds.begin(), SettingsManager::connectionSpeeds.end(),
			SETTING(UPLOAD_SPEED)) == SettingsManager::connectionSpeeds.end()) {
		ctrlConnection.AddString(Text::toT(SETTING(UPLOAD_SPEED)).c_str());
	}

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
	
	TCHAR buf[64];
	GetDlgItemText(IDC_NICK, buf, sizeof(buf) + 1);
	string nick = Text::fromT(buf);
	if(nick != Util::emptyString)
		SettingsManager::getInstance()->set(SettingsManager::NICK, nick );


	TCHAR buf2[64];
	GetDlgItemText(IDC_CONNECTION, buf2, sizeof(buf) + 1);
	string connection = Text::fromT(buf2);
	SettingsManager::getInstance()->set(SettingsManager::UPLOAD_SPEED, connection);

		int value = Util::toInt(connection); //compare as int?

		/*These are just out of the head, Will be more spesific when this is finished*/
		if(value <= 1){
			SettingsManager::getInstance()->set(SettingsManager::SLOTS, 2);
            
		}else if(value > 1 && value <= 2) {
			   SettingsManager::getInstance()->set(SettingsManager::SLOTS, 3);
			
		}else if( value > 2 && value <= 4) {
			SettingsManager::getInstance()->set(SettingsManager::SLOTS, 4);

		}else if( value > 4 && value <= 6) {
			SettingsManager::getInstance()->set(SettingsManager::SLOTS, 5);

		 }else if( value > 6 && value <= 8) {
			SettingsManager::getInstance()->set(SettingsManager::SLOTS, 6);
		   
		}else if( value > 8 && value <= 10) {
			SettingsManager::getInstance()->set(SettingsManager::SLOTS, 8);
		   
		}else if( value > 10 && value <= 20) {
			SettingsManager::getInstance()->set(SettingsManager::SLOTS, 15);
		   
		}else if( value > 50 && value <= 100) {
			SettingsManager::getInstance()->set(SettingsManager::SLOTS, 25);
		  
		}else if( value > 100) {
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

}
	


LRESULT WizardDlg::onNext(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	write();
	//bHandled = FALSE;
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