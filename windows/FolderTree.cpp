/* POSSUM_MOD_BEGIN 

Most of this code was stolen/adapted/massacred from...

Module : FileTreeCtrl.cpp
Purpose: Interface for an MFC class which provides a tree control similiar 
         to the left hand side of explorer

Copyright (c) 1999 - 2003 by PJ Naughter.  (Web: www.naughter.com, Email: pjna@naughter.com)
*/

#include "stdafx.h"
#include "../client/DCPlusPlus.h"
#include "../client/Util.h"
#include "Resource.h"
#include "../client/ResourceManager.h"
#include "../client/version.h"
#include "LineDlg.h"
#include "WinUtil.h"
#include "foldertree.h"

//Pull in the WNet Lib automatically
#pragma comment(lib, "mpr.lib")

FolderTreeItemInfo::FolderTreeItemInfo()
{
  m_pNetResource = NULL;
  m_bNetworkNode = false;
}

FolderTreeItemInfo::FolderTreeItemInfo(const FolderTreeItemInfo& ItemInfo)
{
  m_sFQPath       = ItemInfo.m_sFQPath;
  m_sRelativePath = ItemInfo.m_sRelativePath;
  m_bNetworkNode  = ItemInfo.m_bNetworkNode;
  m_pNetResource = new NETRESOURCE;
  if (ItemInfo.m_pNetResource)
  {
    //Copy the direct member variables of NETRESOURCE
    CopyMemory(m_pNetResource, ItemInfo.m_pNetResource, sizeof(NETRESOURCE));

    //Duplicate the strings which are stored in NETRESOURCE as pointers
    if (ItemInfo.m_pNetResource->lpLocalName)
		  m_pNetResource->lpLocalName	= _tcsdup(ItemInfo.m_pNetResource->lpLocalName);
    if (ItemInfo.m_pNetResource->lpRemoteName)
		  m_pNetResource->lpRemoteName = _tcsdup(ItemInfo.m_pNetResource->lpRemoteName);
    if (ItemInfo.m_pNetResource->lpComment)
		  m_pNetResource->lpComment	= _tcsdup(ItemInfo.m_pNetResource->lpComment);
    if (ItemInfo.m_pNetResource->lpProvider)
		  m_pNetResource->lpProvider	= _tcsdup(ItemInfo.m_pNetResource->lpProvider);
  }
  else
    memzero(m_pNetResource, sizeof(NETRESOURCE));
}

SystemImageList::SystemImageList()
{
	//Get the temp directory. This is used to then bring back the system image list
	TCHAR pszTempDir[_MAX_PATH];
	GetTempPath(_MAX_PATH, pszTempDir);
	TCHAR pszDrive[_MAX_DRIVE + 1];
	_tsplitpath(pszTempDir, pszDrive, NULL, NULL, NULL);
	int nLen = _tcslen(pszDrive);
	if (pszDrive[nLen-1] != _T('\\'))
		_tcscat(pszDrive, _T("\\"));

	//Attach to the system image list
	SHFILEINFO sfi;
	HIMAGELIST hSystemImageList = (HIMAGELIST) SHGetFileInfo(pszTempDir, 0, &sfi, sizeof(SHFILEINFO),
															SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
	m_ImageList.Attach(hSystemImageList);
	//auto tmp = m_ImageList.AddIcon((HICON)::LoadImage(GetModuleHandle(NULL), ResourceLoader::getIconPath(_T("Hub.ico")).c_str(), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR | LR_LOADFROMFILE));
	auto tmp = m_ImageList.AddIcon((HICON)::LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MAGNET), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR));
}

SystemImageList* SystemImageList::getInstance()
{
	static SystemImageList instance;
	return &instance;
}

SystemImageList::~SystemImageList()
{
	//Detach from the image list to prevent problems on 95/98 where
	//the system image list is shared across processes
	m_ImageList.Detach();
}

ShareEnumerator::ShareEnumerator()
{
	//Set out member variables to defaults
	m_pNTShareEnum = NULL;
	m_pWin9xShareEnum = NULL;
	m_pNTBufferFree = NULL;
	m_pNTShareInfo = NULL;
	m_pWin9xShareInfo = NULL;
	m_pWin9xShareInfo = NULL;
	m_hNetApi = NULL;
	m_dwShares = 0;

	//Determine if we are running Windows NT or Win9x
	OSVERSIONINFO osvi;
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	m_bWinNT = (GetVersionEx(&osvi) && osvi.dwPlatformId == VER_PLATFORM_WIN32_NT);
	if (m_bWinNT)
	{
		//Load up the NETAPI dll
		m_hNetApi = LoadLibrary(_T("NETAPI32.dll"));
		if (m_hNetApi)
		{
		//Get the required function pointers
		m_pNTShareEnum = (NT_NETSHAREENUM*) GetProcAddress(m_hNetApi, "NetShareEnum");
		m_pNTBufferFree = (NT_NETAPIBUFFERFREE*) GetProcAddress(m_hNetApi, "NetApiBufferFree");
		}
	}
	else
	{
		//Load up the NETAPI dll
		m_hNetApi = LoadLibrary(_T("SVRAPI.dll"));
		if (m_hNetApi)
		{
		//Get the required function pointer
		m_pWin9xShareEnum = (WIN9X_NETSHAREENUM*) GetProcAddress(m_hNetApi, "NetShareEnum");
		}
	}

	//Update the array of shares we know about
	Refresh();
}

ShareEnumerator::~ShareEnumerator()
{
	if (m_bWinNT)
	{
		//Free the buffer if valid
		if (m_pNTShareInfo)
		m_pNTBufferFree(m_pNTShareInfo);
	}
	else
		//Free up the heap memory we have used
			delete [] m_pWin9xShareInfo;

	//Free the dll now that we are finished with it
	if (m_hNetApi)
	{
		FreeLibrary(m_hNetApi);
		m_hNetApi = NULL;
	}
}

void ShareEnumerator::Refresh()
{
	m_dwShares = 0;
	if (m_bWinNT)
	{
		//Free the buffer if valid
		if (m_pNTShareInfo)
			m_pNTBufferFree(m_pNTShareInfo);

		//Call the function to enumerate the shares
		if (m_pNTShareEnum)
		{
			DWORD dwEntriesRead = 0;
			m_pNTShareEnum(NULL, 502, (LPBYTE*) &m_pNTShareInfo, MAX_PREFERRED_LENGTH, &dwEntriesRead, &m_dwShares, NULL);
		}
	}
	else
	{
		//Free the buffer if valid
		if (m_pWin9xShareInfo)
			delete [] m_pWin9xShareInfo;

		//Call the function to enumerate the shares
		if (m_pWin9xShareEnum)
		{
			//Start with a reasonably sized buffer
			unsigned short cbBuffer = 1024;
			bool bNeedMoreMemory = true;
			bool bSuccess = false;
			while (bNeedMoreMemory && !bSuccess)
			{
				unsigned short nTotalRead = 0;
				m_pWin9xShareInfo = (FolderTree_share_info_50*) new BYTE[cbBuffer];
				memzero(m_pWin9xShareInfo, cbBuffer);
				unsigned short nShares = 0;
				NET_API_STATUS nStatus = m_pWin9xShareEnum(NULL, 50, (char FAR *)m_pWin9xShareInfo, cbBuffer, (unsigned short FAR *)&nShares, (unsigned short FAR *)&nTotalRead);
				if (nStatus == ERROR_MORE_DATA)
				{
					//Free up the heap memory we have used
					delete [] m_pWin9xShareInfo;

					//And double the size, ready for the next loop around
					cbBuffer *= 2;
				}
				else if (nStatus == NERR_Success)
				{
					m_dwShares = nShares;
					bSuccess = true;
				}
				else
					bNeedMoreMemory = false;
			}
		}
	}
}

