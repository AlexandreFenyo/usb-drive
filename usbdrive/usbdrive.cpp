
// usbdrive.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "usbdrive.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// The one and only application object

CWinApp theApp;

using namespace std;

#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <stdlib.h>
#include <share.h>

BOOL GetDriveGeometry(TCHAR *diskname, DISK_GEOMETRY *pdg)
{
	HANDLE hDevice;               // handle to the drive to be examined 
	BOOL bResult;                 // results flag
	DWORD junk;                   // discard results

    TCHAR name[256] = _T("\\\\.\\");
	wcscat(name, diskname);

	hDevice = CreateFile(name,  // drive to open
	                    GENERIC_READ, // read data from the drive
		                FILE_SHARE_READ | // share mode
			            FILE_SHARE_WRITE, 
				        NULL,             // default security attributes
					    OPEN_EXISTING,    // disposition
						0,                // file attributes
	                    NULL);            // do not copy file attributes

	if (hDevice == INVALID_HANDLE_VALUE) // cannot open the drive
	{
		fprintf(stderr, "cannot open the drive (error %d)\n", GetLastError());
		return FALSE;
	}

	bResult = DeviceIoControl(hDevice,  // device to be queried
							  IOCTL_DISK_GET_DRIVE_GEOMETRY,  // operation to perform
							  NULL, 0, // no input buffer
							  pdg, sizeof(*pdg),     // output buffer
						      &junk,                 // # bytes returned
						      (LPOVERLAPPED) NULL);  // synchronous I/O

	if (bResult == FALSE)
	{
		fprintf(stderr, "cannot get geometry (error %d)\n", GetLastError());
		return FALSE;
	}

	CloseHandle(hDevice);
	return bResult;
}

BOOL CopyFiles(TCHAR *src, TCHAR *dst, DWORD disposition)
{
	HANDLE hSrc, hDst;               // handle to the drive to be examined 

	hSrc = CreateFile(src,
		              GENERIC_READ,
					  FILE_SHARE_READ | // share mode
					  FILE_SHARE_WRITE, 
					  NULL,             // default security attributes
			          OPEN_EXISTING,    // disposition
				      0,                // file attributes
					  NULL);            // do not copy file attributes

	if (hSrc == INVALID_HANDLE_VALUE) // cannot open the drive
	{
		printf("CreateFile for SRC returned %d\n", GetLastError());
		return FALSE;
	}

	hDst = CreateFile(dst,
		              GENERIC_WRITE,
			          FILE_SHARE_READ | // share mode
				      FILE_SHARE_WRITE, 
					  NULL,             // default security attributes
					  disposition,
				      0,                // file attributes
					  NULL);            // do not copy file attributes

	if (hDst == INVALID_HANDLE_VALUE) // cannot open the drive
	{
		printf("CreateFile for DST returned %d\n", GetLastError());
		wprintf(_T("[%s]\n"), dst);
		return FALSE;
	}

	BYTE buf[512];
	DWORD nread, nwritten;
	BOOL ret;
	DWORD cnt = 0;
	while (true) {
		ret = ReadFile(hSrc, buf, sizeof(buf), &nread, NULL);
		if (!ret || !nread) {
			if (!ret) printf("\nReadFile from SRC returned %d\n", GetLastError());
			break;
		} else {
			ret = WriteFile(hDst, buf, nread, &nwritten, NULL);
			if (!ret || nwritten < nread) {
				if (!ret) printf("\nWriteFile to DST returned %d\n", GetLastError());
				if (ret) printf("WriteFile wrote only %d bytes\n", nwritten);
				break;
			}
			cnt += nwritten;
			printf("%d Mb\r", cnt / (1024 * 1024));
		}
	}

    CloseHandle(hSrc);
    CloseHandle(hDst);
    return 0;
}

BOOL EraseDisk(TCHAR *dst)
{
	HANDLE hDst;               // handle to the drive to be erased

	hDst = CreateFile(dst,
		              GENERIC_WRITE,
			          FILE_SHARE_READ | // share mode
				      FILE_SHARE_WRITE, 
					  NULL,             // default security attributes
					  OPEN_EXISTING,
				      0,                // file attributes
					  NULL);            // do not copy file attributes

	if (hDst == INVALID_HANDLE_VALUE) // cannot open the drive
	{
		printf("CreateFile for DST returned %d\n", GetLastError());
		wprintf(_T("[%s]\n"), dst);
		return FALSE;
	}

	BYTE buf[512];
    DWORD nwritten;
	BOOL ret;
	DWORD cnt = 0;

    memset(buf, 0, sizeof buf);

    while (true) {
    	ret = WriteFile(hDst, buf, sizeof buf, &nwritten, NULL);
        if (!ret || nwritten < sizeof buf) {
		    if (!ret) printf("\nWriteFile to DST returned %d\n", GetLastError());
			if (ret) printf("WriteFile wrote only %d bytes\n", nwritten);
			break;
		}
		cnt += nwritten;
		printf("%d Mb\r", cnt / (1024 * 1024));
	}

    CloseHandle(hDst);
    return 0;
}

