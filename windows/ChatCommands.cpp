/*
 * Copyright (C) 2012-2013 AirDC++ Project
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
#include "ChatCommands.h"

#define COMPILE_MULTIMON_STUBS 1

#include <MultiMon.h>
#include <psapi.h>
#include <direct.h>
#include <pdh.h>
#include <WinInet.h>
#include <atlwin.h>

#include "../client/Util.h"
#include "../client/SettingsManager.h"
#include "../client/UploadManager.h"
#include "../client/DownloadManager.h"
#include "../client/version.h"
#include "../client/Socket.h"

#include "boost/format.hpp"


tstring ChatCommands::UselessInfo() {
	tstring result = _T("\n");
	TCHAR buf[255];
	
	MEMORYSTATUSEX mem;
	mem.dwLength = sizeof(MEMORYSTATUSEX);
	if( GlobalMemoryStatusEx(&mem) != 0){
		result += _T("Memory\n");
		result += _T("Physical memory (available/total): ");
		result += Util::formatBytesW( mem.ullAvailPhys ) + _T("/") + Util::formatBytesW( mem.ullTotalPhys );
		result += _T("\n");
		result += _T("Pagefile (available/total): ");
		result += Util::formatBytesW( mem.ullAvailPageFile ) + _T("/") + Util::formatBytesW( mem.ullTotalPageFile );
		result += _T("\n");
		result += _T("Memory load: ");
		result += Util::toStringW(mem.dwMemoryLoad);
		result += _T("%\n\n");
	}

	CRegKey key;
	ULONG len = 255;

	if(key.Open( HKEY_LOCAL_MACHINE, _T("Hardware\\Description\\System\\CentralProcessor\\0"), KEY_READ) == ERROR_SUCCESS) {
		result += _T("CPU\n");
        if(key.QueryStringValue(_T("ProcessorNameString"), buf, &len) == ERROR_SUCCESS){
			result += _T("Name: ");
			tstring tmp = buf;
            result += tmp.substr( tmp.find_first_not_of(_T(" ")) );
			result += _T("\n");
		}
		DWORD speed;
		if(key.QueryDWORDValue(_T("~MHz"), speed) == ERROR_SUCCESS){
			result += _T("Speed: ");
			result += Util::toStringW(speed);
			result += _T("MHz\n");
		}
		len = 255;
		if(key.QueryStringValue(_T("Identifier"), buf, &len) == ERROR_SUCCESS) {
			result += _T("Identifier: ");
			result += buf;
		}
		result += _T("\n\n");
	}

	result += _T("OS\n");
	result += Text::toT(Util::getOsVersion());
	result += _T("\n");

	result += _T("\nDisk\n");
	result += _T("Disk space(free/total): ");
	result += DiskSpaceInfo(true);
	result += _T("\n\nTransfer\n");
	result += Text::toT("Upload: " + Util::formatBytes(SETTING(TOTAL_UPLOAD)) + ", Download: " + Util::formatBytes(SETTING(TOTAL_DOWNLOAD)));
	
	if(SETTING(TOTAL_DOWNLOAD) > 0) {
		_stprintf(buf, _T("Ratio (up/down): %.2f"), ((double)SETTING(TOTAL_UPLOAD)) / ((double)SETTING(TOTAL_DOWNLOAD)));
		result += _T('\n');
		result += buf;
	}

	return result;
}

tstring ChatCommands::DiskSpaceInfo(bool onlyTotal /* = false */) {
	tstring ret = Util::emptyStringT;

	int64_t free = 0, totalFree = 0, size = 0, totalSize = 0, netFree = 0, netSize = 0;

   TStringList volumes = FindVolumes();

   for(TStringIter i = volumes.begin(); i != volumes.end(); i++) {
	   if(GetDriveType((*i).c_str()) == DRIVE_CDROM || GetDriveType((*i).c_str()) == DRIVE_REMOVABLE)
		   continue;
	   if(GetDiskFreeSpaceEx((*i).c_str(), NULL, (PULARGE_INTEGER)&size, (PULARGE_INTEGER)&free)){
				totalFree += free;
				totalSize += size;
		}
   }

   //check for mounted Network drives
   ULONG drives = _getdrives();
   TCHAR drive[3] = { _T('A'), _T(':'), _T('\0') };
   
	while(drives != 0) {
		if(drives & 1 && ( GetDriveType(drive) != DRIVE_CDROM && GetDriveType(drive) != DRIVE_REMOVABLE && GetDriveType(drive) == DRIVE_REMOTE) ){
			if(GetDiskFreeSpaceEx(drive, NULL, (PULARGE_INTEGER)&size, (PULARGE_INTEGER)&free)){
				netFree += free;
				netSize += size;
			}
		}
		++drive[0];
		drives = (drives >> 1);
	}
   
	if(totalSize != 0)
		if( !onlyTotal ) {
			ret += _T("\r\n Local HDD space (free/total): ") + Util::formatBytesW(totalFree) + _T("/") + Util::formatBytesW(totalSize);
		if(netSize != 0) {
			ret +=  _T("\r\n Network HDD space (free/total): ") + Util::formatBytesW(netFree) + _T("/") + Util::formatBytesW(netSize);
			ret +=  _T("\r\n Total HDD space (free/total): ") + Util::formatBytesW((netFree+totalFree)) + _T("/") + Util::formatBytesW(netSize+totalSize);
			}
		} else
			ret += Util::formatBytesW(totalFree) + _T("/") + Util::formatBytesW(totalSize);

	return ret;
}