bool ShareEnumerator::IsShared(const tstring& sPath)
{
	//Assume the item is not shared
	bool bShared = false;

	if (m_bWinNT)
	{
		if (m_pNTShareInfo)
		{
			for (DWORD i=0; i<m_dwShares && !bShared; i++)
			{
				tstring sShare(m_pNTShareInfo[i].shi502_path);
				bShared = (stricmp(sPath, sShare) == 0) && ((m_pNTShareInfo[i].shi502_type == STYPE_DISKTREE) || ((m_pNTShareInfo[i].shi502_type == STYPE_PRINTQ)));
			}
		}
	}
	else
	{
		if (m_pWin9xShareInfo)
		{
			for (DWORD i=0; i<m_dwShares && !bShared; i++)
			{
				tstring sShare(Text::toT(m_pWin9xShareInfo[i].shi50_path));
				bShared = (stricmp(sPath, sShare) == 0) &&
						((m_pWin9xShareInfo[i].shi50_type == STYPE_DISKTREE) || ((m_pWin9xShareInfo[i].shi50_type == STYPE_PRINTQ)));
			}
		}
	}

	return bShared;
}

FolderTree::FolderTree(SharePage* aSp) : sp(aSp)
{
	m_dwFileHideFlags = FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM |
						FILE_ATTRIBUTE_OFFLINE | FILE_ATTRIBUTE_TEMPORARY;
	m_bShowCompressedUsingDifferentColor = true;
	m_rgbCompressed = RGB(0, 0, 255);
	m_bShowEncryptedUsingDifferentColor = true;
	m_rgbEncrypted = RGB(255, 0, 0);
	m_dwDriveHideFlags = 0;
	m_hNetworkRoot = NULL;
	m_bShowSharedUsingDifferentIcon = true;
	for (int i=0; i<26; i++)
		m_dwMediaID[i] = 0xFFFFFFFF;

	SHGetMalloc(&m_pMalloc);
	SHGetDesktopFolder(&m_pShellFolder);
	m_bDisplayNetwork = true;
	m_dwNetworkItemTypes = RESOURCETYPE_ANY;
	m_hMyComputerRoot = NULL;
	m_bShowMyComputer = true;
	m_bShowRootedFolder = false;
	m_hRootedFolder = NULL;
	m_bShowDriveLabels = true;

	m_nShareSizeDiff = 0;
}

FolderTree::~FolderTree()
{
	m_pShellFolder->Release();
	m_pMalloc->Release();
}

void FolderTree::PopulateTree()
{
	//attach the image list to the tree control
	SetImageList(*SystemImageList::getInstance()->getImageList(), TVSIL_NORMAL);

	//Force a refresh
	Refresh();

	Expand(m_hMyComputerRoot, TVE_EXPAND );
}

void FolderTree::Refresh()
{
	//Just in case this will take some time
	//CWaitCursor wait;

	SetRedraw(FALSE);

	//Get the item which is currently selected
	HTREEITEM hSelItem = GetSelectedItem();
	tstring sItem;
	bool bExpanded = false;
	if (hSelItem)
	{
		sItem = ItemToPath(hSelItem);
		bExpanded = IsExpanded(hSelItem);
	}

	theSharedEnumerator.Refresh();

	//Remove all nodes that currently exist
	Clear();

	//Display the folder items in the tree
	if (m_sRootFolder.empty()) {
		auto shared = sp->getViewItems(sp->curProfile);
		//Should we insert a "My Computer" node
		if (m_bShowMyComputer) {
			FolderTreeItemInfo* pItem = new FolderTreeItemInfo;
			pItem->m_bNetworkNode = false;
			int nIcon = 0;
			int nSelIcon = 0;

			//Get the localized name and correct icons for "My Computer"
			LPITEMIDLIST lpMCPidl;
			if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, CSIDL_DRIVES, &lpMCPidl))) {
				SHFILEINFO sfi;
				if (SHGetFileInfo((LPCTSTR)lpMCPidl, 0, &sfi, sizeof(sfi), SHGFI_PIDL | SHGFI_DISPLAYNAME))
				pItem->m_sRelativePath = sfi.szDisplayName;
				nIcon = GetIconIndex(lpMCPidl);
				nSelIcon = GetSelIconIndex(lpMCPidl);

				//Free up the pidl now that we are finished with it
				//ASSERT(m_pMalloc);
				m_pMalloc->Free(lpMCPidl);
				m_pMalloc->Release();
			}

			//Add it to the tree control
			m_hMyComputerRoot = InsertFileItem(TVI_ROOT, pItem, false, nIcon, nSelIcon, false, shared);
			SetHasSharedChildren(m_hMyComputerRoot, shared);
		}

		//Display all the drives
		if (!m_bShowMyComputer)
			DisplayDrives(TVI_ROOT, false, shared);

		//Also add network neighborhood if requested to do so
		if (m_bDisplayNetwork) {
			FolderTreeItemInfo* pItem = new FolderTreeItemInfo;
			pItem->m_bNetworkNode = true;
			int nIcon = 0;
			int nSelIcon = 0;

			//Get the localized name and correct icons for "Network Neighborhood"
			LPITEMIDLIST lpNNPidl;
			if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, CSIDL_NETWORK, &lpNNPidl))) {
				SHFILEINFO sfi;
				if (SHGetFileInfo((LPCTSTR)lpNNPidl, 0, &sfi, sizeof(sfi), SHGFI_PIDL | SHGFI_DISPLAYNAME))
				pItem->m_sRelativePath = sfi.szDisplayName;
				nIcon = GetIconIndex(lpNNPidl);
				nSelIcon = GetSelIconIndex(lpNNPidl);

				//Free up the pidl now that we are finished with it
				//ASSERT(m_pMalloc);
				m_pMalloc->Free(lpNNPidl);
				m_pMalloc->Release();
			}

			//Add it to the tree control
			m_hNetworkRoot = InsertFileItem(TVI_ROOT, pItem, false, nIcon, nSelIcon, false, shared);
			SetHasSharedChildren(m_hNetworkRoot, shared);
			//checkRemovedDirs(Util::emptyStringT, TVI_ROOT, shared);
		}
	} else {
		DisplayPath(m_sRootFolder, TVI_ROOT, false);
	}

	//Reselect the initially selected item
	if (hSelItem)
		SetSelectedPath(sItem, bExpanded);

	//Turn back on the redraw flag
	SetRedraw(true);
}

tstring FolderTree::ItemToPath(HTREEITEM hItem) const
{
	tstring sPath;
	if(hItem)
	{
		FolderTreeItemInfo* pItem = (FolderTreeItemInfo*) GetItemData(hItem);
		//ASSERT(pItem);
		sPath = pItem->m_sFQPath;
	}
	return sPath;
}