void usage(TCHAR *prgname) {
	wprintf(_T("copyright A. Fenyo - 2006\n"));
	wprintf(_T("usage:\n%s copy [DRIVE:|FILE] [DRIVE:|FILE]\n"), prgname);
	wprintf(_T("%s info DRIVE:\n"), prgname);
	wprintf(_T("%s erase DRIVE:\n"), prgname);
}

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int nRetCode = 0;

	// initialize MFC and print and error on failure
	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		// TODO: change error code to suit your needs
		_tprintf(_T("Fatal Error: MFC initialization failed\n"));
		nRetCode = 1;

	} else {
		// TODO: code your application's behavior here.

		if (argc < 2) {
			usage(argv[0]);
            return 0;
		}

		if (argc == 3 && !wcscmp(argv[1], _T("info"))) {
			// print disk geometry

			DISK_GEOMETRY pdg;            // disk drive geometry structure
			BOOL bResult;                 // generic results flag
			ULONGLONG DiskSize;           // size of the drive, in bytes

			bResult = GetDriveGeometry(argv[2], &pdg);

			if (bResult) {
				printf("Cylinders = %I64d\n", pdg.Cylinders);
				printf("Tracks per cylinder = %ld\n", (ULONG) pdg.TracksPerCylinder);
				printf("Sectors per track = %ld\n", (ULONG) pdg.SectorsPerTrack);
				printf("Bytes per sector = %ld\n", (ULONG) pdg.BytesPerSector);
				DiskSize = pdg.Cylinders.QuadPart * (ULONG)pdg.TracksPerCylinder *
						(ULONG)pdg.SectorsPerTrack * (ULONG)pdg.BytesPerSector;
				printf("Disk size = %I64d (Bytes) = %I64d (Gb)\n", DiskSize,
						DiskSize / (1024 * 1024 * 1024));
			}
			return 0;
		}

		if (argc == 3 && !wcscmp(argv[1], _T("erase"))) {
			TCHAR drivename[256] = _T("\\\\.\\");

			if (wcslen(argv[2]) != 2 || argv[2][1] != _T(':')) {
				usage(argv[0]);
				return 0;
			}

/*
            if (!wcscmp(argv[2], _T("c:")) || !wcscmp(argv[2], _T("C:")) ||
				!wcscmp(argv[2], _T("d:")) || !wcscmp(argv[2], _T("D:")))
					exit(1);
*/
            if (!wcscmp(argv[2], _T("c:")) || !wcscmp(argv[2], _T("C:")))
					exit(1);

            wcscat(drivename, argv[2]);
			EraseDisk(drivename);
            return 0;
        }

        if (argc == 4 && !wcscmp(argv[1], _T("copy"))) {
			TCHAR drivename[256] = _T("\\\\.\\");

			if ((wcslen(argv[2]) != 2 || argv[2][1] != _T(':')) &&
				(wcslen(argv[3]) != 2 || argv[3][1] != _T(':'))) {
				usage(argv[0]);
				return 0;
			}

			if (wcslen(argv[2]) == 2 && argv[2][1] == _T(':')) {
				wcscat(drivename, argv[2]);
				CopyFiles(drivename, argv[3], CREATE_ALWAYS);
                return 0;
			} else {
/*
                if (!wcscmp(argv[3], _T("c:")) || !wcscmp(argv[3], _T("C:")) ||
					!wcscmp(argv[3], _T("d:")) || !wcscmp(argv[3], _T("D:")))
						exit(1);
*/
                if (!wcscmp(argv[2], _T("c:")) || !wcscmp(argv[2], _T("C:")))
		            exit(1);

				wcscat(drivename, argv[3]);
				CopyFiles(argv[2], drivename, OPEN_EXISTING);
                return 0;
            }
		}

		usage(argv[0]);
		return 0;
	}

	return nRetCode;
}
