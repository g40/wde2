/*
    Core functions to access Win32 disk/partition structures

    Steadfastly UNICODE.

    Use with caution.

    Visit https://github.com/g40

    Copyright (c) Jerry Evans, 2014-2024

    All rights reserved.

    The MIT License (MIT)

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.

*/

#pragma once

#include <winioctl.h>
#include <sys/types.h>  
#include <sys/stat.h>  

#include <stdio.h>
#include <stdlib.h>
#include <Setupapi.h>
#include <Ntddstor.h>
#include <Ntddscsi.h>


/*

[1]

PARTITION_BASIC_DATA_GUID
ebd0a0a2-b9e5-4433-87c0-68b6b72699c7
The data partition type that is created and recognized by Windows.
Only partitions of this type can be assigned drive letters, receive volume GUID paths, host mounted folders (also called volume mount points), and be enumerated by calls to FindFirstVolume and FindNextVolume.

This value can be set only for basic disks, with one exception. If both PARTITION_BASIC_DATA_GUID and GPT_ATTRIBUTE_PLATFORM_REQUIRED are set for a partition on a basic disk that is subsequently converted to a dynamic disk, the partition remains a basic partition, even though the rest of the disk is a dynamic disk. This is because the partition is considered to be an OEM partition on a GPT disk.

PARTITION_ENTRY_UNUSED_GUID
00000000-0000-0000-0000-000000000000
There is no partition.
This value can be set for basic and dynamic disks.

PARTITION_SYSTEM_GUID
c12a7328-f81f-11d2-ba4b-00a0c93ec93b
The partition is an EFI system partition.
This value can be set for basic and dynamic disks.

PARTITION_MSFT_RESERVED_GUID
e3c9e316-0b5c-4db8-817d-f92df00215ae
The partition is a Microsoft reserved partition.
This value can be set for basic and dynamic disks.

PARTITION_LDM_METADATA_GUID
5808c8aa-7e8f-42e0-85d2-e1e90434cfb3
The partition is a Logical Disk Manager (LDM) metadata partition on a dynamic disk.
This value can be set only for dynamic disks.

PARTITION_LDM_DATA_GUID
af9b60a0-1431-4f62-bc68-3311714a69ad
The partition is an LDM data partition on a dynamic disk.
This value can be set only for dynamic disks.

PARTITION_MSFT_RECOVERY_GUID
de94bba4-06d1-4d40-a16a-bfd50179d6ac
The partition is a Microsoft recovery partition.
This value can be set for basic and dynamic disks.

*/
//-------------------------------------------------------------------------
// two macros ensures any macro passed will
// be expanded before being stringified
#define STRINGIZE_INT(x) #x
#define STRINGIZE(x) STRINGIZE_INT(x)
#define LFL __FILE__ "(" STRINGIZE(__LINE__) "): "

// this is emphatically UNICODE only
#define __W(arg) L ## arg
#define _W(arg) __W(arg)

// GPT 4TB
//#define DISK_NUMBER 4
// MBR 250GB *VHD*
#define DISK_NUMBER 9

//-----------------------------------------------------------------------------
#if 0
#define TRACE(arg) OutputDebugStringA(LFL arg "\n");
#else
#define TRACE(arg)
#endif

#include <map>
#include "structs.h"

namespace wde2
{
    namespace w32
    {
        // see [1] above
        static const GUID g0 = { 0xebd0a0a2, 0xb9e5, 0x4433, { 0x87, 0xc0, 0x68, 0xb6, 0xb7, 0x26, 0x99, 0xc7} };
        static const GUID g1 = { 0x00000000, 0x0000, 0x0000, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} };
        static const GUID g2 = { 0xc12a7328, 0xf81f, 0x11d2, { 0xba, 0x4b, 0x00, 0xa0, 0xc9, 0x3e, 0xc9, 0x3b} };
        static const GUID g3 = { 0xe3c9e316, 0x0b5c, 0x4db8, { 0x81, 0x7d, 0xf9, 0x2d, 0xf0, 0x02, 0x15, 0xae} };
        static const GUID g4 = { 0x5808c8aa, 0x7e8f, 0x42e0, { 0x85, 0xd2, 0xe1, 0xe9, 0x04, 0x34, 0xcf, 0xb3} };
        static const GUID g5 = { 0xaf9b60a0, 0x1431, 0x4f62, { 0xbc, 0x68, 0x33, 0x11, 0x71, 0x4a, 0x69, 0xad} };
        static const GUID g6 = { 0xde94bba4, 0x06d1, 0x4d40, { 0xa1, 0x6a, 0xbf, 0xd5, 0x01, 0x79, 0xd6, 0xac} };