bool FolderTree::IsExpanded(HTREEITEM hItem)
{
	TVITEM tvItem;
	tvItem.hItem = hItem;
	tvItem.mask = TVIF_HANDLE | TVIF_STATE;
	return (GetItem(&tvItem) && (tvItem.state & TVIS_EXPANDED));
}

void FolderTree::Clear()
{
	//Delete all the items
	DeleteAllItems();

	//Reset the member variables we have
	m_hMyComputerRoot = NULL;
	m_hNetworkRoot = NULL;
	m_hRootedFolder = NULL;
}

int FolderTree::GetIconIndex(HTREEITEM hItem)
{
	TV_ITEM tvi;
	memzero(&tvi, sizeof(TV_ITEM));
	tvi.mask = TVIF_IMAGE;
	tvi.hItem = hItem;
	if (GetItem(&tvi))
		return tvi.iImage;
	else
		return -1;
}

int FolderTree::GetIconIndex(const tstring &sFilename)
{
	//Retreive the icon index for a specified file/folder
	SHFILEINFO sfi;
	memzero(&sfi, sizeof(SHFILEINFO));
	SHGetFileInfo(sFilename.c_str(), 0, &sfi, sizeof(SHFILEINFO), SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
	return sfi.iIcon;
}

int FolderTree::GetIconIndex(LPITEMIDLIST lpPIDL)
{
	SHFILEINFO sfi;
	memzero(&sfi, sizeof(SHFILEINFO));
	SHGetFileInfo((LPCTSTR)lpPIDL, 0, &sfi, sizeof(sfi), SHGFI_PIDL | SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_LINKOVERLAY);
	return sfi.iIcon;
}

int FolderTree::GetSelIconIndex(LPITEMIDLIST lpPIDL)
{
	SHFILEINFO sfi;
	memzero(&sfi, sizeof(SHFILEINFO));
	SHGetFileInfo((LPCTSTR)lpPIDL, 0, &sfi, sizeof(sfi), SHGFI_PIDL | SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_OPENICON);
	return sfi.iIcon;
}

int FolderTree::GetSelIconIndex(HTREEITEM hItem)
{
	TV_ITEM tvi;
	memzero(&tvi, sizeof(TV_ITEM));
	tvi.mask = TVIF_SELECTEDIMAGE;
	tvi.hItem = hItem;
	if (GetItem(&tvi))
		return tvi.iSelectedImage;
	else
		return -1;
}

int FolderTree::GetSelIconIndex(const tstring &sFilename)
{
	//Retreive the icon index for a specified file/folder
	SHFILEINFO sfi;
	memzero(&sfi, sizeof(SHFILEINFO));
	SHGetFileInfo(sFilename.c_str(), 0, &sfi, sizeof(SHFILEINFO), SHGFI_SYSICONINDEX | SHGFI_OPENICON | SHGFI_SMALLICON);
	return sfi.iIcon;
}

HTREEITEM FolderTree::InsertFileItem(HTREEITEM hParent, FolderTreeItemInfo *pItem, bool bShared, int nIcon, int nSelIcon, bool bCheckForChildren, ShareDirInfo::list& sharedDirs)
{
	tstring sLabel;

	//Correct the label if need be
	if(IsDrive(pItem->m_sFQPath) && m_bShowDriveLabels)
		sLabel = GetDriveLabel(pItem->m_sFQPath);
	else
		sLabel = GetCorrectedLabel(pItem);

	//Add the actual item
	TV_INSERTSTRUCT tvis;
	memzero(&tvis, sizeof(TV_INSERTSTRUCT));
	tvis.hParent = hParent;
	tvis.hInsertAfter = TVI_LAST;
	tvis.item.mask = TVIF_CHILDREN | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_PARAM;
	tvis.item.iImage = nIcon;
	tvis.item.iSelectedImage = nSelIcon;

	tvis.item.lParam = (LPARAM) pItem;
	tvis.item.pszText = (LPWSTR)sLabel.c_str();
	if (bCheckForChildren)
		tvis.item.cChildren = HasGotSubEntries(pItem->m_sFQPath);
	else
		tvis.item.cChildren = true;
	
	if(bShared)
	{
		tvis.item.mask |= TVIF_STATE;
		tvis.item.stateMask |= TVIS_OVERLAYMASK;
		tvis.item.state |= INDEXTOOVERLAYMASK(1); //1 is the index for the shared overlay image
	}

	HTREEITEM hItem = InsertItem(&tvis);

	string path = Text::fromT(pItem->m_sFQPath);
	if( path[ path.length() -1 ] != PATH_SEPARATOR )
		path += PATH_SEPARATOR;

	bool bChecked = sp->shareFolder(path, sharedDirs);
	SetChecked(hItem, bChecked);
	if(!bChecked)
		SetHasSharedChildren(hItem, sharedDirs);

	return hItem;
}

void FolderTree::DisplayDrives(HTREEITEM hParent, bool bUseSetRedraw, ShareDirInfo::list& shared)
{
	//CWaitCursor c;

	//Speed up the job by turning off redraw
	if (bUseSetRedraw)
		SetRedraw(false);

	//Enumerate the drive letters and add them to the tree control
	DWORD dwDrives = GetLogicalDrives();
	DWORD dwMask = 1;
	for (int i=0; i<32; i++)
	{
		if (dwDrives & dwMask)
		{
			tstring sDrive;
			sDrive = (wchar_t)('A' + i);
			sDrive += _T(":\\");

			//check if this drive is one of the types to hide
			if (CanDisplayDrive(sDrive))
			{
				FolderTreeItemInfo* pItem = new FolderTreeItemInfo;
				pItem->m_sFQPath = sDrive;
				pItem->m_sRelativePath = sDrive;

				//Insert the item into the view
				InsertFileItem(hParent, pItem, m_bShowSharedUsingDifferentIcon && IsShared(sDrive), GetIconIndex(sDrive), GetSelIconIndex(sDrive), true, shared);
			}
		}
		dwMask <<= 1;
	}

	if (bUseSetRedraw)
		SetRedraw(true);
}

void FolderTree::DisplayPath(const tstring &sPath, HTREEITEM hParent, bool bUseSetRedraw /* = true */)
{
	//CWaitCursor c;

	//Speed up the job by turning off redraw
	if (bUseSetRedraw)
		SetRedraw(false);

	//Remove all the items currently under hParent
	HTREEITEM hChild = GetChildItem(hParent);
	while (hChild)
	{
		DeleteItem(hChild);
		hChild = GetChildItem(hParent);
	}
	auto shared = sp->getViewItems(sp->curProfile);

	//Should we display the root folder
	if (m_bShowRootedFolder && (hParent == TVI_ROOT))
	{
		FolderTreeItemInfo* pItem = new FolderTreeItemInfo;
		pItem->m_sFQPath = m_sRootFolder;
		pItem->m_sRelativePath = m_sRootFolder;
		m_hRootedFolder = InsertFileItem(TVI_ROOT, pItem, false, GetIconIndex(m_sRootFolder), GetSelIconIndex(m_sRootFolder), true, shared);
		Expand(m_hRootedFolder, TVE_EXPAND);
		return;
	}

	//find all the directories underneath sPath
	int nDirectories = 0;
	
	tstring sFile;
	if (sPath[sPath.size()-1] != _T('\\'))
		sFile = sPath + _T("\\");
	else
		sFile = sPath;

	WIN32_FIND_DATA fData;
	HANDLE hFind;
	hFind = FindFirstFile((sFile + _T("*")).c_str(), &fData);

	if(hFind != INVALID_HANDLE_VALUE)

	{
		do
		{
			tstring filename = fData.cFileName;
			if((fData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && 
				(filename != _T(".")) && (filename != _T("..")))
			{
				++nDirectories;
				tstring sPath = sFile + filename;

				TCHAR szPath[_MAX_PATH];
				TCHAR szFname[_MAX_FNAME];
				TCHAR szExt[_MAX_EXT];
				_tsplitpath(sPath.c_str(), NULL, NULL, szFname, szExt);
				_tmakepath(szPath, NULL, NULL, szFname, szExt);

				FolderTreeItemInfo* pItem = new FolderTreeItemInfo;
				pItem->m_sFQPath = sPath;
				pItem->m_sRelativePath = szPath;
				InsertFileItem(hParent, pItem, m_bShowSharedUsingDifferentIcon && IsShared(sPath), GetIconIndex(sPath), GetSelIconIndex(sPath), true, shared);
			}
		} while (FindNextFile(hFind, &fData));
	}

	FindClose(hFind);

	//Now sort the items we just added
	TVSORTCB tvsortcb;
	tvsortcb.hParent = hParent;
	tvsortcb.lpfnCompare = CompareByFilenameNoCase;
	tvsortcb.lParam = 0;
	SortChildrenCB(&tvsortcb);

	//We want to add them before sorting
	checkRemovedDirs(sFile, hParent, shared);

	//If no items were added then remove the "+" indicator from hParent
	if(nDirectories == 0)
		SetHasPlusButton(hParent, FALSE);

	//Turn back on the redraw flag
	if(bUseSetRedraw)
		SetRedraw(true);
}

void FolderTree::checkRemovedDirs(const tstring& aParentPath, HTREEITEM hParent, ShareDirInfo::list& sharedDirs) {
	string parentPath = Text::fromT(aParentPath);
	for(auto i = sharedDirs.begin(); i != sharedDirs.end(); ++i) {
		if ((*i)->found)
			continue;

		auto dir = Util::getParentDir((*i)->path);
		if (dir == parentPath) {
			//this should have been inserted
			FolderTreeItemInfo* pItem = new FolderTreeItemInfo;
			pItem->m_sFQPath = Text::toT((*i)->path);
			pItem->m_sRelativePath = Text::toT(Util::getLastDir((*i)->path));
			pItem->m_removed = true;

			tstring sLabel;

			//Correct the label if need be
			if(IsDrive(pItem->m_sFQPath) && m_bShowDriveLabels)
				sLabel = GetDriveLabel(pItem->m_sFQPath);
			else
				sLabel = GetCorrectedLabel(pItem);

			sLabel = _T("REMOVED: ") + sLabel;

			//Add the actual item
			TV_INSERTSTRUCT tvis;
			memzero(&tvis, sizeof(TV_INSERTSTRUCT));
			tvis.hParent = hParent;
			tvis.hInsertAfter = TVI_FIRST;
			tvis.item.mask = TVIF_CHILDREN | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_PARAM;
			//tvis.item.iImage = 7;
			//tvis.item.iSelectedImage = 7;
			tvis.item.iImage = 1;
			tvis.item.iSelectedImage = 1;

			tvis.item.lParam = (LPARAM) pItem;
			tvis.item.pszText = (LPWSTR)sLabel.c_str();
			tvis.item.cChildren = false;
	
			tvis.item.mask |= TVIF_STATE;
			tvis.item.stateMask |= TVIS_OVERLAYMASK;
			tvis.item.state = TVIS_BOLD | INDEXTOOVERLAYMASK(1); //1 is the index for the shared overlay image
			tvis.item.state |= TVIS_BOLD;

			//tvis.item.state = TVIS_BOLD;
			//tvis.item.stateMask = TVIS_BOLD;

			HTREEITEM hItem = InsertItem(&tvis);
			//SetItemImage(hItem, MAKEINTRESOURCE(IDI_ERROR), MAKEINTRESOURCE(IDI_ERROR));
			SetChecked(hItem, true);
			SetItemState(hItem, TVIS_BOLD, TVIS_BOLD);
		}
	}
}

HTREEITEM FolderTree::SetSelectedPath(const tstring &sPath, bool bExpanded /* = false */)
{
	tstring sSearch = sPath;
	sSearch = Text::toLower(sSearch);
	int nSearchLength = sSearch.size();
	if(nSearchLength == 0)
	{
		//TRACE(_T("Cannot select a empty path\n"));
		return NULL;
	}

	//Remove initial part of path if the root folder is setup
	tstring sRootFolder = m_sRootFolder;
	sRootFolder = Text::toLower(sRootFolder);
	int nRootLength = sRootFolder.size();
	if (nRootLength)
	{
		if(sSearch.find(sRootFolder) != 0)
		{
			//TRACE(_T("Could not select the path %s as the root has been configued as %s\n"), sPath, m_sRootFolder);
			return NULL;
		}
		sSearch = sSearch.substr(nRootLength);
	}

	//Remove trailing "\" from the path
	nSearchLength = sSearch.size();
	if (nSearchLength > 3 && sSearch[nSearchLength-1] == _T('\\'))
		sSearch = sSearch.substr(0, nSearchLength-1);

	if (sSearch.empty())
		return NULL;

	SetRedraw(FALSE);

	HTREEITEM hItemFound = TVI_ROOT;
	if (nRootLength && m_hRootedFolder)
		hItemFound = m_hRootedFolder;
	bool bDriveMatch = sRootFolder.empty();
	bool bNetworkMatch = m_bDisplayNetwork && ((sSearch.size() > 2) && sSearch.find(_T("\\\\")) == 0);
	if (bNetworkMatch)
	{
		bDriveMatch = false;

		//Working here
		bool bHasPlus = HasPlusButton(m_hNetworkRoot);
		bool bHasChildren = (GetChildItem(m_hNetworkRoot) != NULL);

		if (bHasPlus && !bHasChildren)
			DoExpand(m_hNetworkRoot);
		else
			Expand(m_hNetworkRoot, TVE_EXPAND);

		hItemFound = FindServersNode(m_hNetworkRoot);
		sSearch = sSearch.substr(2);
	}
	if (bDriveMatch)
	{
		if (m_hMyComputerRoot)
		{
			//Working here
			bool bHasPlus = HasPlusButton(m_hMyComputerRoot);
			bool bHasChildren = (GetChildItem(m_hMyComputerRoot) != NULL);

			if (bHasPlus && !bHasChildren)
				DoExpand(m_hMyComputerRoot);
			else
				Expand(m_hMyComputerRoot, TVE_EXPAND);

			hItemFound = m_hMyComputerRoot;
		}
	}

	int nFound = sSearch.find(_T('\\'));
	while(nFound != tstring::npos)
	{
		tstring sMatch;
		if (bDriveMatch)
		{
			sMatch = sSearch.substr(0, nFound + 1);
			bDriveMatch = false;
		}
		else
			sMatch = sSearch.substr(0, nFound);

		hItemFound = FindSibling(hItemFound, sMatch);
		if (hItemFound == NULL)
			break;
		else if (!IsDrive(sPath))
		{
			SelectItem(hItemFound);

			//Working here
			bool bHasPlus = HasPlusButton(hItemFound);
			bool bHasChildren = (GetChildItem(hItemFound) != NULL);

			if (bHasPlus && !bHasChildren)
				DoExpand(hItemFound);
			else
				Expand(hItemFound, TVE_EXPAND);
		}

		sSearch = sSearch.substr(nFound - 1);
		nFound = sSearch.find(_T('\\'));
	};

	//The last item
	if (hItemFound)
	{
		if (sSearch.size())
			hItemFound = FindSibling(hItemFound, sSearch);
		if (hItemFound)
			SelectItem(hItemFound);

		if (bExpanded)
		{
			//Working here
			bool bHasPlus = HasPlusButton(hItemFound);
			bool bHasChildren = (GetChildItem(hItemFound) != NULL);

			if (bHasPlus && !bHasChildren)
				DoExpand(hItemFound);
			else
				Expand(hItemFound, TVE_EXPAND);
		}
	}

	//Turn back on the redraw flag
	SetRedraw(TRUE);

	return hItemFound;
}

bool FolderTree::IsDrive(HTREEITEM hItem)
{
	return IsDrive(ItemToPath(hItem));
}

bool FolderTree::IsDrive(const tstring &sPath)
{
	return (sPath.size() == 3 && sPath[1] == _T(':') && sPath[2] == _T('\\'));
}

tstring FolderTree::GetDriveLabel(const tstring &sDrive)
{
	USES_CONVERSION;
	//Let's start with the drive letter
	tstring sLabel = sDrive;

	//Try to find the item directory using ParseDisplayName
	LPITEMIDLIST lpItem;
	HRESULT hr = m_pShellFolder->ParseDisplayName(NULL, NULL, T2W((LPTSTR) (LPCTSTR)sDrive.c_str()), NULL, &lpItem, NULL);
	if(SUCCEEDED(hr))
	{
		SHFILEINFO sfi;
		if (SHGetFileInfo((LPCTSTR)lpItem, 0, &sfi, sizeof(sfi), SHGFI_PIDL | SHGFI_DISPLAYNAME))
		sLabel = sfi.szDisplayName;

		//Free the pidl now that we are finished with it
		m_pMalloc->Free(lpItem);
	}

  return sLabel;
}

tstring FolderTree::GetCorrectedLabel(FolderTreeItemInfo *pItem)
{
	tstring sLabel = pItem->m_sRelativePath;	
	return sLabel;
}

bool FolderTree::HasGotSubEntries(const tstring &sDirectory)
{
	if(sDirectory.empty())
		return false;

	if(DriveHasRemovableMedia(sDirectory))
	{
		return true; //we do not bother searching for files on drives
					//which have removable media as this would cause
					//the drive to spin up, which for the case of a
					//floppy is annoying
	}
	else
	{
		//First check to see if there is any sub directories
		tstring sFile;
		if (sDirectory[sDirectory.size()-1] == _T('\\'))
			sFile = sDirectory + _T("*.*");
		else
			sFile = sDirectory + _T("\\*.*");

		WIN32_FIND_DATA fData;
		HANDLE hFind;
		hFind = FindFirstFile(sFile.c_str(), &fData);
		if(hFind != INVALID_HANDLE_VALUE)
		{
			do
			{
				tstring cFileName = fData.cFileName;
				if((fData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && 
					(cFileName != _T(".")) && (cFileName != _T("..")))
				{
					FindClose(hFind);
					return true;
				}
			} while (FindNextFile(hFind, &fData));
		}

		FindClose(hFind);
	}

	return false;
}

bool FolderTree::CanDisplayDrive(const tstring &sDrive)
{
	//check if this drive is one of the types to hide
	bool bDisplay = true;
	UINT nDrive = GetDriveType(sDrive.c_str());
	switch (nDrive)
	{
		case DRIVE_REMOVABLE:
		{
			if (m_dwDriveHideFlags & DRIVE_ATTRIBUTE_REMOVABLE)
				bDisplay = false;
			break;
		}
		case DRIVE_FIXED:
		{
			if (m_dwDriveHideFlags & DRIVE_ATTRIBUTE_FIXED)
				bDisplay = false;
			break;
		}
		case DRIVE_REMOTE:
		{
			if (m_dwDriveHideFlags & DRIVE_ATTRIBUTE_REMOTE)
				bDisplay = false;
			break;
		}
		case DRIVE_CDROM:
		{
			if (m_dwDriveHideFlags & DRIVE_ATTRIBUTE_CDROM)
				bDisplay = false;
			break;
		}
		case DRIVE_RAMDISK:
		{
			if (m_dwDriveHideFlags & DRIVE_ATTRIBUTE_RAMDISK)
				bDisplay = false;
			break;
		}
		default:
		{
			break;
		}
	}

	return bDisplay;
}

bool FolderTree::IsShared(const tstring &sPath)
{
	//Defer all the work to the share enumerator class
	return theSharedEnumerator.IsShared(sPath);
}

int CALLBACK FolderTree::CompareByFilenameNoCase(LPARAM lParam1, LPARAM lParam2, LPARAM /*lParamSort*/)
{
	FolderTreeItemInfo* pItem1 = (FolderTreeItemInfo*) lParam1;
	FolderTreeItemInfo* pItem2 = (FolderTreeItemInfo*) lParam2;

	return Util::DefaultSort(pItem1->m_sRelativePath.c_str(), pItem2->m_sRelativePath.c_str());
}

void FolderTree::SetHasPlusButton(HTREEITEM hItem, bool bHavePlus)
{
	//Remove all the child items from the parent
	TV_ITEM tvItem;
	tvItem.hItem = hItem;
	tvItem.mask = TVIF_CHILDREN;
	tvItem.cChildren = bHavePlus;
	SetItem(&tvItem);
}

bool FolderTree::HasPlusButton(HTREEITEM hItem)
{
	TVITEM tvItem;
	tvItem.hItem = hItem;
	tvItem.mask = TVIF_HANDLE | TVIF_CHILDREN;
	return GetItem(&tvItem) && (tvItem.cChildren != 0);
}

void FolderTree::DoExpand(HTREEITEM hItem)
{
	FolderTreeItemInfo* pItem = (FolderTreeItemInfo*) GetItemData(hItem);

	//Reset the drive node if the drive is empty or the media has changed
	if (IsMediaValid(pItem->m_sFQPath))
	{
		//Delete the item if the path is no longer valid
		if (IsFolder(pItem->m_sFQPath))
		{
			//Add the new items to the tree if it does not have any child items
			//already
			if (!GetChildItem(hItem))
				DisplayPath(pItem->m_sFQPath, hItem);
		}
		else if (hItem == m_hMyComputerRoot)
		{
			//Display an hour glass as this may take some time
			//CWaitCursor wait;

			//Enumerate the local drive letters
			auto shared = sp->getViewItems(sp->curProfile);
			DisplayDrives(m_hMyComputerRoot, FALSE, shared);
		}
		else if ((hItem == m_hNetworkRoot) || (pItem->m_pNetResource))
		{
			//Display an hour glass as this may take some time
			//CWaitCursor wait;

			//Enumerate the network resources
			EnumNetwork(hItem);
		}
		else
		{
			//Before we delete it see if we are the only child item
			HTREEITEM hParent = GetParentItem(hItem);

			//Delete the item
			DeleteItem(hItem);

			//Remove all the child items from the parent
			SetHasPlusButton(hParent, false);
		}
	}
	else
	{
		//Display an hour glass as this may take some time
		//CWaitCursor wait;

		//Collapse the drive node and remove all the child items from it
		Expand(hItem, TVE_COLLAPSE);
		DeleteChildren(hItem, true);
	}
}

HTREEITEM FolderTree::FindServersNode(HTREEITEM hFindFrom) const
{
	if (m_bDisplayNetwork)
	{
		//Try to find some "servers" in the child items of hFindFrom
		HTREEITEM hChild = GetChildItem(hFindFrom);
		while (hChild)
		{
			FolderTreeItemInfo* pItem = (FolderTreeItemInfo*) GetItemData(hChild);
			
			if (pItem->m_pNetResource)
			{
				//Found a share
				if (pItem->m_pNetResource->dwDisplayType == RESOURCEDISPLAYTYPE_SERVER)
					return hFindFrom;
			}

			//Get the next sibling for the next loop around
			hChild = GetNextSiblingItem(hChild);
		}

		//Ok, since we got here, we did not find any servers in any of the child nodes of this
		//item. In this case we need to call ourselves recursively to find one
		hChild = GetChildItem(hFindFrom);
		while (hChild)
		{
			HTREEITEM hFound = FindServersNode(hChild);
			if (hFound)
				return hFound;

			//Get the next sibling for the next loop around
			hChild = GetNextSiblingItem(hChild);
		}
	}

	//If we got as far as here then no servers were found.
	return NULL;
}

HTREEITEM FolderTree::FindSibling(HTREEITEM hParent, const tstring &sItem) const
{
	HTREEITEM hChild = GetChildItem(hParent);
	while(hChild)
	{
		FolderTreeItemInfo* pItem = (FolderTreeItemInfo*) GetItemData(hChild);
		
		if (stricmp(pItem->m_sRelativePath, sItem) == 0)
			return hChild;
		hChild = GetNextItem(hChild, TVGN_NEXT);
	}
	return NULL;
}

bool FolderTree::DriveHasRemovableMedia(const tstring &sPath)
{
	bool bRemovableMedia = false;
	if(IsDrive(sPath))
	{
		UINT nDriveType = GetDriveType(sPath.c_str());
		bRemovableMedia = ((nDriveType == DRIVE_REMOVABLE) ||
						(nDriveType == DRIVE_CDROM));
	}

	return bRemovableMedia;
}

bool FolderTree::IsMediaValid(const tstring &sDrive)
{
	//return TRUE if the drive does not support removable media
	UINT nDriveType = GetDriveType(sDrive.c_str());
	if ((nDriveType != DRIVE_REMOVABLE) && (nDriveType != DRIVE_CDROM))
		return true;

	//Return FALSE if the drive is empty (::GetVolumeInformation fails)
	DWORD dwSerialNumber;
	int nDrive = sDrive[0] - _T('A');
	if(GetSerialNumber(sDrive, dwSerialNumber))
		m_dwMediaID[nDrive] = dwSerialNumber;
	else
	{
		m_dwMediaID[nDrive] = 0xFFFFFFFF;
		return false;
	}

	//Also return FALSE if the disk's serial number has changed
	if ((m_dwMediaID[nDrive] != dwSerialNumber) &&
		(m_dwMediaID[nDrive] != 0xFFFFFFFF))
	{
		m_dwMediaID[nDrive] = 0xFFFFFFFF;
		return false;
	}

	return true;
}

bool FolderTree::IsFolder(const tstring &sPath)
{
	DWORD dwAttributes = GetFileAttributes(sPath.c_str());
	return ((dwAttributes != 0xFFFFFFFF) && (dwAttributes & FILE_ATTRIBUTE_DIRECTORY));
}

bool FolderTree::EnumNetwork(HTREEITEM hParent)
{
	auto shared = sp->getViewItems(sp->curProfile);
	//What will be the return value from this function
	bool bGotChildren = false;

	//Check if the item already has a network resource and use it.
	FolderTreeItemInfo* pItem = (FolderTreeItemInfo*) GetItemData(hParent);
	
	NETRESOURCE* pNetResource = pItem->m_pNetResource;

	//Setup for the network enumeration
	HANDLE hEnum;
	DWORD dwResult = WNetOpenEnum(pNetResource ? RESOURCE_GLOBALNET : RESOURCE_CONTEXT, m_dwNetworkItemTypes,
      											0, pNetResource ? pNetResource : NULL, &hEnum);

	//Was the read sucessful
	if (dwResult != NO_ERROR)
	{
		//TRACE(_T("Cannot enumerate network drives, Error:%d\n"), dwResult);
		return FALSE;
	}

	//Do the network enumeration
	DWORD cbBuffer = 16384;

	bool bNeedMoreMemory = true;
	bool bSuccess = false;
	LPNETRESOURCE lpnrDrv = NULL;
	DWORD cEntries = 0;
	while (bNeedMoreMemory && !bSuccess)
	{
		//Allocate the memory and enumerate
  		lpnrDrv = (LPNETRESOURCE) new BYTE[cbBuffer];
		cEntries = 0xFFFFFFFF;
		dwResult = WNetEnumResource(hEnum, &cEntries, lpnrDrv, &cbBuffer);

		if (dwResult == ERROR_MORE_DATA)
		{
			//Free up the heap memory we have used
			delete [] lpnrDrv;

			cbBuffer *= 2;
		}
		else if (dwResult == NO_ERROR)
			bSuccess = true;
		else
			bNeedMoreMemory = false;
	}

	//Enumeration successful?
	if (bSuccess)
	{
		//Scan through the results
		for (DWORD i=0; i<cEntries; i++)
		{
			tstring sNameRemote;
			if(lpnrDrv[i].lpRemoteName != NULL)
				sNameRemote = lpnrDrv[i].lpRemoteName;
			else
				sNameRemote = lpnrDrv[i].lpComment;

			//Remove leading back slashes
			if (sNameRemote.size() > 0 && sNameRemote[0] == _T('\\'))
				sNameRemote = sNameRemote.substr(1);
			if (sNameRemote.size() > 0 && sNameRemote[0] == _T('\\'))
				sNameRemote = sNameRemote.substr(1);

			//Setup the item data for the new item
			FolderTreeItemInfo* pItem = new FolderTreeItemInfo;
			pItem->m_pNetResource = new NETRESOURCE;
			memzero(pItem->m_pNetResource, sizeof(NETRESOURCE));
			*pItem->m_pNetResource = lpnrDrv[i];
			
			if (lpnrDrv[i].lpLocalName)
				pItem->m_pNetResource->lpLocalName	= _tcsdup(lpnrDrv[i].lpLocalName);
			if (lpnrDrv[i].lpRemoteName)
				pItem->m_pNetResource->lpRemoteName = _tcsdup(lpnrDrv[i].lpRemoteName);
			if (lpnrDrv[i].lpComment)
				pItem->m_pNetResource->lpComment	= _tcsdup(lpnrDrv[i].lpComment);
			if (lpnrDrv[i].lpProvider)
				pItem->m_pNetResource->lpProvider	= _tcsdup(lpnrDrv[i].lpProvider);
			if (lpnrDrv[i].lpRemoteName)
				pItem->m_sFQPath = lpnrDrv[i].lpRemoteName;
			else
				pItem->m_sFQPath = sNameRemote;
			
			pItem->m_sRelativePath = sNameRemote;
			pItem->m_bNetworkNode = true;

			//Display a share and the appropiate icon
			if (lpnrDrv[i].dwDisplayType == RESOURCEDISPLAYTYPE_SHARE)
			{
				//Display only the share name
				int nPos = pItem->m_sRelativePath.find(_T('\\'));
				if (nPos >= 0)
					pItem->m_sRelativePath = pItem->m_sRelativePath.substr(nPos+1);

				//Now add the item into the control
				InsertFileItem(hParent, pItem, m_bShowSharedUsingDifferentIcon, GetIconIndex(pItem->m_sFQPath),
							GetSelIconIndex(pItem->m_sFQPath), TRUE, shared);
			}
			else if (lpnrDrv[i].dwDisplayType == RESOURCEDISPLAYTYPE_SERVER)
			{
				//Now add the item into the control
				tstring sServer = _T("\\\\");
				sServer += pItem->m_sRelativePath;
				InsertFileItem(hParent, pItem, false, GetIconIndex(sServer), GetSelIconIndex(sServer), false, shared);
			}
			else
			{
				//Now add the item into the control
				//Just use the generic Network Neighborhood icons for everything else
				LPITEMIDLIST lpNNPidl;
				int nIcon = 0xFFFF;
				int nSelIcon = nIcon;
				if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, CSIDL_NETWORK, &lpNNPidl)))
				{
					nIcon = GetIconIndex(lpNNPidl);
					nSelIcon = GetSelIconIndex(lpNNPidl);

					//Free up the pidl now that we are finished with it
					//ASSERT(m_pMalloc);
					m_pMalloc->Free(lpNNPidl);
					m_pMalloc->Release();
				}

				InsertFileItem(hParent, pItem, false, nIcon, nSelIcon, false, shared);
			}
			bGotChildren = true;
		}
	}
/*	else
		TRACE(_T("Cannot complete network drive enumeration, Error:%d\n"), dwResult);
*/
	//Clean up the enumeration handle
	WNetCloseEnum(hEnum);

	//Free up the heap memory we have used
	delete [] lpnrDrv;

	//Return whether or not we added any items
	return bGotChildren;
}

