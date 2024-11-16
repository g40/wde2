/*

    Console driver for Windows disk/partition enumeration
    and editing.

    Use with caution.

    Visit https://github.com/g40

    Copyright (c) Jerry Evans, 1999-2024

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

#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <string.h>
#include <tchar.h>

#include <set>

//-----------------------------------------------------------------------------
#define _USE_CONSOLE

#include <g40/nv2_opt.h>
#include <g40/nv2_w32.h>

#include "wde2.h"
#include "vhd_ex.h"
#include "w32_sig.h"
#include "w32_vss.h"

#pragma comment( lib, "setupapi.lib" )


/*

https://learn.microsoft.com/en-us/windows/win32/api/virtdisk/nf-virtdisk-getvirtualdiskoperationprogress

DWORD GetVirtualDiskOperationProgress(
  [in]  HANDLE                 VirtualDiskHandle,
  [in]  LPOVERLAPPED           Overlapped,
  [out] PVIRTUAL_DISK_PROGRESS Progress
);

*/

//-----------------------------------------------------------------------------
#ifdef _UNICODE
int _tmain(int argc, char_t* argv[])
#else
int main(int argc, char* argv[])
#endif
{
    int ret = -1;
    try
    {
        //
        {
            uw32::ManualHandle handle;
            OVERLAPPED ov{0};
            ov.hEvent = handle.handle();
        }

        //
        nv2::throw_if(!uw32::IsProcessElevated(),
                    nv2::acc("This application requires administrative privileges. Please run as Administrator."));

        //---------------------------------------------------------------------
        // see Opt help strings for details
        bool count = false;
        bool help = false;
        bool terse = false;
        bool verbose = false;
        bool signature = false;
        bool dos_name = false;
        bool list_partitions = false;
        //
        string_t disk_index = _T("");
        string_t partition_range = _T("");
        bool vhd_create = false;
        bool vhd_attach = false;
        bool vhd_detach = false;
        bool shadow_copy = false;
        std::wstring vhdName;
        bool  modifyMBRSignature = false;
        bool  checkMBRSignature = false;
        bool test_volume_access = false;
        // map options to default values
        std::vector<nv2::ap::Opt> opts = 
        {
            { _T("-?"), help, _T("Display help text") },
            { _T("--help"), help, _T("Display help text") },
            { _T("-c"), count, _T("Display count of disks only") },
            { _T("-p"), list_partitions, _T("Display Partition data") },
            { _T("-t"), terse, _T("Display Terse partition data") },
            { _T("-v"), verbose, _T("Display Verbose partition data") },
            { _T("-s"), signature, _T("Display partition signature (Implies Terse)") },
            { _T("-d"), dos_name, _T("Display DOS name mappings (Implies Terse)") },
            { _T("-i"), disk_index, _T("Display disks matching Index by range or individually (1, 0-2 or 0,3,4)") },

            //{ _T("-pr"), partition_range, _T("List partition range") },
            { _T("-cv"), vhd_create, _T("Clone a disk to VHD: 'diskNumber' '/path/to/file.vhd'") },
            { _T("-av"), vhd_attach, _T("Attach VHD: '/path/to/file.vhd'") },
            { _T("-dv"), vhd_detach, _T("Detach VHD: '/path/to/file.vhd'") },
            { _T("-ms"), modifyMBRSignature, _T("Modify MBR signature: 'diskNumber' 'signature'") },
            { _T("-cs"), checkMBRSignature, _T("Check MBR signature for collisions/duplicates") },

            // disable these experimental, PoC, options
            // create a shadow copy from 'volume', allow access via 'Destination DOS name'.
            // 
            // { _T("-x-sc"), shadow_copy, _T("(Experimental: Shadow Copy: 'volume' 'Destination DOS name'") },
            // testing. check path naming is correct and volume can be opened
            // { _T("-x-tva"), test_volume_access, _T("test_volume_access") },
        };

        // parse the command line. returns any positionals in vp
        std::vector<string_t> vp = nv2::ap::parse(argc, (const char_t**) argv, opts);

        if (help) {
            string_t s = nv2::ap::to_string(opts, _T("wde2: Explorer/Cloner for Windows disks/partitions"));
            std::wcout << nv2::t2w(s);
            return 0;
        }

        // e.g. -sc g:\ u:\test\copied -d 6 -p
        if (shadow_copy)
        {
            if (vp.size() != 2)
                throw std::runtime_error("Shadow Copy: expecting {volume} {Destination DOS name}");
            vss::VSSWrapper vssw;
            vssw.doSnapshotCopy(vp[0],vp[1]);
        }
        // -cv
        else if (vhd_create)
        {
            if (vp.size() != 2)
                throw std::runtime_error("Expecting drivenumber and path/to/VHD");
            DWORD dwError = 0;
            if (!vhdc::CloneVHDFromDisk(vp[0].c_str(),vp[1].c_str(),&dwError)) {
                throw dwError;
            }
        }
        // -ca
        else if (vhd_attach)
        {
            if (vp.size() != 1)
                throw std::runtime_error("Expecting path/to/VHD");
            DWORD dwError = 0;
            if (!vhdc::VHDAttach(vp[0].c_str(), &dwError)) {
                throw dwError;
            }
            std::wcout << "Attached " << vp[0] << std::endl;
        }
        // -cd
        else if (vhd_detach)
        {
            if (vp.size() != 1)
                throw std::runtime_error("Expecting VHD location");
            DWORD dwError = 0;
            if (!vhdc::VHDDetach(vp[0].c_str(), &dwError)) {
                throw dwError;
            }
            std::wcout << "Detached " << vp[0] << std::endl;
        }
        else if (modifyMBRSignature) {
            if (vp.size() != 2)
                throw std::runtime_error("Expecting disk number and signature");
            //
            int diskNumber = wde2::xstoi(vp[0]);
            DWORD newSignature = wde2::xstoi(vp[1]);
            std::cout << "Updating signature for " << diskNumber << " (" << newSignature << ")" << std::endl;
            ret = wde2::UpdateMBRSignature(diskNumber, newSignature);
        }
        //
        else if (checkMBRSignature) 
        {
            std::cout << "Checking for MBR drive signature collisions" << std::endl;
            // map signature to drive index
            using map_t = std::map<DWORD,int>;
            using iter_t = map_t::iterator;
            map_t mapper;
            // drive index => disk info
            std::map<int, wde2::DiskInfo> vdi = wde2::enumerate();
            for (auto& di : vdi)
            {
                // applies to MBR only
                if (di.second.DriveLayout.PartitionStyle == PARTITION_STYLE_MBR) 
                {
                    std::cout << "\t\\\\.\\PhysicalDrive" << di.first << " => " << nv2::to_hex(di.second.DriveLayout.Mbr.Signature) << std::endl;

                    iter_t iter = mapper.find(di.second.DriveLayout.Mbr.Signature);
                    if (iter == mapper.end()) {
                        mapper[di.second.DriveLayout.Mbr.Signature] = di.first;
                    }
                    else {
                        std::cout << "\tMBR signature collision: "
                                    << "\\\\.\\PhysicalDrive" << iter->second
                                    << " and "
                                    << "\\\\.\\PhysicalDrive" << di.first
                                    << " => "
                                    << nv2::to_hex(iter->first)
                                    << std::endl;
                    }
                }
            }
        }
        else
        {
            //
            if (signature || dos_name) {
                terse = true;
            }
            if (count)
            {
                verbose = false;
                list_partitions = false;
            }
            //
            if (verbose || test_volume_access)
            {
                //
                list_partitions = true;
            }
            //
            std::map<int, wde2::DiskInfo> vdi = wde2::enumerate();
            //
            int diskCount = (int)vdi.size();
            //
            printf("Detected %d disks\n", diskCount);
            //
            if (diskCount == 0) {
                throw std::runtime_error("Unlikely! Zero (0) disks detected");
            }
            //
            std::set<int> disks;
            // range
            std::vector<string_t> dr;
            //
            if (disk_index.size() && disk_index.find(_T("-")) != std::string::npos)
            {
                //
                dr = nv2::split(disk_index, _T("-"));
                if (dr.size() == 1)
                {
                    // start
                    int rs = std::stoi(dr[0]);
                    rs = (std::max)(0, rs);
                    rs = (std::min)(rs, diskCount);
                    for (int i = rs; i < diskCount; i++)
                        disks.insert(i);
                }
                else if (dr.size() == 2)
                {
                    // start
                    int rs = std::stoi(dr[0]);
                    int re = std::stoi(dr[1]);
                    // do reverse checks
                    int ts = (std::min)(rs, re);
                    int te = (std::max)(rs, re);
                    rs = ts;
                    re = te;
                    // do min/max checks
                    rs = (std::max)(0, rs);
                    re = (std::min)(re, diskCount);
                    for (int i = rs; i <= re; i++)
                        disks.insert(i);
                }
                else
                {
                    throw std::runtime_error("Invalid range???");
                }
            }
            else if (disk_index.size() && disk_index.find(_T(",")) != std::string::npos)
            {
                dr = nv2::split(disk_index, _T(","));
                if (dr.empty())
                {
                    throw std::runtime_error("Must specify a valid set of disks (0,1,2)");
                }

                // start
                for (auto& ndx : dr)
                {
                    disks.insert(std::stoi(ndx));
                }
            }
            else if (disk_index.size())
            {
                //
                dr = nv2::split(disk_index, _T(",- "));
                // start
                for (auto& ndx : dr)
                {
                    disks.insert(std::stoi(ndx));
                }
            }
            else
            {
                // do all disks
                for (int i = 0; i <= diskCount; i++)
                    disks.insert(i);
            }

            // improved enumeration
            for (auto& id : vdi)
            {
                if (disks.find(id.first) == disks.end())
                    continue;
                
                wde2::DiskInfo di = id.second;

                DBMSG("----------------- #" << di.StorageDeviceNumber.DeviceNumber);
                DBMSG("DeviceName: \\\\.\\PhysicalDrive" << di.StorageDeviceNumber.DeviceNumber);
                //
                DBMSG("ProductId: " << di.ProductId);

                DBMSG("DiskSize: "
                    << (di.DiskSize.QuadPart / uw32::_1GB) << "GB "
                    << "(" << (di.DiskSize.QuadPart / uw32::_1MB) << "MB)"
                    );

                if (terse)
                {
                    if (signature)
                    {
                        if (di.DriveLayout.PartitionStyle == PARTITION_STYLE_MBR)
                        {
                            DBMSG("Mbr.Signature (Disk ID): "
                                << nv2::to_hex(di.DriveLayout.Mbr.Signature)
                                << "(" << di.DriveLayout.Mbr.Signature << ")");
                        }
                    }
                    // get any DOS names mapped to this disk
                    if (dos_name) {
                        string_t dos_names = _T("");
                        for (auto& partition : di.partitions)
                        {
                            std::vector<std::wstring> vname = wde2::getDOSNamesFromPartitionInfo(partition.second);
                            for (auto name : vname)
                            {
                                dos_names += name;
                                dos_names += _T(" ");
                            }
                        }
                        //
                        if (dos_names.size()) {
                            DBMSG("DOS names: " << dos_names);
                        }
                            
                    }

                }
                else
                {
                    DBMSG("DevicePath: " << di.DevicePath);
                    DBMSG("VendorId: " << di.VendorId);
                    DBMSG("SerialNumber: " << di.SerialNumber);
                    DBMSG("ProductRevision: " << di.ProductRevision);
                    DBMSG("BytesPerSector: " << di.Geometry.BytesPerSector);

                    if (di.DriveLayout.PartitionStyle == PARTITION_STYLE_MBR)
                    {
                        DBMSG("Mbr.CheckSum: " << nv2::to_hex(di.DriveLayout.Mbr.CheckSum));
                        DBMSG("Mbr.Signature (Disk ID): "
                            << nv2::to_hex(di.DriveLayout.Mbr.Signature)
                            << "(" << di.DriveLayout.Mbr.Signature << ")");
                    }
                    else if (di.DriveLayout.PartitionStyle == PARTITION_STYLE_GPT)
                    {
                        DBMSG("Gpt.DiskId: " << di.DriveLayout.Gpt.DiskId);
                    }
                    //
                    if (di.partitions.empty()) {
                        DBMSG("Disk has no defined partitions.");
                    }
                    if (list_partitions)
                    {
                        // <int,PARTITION_INFORMATION_EX>
                        for (auto& partition : di.partitions)
                        {
                            PARTITION_INFORMATION_EX piex = partition.second.piex;
                            std::wstring PartitionType;
                            GUID guidVolume = { 0 };
                            if (di.DriveLayout.PartitionStyle == PARTITION_STYLE_MBR) {
                                // the critical link ...
                                guidVolume = piex.Mbr.PartitionId;
                                PartitionType = wde2::partitionIDToString(piex.Mbr.PartitionType);
                            }
                            else if (di.DriveLayout.PartitionStyle == PARTITION_STYLE_GPT)
                            {
                                guidVolume = piex.Gpt.PartitionId;
                                PartitionType = wde2::w32::GUIDToPartitionTypeString(piex.Gpt.PartitionType);
                            }

                            if (test_volume_access)
                            {
                                nv2::acc key;
                                // essential! must have trailing slash
                                key << _T("\\\\?\\Volume") << guidVolume; // << _T("\\");
                                DBMSG("\tVolume: " << key);
                                uw32::FileHandle fh(key.wstr());
                                if (fh) {
                                    DBMSG("\tPass: tva: " << key);
                                }
                                else {
                                    DBMSG("\tFAIL: tva: " << nv2::s_error(fh.GetErrorCode()));
                                }
                            }

                            DBMSG("\t----");
                            DBMSG("\tPartitionNumber: " << piex.PartitionNumber << " (" << partition.first << ")");
                            //
                            std::vector<std::wstring> names = wde2::getDOSNamesFromVolumeGUID(guidVolume);
                            if (names.size()) {
                                DBMSG("\tDOS device: " << names[0]);
                            }
                            else {
                                DBMSG("\tNo DOS device name assigned");
                            }
                            DBMSG("\tPartitionStyle: " << wde2::pps[piex.PartitionStyle]);
                            DBMSG("\tPartitionType: " << PartitionType);
                            DBMSG("\tPartitionLength: "
                                << (piex.PartitionLength.QuadPart / uw32::_1MB) << "MB "
                                << (piex.PartitionLength.QuadPart / uw32::_1GB) << "GB");

                            if (verbose)
                            {
                                DBMSG("\tpartInfoEx.StartingOffset: " << piex.StartingOffset.QuadPart);
                                DBMSG("\tpartInfoEx.PartitionLength: " << piex.PartitionLength.QuadPart);
                                DBMSG("\tpartInfoEx.RewritePartition: " << piex.RewritePartition);

                                if (di.DriveLayout.PartitionStyle == PARTITION_STYLE_MBR) {
                                    DBMSG("\tpartInfoEx.Mbr.PartitionId: " << piex.Mbr.PartitionId);
                                    DBMSG("\tpartInfoEx.Mbr.BootIndicator: " << piex.Mbr.BootIndicator);
                                    DBMSG("\tpartInfoEx.Mbr.PartitionType: " << piex.Mbr.PartitionType);
                                    DBMSG("\tpartInfoEx.Mbr.RecognizedPartition: " << piex.Mbr.RecognizedPartition);
                                    DBMSG("\tpartInfoEx.Mbr.HiddenSectors: " << piex.Mbr.HiddenSectors);
                                }
                                else if (di.DriveLayout.PartitionStyle == PARTITION_STYLE_GPT)
                                {
                                    DBMSG("\tpartInfoEx.Gpt.PartitionId: " << piex.Gpt.PartitionId);
                                    DBMSG("\tpartInfoEx.Gpt.PartitionType: " << piex.Gpt.PartitionType);
                                    DBMSG("\tpartInfoEx.Gpt.Attributes: " << piex.Gpt.Attributes);
                                    DBMSG("\tpartInfoEx.Gpt.Name: " << piex.Gpt.Name);
                                }
                                //
                                DBMSG("\tvolumeID: " << partition.second.volumeID);
                            }
                        }
                    }
                }
            }
        }
        //
        ret = 0;
    }
    catch (const std::exception& ex)
    {
        std::cout << "Error: " << ex.what() << std::endl;
    }
    catch (const DWORD& ex)
    {
        std::wcout << "Error: " << nv2::s_error(ex).c_str() << std::endl;
    }
    catch (...)
    {
        std::cout << "Unknown error ..." << std::endl;
    }
    //
    return ret;
}


