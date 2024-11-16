/*

    Modify MBR disk signature via the Win32 API. 
    
    Use with *extreme* caution as colliding values 
    may render your MBR based system unbootable.

    Visit https://github.com/g40

    Copyright (c) Jerry Evans, 2023-2024

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

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include <g40/nv2_w32.h>

namespace wde2
{

    //-----------------------------------------------------------------------------
    //
    //
    int UpdateMBRSignature(int diskNumber,DWORD dwMBRSignature) 
    {
        nv2::acc diskName = L"\\\\.\\PhysicalDrive";
        diskName <<diskNumber;
        // Open a handle to the physical disk (e.g., PhysicalDrive0)
        HANDLE hDisk = CreateFile(diskName.wstr().c_str(),
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            0,
            NULL);

        if (hDisk == INVALID_HANDLE_VALUE) {
            printf("Failed to open disk %s. Error: %lu\n", diskName.c_str(), GetLastError());
            return 1;
        }

        // Step 1: Retrieve the current drive layout using IOCTL_DISK_GET_DRIVE_LAYOUT_EX
        BOOL result = FALSE;
        DWORD bytesReturned = 0;
    #if 0
        // not working as expected.
        // First call to get the required buffer size
        result = DeviceIoControl(hDisk,
            IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
            NULL,
            0,
            NULL,
            0,
            &bytesReturned,
            NULL);
        if (!result)
        {
            DWORD dwError = ::GetLastError();
            std::string s = nv2::s_error(dwError);
            DBMSG("Failed: " << s);
        }
    #endif
        // Start with minimal buffer size
        DWORD outBufferSize = sizeof(DRIVE_LAYOUT_INFORMATION_EX);
        // GPT can hold up to 128 partitions
        outBufferSize += (128*sizeof(PARTITION_INFORMATION_EX));
        //
        std::vector<byte> buffer(outBufferSize,0);
        //
        DRIVE_LAYOUT_INFORMATION_EX* driveLayoutEx = (DRIVE_LAYOUT_INFORMATION_EX*)&buffer[0];

        // First call to get the required buffer size
        result = DeviceIoControl(hDisk,
            IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
            NULL,
            0,
            driveLayoutEx,
            outBufferSize,
            &bytesReturned,
            NULL);

        if (!result) {
            // Some other error occurred
            printf("Failed to get drive layout. Error: %lu\n", GetLastError());
            CloseHandle(hDisk);
            return 1;
        }
        // bail if signature == 0
        if (dwMBRSignature == 0) {
            return -1;
        }
        // very unlikely. don't mess with boot disk
        if (diskNumber == 0) {
            return -2;
        }
        // Step 2: Modify only the MBR signature (assuming the disk is MBR-based)
        if (driveLayoutEx->PartitionStyle == PARTITION_STYLE_MBR) {
            printf("Original MBR signature: 0x%08lx\n", driveLayoutEx->Mbr.Signature);

            // Set a new MBR signature (example: 0x12345678)
            driveLayoutEx->Mbr.Signature = dwMBRSignature;
            printf("New MBR signature: 0x%08lx\n", driveLayoutEx->Mbr.Signature);
        }
        else {
            printf("This disk is not using MBR. Aborting.\n");
            CloseHandle(hDisk);
            return -3;
        }

        // Step 3: Apply the modified layout back using IOCTL_DISK_SET_DRIVE_LAYOUT_EX
        result = DeviceIoControl(hDisk,
            IOCTL_DISK_SET_DRIVE_LAYOUT_EX,
            driveLayoutEx,
            outBufferSize,
            NULL,
            0,
            &bytesReturned,
            NULL);

        if (result) {
            printf("Successfully updated MBR signature.\n");
        }
        else {
            printf("Failed to update MBR signature. Error: %lu\n", GetLastError());
        }

        // re-read to verify

        // Clean up
        CloseHandle(hDisk);
        // OK
        return 0;
    }
}
