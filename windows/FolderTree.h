/* POSSUM_MOD_BEGIN 

Most of this code was stolen/adapted/massacred from...

Module : FileTreeCtrl.h
Purpose: Interface for an MFC class which provides a tree control similiar 
         to the left hand side of explorer

Copyright (c) 1999 - 2003 by PJ Naughter.  (Web: www.naughter.com, Email: pjna@naughter.com)
*/

#pragma once
#include <shlobj.h>
#include <lm.h>
#include "ShareDirectories.h"

class ShareDirectories;

//Class which gets stored int the item data on the tree control

class FolderTreeItemInfo
{
public:
	//Constructors / Destructors
	FolderTreeItemInfo();
	~FolderTreeItemInfo() {};

	//Member variables
	tstring			m_sFQPath;          //Fully qualified path for this item
	tstring			m_sRelativePath;    //The relative bit of the path
	NETRESOURCE*	m_pNetResource;     //Used if this item is under Network Neighborhood
	bool			m_bNetworkNode;     //Item is "Network Neighborhood" or is underneath it
	bool			m_removed;			//Item for a folder which doesn't exist anymore
};


//class which manages enumeration of shares. This is used for determining 
//if an item is shared or not
class ShareEnumerator
{
public:
	//Constructors / Destructors
	ShareEnumerator();
	~ShareEnumerator();

	//Methods
	void Refresh(); //Updates the internal enumeration list
	bool IsShared(const tstring& sPath);

protected:
	//Defines
	typedef NET_API_STATUS (WINAPI NT_NETSHAREENUM)(LPWSTR, DWORD, LPBYTE*, DWORD, LPDWORD, LPDWORD, LPDWORD);
	typedef NET_API_STATUS (WINAPI NT_NETAPIBUFFERFREE)(LPVOID);
	typedef NET_API_STATUS (WINAPI WIN9X_NETSHAREENUM)(const char FAR *, short, char FAR *, unsigned short, unsigned short FAR *, unsigned short FAR *);

	//Data
	HMODULE                  m_hNetApi;         //Handle to the net api dll
	NT_NETSHAREENUM*         m_pNTShareEnum;    //NT function pointer for NetShareEnum
	NT_NETAPIBUFFERFREE*     m_pNTBufferFree;   //NT function pointer for NetAPIBufferFree
	SHARE_INFO_502*          m_pNTShareInfo;    //NT share info
	DWORD                    m_dwShares;        //The number of shares enumerated
};

//Class which encapsulates access to the System image list which contains
//all the icons used by the shell to represent the file system

class SystemImageList
{
public:
	static SystemImageList* getInstance();
	~SystemImageList();

	CImageList* getImageList()
	{
		return &m_ImageList;
	}
protected:
	CImageList m_ImageList;          

private:
	SystemImageList();
};


//Allowable bit mask flags in SetDriveHideFlags / GetDriveHideFlags
const DWORD DRIVE_ATTRIBUTE_REMOVABLE   = 0x00000001;
const DWORD DRIVE_ATTRIBUTE_FIXED       = 0x00000002;
const DWORD DRIVE_ATTRIBUTE_REMOTE      = 0x00000004;
const DWORD DRIVE_ATTRIBUTE_CDROM       = 0x00000010;
const DWORD DRIVE_ATTRIBUTE_RAMDISK     = 0x00000020;

class FolderTree : public CWindowImpl<FolderTree, CTreeViewCtrl>
{
public:
	FolderTree(ShareDirectories* sp);
	~FolderTree();
	
	BEGIN_MSG_MAP(FolderTree)
		REFLECTED_NOTIFY_CODE_HANDLER(NM_CLICK, OnClick)
		REFLECTED_NOTIFY_CODE_HANDLER(TVN_SELCHANGED, OnSelChanged)
		REFLECTED_NOTIFY_CODE_HANDLER(TVN_ITEMEXPANDING, OnItemExpanding)
		REFLECTED_NOTIFY_CODE_HANDLER(TVN_DELETEITEM, OnDeleteItem)
		DEFAULT_REFLECTION_HANDLER()
	END_MSG_MAP()

	LRESULT OnClick(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& bHandled)
	{
		DWORD dwPos = GetMessagePos();
	    POINT ptPos;
		ptPos.x = GET_X_LPARAM(dwPos);
		ptPos.y = GET_Y_LPARAM(dwPos);

		ScreenToClient(&ptPos);

		UINT uFlags;
		HTREEITEM htItemClicked = HitTest(ptPos, &uFlags);

		// if the item's checkbox was clicked ...
		if (uFlags & TVHT_ONITEMSTATEICON)
		{
			// retrieve is's soon-to-be former state
			if(!GetChecked(htItemClicked))
				return OnChecked(htItemClicked, bHandled);
			else
				return OnUnChecked(htItemClicked, bHandled);
		}

		bHandled = FALSE;
		return 0;
	}

