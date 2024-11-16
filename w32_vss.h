/*

    C++ VSS wrapper for Windows

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

#include <vss.h>
#include <vswriter.h>
#include <vsbackup.h>
#include <atlstr.h>
// --std=c++17
#include <filesystem>

#pragma comment(lib, "vssapi.lib")

namespace vss
{
    //-------------------------------------------------------------------------
    //
    class ComInit 
    {
        bool  ok = false;
    public:
        ComInit() 
        {
            // initialize COM (must do before InitializeForBackup works)
            HRESULT result = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
            uw32::trace_hresult(LFL "ComInit", result);
            nv2::throw_if(result != S_OK,nv2::acc(LFL));
            ok = true;
        }
        ~ComInit() 
        {
            if (ok) {
                CoUninitialize();
                ok = false;
            }
        }
    };
	
    //-------------------------------------------------------------------------
    // assign a DOS drive letter to a volume path
    class DOSNameWrapper
    {
        // i.e. g:
        std::wstring m_dosDevice;

    public:

        // no trailing '\\'
        DOSNameWrapper(std::wstring dosDevice, const wchar_t* wszSource)
        {
            //
            BOOL ok = DefineDosDevice(DDD_RAW_TARGET_PATH, dosDevice.c_str(), wszSource);
            uw32::throw_on_fail(LFL "Error calling DefineDosDevice",ok == FALSE);
            m_dosDevice = dosDevice;
        }

        ~DOSNameWrapper()
        {
            if (!m_dosDevice.empty())
            {
                BOOL ok = DefineDosDevice(DDD_REMOVE_DEFINITION, m_dosDevice.c_str(), NULL);
                uw32::throw_on_fail(LFL "Error calling RemoveDosDevice", ok == FALSE);
                m_dosDevice.clear();
            }
        }
    };

    //-------------------------------------------------------------------------
    class VSSWrapper
	{
        //
        // https://learn.microsoft.com/en-us/windows/win32/api/virtdisk/nf-virtdisk-getvirtualdiskoperationprogress
        // 
        //  DWORD GetVirtualDiskOperationProgress(
        //    [in]  HANDLE                 VirtualDiskHandle,
        //    [in]  LPOVERLAPPED           Overlapped,
        //    [out] PVIRTUAL_DISK_PROGRESS Progress
        // );
       
        //---------------------------------------------------------------------
        // 
        void
            CheckStatusAndCancellation(CComPtr<IVssAsync>& pStatus, 
                    DWORD sleepMS = 250)
        {
            int counter = 0;
            HRESULT asyncResult = E_FAIL;
            while (asyncResult != VSS_S_ASYNC_CANCELLED &&
                asyncResult != VSS_S_ASYNC_FINISHED)
            {
                HRESULT result = pStatus->QueryStatus(&asyncResult, NULL);
                uw32::throw_on_fail(LFL "Unable to query vss async status", result != S_OK);
                Sleep(sleepMS);
                if (counter % 4 == 0) {
                    DBMSG("Waited " << (counter / 4) << "s");
                }
                counter++;
            }
            uw32::throw_on_fail(LFL "Operation was cancelled.", asyncResult == VSS_S_ASYNC_CANCELLED);
        }

        //---------------------------------------------------------------------
        // this should be improved.
        void
            VerifyWriterStatus(CComPtr<IVssBackupComponents>& pBackupComponents)
        {
            DBMSG("--------> VerifyWriterStatus()\n");

            // verify writer status
            CComPtr<IVssAsync> pWriterStatus;
            HRESULT result = pBackupComponents->GatherWriterStatus(&pWriterStatus);
            uw32::trace_hresult(LFL "COM check: ", result);
            uw32::throw_on_fail(LFL "GatherWriterStatus failure", result != S_OK);
            CheckStatusAndCancellation(pWriterStatus);

            // get count of writers
            UINT writerCount = 0;
            result = pBackupComponents->GetWriterStatusCount(&writerCount);
            uw32::trace_hresult(LFL "COM check: ", result);
            uw32::throw_on_fail(LFL "GetWriterStatusCount failure", result != S_OK);
            
            // check status of writers
            for (UINT i = 0; i < writerCount; i++)
            {
                VSS_ID pidInstance = {};
                VSS_ID pidWriter = {};
                CComBSTR nameOfWriter;
                VSS_WRITER_STATE state = { };
                HRESULT vssFailure = {};

                // [1]
                result = pBackupComponents->GetWriterStatus(i,
                    &pidInstance,
                    &pidWriter,
                    &nameOfWriter,
                    &state,
                    &vssFailure);
                //
                uw32::trace_hresult(LFL "COM check: ", result);
                uw32::throw_on_fail(LFL "GetWriterStatus failure", result != S_OK);
                //
                DBMSG("[" << i << "] " << (BSTR)nameOfWriter << " " << nv2::to_hex(vssFailure));
            }
            // https://learn.microsoft.com/en-us/windows/win32/api/vsbackup/nf-vsbackup-ivssbackupcomponents-gatherwriterstatus
            // yack. the docs are quite confusing. I have inferred [1] and [2] must match
            // [2]
            result = pBackupComponents->FreeWriterStatus();
            uw32::trace_hresult(LFL "COM check: ", result);
            uw32::throw_on_fail(LFL "FreeWriterStatus failure", result != S_OK);
        }

        //-----------------------------------------------------------------------------
        // i.e. ipVolume will look something like:
        // \\?\GLOBALROOT\Device\HarddiskVolumeShadowCopy130
        virtual
        void 
        doCopy(const std::wstring& ipVolume,
            const std::wstring& opPath)
        {
            if (1)
            {
                // this should provide access to the raw volume
                uw32::FileHandle fh(ipVolume);
                if (fh) 
                {
                    DBMSG("\tPass: tva: " << ipVolume);
                    //
                    {
                        // Get disk geometry to determine sector size
                        DISK_GEOMETRY diskGeometry = {};
                        DWORD bytesReturned = 0;
                        BOOL success = DeviceIoControl(fh.handle(),
                            IOCTL_DISK_GET_DRIVE_GEOMETRY,
                            NULL,
                            0,
                            &diskGeometry,
                            sizeof(diskGeometry),
                            &bytesReturned,
                            NULL
                        );
                        uw32::throw_on_fail(LFL "::DeviceIoControl failure", !success);
                        DWORD sectorSize = diskGeometry.BytesPerSector;
                        DBMSG("Sector size: " << sectorSize << " bytes");
                    }
                    // what is the file size?
                    LARGE_INTEGER liFileSize{0};
                    if (::GetFileSizeEx(fh.handle(), &liFileSize) == FALSE)
                    {
                        DBMSG("\tFAIL: tva: " << nv2::s_error(fh.GetErrorCode()));
                    }
                    else {
                        DBMSG("Size: " << (liFileSize.QuadPart / uw32::_1GB));
                    }
                }
                else {
                    DBMSG("\tFAIL: tva: " << nv2::s_error(fh.GetErrorCode()));
                }
            }

            DBMSG("-------------------");
            DBMSG("IP: " << ipVolume << " OP: " << opPath);
            std::wstring ipFQN = ipVolume;
            std::wstring opFQN = opPath;
            if (ipFQN.back()!= _T('\\'))
                ipFQN += _T('\\');
            if (opFQN.back() != _T('\\'))
                opFQN += _T('\\');

            ipFQN += _T("sg.zip");
            opFQN += _T("sg.zip");
            BOOL ok = ::CopyFile(ipFQN.c_str(),opFQN.c_str(),FALSE);
            uw32::throw_on_fail(LFL "::CopyFile failure", !ok);
        }

	public:
		
        //-----------------------------------------------------------------------------
        //
		VSSWrapper() {}
		
        //-----------------------------------------------------------------------------
        //
		virtual ~VSSWrapper() {}

        //-----------------------------------------------------------------------------
        // do snapshot. persists until destructor invoked
        // \\?\GLOBALROOT\Device\HarddiskVolumeShadowCopy130
		void 
            doSnapshotCopy(const std::wstring& ipVolume,
							const std::wstring& opPath) throw()
		{
            //
            DBMSG("IP: " << ipVolume << " OP: " << opPath);

            /// The backup components VSS object.
            CComPtr<IVssBackupComponents> pBackupComponents;

            // \\?\GLOBALROOT\Device\HarddiskVolumeShadowCopy117
            std::wstring snapshotDeviceObject;
            
            // [1 Initialize COM
            vss::ComInit comInit;

            // [2
            HRESULT result = CreateVssBackupComponents(&pBackupComponents);
            uw32::throw_on_fail(LFL "Failed to create the VSS backup components as access was denied. Is this being run with elevated permissions?", result == E_ACCESSDENIED);
            uw32::throw_on_fail(LFL "CreateVssBackupComponents", result != S_OK);

            // [3] InitializeForBackup
            result = pBackupComponents->InitializeForBackup();
            uw32::trace_hresult(LFL "COM error: ", result);
            uw32::throw_on_fail(LFL "InitializeForBackup", result != S_OK);

            // [4] gather writer metadata
            {
                CComPtr<IVssAsync> pVssAsync;
                result = pBackupComponents->GatherWriterMetadata(&pVssAsync);
                uw32::throw_on_fail(LFL "GatherWriterMetadata", result != S_OK);
                // 5 
                CheckStatusAndCancellation(pVssAsync);
            }

            // [5] snapshot preparation 
            result = pBackupComponents->SetBackupState(false, false, VSS_BT_FULL, false);
            uw32::throw_on_fail(LFL "SetBackupState", result != S_OK);

            std::vector<byte> vSnapshotSetId(sizeof(VSS_ID), 0);
            VSS_ID* snapshotSetId = (VSS_ID*)&vSnapshotSetId[0];

            // [6] start a snapshot
            result = pBackupComponents->StartSnapshotSet(snapshotSetId);
            uw32::throw_on_fail(LFL "StartSnapshotSet", result != S_OK);

            std::vector<byte> vSnapshotId(sizeof(VSS_ID),0);
            VSS_ID* snapshotId = (VSS_ID*)&vSnapshotId[0];

            {
                // [7]
                // add volumes to snapshot set AddToSnapshotSet -- we will only add the first drive spec -- all source files must be on the same volume
                LPWSTR lpwstr = (LPWSTR)ipVolume.c_str();
                result = pBackupComponents->AddToSnapshotSet(lpwstr, GUID_NULL, snapshotId);
                uw32::throw_on_fail(LFL "AddToSnapshotSet", result != S_OK);
            }

            // [8] notify writers of impending backup
            {
                // 
                CComPtr<IVssAsync> pPrepareForBackupResults;
                result = pBackupComponents->PrepareForBackup(&pPrepareForBackupResults);
                uw32::throw_on_fail(LFL "PrepareForBackup", result != S_OK);

                OutputDebugStringA(LFL "Waiting for VSS writers\n");
                // 5 
                CheckStatusAndCancellation(pPrepareForBackupResults);
            }

            // verify all VSS writers are in the correct state
            VerifyWriterStatus(pBackupComponents);

            // [9
            {
                // request shadow copy
                OutputDebugStringA(LFL "DoSnapshotSet()\n");

                CComPtr<IVssAsync> pDoSnapshotSetResults;
                result = pBackupComponents->DoSnapshotSet(&pDoSnapshotSetResults);
                uw32::throw_on_fail(LFL "DoSnapshotSet", result != S_OK);
                CheckStatusAndCancellation(pDoSnapshotSetResults);
                OutputDebugStringA(LFL "DoSnapshotSet OK\n");
            }

            // verify all VSS writers are in the correct state
            // JME check me
            VerifyWriterStatus(pBackupComponents);

            // [10]
            {
                // GetSnapshotProperties to get device to copy from
                VSS_SNAPSHOT_PROP snapshotProp{};
                result = pBackupComponents->GetSnapshotProperties(*snapshotId, &snapshotProp);
                uw32::throw_on_fail(LFL "GetSnapshotProperties", result != S_OK);

                OutputDebugStringA(LFL "** Snapshot ID: ");
                OutputDebugString(snapshotProp.m_pwszSnapshotDeviceObject);
                OutputDebugStringA("\n");
                
                //
                snapshotDeviceObject = snapshotProp.m_pwszSnapshotDeviceObject;

                // free writer metadata
                result = pBackupComponents->FreeWriterMetadata();
                uw32::throw_on_fail(LFL "FreeWriterMetadata", result != S_OK);

                VssFreeSnapshotProperties(&snapshotProp);

                OutputDebugStringA(LFL "GetSnapshotProperties OK\n");
            }

            // [11]
            {
                // actually do the copy
                doCopy(snapshotDeviceObject,opPath);
            }

            printf("Completed all copy operations successfully.\n\n");
            printf("Notifying VSS components backup completion ...\n");

            // [12]
            // set backup succeeded
            {
                CComPtr<IVssAsync> pBackupCompleteResults;
                result = pBackupComponents->BackupComplete(&pBackupCompleteResults);
                uw32::throw_on_fail(LFL "BackupComplete", result != S_OK);
                CheckStatusAndCancellation(pBackupCompleteResults);
                OutputDebugStringA(LFL "BackupComplete OK\n");
            }

            // final verification of writer status
            VerifyWriterStatus(pBackupComponents);

            // done 
            std::cout << "VSS copy completed" << std::endl;
		}
	};
}