int FolderTree::DeleteChildren(HTREEITEM hItem, bool bUpdateChildIndicator)
{
	int nCount = 0;
	HTREEITEM hChild = GetChildItem(hItem);
	while (hChild)
	{
		//Get the next sibling before we delete the current one
		HTREEITEM hNextItem = GetNextSiblingItem(hChild);

		//Delete the current child
		DeleteItem(hChild);

		//Get ready for the next loop
		hChild = hNextItem;
		++nCount;
	}

	//Also update its indicator to suggest that their is children
	if (bUpdateChildIndicator)
		SetHasPlusButton(hItem, (nCount != 0));

	return nCount;
}

BOOL FolderTree::GetSerialNumber(const tstring &sDrive, DWORD &dwSerialNumber)
{
	return GetVolumeInformation(sDrive.c_str(), NULL, 0, &dwSerialNumber, NULL, NULL, NULL, 0);
}

LRESULT FolderTree::OnSelChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL &bHandled)
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pnmh;

	//Nothing selected
	if (pNMTreeView->itemNew.hItem == NULL)
	{
		bHandled = FALSE;
		return 0;
	}

	//Check to see if the current item is valid, if not then delete it (Exclude network items from this check)
	FolderTreeItemInfo* pItem = (FolderTreeItemInfo*) GetItemData(pNMTreeView->itemNew.hItem);
	//ASSERT(pItem);
	tstring sPath = pItem->m_sFQPath;
	if (!pItem->m_removed && (pNMTreeView->itemNew.hItem != m_hNetworkRoot) && (pItem->m_pNetResource == NULL) &&
		(pNMTreeView->itemNew.hItem != m_hMyComputerRoot) && !IsDrive(sPath) && (GetFileAttributes(sPath.c_str()) == 0xFFFFFFFF))
	{
		//Before we delete it see if we are the only child item
		HTREEITEM hParent = GetParentItem(pNMTreeView->itemNew.hItem);

		//Delete the item
		DeleteItem(pNMTreeView->itemNew.hItem);

		//Remove all the child items from the parent
		SetHasPlusButton(hParent, false);

		bHandled = FALSE; //Allow the message to be reflected again
		return 1;
	}

	//Remeber the serial number for this item (if it is a drive)
	if (IsDrive(sPath))
	{
		int nDrive = sPath[0] - _T('A');
		GetSerialNumber(sPath, m_dwMediaID[nDrive]);
	}
	
	bHandled = FALSE; //Allow the message to be reflected again
	
	return 0;	
}

