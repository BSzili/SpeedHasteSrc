#include <base.h>
#include <hordraw.h>
#include <vertdraw.h> // DRW_Tile

void DRW_DoHorizontalDraw(byte *dest, const byte *data, int skip, int width)
{
	byte c;

	// TODO handle skip, width and DRW_Tile
	c = *data++;
	do {
		dest += c;
		if ((c = *data++) == 0) {
			break;
		} else {
			word count = c;
			while (count-- > 0) {
				*dest++ = *data++;
			}
		}
		c = *data++;
	} while (c > 0);
}
