/* 
* Copyright (C) 2003 Twink,  spm7@waikato.ac.nz
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

#include "../Client/FavoriteManager.h"

#include "AVIPreview.h"
#include "PreviewDlg.h"

PropPage::TextItem AVIPreview::texts[] = {
	{ IDC_ADD_MENU, ResourceManager::ADD },
	{ IDC_CHANGE_MENU, ResourceManager::SETTINGS_CHANGE },
	{ IDC_REMOVE_MENU, ResourceManager::REMOVE },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};


LRESULT AVIPreview::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	PropPage::translate((HWND)(*this), texts); 

	CRect rc;

	ctrlCommands.Attach(GetDlgItem(IDC_MENU_ITEMS));
	ctrlCommands.GetClientRect(rc);

	ctrlCommands.InsertColumn(0, CTSTRING(SETTINGS_NAME), LVCFMT_LEFT, rc.Width()/5, 0);
	ctrlCommands.InsertColumn(1, CTSTRING(SETTINGS_COMMAND), LVCFMT_LEFT, rc.Width()*2 / 5, 1);
	ctrlCommands.InsertColumn(2, CTSTRING(SETTINGS_ARGUMENT), LVCFMT_LEFT, rc.Width() / 5, 2);
	ctrlCommands.InsertColumn(3, CTSTRING(SETTINGS_EXTENSIONS), LVCFMT_LEFT, rc.Width() / 5, 3);

	ctrlCommands.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

	// Do specialized reading here
	PreviewApplication::List lst = FavoriteManager::getInstance()->getPreviewApps();
	for(PreviewApplication::Iter i = lst.begin(); i != lst.end(); ++i) {
		PreviewApplication::Ptr pa = *i;	
		addEntry(pa, ctrlCommands.GetItemCount());
	}
	return 0;
}

void AVIPreview::write(){ }

void AVIPreview::addEntry(PreviewApplication::Ptr pa, int pos) {
	TStringList lst;
	lst.push_back(Text::toT(pa->getName()));
	lst.push_back(Text::toT(pa->getApplication())); 
	lst.push_back(Text::toT(pa->getArguments()));
	lst.push_back(Text::toT(pa->getExtension()));
	ctrlCommands.insert(pos, lst, 0, 0);
}


LRESULT AVIPreview::onAddMenu(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/){
	PreviewDlg dlg;
	if(dlg.DoModal() == IDOK){
		addEntry(FavoriteManager::getInstance()->addPreviewApp(Text::fromT(dlg.name), Text::fromT(dlg.application), Text::fromT(dlg.argument), Text::fromT(dlg.extensions)), ctrlCommands.GetItemCount());
	}
	return 0;
}

LRESULT AVIPreview::onItemchangedDirectories(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
	NM_LISTVIEW* lv = (NM_LISTVIEW*) pnmh;
	::EnableWindow(GetDlgItem(IDC_CHANGE_MENU), (lv->uNewState & LVIS_FOCUSED));
	::EnableWindow(GetDlgItem(IDC_REMOVE_MENU), (lv->uNewState & LVIS_FOCUSED));
	return 0;		
}

LRESULT AVIPreview::onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
	NMLVKEYDOWN* kd = (NMLVKEYDOWN*) pnmh;
	switch(kd->wVKey) {
	case VK_INSERT:
		PostMessage(WM_COMMAND, IDC_ADD_MENU, 0);
		break;
	case VK_DELETE:
		PostMessage(WM_COMMAND, IDC_REMOVE_MENU, 0);
		break;
	default:
		bHandled = FALSE;
	}
	return 0;
}

LRESULT AVIPreview::onChangeMenu(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/){
	if(ctrlCommands.GetSelectedCount() == 1) {
		int sel = ctrlCommands.GetSelectedIndex();
		PreviewApplication pa;
		FavoriteManager::getInstance()->getPreviewApp(sel, pa);

		PreviewDlg dlg;
		dlg.name = Text::toT(pa.getName());
		dlg.application = Text::toT(pa.getApplication());
		dlg.argument = Text::toT(pa.getArguments());
		dlg.extensions = Text::toT(pa.getExtension());

		if(dlg.DoModal() == IDOK) {
			pa.setName(Text::fromT(dlg.name));
			pa.setApplication(Text::fromT(dlg.application));
			pa.setArguments(Text::fromT(dlg.argument));
			pa.setExtension(Text::fromT(dlg.extensions));

			FavoriteManager::getInstance()->updatePreviewApp(sel, pa);

			ctrlCommands.SetItemText(sel, 0, dlg.name.c_str());
			ctrlCommands.SetItemText(sel, 1, dlg.application.c_str());
			ctrlCommands.SetItemText(sel, 2, dlg.argument.c_str());
			ctrlCommands.SetItemText(sel, 3, dlg.extensions.c_str());
		}
	}

	return 0;
}

LRESULT AVIPreview::onRemoveMenu(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/){
	if(ctrlCommands.GetSelectedCount() == 1) {
		int sel = ctrlCommands.GetSelectedIndex();
		FavoriteManager::getInstance()->removePreviewApp(sel);
		ctrlCommands.DeleteItem(sel);
	}
	return 0;
}