LRESULT FolderTree::OnItemExpanding(int /*idCtrl*/, LPNMHDR pnmh, BOOL &bHandled)
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pnmh;
	if (pNMTreeView->action == TVE_EXPAND)
	{
		bool bHasPlus = HasPlusButton(pNMTreeView->itemNew.hItem);
		bool bHasChildren = (GetChildItem(pNMTreeView->itemNew.hItem) != NULL);

		if (bHasPlus && !bHasChildren)
			DoExpand(pNMTreeView->itemNew.hItem);
	}
	else if(pNMTreeView->action == TVE_COLLAPSE)
	{
		FolderTreeItemInfo* pItem = (FolderTreeItemInfo*) GetItemData(pNMTreeView->itemNew.hItem);
		//ASSERT(pItem);

		//Display an hour glass as this may take some time
		//CWaitCursor wait;

		tstring sPath = ItemToPath(pNMTreeView->itemNew.hItem);
		
		//Collapse the node and remove all the child items from it
		Expand(pNMTreeView->itemNew.hItem, TVE_COLLAPSE);

		//Never uppdate the child indicator for a network node which is not a share
		bool bUpdateChildIndicator = true;
		if (pItem->m_bNetworkNode)
		{
			if (pItem->m_pNetResource)
				bUpdateChildIndicator = (pItem->m_pNetResource->dwDisplayType == RESOURCEDISPLAYTYPE_SHARE);
			else
				bUpdateChildIndicator = false;
		}
		DeleteChildren(pNMTreeView->itemNew.hItem, bUpdateChildIndicator);
	}

	bHandled = FALSE; //Allow the message to be reflected again
	return 0;
}

