/*

	wde2 structures

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

namespace wde2
{

	// literally not used
#define _UNUSED 

	// partition
	struct PartitionInfo
	{
		// volume may be mapped to a DOS drive name 'g:'
		std::wstring volumeID;
		// raw partition information
		PARTITION_INFORMATION_EX piex;
	};

	// disk contains 0+ partitions
	struct DiskInfo
	{
		//
		STORAGE_DEVICE_NUMBER StorageDeviceNumber;
		//
		std::wstring DevicePath;
		// i.e. \\\\.\\PhysicalDrive0
		// see https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilew#physical-disks-and-volumes
		std::wstring DeviceName;
		//
		std::wstring SerialNumber;
		//
		std::wstring VendorId;
		std::wstring ProductId;
		std::wstring ProductRevision;
		// std::wstring szDevicePath;
		bool canBePartitioned{ false };
		//
		DISK_GEOMETRY Geometry; // Standard disk geometry: may be faked by driver.
		LARGE_INTEGER DiskSize; // Must always be correct
		DRIVE_LAYOUT_INFORMATION_EX DriveLayout;
		// partitions indexed by 1-relative key
		std::map<DWORD,PartitionInfo> partitions;
	};

}	// vde2