	LRESULT OnSelChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT OnItemExpanding(int idCtrl, LPNMHDR pnmh, BOOL &bHandled);
	LRESULT OnDeleteItem(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

	LRESULT OnChecked(HTREEITEM hItem, BOOL &bHandled);
	LRESULT OnUnChecked(HTREEITEM hItem, BOOL &bHandled);

	void PopulateTree();
	void Refresh();
	tstring ItemToPath(HTREEITEM hItem) const;
	void Clear();
	HTREEITEM SetSelectedPath(const tstring& sPath, bool bExpanded = false);
	bool IsDrive(HTREEITEM hItem);
	bool IsDrive(const tstring& sPath);
	bool IsFolder(const tstring& sPath);
	bool GetChecked(HTREEITEM hItem) const;
    BOOL SetChecked(HTREEITEM hItem, bool fCheck);

protected:
	ShareDirectories* sp;
	bool IsExpanded(HTREEITEM hItem);
	int GetIconIndex(const tstring& sFilename);
	int GetIconIndex(HTREEITEM hItem);
	int GetIconIndex(LPITEMIDLIST lpPIDL);
	int GetSelIconIndex(const tstring& sFilename);
	int GetSelIconIndex(HTREEITEM hItem);
	int GetSelIconIndex(LPITEMIDLIST lpPIDL);
	HTREEITEM InsertFileItem(HTREEITEM hParent, FolderTreeItemInfo* pItem, bool bShared, int nIcon, int nSelIcon, bool bCheckForChildren);
	void DisplayDrives(HTREEITEM hParent, bool bUseSetRedraw);
	void DisplayPath(const tstring& sPath, HTREEITEM hParent, bool bUseSetRedraw = true);
	tstring GetDriveLabel(const tstring& sDrive);
	tstring GetCorrectedLabel(FolderTreeItemInfo* pItem);
	bool HasGotSubEntries(const tstring& sDirectory);
	bool CanDisplayDrive(const tstring& sDrive);
	bool IsShared(const tstring& sPath);
	static int CALLBACK CompareByFilenameNoCase(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
	void SetHasPlusButton(HTREEITEM hItem, bool bHavePlus);
	bool HasPlusButton(HTREEITEM hItem);
	void DoExpand(HTREEITEM hItem);
	HTREEITEM FindServersNode(HTREEITEM hFindFrom) const;
	HTREEITEM FindSibling(HTREEITEM hParent, const tstring& sItem) const;
	bool DriveHasRemovableMedia(const tstring& sPath);
	bool IsMediaValid(const tstring& sDrive);
	bool EnumNetwork(HTREEITEM hParent);
	int DeleteChildren(HTREEITEM hItem, bool bUpdateChildIndicator);
	BOOL GetSerialNumber(const tstring& sDrive, DWORD& dwSerialNumber);
	void SetHasSharedChildren(HTREEITEM hItem, bool bHasSharedChildren);
	void SetHasSharedChildren(HTREEITEM hItem);
	bool GetHasSharedChildren(HTREEITEM hItem);
	HTREEITEM HasSharedParent(HTREEITEM hItem);
	void ShareParentButNotSiblings(HTREEITEM hItem);
	void UpdateChildItems(HTREEITEM hItem, bool bChecked);
	void UpdateParentItems(HTREEITEM hItem);

	void checkRemovedDirs(const tstring& aParentPath, HTREEITEM hParent);
	
	//Member variables
	tstring			m_sRootFolder;
	HTREEITEM       m_hNetworkRoot;
	HTREEITEM       m_hMyComputerRoot;
	HTREEITEM       m_hRootedFolder;
	bool            m_bShowMyComputer;
	CImageList*     m_pilDrag;
	DWORD           m_dwDriveHideFlags;
	DWORD           m_dwFileHideFlags;
	COLORREF        m_rgbCompressed;
	bool            m_bShowCompressedUsingDifferentColor;
	COLORREF        m_rgbEncrypted;
	bool            m_bShowEncryptedUsingDifferentColor;
	bool            m_bDisplayNetwork;
	bool			m_bShowSharedUsingDifferentIcon;
	DWORD			m_dwMediaID[26];
	IMalloc*        m_pMalloc;
	IShellFolder*   m_pShellFolder;
	DWORD           m_dwNetworkItemTypes;
	bool            m_bShowDriveLabels;
	bool            m_bShowRootedFolder;
	int64_t			m_nShareSizeDiff;

	ShareEnumerator theSharedEnumerator;
};

