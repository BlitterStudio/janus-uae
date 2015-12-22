

/* Determines if this drive-letter currently has a disk inserted */
int CheckRM (const TCHAR *DriveName)
{
	TCHAR filename[MAX_DPATH];
	DWORD dwHold;
	BOOL result = FALSE;

	_stprintf (filename, _T("\\\\?\\%s"), DriveName);
	dwHold = GetFileAttributes (filename);
	if(dwHold != 0xFFFFFFFF)
		result = TRUE;
	return result;
}

/* This function makes sure the volume-name being requested is not already in use, or any of the following
illegal values: */
static const TCHAR *illegal_volumenames[] = { _T("SYS"), _T("DEVS"), _T("LIBS"), _T("FONTS"), _T("C"), _T("L"), _T("S") };

static int valid_volumename (struct uaedev_mount_info *mountinfo, const TCHAR *volumename, int fullcheck)
{
	int i, result = 1, illegal_count = sizeof (illegal_volumenames) / sizeof(TCHAR*);
	for (i = 0; i < illegal_count; i++) {
		if(_tcscmp (volumename, illegal_volumenames[i]) == 0) {
			result = 0;
			break;
		}
	}
	/* if result is still good, we've passed the illegal names check, and must check for duplicates now */
	if(result && fullcheck) {
		for (i = 0; i < MAX_FILESYSTEM_UNITS; i++) {
			if (mountinfo->ui[i].open && mountinfo->ui[i].volname && _tcscmp (mountinfo->ui[i].volname, volumename) == 0) {
				result = 0;
				break;
			}
		}
	}
	return result;
}

/* Returns 1 if an actual volume-name was found, 2 if no volume-name (so uses some defaults) */
int target_get_volume_name (struct uaedev_mount_info *mtinf, const TCHAR *volumepath, TCHAR *volumename, int size, bool inserted, bool fullcheck)
{
	int result = 2;
	int drivetype;

	drivetype = GetDriveType (volumepath);
	if (inserted) {
		if (GetVolumeInformation (volumepath, volumename, size, NULL, NULL, NULL, NULL, 0) &&
			volumename[0] && 
			valid_volumename (mtinf, volumename, fullcheck)) {
				// +++Bernd Roesch
				if(!_tcscmp (volumename, _T("AmigaOS35")))
					_tcscpy (volumename, _T("AmigaOS3.5"));
				if(!_tcscmp (volumename, _T("AmigaOS39")))
					_tcscpy (volumename, _T("AmigaOS3.9"));
				if(!_tcscmp (volumename, _T("AmigaOS_XL")))
					_tcscpy (volumename, _T("AmigaOS XL"));
				// ---Bernd Roesch
				if (_tcslen (volumename) > 0)
					result = 1;
		}
	}

	if(result == 2) {
		switch(drivetype)
		{
		case DRIVE_FIXED:
			_stprintf (volumename, _T("WinDH_%c"), volumepath[0]);
			break;
		case DRIVE_CDROM:
			_stprintf (volumename, _T("WinCD_%c"), volumepath[0]);
			break;
		case DRIVE_REMOVABLE:
			_stprintf (volumename, _T("WinRMV_%c"), volumepath[0]);
			break;
		case DRIVE_REMOTE:
			_stprintf (volumename, _T("WinNET_%c"), volumepath[0]);
			break;
		case DRIVE_RAMDISK:
			_stprintf (volumename, _T("WinRAM_%c"), volumepath[0]);
			break;
		case DRIVE_UNKNOWN:
		case DRIVE_NO_ROOT_DIR:
		default:
			result = 0;
			break;
		}
	}

	return result;
}