TStringList ChatCommands::FindVolumes() {

  BOOL  found;
  TCHAR   buf[MAX_PATH];  
  HANDLE  hVol;
  TStringList volumes;

   hVol = FindFirstVolume(buf, MAX_PATH);

   if(hVol != INVALID_HANDLE_VALUE) {
		volumes.push_back(buf);

		found = FindNextVolume(hVol, buf, MAX_PATH);

		//while we find drive volumes.
		while(found) { 
			volumes.push_back(buf);
			found = FindNextVolume(hVol, buf, MAX_PATH); 
		}
   
	found = FindVolumeClose(hVol);
   }

    return volumes;
}

tstring ChatCommands::diskInfo() {

	tstring result = Util::emptyStringT;
		
	TCHAR   buf[MAX_PATH];
	int64_t free = 0, size = 0 , totalFree = 0, totalSize = 0;
	int disk_count = 0;
   
	std::vector<tstring> results; //add in vector for sorting, nicer to look at :)
	// lookup drive volumes.
	TStringList volumes = FindVolumes();

	for(TStringIter i = volumes.begin(); i != volumes.end(); i++) {
		if(GetDriveType((*i).c_str()) == DRIVE_CDROM || GetDriveType((*i).c_str()) == DRIVE_REMOVABLE)
			continue;
	    
		if((GetVolumePathNamesForVolumeName((*i).c_str(), buf, 256, NULL) != 0) &&
			(GetDiskFreeSpaceEx((*i).c_str(), NULL, (PULARGE_INTEGER)&size, (PULARGE_INTEGER)&free) !=0)){
			tstring mountpath = buf; 
			if(!mountpath.empty()) {
				totalFree += free;
				totalSize += size;
				results.push_back((_T("MountPath: ") + mountpath + _T(" Disk Space (free/total) ") + Util::formatBytesW(free) + _T("/") +  Util::formatBytesW(size)));
			}
		}
	}
      
	// and a check for mounted Network drives, todo fix a better way for network space
	ULONG drives = _getdrives();
	TCHAR drive[3] = { _T('A'), _T(':'), _T('\0') };
   
	while(drives != 0) {
		if(drives & 1 && ( GetDriveType(drive) != DRIVE_CDROM && GetDriveType(drive) != DRIVE_REMOVABLE && GetDriveType(drive) == DRIVE_REMOTE) ){
			if(GetDiskFreeSpaceEx(drive, NULL, (PULARGE_INTEGER)&size, (PULARGE_INTEGER)&free)){
				totalFree += free;
				totalSize += size;
				results.push_back((_T("Network MountPath: ") + (tstring)drive + _T(" Disk Space (free/total) ") + Util::formatBytesW(free) + _T("/") +  Util::formatBytesW(size)));
			}
		}

		++drive[0];
		drives = (drives >> 1);
	}

	sort(results.begin(), results.end()); //sort it
	for(std::vector<tstring>::iterator i = results.begin(); i != results.end(); ++i) {
		disk_count++;
		result += _T("\r\n ") + *i; 
	}
	result +=  _T("\r\n\r\n Total HDD space (free/total): ") + Util::formatBytesW((totalFree)) + _T("/") + Util::formatBytesW(totalSize);
	result += _T("\r\n Total Drives count: ") + Text::toT(Util::toString(disk_count));
   
	results.clear();

   return result;
}

