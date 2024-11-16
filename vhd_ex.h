/*

    Core functions to access Win32 VHD capabilities

    Visit https://github.com/g40

    Copyright (c) Jerry Evans, 2011-2024

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

//
#include <initguid.h>
#include <virtdisk.h>
#include <rpc.h>
#include <sddl.h>

// autolink
#pragma comment( lib, "virtdisk.lib")
#pragma comment( lib, "rpcrt4.lib")

namespace vhdc
{

    //
    // CREATE_VIRTUAL_DISK_VERSION_2 allows specifying a richer set a values and returns
    // a V2 handle.
    //
    // VIRTUAL_DISK_ACCESS_NONE is the only acceptable access mask for V2 handle opens.
    //
    // Valid BlockSize values are as follows (use 0 to indicate default value):
    //      Fixed VHD: 0
    //      Dynamic VHD: 512kb, 2mb (default)
    //      Differencing VHD: 512kb, 2mb (if parent is fixed, default is 2mb; if parent is dynamic or differencing, default is parent blocksize)
    //      Fixed VHDX: 0
    //      Dynamic VHDX: 1mb, 2mb, 4mb, 8mb, 16mb, 32mb (default), 64mb, 128mb, 256mb
    //      Differencing VHDX: 1mb, 2mb (default), 4mb, 8mb, 16mb, 32mb, 64mb, 128mb, 256mb
    //
    // Valid LogicalSectorSize values are as follows (use 0 to indicate default value):
    //      VHD: 512 (default)
    //      VHDX: 512 (for fixed or dynamic, default is 512; for differencing, default is parent logicalsectorsize), 4096
    //
    // Valid PhysicalSectorSize values are as follows (use 0 to indicate default value):
    //      VHD: 512 (default)
    //      VHDX: 512, 4096 (for fixed or dynamic, default is 4096; for differencing, default is parent physicalsectorsize)
    //
    //-----------------------------------------------------------------------------
    bool
        CloneVHDFromDisk(LPCWSTR DiskNumber,    // L"\\\\.\\PhysicalDrive6"
                         LPCWSTR VHDPath,      // L"u:\\test\\disk6.vhd"
                          DWORD* pdwError = nullptr,
                          OVERLAPPED* pov = nullptr)  // handle must exist at start to monitor
    {
        GUID uniqueId{ 0 };
        if (RPC_S_OK != UuidCreate((UUID*)&uniqueId))
        {
            if (pdwError) {
                *pdwError = ::GetLastError();
            }
            //
            return false;
        }

        // Specify UNKNOWN for both device and vendor so the system will use the
        // file extension to determine the correct VHD format.
        VIRTUAL_STORAGE_TYPE storageType{ 0 };
        storageType.DeviceId = VIRTUAL_STORAGE_TYPE_DEVICE_UNKNOWN;
        storageType.VendorId = VIRTUAL_STORAGE_TYPE_VENDOR_UNKNOWN;

        CREATE_VIRTUAL_DISK_PARAMETERS parameters;
        memset(&parameters, 0, sizeof(parameters));
        parameters.Version = CREATE_VIRTUAL_DISK_VERSION_2;
        parameters.Version2.UniqueId = uniqueId;
        parameters.Version2.MaximumSize = 0;
        parameters.Version2.BlockSizeInBytes = CREATE_VIRTUAL_DISK_PARAMETERS_DEFAULT_BLOCK_SIZE;
        parameters.Version2.SectorSizeInBytes = CREATE_VIRTUAL_DISK_PARAMETERS_DEFAULT_SECTOR_SIZE;
        parameters.Version2.PhysicalSectorSizeInBytes = CREATE_VIRTUAL_DISK_PARAMETERS_DEFAULT_SECTOR_SIZE;
        parameters.Version2.ParentPath = NULL;
        //
        string_t physicaldisk = _T("\\\\.\\PhysicalDrive");     
        physicaldisk += DiskNumber;
        parameters.Version2.SourcePath = physicaldisk.c_str();
        //
        CREATE_VIRTUAL_DISK_FLAG Flags = CREATE_VIRTUAL_DISK_FLAG_FULL_PHYSICAL_ALLOCATION;
        //
        SECURITY_DESCRIPTOR* lpsd = nullptr;
        // 
        HANDLE vhdHandle = INVALID_HANDLE_VALUE;
        // slow if creating large disk
        DWORD opStatus = CreateVirtualDisk(
            &storageType,
            VHDPath,
            VIRTUAL_DISK_ACCESS_NONE,
            lpsd,
            Flags,
            0,
            &parameters,
            pov,            // must be shared too?
            &vhdHandle);
        //
        if (opStatus != ERROR_SUCCESS)
        {
            if (pdwError) {
                *pdwError = ::GetLastError();
            }
        }
        //
        if (vhdHandle != INVALID_HANDLE_VALUE)
        {
            CloseHandle(vhdHandle);
        }
        //
        return (opStatus == ERROR_SUCCESS);
    }

    //-----------------------------------------------------------------------------
    // L"u:\\test\\disk6.vhd"
    bool
        VHDAttach(LPCWSTR VHDPath,      
                    DWORD* pdwError = nullptr)
    {
        //
        PSECURITY_DESCRIPTOR sd = NULL;

        //
        VIRTUAL_STORAGE_TYPE storageType;
        storageType.DeviceId = VIRTUAL_STORAGE_TYPE_DEVICE_UNKNOWN;
        storageType.VendorId = VIRTUAL_STORAGE_TYPE_VENDOR_UNKNOWN;

        OPEN_VIRTUAL_DISK_PARAMETERS openParameters;
        memset(&openParameters, 0, sizeof(openParameters));
        openParameters.Version = OPEN_VIRTUAL_DISK_VERSION_2;
        openParameters.Version2.GetInfoOnly = FALSE;

        VIRTUAL_DISK_ACCESS_MASK accessMask = VIRTUAL_DISK_ACCESS_NONE;

        HANDLE vhdHandle = INVALID_HANDLE_VALUE;
        DWORD opStatus = OpenVirtualDisk(
            &storageType,
            VHDPath,
            accessMask,
            OPEN_VIRTUAL_DISK_FLAG_NONE,
            &openParameters,
            &vhdHandle);
        if (opStatus != ERROR_SUCCESS)
        {
            goto Cleanup;
        }

        if (!::ConvertStringSecurityDescriptorToSecurityDescriptor(
            L"O:BAG:BAD:(A;;GA;;;WD)",
            SDDL_REVISION_1,
            &sd,
            NULL))
        {
            opStatus = ERROR_ACCESS_DENIED;
            goto Cleanup;
        }

        //
        ATTACH_VIRTUAL_DISK_PARAMETERS attachParameters;
        memset(&attachParameters, 0, sizeof(attachParameters));
        attachParameters.Version = ATTACH_VIRTUAL_DISK_VERSION_1;

        // A "Permanent" surface persists even when the handle is closed.
        ATTACH_VIRTUAL_DISK_FLAG attachFlags = ATTACH_VIRTUAL_DISK_FLAG_PERMANENT_LIFETIME;

        opStatus = AttachVirtualDisk(
            vhdHandle,
            sd,
            attachFlags,
            0,
            &attachParameters,
            NULL);
        if (opStatus != ERROR_SUCCESS) {
            goto Cleanup;
        }
    
    Cleanup:
        //
        if (opStatus != ERROR_SUCCESS
            && pdwError) {
            *pdwError = ::GetLastError();
        }
        if (sd != NULL)
        {
            LocalFree(sd);
            sd = NULL;
        }
        if (vhdHandle != INVALID_HANDLE_VALUE)
        {
            CloseHandle(vhdHandle);
        }
        return (opStatus == ERROR_SUCCESS);
    }

    //-----------------------------------------------------------------------------
    bool
        VHDDetach(LPCWSTR VHDPath,      // L"u:\\test\\disk6.vhd"
            DWORD* pdwError = nullptr)
    {
        VIRTUAL_STORAGE_TYPE storageType;
        storageType.DeviceId = VIRTUAL_STORAGE_TYPE_DEVICE_UNKNOWN;
        storageType.VendorId = VIRTUAL_STORAGE_TYPE_VENDOR_UNKNOWN;

        OPEN_VIRTUAL_DISK_PARAMETERS openParameters;
        memset(&openParameters, 0, sizeof(openParameters));
        openParameters.Version = OPEN_VIRTUAL_DISK_VERSION_2;
        openParameters.Version2.GetInfoOnly = FALSE;

        VIRTUAL_DISK_ACCESS_MASK accessMask = VIRTUAL_DISK_ACCESS_NONE;

        HANDLE vhdHandle = INVALID_HANDLE_VALUE;
        DWORD opStatus = OpenVirtualDisk(
            &storageType,
            VHDPath,
            accessMask,
            OPEN_VIRTUAL_DISK_FLAG_NONE,
            &openParameters,
            &vhdHandle);
        if (opStatus != ERROR_SUCCESS) {
            goto Cleanup;
        }

        //
        // Detach the VHD/VHDX/ISO.
        //
        // DETACH_VIRTUAL_DISK_FLAG_NONE is the only flag currently supported for detach.
        //
        opStatus = DetachVirtualDisk(
            vhdHandle,
            DETACH_VIRTUAL_DISK_FLAG_NONE,
            0);

        if (opStatus != ERROR_SUCCESS) {
            goto Cleanup;
        }
    Cleanup:
        //
        if (opStatus != ERROR_SUCCESS
            && pdwError) {
            *pdwError = ::GetLastError();
        }
        //
        if (vhdHandle != INVALID_HANDLE_VALUE) {
            CloseHandle(vhdHandle);
        }
        return (opStatus == ERROR_SUCCESS);
    }
}
