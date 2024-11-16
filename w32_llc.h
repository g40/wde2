/*
	[1] Discovery:
	open source disk as physical drive
	get partition info
	select which partitions to copy - need ID string
	i.e VolID: \\?\Volume{682FA6CA-76B7-497E-9B64-7F495F731E5D}\
	establish total size - and layout

	[2] Disk  Creation
	create VHD with correct size and format (MBR/GPT)
	mount VHD so we have physical disk. 
	Needs to be initialized hence IOCTL CREATE_DISK
	IOCTL CREATE_DISK_LAYOUT
	format volume?
	for each source partition
		create VSS snapshot and mount temporary DOS device
		copy from TDD to destination partition on VHD

	https://learn.microsoft.com/en-us/windows/win32/fileio/defining-an-ms-dos-device-name
*/

#pragma once

#include <vss.h>
#include <vswriter.h>
#include <vsbackup.h>


namespace dc
{


}

