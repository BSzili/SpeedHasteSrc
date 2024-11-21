
#include <base.h>

#include <clib/debug_protos.h>

static const byte **mapaddr;
static const byte *transaddr;

void FL_SetMap(const byte *map[], const byte trans[])
{
	//kprintf("%s(%lx,%lx)\n", __FUNCTION__, map, trans);
	//if (map != NULL) {
		mapaddr = map;
	//}
	//if (trans != NULL) {
		transaddr = trans;
	//}
}

void FL_DrawRaster(byte *dest, int width, uint32 x, uint32 y, uint32 dx, uint32 dy)
{
	//kprintf("%s(%lx, %ld, %lu, %lu, %lu, %lu)\n", __FUNCTION__, dest, width, x, y, dx, dy);
	const byte **map = mapaddr;
	const byte *trans = transaddr;

	if (trans) {
		LOOP(j, width) {
			byte c;
			const byte *tile;

			tile = map[(y >> 25)*128 + (x >> 25)];
			c = tile[((y & 0x1FFFFFF) >> 19)*64 + ((x & 0x1FFFFFF) >> 19)];
			*dest++ = trans[c];

			x += dx;
			y += dy;
		}
	} else {
		LOOP(j, width) {
			byte c;
			const byte *tile;

			tile = map[(y >> 25)*128 + (x >> 25)];
			c = tile[((y & 0x1FFFFFF) >> 19)*64 + ((x & 0x1FFFFFF) >> 19)];
			*dest++ = c;

			x += dx;
			y += dy;
		}
	}
}

void FL_DrawRasterLo(byte *dest, int width, uint32 x, uint32 y, uint32 dx, uint32 dy)
{
	// Advance one pixel
	x += dx;
	y += dy;
	// Double the deltas
	dx += dx;
	dy += dy;

	const byte **map = mapaddr;
	const byte *trans = transaddr;

	if (trans) {
		LOOP(j, width >> 1) {
			byte c;
			const byte *tile;

			tile = map[(y >> 25)*128 + (x >> 25)];
			c = tile[((y & 0x1FFFFFF) >> 19)*64 + ((x & 0x1FFFFFF) >> 19)];
			c = trans[c];
			*dest++ = c;
			*dest++ = c;

			x += dx;
			y += dy;
		}
	} else {
		LOOP(j, width >> 1) {
			byte c;
			const byte *tile;

			tile = map[(y >> 25)*128 + (x >> 25)];
			c = tile[((y & 0x1FFFFFF) >> 19)*64 + ((x & 0x1FFFFFF) >> 19)];
			*dest++ = c;
			*dest++ = c;

			x += dx;
			y += dy;
		}
	}
}

void FL_DrawRasterTrans(byte *dest, int width, uint32 x, uint32 y, uint32 dx, uint32 dy)
{
	const byte **map = mapaddr;
	const byte *trans = transaddr;

	LOOP(j, width) {
		byte c;
		const byte *tile;

		tile = map[(y >> 25)*128 + (x >> 25)];
		c = tile[((y & 0x1FFFFFF) >> 19)*64 + ((x & 0x1FFFFFF) >> 19)];
		if (c < 160 || c >= 192) {
			*dest = trans[0x1800 | *dest];
		} else {
			*dest = 185;
		}
		dest++;

		x += dx;
		y += dy;
	}
}

void FL_DrawRasterNoTile(byte *dest, int width, uint32 x, uint32 y, uint32 dx, uint32 dy)
{
	kprintf("%s:%ld\n", __FUNCTION__, __LINE__);
}