        struct GUIDComparer
        {
            bool operator()(const GUID& Left, const GUID& Right) const
            {
                // comparison logic goes here
                return memcmp(&Left, &Right, sizeof(Right)) < 0;
            }
        };

        // https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ns-winioctl-partition_information_gpt
        static std::map<GUID, std::wstring, GUIDComparer> mapper =
        {
            { g0, _W("PARTITION_BASIC_DATA_GUID") } ,
            { g1, _W("PARTITION_ENTRY_UNUSED_GUID") },
            { g2, _W("PARTITION_SYSTEM_GUID") },
            { g3, _W("PARTITION_MSFT_RESERVED_GUID")},
            { g4, _W("PARTITION_LDM_METADATA_GUID")},
            { g5, _W("PARTITION_LDM_DATA_GUID")},
            { g6, _W("PARTITION_MSFT_RECOVERY_GUID")},
        };

        std::wstring GUIDToPartitionTypeString(const GUID& guid)
        {
            //
            using iter = std::map<GUID, std::wstring, GUIDComparer>::iterator;
            //
            std::wstring ret = _W("");
            iter it = mapper.find(guid);
            if (it != mapper.end())
                ret = it->second;
            return ret;
        }
    }

    // convert to human units
    static const uint64_t _8KB = 8 * 1024ull;
    static const uint64_t _1MB = 1024ull * 1024ull;
    static const uint64_t _1GB = 1024ull * 1024ull * 1024ull;

    //-----------------------------------------------------------------------------
    static const wchar_t* pps[] = {
        _W("PARTITION_STYLE_MBR"),
        _W("PARTITION_STYLE_GPT"),
        _W("PARTITION_STYLE_RAW")
    };

    //-----------------------------------------------------------------------------
    std::wstring GUIDToString(GUID guid)
    {
        std::wstring sv = _W("");
        OLECHAR buffer[64] = { 0 };
        int ret = StringFromGUID2(guid, buffer, 64);
        if (ret)
        {
            sv = buffer;
        }
        return sv;
    }

    //----------------------------------------------------------------------------
    // optional hex or decimal string to int        
    int xstoi(const string_t& arg)
    {
        int base = 10;
        if ((arg.find(_T("0x")) == 0) ||
            (arg.find(_T("0X")) == 0)) {
            base = 16;
        }
        std::size_t pos = 0;
        int val = std::stoi(arg, &pos, base);
        return val;
    }

    //----------------------------------------------------------------------------
    // https://learn.microsoft.com/en-us/windows/win32/fileio/disk-partition-types
    static
        std::wstring partitionIDToString(int id)
    {
        using iter = std::map<int, std::wstring>::iterator;
        std::map<int, std::wstring> mapper = {
            {PARTITION_ENTRY_UNUSED,_W("PARTITION_ENTRY_UNUSED")},
            {PARTITION_EXTENDED,_W("PARTITION_EXTENDED")},
            {PARTITION_FAT_12,_W("PARTITION_FAT_12")},
            {PARTITION_FAT_16,_W("PARTITION_FAT_16")},
            {PARTITION_FAT32,_W("PARTITION_FAT32")},
            {PARTITION_IFS,_W("PARTITION_IFS")},
            {PARTITION_MSFT_RECOVERY,_W("PARTITION_MSFT_RECOVERY")},
        };

        iter it = mapper.find(id);
        return (it == mapper.end() ? _W("UNKNOWN") : it->second);
    }

    //-----------------------------------------------------------------------------
    // given a raw GUID (e.g. Partition ID)
    static std::vector<std::wstring> getDOSNamesFromVolumeGUID(const GUID& guidVolume)
    {
        //
        std::vector<std::wstring> v;
        //
        nv2::acc key;
        // essential! must have trailing slash
        key << _T("\\\\?\\Volume") << guidVolume << _T("\\");
        // test
        if (0)
        {
            // must not have trailing slash. WTF?
            std::wstring path = key.wstr();
            path.pop_back();
            // Open the drive. Ultra-high caution required here.
            HANDLE hDevice = CreateFile(path.c_str(),
                //
                GENERIC_READ | GENERIC_WRITE,		  // no access to the drive 
                // FILE_SHARE_READ | FILE_SHARE_WRITE, // share mode 
                0,		// NO sharing?
                NULL,								  // default security attributes 
                OPEN_EXISTING,					  // disposition 
                0,								  // file attributes 
                NULL);							  // do not copy file attributes 
            if (hDevice == INVALID_HANDLE_VALUE)    // cannot open the drive
            {
                DBMSG("\tError: " << path << " (" << nv2::s_error(::GetLastError()) << ")");
            }
            else {
                DBMSG("\tSuccess: " << path);
            }
            //
            uw32::Handle wh(hDevice);
        }
        //
        DWORD length = 0;
        wchar_t wszDosDrive[MAX_PATH + 1] = { 0 };
        BOOL ok = GetVolumePathNamesForVolumeNameW(key.wstr().c_str(), wszDosDrive, MAX_PATH, &length);
        if (ok)
        {
            //
            int count = 0;
            for (wchar_t* p = wszDosDrive; p[0] != L'\0'; p += wcslen(p) + 1)
            {
                v.push_back(p);
                count++;
            }
        }
        return v;
    }

    //-----------------------------------------------------------------------------
    // given a partition descriptor, get any mapped DOS drive names.
    static 
    std::vector<std::wstring> 
        getDOSNamesFromPartitionInfo(wde2::PartitionInfo& arg)
    {
        std::vector<std::wstring> v;
        PARTITION_INFORMATION_EX piex = arg.piex;
        GUID guidVolume = { 0 };
        if (piex.PartitionStyle == PARTITION_STYLE_MBR) {
            // the critical link ...
            guidVolume = piex.Mbr.PartitionId;
        }
        else if (piex.PartitionStyle == PARTITION_STYLE_GPT) {
            // see [1]
            // std::map<std::string, string_t, pm::GUIDComparer>::iterator it = pm::mapper.find(partInfoEx.Gpt.PartitionId);
            guidVolume = piex.Gpt.PartitionId;
        }
        v = getDOSNamesFromVolumeGUID(guidVolume);
        return v;
    }

    //-----------------------------------------------------------------------------
    static 
        std::map<int, wde2::DiskInfo> BuildDeviceList(void)
    {
        TRACE("BuildDeviceList");

        std::map<int, wde2::DiskInfo> vdi;

        //
        // Get the handle to the device information set for installed
        // disk class devices. Returns only devices that are currently
        // present in the system and have an enabled disk device
        // interface.
        //
        GUID diskClassDeviceInterfaceGuid = GUID_DEVINTERFACE_DISK;
        HDEVINFO diskClassDevices = SetupDiGetClassDevs(&diskClassDeviceInterfaceGuid,
            NULL,
            NULL,
            DIGCF_PRESENT |
            DIGCF_DEVICEINTERFACE);

        SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
        ZeroMemory(&deviceInterfaceData, sizeof(SP_DEVICE_INTERFACE_DATA));
        deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

        SP_DEVINFO_DATA deviceInfoData;
        ZeroMemory(&deviceInfoData, sizeof(SP_DEVINFO_DATA));
        deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

        DWORD deviceIndex = 0;
        while (SetupDiEnumDeviceInterfaces(diskClassDevices,
            NULL,
            &diskClassDeviceInterfaceGuid,
            deviceIndex,
            &deviceInterfaceData))
        {

            ++deviceIndex;

            DWORD requiredSize = 0;
            BOOL ok = SetupDiGetDeviceInterfaceDetail(diskClassDevices,
                &deviceInterfaceData,
                NULL,
                0,
                &requiredSize,
                NULL);

            // variable size structure. store in zero'd vector
            std::vector<BYTE> buffer(requiredSize,0);
            SP_DEVICE_INTERFACE_DETAIL_DATA* deviceInterfaceDetailData = 
                (SP_DEVICE_INTERFACE_DETAIL_DATA*) &buffer[0];
            ZeroMemory(deviceInterfaceDetailData, requiredSize);
            deviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

            ok = SetupDiGetDeviceInterfaceDetail(diskClassDevices,
                &deviceInterfaceData,
                deviceInterfaceDetailData,
                requiredSize,
                &requiredSize,
                &deviceInfoData);

            DBMSG2("deviceInterfaceDetailData->DevicePath: " << deviceInterfaceDetailData->DevicePath << " " << deviceIndex);

            //
            HANDLE hDevice = CreateFile(deviceInterfaceDetailData->DevicePath,
                GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                OPEN_EXISTING,
                0,
                NULL);

            DWORD bytesReturned = 0;
            STORAGE_DEVICE_NUMBER StorageDeviceNumber{ 0 };
            ok = DeviceIoControl(hDevice,
                IOCTL_STORAGE_GET_DEVICE_NUMBER,
                NULL,
                0,
                &StorageDeviceNumber,
                sizeof(STORAGE_DEVICE_NUMBER),
                &bytesReturned,
                NULL);
            //
            nv2::throw_if(!ok,nv2::acc("DeviceIoControl:") << nv2::s_error(::GetLastError()));
            //
            //if (StorageDeviceNumber.DeviceNumber == DISK_NUMBER)
            if (ok)
            {
                DBMSG2("-----Disk------");

                nv2::acc ac;
                DiskInfo diskInfo;
                diskInfo.DevicePath = deviceInterfaceDetailData->DevicePath;
                diskInfo.StorageDeviceNumber = StorageDeviceNumber;

                    
                LPOVERLAPPED lpov = nullptr;
                // If we are here it means everything was ok with this drive => get info
                // DBMSG2("diskNumber.DeviceNumber: " << diskNumber.DeviceNumber);
                DBMSG2("Drive " << StorageDeviceNumber.DeviceNumber << ":" << deviceIndex);
                DBMSG2("DevicePath: " << deviceInterfaceDetailData->DevicePath);
                // di.u32DeviceNumber = diskNumber.DeviceNumber;
                // strcpy_s(rawDevEntry->DiskInfo.szDevicePath, MAX_PATH, deviceInterfaceDetailData->DevicePath);
                // strcpy_s(rawDevEntry->DiskInfo.szShortDevicePath, MAX_PATH, deviceInterfaceDetailData->DevicePath);
                if (StorageDeviceNumber.DeviceType == FILE_DEVICE_DISK) {
                    // snprintf(rawDevEntry->DiskInfo.szShortDevicePath, MAX_PATH, "\\\\.\\PhysicalDrive%lu", diskNumber.DeviceNumber);
                    ac = _W("\\\\.\\PhysicalDrive");
                    ac << StorageDeviceNumber.DeviceNumber;
                    diskInfo.DeviceName = ac.wstr();
                    DBMSG2("DeviceName: \\\\.\\PhysicalDrive" << StorageDeviceNumber.DeviceNumber);
                }
                //
                // di.canBePartitioned = (diskNumber.PartitionNumber == 0);
                //DBMSG2("canBePartitioned: " << (diskNumber.PartitionNumber == 0));


                STORAGE_PROPERTY_QUERY storagePropertyQuery;
                ZeroMemory(&storagePropertyQuery, sizeof(STORAGE_PROPERTY_QUERY));
                storagePropertyQuery.PropertyId = StorageDeviceProperty;
                storagePropertyQuery.QueryType = PropertyStandardQuery;

                char propQueryOut[_8KB] = { 0 };
                ok = DeviceIoControl(hDevice, 
                                    IOCTL_STORAGE_QUERY_PROPERTY,
                                    &storagePropertyQuery, sizeof(storagePropertyQuery),
                                    &propQueryOut, sizeof(propQueryOut), 
                                    &bytesReturned, 
                                    lpov);

                STORAGE_DEVICE_DESCRIPTOR* pDevDesc = (STORAGE_DEVICE_DESCRIPTOR*)&propQueryOut[0];
                if (pDevDesc)
                {   
                    DBMSG2("pDevDesc->RemovableMedia: " << (bool)pDevDesc->RemovableMedia);

                    // Vendor ID string
                    const char* p = &propQueryOut[pDevDesc->VendorIdOffset];
                    if (p && pDevDesc->VendorIdOffset)
                    {
                        // strcpy_s(rawDevEntry->DiskInfo.szVendorId, MAX_PATH, p);
                        DBMSG2("VendorId: " << p);
                        diskInfo.VendorId = nv2::n2w(p);
                    }
                    p = &propQueryOut[pDevDesc->ProductIdOffset];
                    if (p && pDevDesc->ProductIdOffset)
                    {
                        // strcpy_s(rawDevEntry->DiskInfo.szModelNumber, MAX_PATH, p);
                        DBMSG2("ProductId: " << p);
                        diskInfo.ProductId = nv2::n2w(p);
                    }
                    p = &propQueryOut[pDevDesc->ProductRevisionOffset];
                    if (p && pDevDesc->ProductRevisionOffset)
                    {
                        // strcpy_s(rawDevEntry->DiskInfo.szProductRevision, MAX_PATH, p);
                        DBMSG2("ProductRevision: " << p);
                        diskInfo.ProductRevision = nv2::n2w(p);
                    }
                    p = &propQueryOut[pDevDesc->SerialNumberOffset];
                    if (p && pDevDesc->SerialNumberOffset)
                    {
                        // strcpy_s(rawDevEntry->DiskInfo.szSerialNumber, MAX_PATH, p);
                        DBMSG2("SerialNumber: " << p);
                        diskInfo.SerialNumber = nv2::n2w(p);
                    }
                }

                char propQueryOut2[sizeof(DISK_GEOMETRY_EX)];
                ok = DeviceIoControl(hDevice, 
                                    IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,
                                    NULL, 0,
                                    &propQueryOut2, sizeof(DISK_GEOMETRY_EX), 
                                    &bytesReturned, 
                                    lpov);

                DISK_GEOMETRY_EX* geom = (PDISK_GEOMETRY_EX)&propQueryOut2[0];
                DBMSG2("geom->Geometry.BytesPerSector: " << geom->Geometry.BytesPerSector);
                diskInfo.Geometry = geom->Geometry;
                diskInfo.DiskSize = geom->DiskSize;

                // memcpy(&(rawDevEntry->DiskInfo.diskGeometry), geom, sizeof(DISK_GEOMETRY_EX));
                // DWORD BytesPerSector = rawDevEntry->DiskInfo.diskGeometry.Geometry.BytesPerSector;

                char propQueryOut3[sizeof(DRIVE_LAYOUT_INFORMATION_EX) + (128 - 1) * sizeof(PARTITION_INFORMATION_EX)];
                ok = DeviceIoControl(hDevice, 
                                    IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
                                    NULL, 0, 
                                    &propQueryOut3, sizeof(propQueryOut3), 
                                    &bytesReturned, 
                                    lpov);

                DRIVE_LAYOUT_INFORMATION_EX* pDriveLayout = (DRIVE_LAYOUT_INFORMATION_EX*)&propQueryOut3[0];
                diskInfo.DriveLayout = *pDriveLayout;

                if (pDriveLayout->PartitionStyle == PARTITION_STYLE_MBR)
                {
                    DBMSG2("Mbr.CheckSum: " << nv2::to_hex(pDriveLayout->Mbr.CheckSum));
                    DBMSG2("Mbr.Signature (Disk ID): " << nv2::to_hex(pDriveLayout->Mbr.Signature));
                }
                else if (pDriveLayout->PartitionStyle == PARTITION_STYLE_GPT)
                {
                    DBMSG2("Gpt.DiskId: " << pDriveLayout->Gpt.DiskId);
                }

                //
                DWORD maxPart = min(pDriveLayout->PartitionCount, 128);
                DBMSG2("Disk has " << maxPart << " partitions");
                if (1)
                {
                    for (DWORD iPart = 0; iPart < maxPart; iPart++)
                    {
                        PARTITION_INFORMATION_EX piex = pDriveLayout->PartitionEntry[iPart];
                        if (piex.PartitionLength.QuadPart > 0)
                        {
                            wde2::PartitionInfo partitionInfo;
                            partitionInfo.piex = piex;
                            DBMSG2("------");
                            DBMSG2("\tpartInfoEx.PartitionNumber: " << piex.PartitionNumber);
                            DBMSG2("\tpartInfoEx.PartitionStyle: " << pps[piex.PartitionStyle]);
                            //DBMSG2("\tpartInfoEx.StartingOffset: " << partInfoEx.StartingOffset.QuadPart);
                            //DBMSG2("\tpartInfoEx.PartitionLength: " << partInfoEx.PartitionLength.QuadPart);
                            //DBMSG2("\tpartInfoEx.RewritePartition: " << partInfoEx.RewritePartition);
                            GUID guidVolume = { 0 };
                            if (pDriveLayout->PartitionStyle == PARTITION_STYLE_MBR) {
                                // the critical link ...
                                guidVolume = piex.Mbr.PartitionId;
                                DBMSG2("\tpartInfoEx.Mbr.PartitionId: " << piex.Mbr.PartitionId);
                                DBMSG2("\tpartInfoEx.Mbr.BootIndicator: " << piex.Mbr.BootIndicator);
                                DBMSG2("\tpartInfoEx.Mbr.PartitionType: " << piex.Mbr.PartitionType);
                                DBMSG2("\tpartInfoEx.Mbr.PartitionType: " << partitionIDToString(piex.Mbr.PartitionType));
                                DBMSG2("\tpartInfoEx.Mbr.RecognizedPartition: " << piex.Mbr.RecognizedPartition);
                                DBMSG2("\tpartInfoEx.Mbr.HiddenSectors: " << piex.Mbr.HiddenSectors);
                            }
                            else if (pDriveLayout->PartitionStyle == PARTITION_STYLE_GPT) {
                                // see [1]
                                // std::map<std::string, string_t, pm::GUIDComparer>::iterator it = pm::mapper.find(partInfoEx.Gpt.PartitionId);
                                guidVolume = piex.Gpt.PartitionId;
                                DBMSG2("\tpartInfoEx.Gpt.PartitionId: " << piex.Gpt.PartitionId);

                                DBMSG2("\tpartInfoEx.Gpt.PartitionType: " << piex.Gpt.PartitionType);
                                DBMSG2("\tpartInfoEx.Gpt.PartitionType: " << w32::GUIDToPartitionTypeString(piex.Gpt.PartitionType));
                                DBMSG2("\tpartInfoEx.Gpt.Attributes: " << piex.Gpt.Attributes);
                                DBMSG2("\tpartInfoEx.Gpt.Name: " << piex.Gpt.Name);

                                GUID guidPartitionId{ 0 };
                                // BOOL bg = GUIDFromString(_T("{ebd0a0a2-b9e5-4433-87c0-68b6b72699c7}"),&guidPartitionId);
                                //
                            }

                            DBMSG2("\tpartInfoEx.RewritePartition: " << piex.RewritePartition);
                            DBMSG2("\tpartInfoEx.PartitionLength: " << piex.PartitionLength.QuadPart);
                            DBMSG2("\tpartInfoEx.PartitionLength (MB): " << piex.PartitionLength.QuadPart / _1MB);
                            DBMSG2("\tpartInfoEx.PartitionLength (GB): " << piex.PartitionLength.QuadPart / _1GB);
                            DBMSG2("\tpartInfoEx.StartingOffset: " << piex.StartingOffset.QuadPart);
                            DBMSG2("\tpartInfoEx.EndingOffset: " << piex.StartingOffset.QuadPart + piex.PartitionLength.QuadPart);

                            //
                            nv2::acc key;
                            // essential! must have  trailing slash
                            key << _T("\\\\?\\Volume") << guidVolume << _T("\\");
                            partitionInfo.volumeID = key.wstr();
                            std::vector<std::wstring> names = getDOSNamesFromVolumeGUID(guidVolume);

                            diskInfo.partitions[iPart] = partitionInfo;
                        }
                    }
                }

                // should have a verbose mode.
                DBMSG2("diskInfo.StorageDeviceNumber.DeviceNumber " << diskInfo.StorageDeviceNumber.DeviceNumber << " => " << diskInfo.DevicePath << " (" << deviceIndex << ")");
                vdi[diskInfo.StorageDeviceNumber.DeviceNumber] = diskInfo;
            }
            //
            CloseHandle(hDevice);
            hDevice = INVALID_HANDLE_VALUE;
        }

        if (INVALID_HANDLE_VALUE != diskClassDevices) 
        {
            SetupDiDestroyDeviceInfoList(diskClassDevices);
        }
        //
        return vdi;
    }

	std::map<int,wde2::DiskInfo> enumerate()
	{
        std::map<int, wde2::DiskInfo> vdi = BuildDeviceList();
		return vdi;	
	}
}   