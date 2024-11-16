# wde2: tooling for Windows disk exploration

Low level drive enumeration for bootable VHD clone creation, disk signature rewriting, VSS snapshots and scripting to add VHD boot entries. Can clone a Windows system to bootable VHD. Use with caution.

*Nearly* an open-source alternative to Disk2VHD (https://learn.microsoft.com/en-us/sysinternals/downloads/disk2vhd).

Basic usage options are:

```
>wde2 -?

        wde2: Explorer/Cloner for Windows disks/partitions

        -?: Display help text (true)
        --help: Display help text (true)
        -c: Display count of disks only (false)
        -p: Display Partition data (false)
        -t: Display Terse partition data (false)
        -v: Display Verbose partition data (false)
        -s: Display partition signature (Implies Terse) (false)
        -d: Display DOS name mappings (Implies Terse) (false)
        -i: Display disks matching Index by range or individually (1, 0-2 or 0,3,4) ()
        -cv: Clone a disk to VHD: 'diskNumber' '/path/to/file.vhd' (false)
        -av: Attach VHD: '/path/to/file.vhd' (false)
        -dv: Detach VHD: '/path/to/file.vhd' (false)
        -ms: Modify MBR signature: 'diskNumber' 'signature' (false)
        -cs: Check MBR signature for collisions/duplicates (false)        

```

#### Enumerate and capture drive data ####

Display the data for drive 4 excluding partition information

```
>wde2 -i 4
    Detected 9 disks
    R:\src\win32\disk\wde2\main.cpp(338): ----------------- #4
    R:\src\win32\disk\wde2\main.cpp(339): DeviceName: \\.\PhysicalDrive4
    R:\src\win32\disk\wde2\main.cpp(341): ProductId: Samsung SSD 870 QVO 4TB
    R:\src\win32\disk\wde2\main.cpp(346): DiskSize: 3726GB (3815447MB)
    R:\src\win32\disk\wde2\main.cpp(381): DevicePath: \\?\scsi#disk&ven_samsung&prod_ssd_870_qvo_4tb#4&268c595a&0&040000#{53f56307-b6bf-11d0-94f2-00a0c91efb8b}
    R:\src\win32\disk\wde2\main.cpp(382): VendorId:
    R:\src\win32\disk\wde2\main.cpp(383): SerialNumber: S5STNF0W200695B
    R:\src\win32\disk\wde2\main.cpp(384): ProductRevision: SVQ02B6Q
    R:\src\win32\disk\wde2\main.cpp(385): BytesPerSector: 512
    R:\src\win32\disk\wde2\main.cpp(396): Gpt.DiskId: {C8D15F5D-8396-4FEC-B60C-777074654498}

```

Display the data for drive 4 including partition information

```
wde2 -i 4 -p
Detected 9 disks
R:\src\win32\disk\wde2\main.cpp(338): ----------------- #4
R:\src\win32\disk\wde2\main.cpp(339): DeviceName: \\.\PhysicalDrive4
R:\src\win32\disk\wde2\main.cpp(341): ProductId: Samsung SSD 870 QVO 4TB
R:\src\win32\disk\wde2\main.cpp(346): DiskSize: 3726GB (3815447MB)
R:\src\win32\disk\wde2\main.cpp(381): DevicePath: \\?\scsi#disk&ven_samsung&prod_ssd_870_qvo_4tb#4&268c595a&0&040000#{53f56307-b6bf-11d0-94f2-00a0c91efb8b}
R:\src\win32\disk\wde2\main.cpp(382): VendorId:
R:\src\win32\disk\wde2\main.cpp(383): SerialNumber: S5STNF0W200695B
R:\src\win32\disk\wde2\main.cpp(384): ProductRevision: SVQ02B6Q
R:\src\win32\disk\wde2\main.cpp(385): BytesPerSector: 512
R:\src\win32\disk\wde2\main.cpp(396): Gpt.DiskId: {C8D15F5D-8396-4FEC-B60C-777074654498}
R:\src\win32\disk\wde2\main.cpp(436):   ----
R:\src\win32\disk\wde2\main.cpp(437):   PartitionNumber: 1 (0)
R:\src\win32\disk\wde2\main.cpp(444):   No DOS device name assigned
R:\src\win32\disk\wde2\main.cpp(446):   PartitionStyle: PARTITION_STYLE_GPT
R:\src\win32\disk\wde2\main.cpp(447):   PartitionType: PARTITION_MSFT_RESERVED_GUID
R:\src\win32\disk\wde2\main.cpp(450):   PartitionLength: 15MB 0GB
R:\src\win32\disk\wde2\main.cpp(436):   ----
R:\src\win32\disk\wde2\main.cpp(437):   PartitionNumber: 2 (1)
R:\src\win32\disk\wde2\main.cpp(441):   DOS device: U:\
R:\src\win32\disk\wde2\main.cpp(446):   PartitionStyle: PARTITION_STYLE_GPT
R:\src\win32\disk\wde2\main.cpp(447):   PartitionType: PARTITION_BASIC_DATA_GUID
R:\src\win32\disk\wde2\main.cpp(450):   PartitionLength: 3815430MB 3726GB

```



#### Clone a physical disk to VHD file ####

Assuming boot disk is physicaldrive0, then the command below will create a bootable image stored in the VHD file

```
wde2 -cv 0 u:\test\boot0.vhd
```

Prepare for boot disk signature modification:

[1] Attach VHD.
```
wde2 -av u:\test\boot0.vhd
```

[2] Check for MBR signature collisions

```
>wde2 -cs
Checking for MBR drive signature collisions
        \\.\PhysicalDrive0 => 0x0005409A
        \\.\PhysicalDrive1 => 0x00BC614E
        \\.\PhysicalDrive3 => 0x27147B89
        \\.\PhysicalDrive5 => 0x5FF724D7
        \\.\PhysicalDrive6 => 0x3740C893
        \\.\PhysicalDrive7 => 0xDB3B9F56
        \\.\PhysicalDrive9 => 0x0005409A
        MBR signature collision: \\.\PhysicalDrive0 and \\.\PhysicalDrive9 => 0x0005409A
```

[3] Modify signature

```
wde2 -ms N <signature>
```
e.g.

```
wde2 -ms 9 0x0005409B
```

[4] Detach VHD.

```
wde2 -dv u:\test\boot0.vhd
```

Use the `bcd_add.cmd` script to add the VHD file as a bootable drive.

```
bcd_add.cmd <VHD file path> <BCD entry name>
```
is therefore

```
bcd_add.cmd u:\test\boot0.vhd "Cloned boot0"
```