static int getidfromhandle (HANDLE h)
{
	int drvnum = -1;
	DWORD written, outsize;
	VOLUME_DISK_EXTENTS *vde;

	outsize = sizeof (VOLUME_DISK_EXTENTS) + sizeof (DISK_EXTENT) * 32;
	vde = (VOLUME_DISK_EXTENTS*)xmalloc (uae_u8, outsize);
	if (DeviceIoControl (h, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, NULL, 0, vde, outsize, &written, NULL)) {
		if (vde->NumberOfDiskExtents > 0)
			drvnum = vde->Extents[0].DiskNumber;
	}
	xfree (vde);
	return drvnum;
}

static int hfdcheck (TCHAR drive)
{
	HANDLE h;
	TCHAR tmp[16];
	int disknum, i;

	_stprintf (tmp, _T("\\\\.\\%c:"), drive);
	h = CreateFile (tmp, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (h == INVALID_HANDLE_VALUE)
		return 0;
	disknum = getidfromhandle (h);
	CloseHandle (h);
	for (i = 0; i < MAX_FILESYSTEM_UNITS; i++) {
		struct hardfiledata *hfd = get_hardfile_data (i);
		int reopen = 0;
		if (!hfd || !(hfd->flags & HFD_FLAGS_REALDRIVE) || !hfd->handle_valid)
			continue;
		if (getidfromhandle (hfd->handle) == disknum)
			return 1;
	}
	return 0;
}

void filesys_addexternals (void)
{
	int drive, drivetype;
	UINT errormode;
	DWORD dwDriveMask;
	int drvnum = 0;

	if (!currprefs.win32_automount_cddrives && !currprefs.win32_automount_netdrives
		&& !currprefs.win32_automount_drives && !currprefs.win32_automount_removabledrives)
		return;
	errormode = SetErrorMode (SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
	dwDriveMask = GetLogicalDrives ();
	dwDriveMask >>= 2; // Skip A and B drives...

	for(drive = 'C'; drive <= 'Z'; ++drive) {
		struct uaedev_config_info ci = { 0 };
		_stprintf (ci.rootdir, _T("%c:\\"), drive);
		/* Is this drive-letter valid (it used to check for media in drive) */
		if(dwDriveMask & 1) {
			bool inserted = CheckRM (ci.rootdir) != 0; /* Is there a disk inserted? */
			int nok = FALSE;
			int rw = 1;

			drivetype = GetDriveType (ci.rootdir);
			if (inserted && drivetype != DRIVE_NO_ROOT_DIR && drivetype != DRIVE_UNKNOWN) {
				if (hfdcheck (drive)) {
					write_log (_T("Drive %c:\\ ignored, was configured as a harddrive\n"), drive);
					continue;
				}
			}
			for (;;) {
				if (!inserted) {
					nok = TRUE;
					break;
				}
				if (drivetype == DRIVE_REMOTE && currprefs.win32_automount_netdrives)
					break;
				if (drivetype == DRIVE_FIXED && currprefs.win32_automount_drives)
					break;
				if (drivetype == DRIVE_REMOVABLE && currprefs.win32_automount_removabledrives)
					break;
				nok = TRUE;
				break;
			}
			if (nok)
				continue;
			if (inserted) {
				target_get_volume_name (&mountinfo, ci.rootdir, ci.volname, MAX_DPATH, inserted, true);
				if (!ci.volname[0])
					_stprintf (ci.volname, _T("WinUNK_%c"), drive);
			}
			if (drivetype == DRIVE_REMOTE)
				_tcscat (ci.rootdir, _T("."));
			else
				_tcscat (ci.rootdir, _T(".."));
#if 0
			if (currprefs.win32_automount_drives > 1) {
				devname[0] = drive;
				devname[1] = 0;
			}
#endif
			ci.readonly = !rw;
			ci.bootpri = -20 - drvnum;
			//write_log (_T("Drive type %d: '%s' '%s'\n"), drivetype, volumepath, volumename);
			add_filesys_unit (&ci);
			drvnum++;
		} /* if drivemask */
		dwDriveMask >>= 1;
	}
	SetErrorMode (errormode);
}
