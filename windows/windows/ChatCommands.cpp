/*
 * Copyright (C) 2012-2024 AirDC++ Project
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
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

#include <windows/stdafx.h>
#include <windows/ChatCommands.h>

#include <windows/HttpLinks.h>
#include <windows/util/WinUtil.h>

#define COMPILE_MULTIMON_STUBS 1

#include <MultiMon.h>
#include <psapi.h>
#include <direct.h>
#include <pdh.h>
#include <WinInet.h>
#include <atlwin.h>

#include <airdcpp/hub/ClientManager.h>
#include <airdcpp/util/Util.h>
#include <airdcpp/settings/SettingsManager.h>
#include <airdcpp/share/ShareManager.h>
#include <airdcpp/transfer/upload/UploadManager.h>
#include <airdcpp/transfer/download/DownloadManager.h>
#include <airdcpp/util/SystemUtil.h>
#include <airdcpp/core/timer/TimerManager.h>
#include <airdcpp/core/version.h>
#include <airdcpp/connection/socket/Socket.h>

#include <windows/util/FormatUtil.h>

#include "boost/format.hpp"


namespace wingui {
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
		result += WinUtil::toStringW(mem.dwMemoryLoad);
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
			result += WinUtil::toStringW(speed);
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
	result += Text::toT(SystemUtil::getOsVersion());
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

   auto volumes = FindVolumes();
   for (const auto& vol: volumes) {
	   if(GetDriveType(vol.c_str()) == DRIVE_CDROM || GetDriveType(vol.c_str()) == DRIVE_REMOVABLE)
		   continue;
	   if(GetDiskFreeSpaceEx(vol.c_str(), NULL, (PULARGE_INTEGER)&size, (PULARGE_INTEGER)&free)){
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
   
	TStringList results; //add in vector for sorting, nicer to look at :)
	// lookup drive volumes.
	auto volumes = FindVolumes();
	for (const auto& vol: volumes) {
		if(GetDriveType(vol.c_str()) == DRIVE_CDROM || GetDriveType(vol.c_str()) == DRIVE_REMOVABLE)
			continue;
	    
		if((GetVolumePathNamesForVolumeName(vol.c_str(), buf, 256, NULL) != 0) &&
			(GetDiskFreeSpaceEx(vol.c_str(), NULL, (PULARGE_INTEGER)&size, (PULARGE_INTEGER)&free) !=0)){
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
	for(const auto& res: results) {
		disk_count++;
		result += _T("\r\n ") + res; 
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
result += WinUtil::toStringW(DownloadManager::getInstance()->getTotalDownloadConnectionCount()) + _T("]");
result += _T(" =- ");
result += _T(" -= ");
result += _T("Uploading: ");
result += Util::formatBytesW(UploadManager::getInstance()->getRunningAverage()) + _T("/s  [");
result += WinUtil::toStringW(UploadManager::getInstance()->getUploadCount()) + _T("]");
result += _T(" =-");

return result;

}

string ChatCommands::ClientVersionInfo() {
	//TODO: add something cleaner / better?
	return shortVersionString + " / Build: " + BUILD_NUMBER_STR + " / Version Date: " + FormatUtil::formatDateTime(getVersionDate());
}

string ChatCommands::getSysUptime(){
			
	static HINSTANCE kernel32lib = NULL;
	if(!kernel32lib)
		kernel32lib = LoadLibrary(_T("kernel32"));
		
	//apexdc
	typedef ULONGLONG (CALLBACK* LPFUNC2)(void);
	LPFUNC2 _GetTickCount64 = (LPFUNC2)GetProcAddress(kernel32lib, "GetTickCount64");
	time_t sysUptime = (_GetTickCount64 ? _GetTickCount64() : GetTickCount()) / 1000;

	return Util::formatDuration(sysUptime, false);

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
"\r\n\t-=[ %s   %s ]=-\r\n\
\t-=[ Uptime: %s ][ CPU time: %s ]=-\r\n\
\t-=[ Memory usage (peak): %s (%s) ]=-\r\n\
\t-=[ Virtual Memory usage (peak): %s (%s) ]=-\r\n\
\t-=[ Downloaded: %s ][  Uploaded %s ]=-\r\n\
\t-=[ Total download %s ][ Total upload %s ]=-\r\n\
\t-=[ System: %s (Uptime: %s) ]=-\r\n\
\t-=[ CPU: %s ]=-")

		% fullVersionString
		% Text::fromT(HttpLinks::homepage)
		% Util::formatDuration(TimerManager::getUptime(), false)
		% Util::formatSeconds((kernelTime + userTime) / (10I64 * 1000I64 * 1000I64))
		% Util::formatBytes(pmc.WorkingSetSize)
		% Util::formatBytes(pmc.PeakWorkingSetSize)
		% Util::formatBytes(pmc.PagefileUsage)
		% Util::formatBytes(pmc.PeakPagefileUsage)
		% Util::formatBytes(Socket::getTotalDown())
		% Util::formatBytes(Socket::getTotalUp())
		% Util::formatBytes(SETTING(TOTAL_DOWNLOAD))
		% Util::formatBytes(SETTING(TOTAL_UPLOAD))
		% SystemUtil::getOsVersion()
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
			result += WinUtil::toStringW((uint32_t)speed);
			result += _T(" MHz)");
		}
	}
	return result.empty() ? "Unknown" : Text::fromT(result);
}

string ChatCommands::uptimeInfo() {
	char buf[512]; 
	snprintf(buf, sizeof(buf), "\n-=[ Uptime: %s]=-\r\n-=[ System Uptime: %s]=-\r\n", 
	Util::formatDuration(TimerManager::getUptime(), false).c_str(),
	getSysUptime().c_str());
	return buf;
}


string ChatCommands::shareStats() noexcept {
	auto optionalItemStats = ShareManager::getInstance()->getShareItemStats();
	if (!optionalItemStats) {
		return "No files shared";
	}

	auto itemStats = *optionalItemStats;

	string ret = boost::str(boost::format(
		"\r\n\r\n-=[ Share statistics ]=-\r\n\r\n\
Share profiles: %d\r\n\
Shared paths: %d\r\n\
Total share size: %s\r\n\
Total shared files: %d (of which %d%% are lowercase)\r\n\
Unique TTHs: %d (%d%%)\r\n\
Total shared directories: %d (%d files per directory)\r\n\
Average age of a file: %s\r\n\
Average name length of a shared item: %d bytes (total size %s)")

% itemStats.profileCount
% itemStats.rootDirectoryCount
% Util::formatBytes(itemStats.totalSize)
% itemStats.totalFileCount % Util::countPercentage(itemStats.lowerCaseFiles, itemStats.totalFileCount)
% itemStats.uniqueFileCount % Util::countPercentage(itemStats.uniqueFileCount, itemStats.totalFileCount)
% itemStats.totalDirectoryCount % Util::countAverage(itemStats.totalFileCount, itemStats.totalDirectoryCount)
% Util::formatDuration(itemStats.averageFileAge, false, true)
% itemStats.averageNameLength
% Util::formatBytes(itemStats.totalNameSize)
);

	auto searchStats = ShareManager::getInstance()->getSearchMatchingStats();
	ret += boost::str(boost::format(
		"\r\n\r\n-=[ Search statistics ]=-\r\n\r\n\
Total incoming searches: %d (%d per second)\r\n\
Incoming text searches: %d (of which %d were matched per second)\r\n\
Filtered text searches: %d%% (%d%% of the matched ones returned results)\r\n\
Average search tokens (non-filtered only): %d (%d bytes per token)\r\n\
Auto searches (text, ADC only): %d%%\r\n\
Average time for matching a recursive search: %d ms\r\n\
TTH searches: %d%% (hash bloom mode: %s)")

% searchStats.totalSearches % searchStats.totalSearchesPerSecond
% searchStats.recursiveSearches % searchStats.unfilteredRecursiveSearchesPerSecond
% Util::countPercentage(searchStats.filteredSearches, searchStats.recursiveSearches) % Util::countPercentage(searchStats.recursiveSearchesResponded, searchStats.recursiveSearches - searchStats.filteredSearches)
% searchStats.averageSearchTokenCount % searchStats.averageSearchTokenLength
% Util::countAverage(searchStats.autoSearches, searchStats.recursiveSearches)
% searchStats.averageSearchMatchMs
% Util::countPercentage(searchStats.tthSearches, searchStats.totalSearches)
% (SETTING(BLOOM_MODE) != SettingsManager::BLOOM_DISABLED ? "Enabled" : "Disabled") // bloom mode
);

	return ret;
}


string ChatCommands::hubStats() noexcept {
	auto optionalStats = ClientManager::getInstance()->getClientStats();
	if (!optionalStats) {
		return "No hubs";
	}

	auto stats = *optionalStats;

	string lb = "\r\n";
	string ret = boost::str(boost::format(
		"\r\n\r\n-=[ Hub statistics ]=-\r\n\r\n\
All users: %d\r\n\
Unique users: %d (%d%%)\r\n\
Active/operators/bots/hidden: %d (%d%%) / %d (%d%%) / %d (%d%%) / %d (%d%%)\r\n\
Protocol users (ADC/NMDC): %d / %d\r\n\
Total share: %s (%s per user)\r\n\
Average ADC connection speed: %s down, %s up\r\n\
Average NMDC connection speed: %s")

% stats.totalUsers
% stats.uniqueUsers % Util::countPercentage(stats.uniqueUsers, stats.totalUsers)
% stats.activeUsers % Util::countPercentage(stats.activeUsers, stats.uniqueUsers)
% stats.operators % Util::countPercentage(stats.operators, stats.uniqueUsers)
% stats.bots % Util::countPercentage(stats.bots, stats.uniqueUsers)
% stats.hiddenUsers % Util::countPercentage(stats.hiddenUsers, stats.uniqueUsers)
% stats.adcUsers % stats.nmdcUsers
% Util::formatBytes(stats.totalShare) % Util::formatBytes(Util::countAverageInt64(stats.totalShare, stats.uniqueUsers))
% FormatUtil::formatConnectionSpeed(stats.downPerAdcUser) % FormatUtil::formatConnectionSpeed(stats.upPerAdcUser)
% FormatUtil::formatConnectionSpeed(stats.nmdcSpeedPerUser)

);

	ret += lb;
	ret += lb;
	ret += "Clients (from unique users)";
	ret += lb;

	for (const auto& [name, count] : stats.clients) {
		ret += name + ":\t\t" + Util::toString(count) + " (" + Util::toString(Util::countPercentage(count, stats.uniqueUsers)) + "%)" + lb;
	}

	return ret;
}
}