LRESULT FolderTree::OnDeleteItem(int /*idCtrl*/, LPNMHDR pnmh, BOOL &bHandled)
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pnmh;
	if (pNMTreeView->itemOld.hItem != TVI_ROOT)
	{
		FolderTreeItemInfo* pItem = (FolderTreeItemInfo*) pNMTreeView->itemOld.lParam;
		if (pItem->m_pNetResource)
		{
			free(pItem->m_pNetResource->lpLocalName);
			free(pItem->m_pNetResource->lpRemoteName);
			free(pItem->m_pNetResource->lpComment);
			free(pItem->m_pNetResource->lpProvider);
			delete pItem->m_pNetResource;
		}
		delete pItem;
	}
	
	bHandled = FALSE; //Allow the message to be reflected again
	return 0;	
}

bool FolderTree::GetChecked(HTREEITEM hItem) const
{
	UINT u = GetItemState(hItem, TVIS_STATEIMAGEMASK);
	return ((u >> 12) - 1) > 0;
}

BOOL FolderTree::SetChecked(HTREEITEM hItem, bool fCheck)
{
	TVITEM item;
	item.mask = TVIF_HANDLE | TVIF_STATE;
	item.hItem = hItem;
	item.stateMask = TVIS_STATEIMAGEMASK;

	/*
	Since state images are one-based, 1 in this macro turns the check off, and
	2 turns it on.
	*/
	item.state = INDEXTOSTATEIMAGEMASK((fCheck ? 2 : 1));

	return SetItem(&item);
}

