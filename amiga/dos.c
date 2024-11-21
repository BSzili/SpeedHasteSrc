
#include <string.h>
#include <sys/stat.h>
//#include <errno.h>
#include <unistd.h>
#include <fnmatch.h>
#include <dirent.h>
#include <stdio.h>

#include <clib/debug_protos.h>

#include "dos.h"
#include "vga.h"

unsigned int _dos_findfirst(char *name, unsigned int attr, struct find_t *result)
{
	char dir[108];
	char *s = strrchr(name, '/');
#ifdef __AMIGA__
	if (!s) s = strrchr(name, ':');
#endif
	if (s)
	{
		result->dd_filespec = s + 1;
		strncpy(dir, name, (size_t)(s-name));
	}
	else
	{
#ifdef __AMIGA__
		strcpy(dir, "");
#else
		strcpy(dir, ".");
#endif
		result->dd_filespec = name;
	}
	result->attrib = attr;
	result->dd_dirp = opendir(dir);
	return _dos_findnext(result);
}

unsigned int _dos_findnext(struct find_t *result)
{
	struct stat dd_sstat;
	struct dirent *dd_dp;

	if (!result->dd_dirp) return -1;

	while ((dd_dp = readdir(result->dd_dirp)) != NULL)
	{
		//printf("%s %s %s\n", __FUNCTION__, result->dd_filespec, dd_dp->d_name);
		if (stat(dd_dp->d_name, &dd_sstat))
			continue;
		if (dd_sstat.st_mode & S_IFDIR && !(result->attrib & _A_SUBDIR))
			continue;
		if (!fnmatch(result->dd_filespec, dd_dp->d_name, FNM_PATHNAME))
		{
			strncpy(result->name, dd_dp->d_name, sizeof(result->name));
			result->size = dd_sstat.st_size;
			return 0;
		}
	}

	closedir(result->dd_dirp);
	result->dd_dirp = NULL;

	return -1;
}

unsigned _dos_findclose(struct find_t *result)
{
	return 0;
}

void delay(unsigned msec)
{
	usleep(msec * 1000);
}

#ifndef NDEBUG
/*
extern unsigned inp(unsigned __port);
extern unsigned inpw(unsigned __port);
extern unsigned outp(unsigned __port, unsigned __value);
extern unsigned outpw(unsigned __port,unsigned __value);
*/

static int palidx = 0;
static byte paldata[4];

unsigned outp(unsigned port, unsigned data)
{
	switch (port)
	{
		case 0x3C8:
			palidx = 0;
			paldata[palidx++] = data;
			break;
		case 0x3C9:
			paldata[(palidx++) & 3] = data;
			if (palidx == 0) VGA_DumpPalette(paldata + 1, paldata[0], 1);
			break;
		default:
			kprintf("%s(%04xl,%lu)\n", __FUNCTION__, port, data);
			break;
	}
	
	return 0;
}

unsigned outpw(unsigned __port,unsigned __value)
{
	printf("%s(%04x,%u)\n", __FUNCTION__, __port, __value);
	return 0;
}

unsigned inp(unsigned port)
{
	printf("%s(%04x)\n", __FUNCTION__, port);
	return 0;
}
#endif
