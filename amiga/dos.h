#ifndef __DOS_H__
#define __DOS_H__

#define _A_NORMAL (0x00)
#define _A_SUBDIR (0x10)

struct find_t {
#if 1
  void *dd_dirp;
  char *dd_filespec;
#else
  char reserved[21];
#endif
  unsigned char attrib;
//  unsigned short wr_time;
//  unsigned short wr_date;
  unsigned long size;
  char name[256];
};

unsigned int _dos_findfirst(char *name, unsigned int attr, struct find_t *result);
unsigned int _dos_findnext(struct find_t *result);
unsigned _dos_findclose(struct find_t *result);

void delay(unsigned msec);

// TODO is this the right header?
#ifdef NDEBUG
/*#define outpw(_port, _data) kprintf("%s:%ls outpw(%lu,%lu)\n", __FUNCTION__, __LINE__, (_port), (_data))
#define outp(_port, _data) kprintf("%s:%ls outp(%lu,%lu)\n", __FUNCTION__, __LINE__, (_port), (_data))
#define inp(_port) kprintf("%s:%ls inp(%lu)\n", __FUNCTION__, __LINE__, (_port))*/
#else
unsigned outp(unsigned _port, unsigned _data);
unsigned inp(unsigned _port);
#endif

#endif