LRESULT FolderTree::OnChecked(HTREEITEM hItem, BOOL &bHandled)
{
	FolderTreeItemInfo* pItem = (FolderTreeItemInfo*) GetItemData(hItem);
	if(!Util::validatePath(Text::fromT(pItem->m_sFQPath)))
	{
		// no checking myComp or network
		bHandled = TRUE;
		return 1;
	}
	
	HTREEITEM hSharedParent = HasSharedParent(hItem);
	// if a parent is checked then this folder should be removed from the ex list
	if(hSharedParent != NULL)
	{
		ShareParentButNotSiblings(hItem);
	}
	else
	{
        // if no parent folder is checked then this is a new root dir
		LineDlg virt;
		virt.title = TSTRING(VIRTUAL_NAME);
		virt.description = TSTRING(VIRTUAL_NAME_LONG);

		tstring path = pItem->m_sFQPath;
		if( path[ path.length() -1 ] != '\\' )
			path += '\\';

		if (!sp->addDirectory(path))
			return 1;

		UpdateParentItems(hItem);
	}
	
	UpdateChildItems(hItem, true);
	
	return 0;
}

LRESULT FolderTree::OnUnChecked(HTREEITEM hItem, BOOL& /*bHandled*/)
{
	FolderTreeItemInfo* pItem = (FolderTreeItemInfo*) GetItemData(hItem);

	HTREEITEM hSharedParent = HasSharedParent(hItem);
	// if no parent is checked remove this root folder from share
	if(hSharedParent == NULL) {
		string path = Text::fromT(pItem->m_sFQPath);
		if( path[ path.length() -1 ] != PATH_SEPARATOR )
			path += PATH_SEPARATOR;

		sp->removeExcludeFolder(path);
		int8_t confirmOption = SharePage::CONFIRM_ASK;
		sp->removeDir(path, sp->curProfile, confirmOption);
		UpdateParentItems(hItem);
	} else if(GetChecked(GetParentItem(hItem))) {
		// if the parent is checked add this folder to excludes
		string path = Text::fromT(pItem->m_sFQPath);
		if(	path[ path.length() -1 ] != PATH_SEPARATOR )
			path += PATH_SEPARATOR;
		sp->addExcludeFolder(path);
	}
	
	UpdateChildItems(hItem, false);

	return 0;
}