float ProcSpeedCalc() {
#ifndef _WIN64
#define RdTSC __asm _emit 0x0f __asm _emit 0x31
__int64 cyclesStart = 0, cyclesStop = 0;
unsigned __int64 nCtr = 0, nFreq = 0, nCtrStop = 0;
    if(!QueryPerformanceFrequency((LARGE_INTEGER *) &nFreq)) return 0;
    QueryPerformanceCounter((LARGE_INTEGER *) &nCtrStop);
    nCtrStop += nFreq;
    _asm {
		RdTSC
        mov DWORD PTR cyclesStart, eax
        mov DWORD PTR [cyclesStart + 4], edx
    } do {
		QueryPerformanceCounter((LARGE_INTEGER *) &nCtr);
    } while (nCtr < nCtrStop);
    _asm {
		RdTSC
        mov DWORD PTR cyclesStop, eax
        mov DWORD PTR [cyclesStop + 4], edx
    }
	return ((float)cyclesStop-(float)cyclesStart) / 1000000;
#else
	HKEY hKey;
	DWORD dwSpeed;

	// Get the key name
	wchar_t szKey[256];
	_snwprintf(szKey, sizeof(szKey)/sizeof(wchar_t),
		L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\%d\\", 0);

	// Open the key
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,szKey, 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
	{
		return 0;
	}

	// Read the value
	DWORD dwLen = 4;
	if(RegQueryValueEx(hKey, L"~MHz", NULL, NULL, (LPBYTE)&dwSpeed, &dwLen) != ERROR_SUCCESS)
	{
		RegCloseKey(hKey);
		return 0;
	}

	// Cleanup and return
	RegCloseKey(hKey);
	
	return dwSpeed;
#endif
}

tstring ChatCommands::Speedinfo() {
	tstring result = _T("\n");

result += _T("-= ");
result += _T("Downloading: ");
result += Util::formatBytesW(DownloadManager::getInstance()->getRunningAverage()) + _T("/s  [");
result += Util::toStringW(DownloadManager::getInstance()->getDownloadCount()) + _T("]");
result += _T(" =- ");
result += _T(" -= ");
result += _T("Uploading: ");
result += Util::formatBytesW(UploadManager::getInstance()->getRunningAverage()) + _T("/s  [");
result += Util::toStringW(UploadManager::getInstance()->getUploadCount()) + _T("]");
result += _T(" =-");

return result;

}
string ChatCommands::getSysUptime(){
			
	static HINSTANCE kernel32lib = NULL;
	if(!kernel32lib)
		kernel32lib = LoadLibrary(_T("kernel32"));
		
	//apexdc
	typedef ULONGLONG (CALLBACK* LPFUNC2)(void);
	LPFUNC2 _GetTickCount64 = (LPFUNC2)GetProcAddress(kernel32lib, "GetTickCount64");
	time_t sysUptime = (_GetTickCount64 ? _GetTickCount64() : GetTickCount()) / 1000;

	return Util::formatTime(sysUptime, false);

}


string ChatCommands::generateStats() {
	PROCESS_MEMORY_COUNTERS pmc;
	pmc.cb = sizeof(pmc);
	typedef bool (CALLBACK* LPFUNC)(HANDLE Process, PPROCESS_MEMORY_COUNTERS ppsmemCounters, DWORD cb);
		
	LPFUNC _GetProcessMemoryInfo = (LPFUNC)GetProcAddress(LoadLibrary(_T("psapi")), "GetProcessMemoryInfo");
	_GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
	FILETIME tmpa, tmpb, kernelTimeFT, userTimeFT;
	GetProcessTimes(GetCurrentProcess(), &tmpa, &tmpb, &kernelTimeFT, &userTimeFT);
	int64_t kernelTime = kernelTimeFT.dwLowDateTime | (((int64_t)kernelTimeFT.dwHighDateTime) << 32);
	int64_t userTime = userTimeFT.dwLowDateTime | (((int64_t)userTimeFT.dwHighDateTime) << 32);  

	string ret = boost::str(boost::format(
"\r\n\t-=[ %s   http://www.airdcpp.net ]=-\r\n\
\t-=[ Uptime: %s ][ CPU time: %s ]=-\r\n\
\t-=[ Memory usage (peak): %s (%s) ]=-\r\n\
\t-=[ Virtual Memory usage (peak): %s (%s) ]=-\r\n\
\t-=[ Downloaded: %s ][  Uploaded %s ]=-\r\n\
\t-=[ Total download %s ][ Total upload %s ]=-\r\n\
\t-=[ System: %s (Uptime: %s) ]=-\r\n\
\t-=[ CPU: %s ]=-")

		% Text::fromT(COMPLETEVERSIONSTRING)
		% Util::formatTime(Util::getUptime(), false)
		% Util::formatSeconds((kernelTime + userTime) / (10I64 * 1000I64 * 1000I64))
		% Util::formatBytes(pmc.WorkingSetSize)
		% Util::formatBytes(pmc.PeakWorkingSetSize)
		% Util::formatBytes(pmc.PagefileUsage)
		% Util::formatBytes(pmc.PeakPagefileUsage)
		% Util::formatBytes(Socket::getTotalDown())
		% Util::formatBytes(Socket::getTotalUp())
		% Util::formatBytes(SETTING(TOTAL_DOWNLOAD))
		% Util::formatBytes(SETTING(TOTAL_UPLOAD))
		% Util::getOsVersion()
		% getSysUptime()
		% CPUInfo());
	return ret;
}


string ChatCommands::CPUInfo() {
	tstring result = _T("");

	CRegKey key;
	ULONG len = 255;

	if(key.Open( HKEY_LOCAL_MACHINE, _T("Hardware\\Description\\System\\CentralProcessor\\0"), KEY_READ) == ERROR_SUCCESS) {
		TCHAR buf[255];
		if(key.QueryStringValue(_T("ProcessorNameString"), buf, &len) == ERROR_SUCCESS){
			tstring tmp = buf;
			result = tmp.substr( tmp.find_first_not_of(_T(" ")) );
		}
		DWORD speed;
		if(key.QueryDWORDValue(_T("~MHz"), speed) == ERROR_SUCCESS){
			result += _T(" (");
			result += Util::toStringW((uint32_t)speed);
			result += _T(" MHz)");
		}
	}
	return result.empty() ? "Unknown" : Text::fromT(result);
}

string ChatCommands::uptimeInfo() {
	char buf[512]; 
	snprintf(buf, sizeof(buf), "\n-=[ Uptime: %s]=-\r\n-=[ System Uptime: %s]=-\r\n", 
	Util::formatTime(Util::getUptime(), false).c_str(), 
	getSysUptime().c_str());
	return buf;
}