bool FolderTree::GetHasSharedChildren(HTREEITEM hItem, const ShareDirInfo::list& aShared)
{
	string searchStr;
	int startPos = 0;

	if(hItem == m_hMyComputerRoot)
	{
		startPos = 1;
		searchStr = ":\\";
	}
	else if(hItem == m_hNetworkRoot)
		searchStr = "\\\\";
	else if(hItem != NULL)
	{
		FolderTreeItemInfo* pItem  = (FolderTreeItemInfo*) GetItemData(hItem);
		searchStr = Text::fromT(pItem->m_sFQPath);

		if(searchStr.empty())
			return false;

	}
	else
        return false;

	for(auto i = aShared.begin(); i != aShared.end(); ++i)
	{
		if((*i)->path.size() > searchStr.size() + startPos)
		{
			if(stricmp((*i)->path.substr(startPos, searchStr.size()), searchStr) == 0) {
				if(searchStr.size() <= 3) {
					//if(Util::fileExists(i->path + PATH_SEPARATOR))
						return true;
				} else {
					if((*i)->path.substr(searchStr.size()).substr(0,1) == "\\") {
						//if(Util::fileExists(i->path + PATH_SEPARATOR))
							return true;
					} else
						return false;
				}
			}
		}
	}

	return false;
}

void FolderTree::SetHasSharedChildren(HTREEITEM hItem, const ShareDirInfo::list& aShared)
{
	SetHasSharedChildren(hItem, GetHasSharedChildren(hItem, aShared));
}

void FolderTree::SetHasSharedChildren(HTREEITEM hItem, bool bHasSharedChildren)
{
	if(bHasSharedChildren)
		SetItemState(hItem, TVIS_BOLD, TVIS_BOLD);
	else
		SetItemState(hItem, 0, TVIS_BOLD);
}

void FolderTree::UpdateChildItems(HTREEITEM hItem, bool bChecked)
{
	HTREEITEM hChild = GetChildItem(hItem);
	while(hChild)
	{
		SetChecked(hChild, bChecked);
		UpdateChildItems(hChild, bChecked);
		hChild = GetNextSiblingItem(hChild);
	}
}

void FolderTree::UpdateParentItems(HTREEITEM hItem)
{
	HTREEITEM hParent = GetParentItem(hItem);
	if(hParent != NULL && HasSharedParent(hParent) == NULL)
	{
		SetHasSharedChildren(hParent, sp->getViewItems(sp->curProfile));
		UpdateParentItems(hParent);
	}
}

HTREEITEM FolderTree::HasSharedParent(HTREEITEM hItem)
{
	HTREEITEM hParent = GetParentItem(hItem);
	while(hParent != NULL)
	{
		if(GetChecked(hParent))
			return hParent;

		hParent = GetParentItem(hParent);
	}

	return NULL;
}

void FolderTree::ShareParentButNotSiblings(HTREEITEM hItem)
{
	FolderTreeItemInfo* pItem;
	HTREEITEM hParent = GetParentItem(hItem);
	if(!GetChecked(hParent))
	{
		SetChecked(hParent, true);
		pItem = (FolderTreeItemInfo*) GetItemData(hParent);
		string path = Text::fromT(pItem->m_sFQPath);
		if( path[ path.length() -1 ] != PATH_SEPARATOR )
			path += PATH_SEPARATOR;
		//m_nShareSizeDiff += ShareManager::getInstance()->removeExcludeFolder(path);
		sp->removeExcludeFolder(path);

		ShareParentButNotSiblings(hParent);
		
		HTREEITEM hChild = GetChildItem(hParent);
		while(hChild)
		{
			HTREEITEM hNextItem = GetNextSiblingItem(hChild);
			if(!GetChecked(hChild))
			{
				pItem = (FolderTreeItemInfo*) GetItemData(hChild);
				if(hChild != hItem) {
					string path = Text::fromT(pItem->m_sFQPath);
					if( path[ path.length() -1 ] != PATH_SEPARATOR )
						path += PATH_SEPARATOR;
					//m_nShareSizeDiff -= ShareManager::getInstance()->addExcludeFolder(path);
					sp->addExcludeFolder(path);
				}
			}
			hChild = hNextItem;
		}
	}
	else
	{
		pItem = (FolderTreeItemInfo*) GetItemData(hItem);
		string path = Text::fromT(pItem->m_sFQPath);
		if( path[ path.length() -1 ] != PATH_SEPARATOR )
			path += PATH_SEPARATOR;
		sp->removeExcludeFolder(path);
	}
}
