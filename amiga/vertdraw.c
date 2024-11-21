#include <vertdraw.h>

#include <clib/debug_protos.h>

// DRW_TranslatePtr is ignored as it always points to the identity colormap

//static dword maxvisible = 0;
void DRW_DoVerticalDraw(byte *dest, const byte *data) {
	byte c;

	// top clipping
	//int skip = DRW_Skip;
	sint32 skip = DRW_Skip << 16;
	//dword start = DRW_DDAStart;

	// bottom clipping
	int height = DRW_Height;

	// 16.16 texels per screen pixel
	dword inc = DRW_DDAInc;
	// 16.16 screen pixels per texel
	dword incinv = DRW_DDAIncInv;

	int textop = 0;//-start; // TODO -start
//if (DRW_Skip > 0) kprintf("%s(%lx, %lx) start %08lx skip %ld\n", __FUNCTION__, dest, data, DRW_DDAStart, DRW_Skip);
/*	if (skip > 0)
		return;*///0003D551 251217 3,83

	c = *data++;
	do {
		word count = c;
		textop += count;//if (DRW_Skip > 0) kprintf("data %lx textop %ld count %lu\n", data, textop, count);
		if ((c = *data++) == 0) {
			break;
		} else {
			word count = c;

			sint32 topscreen = (textop * incinv) - skip;
			sint32 bottomscreen = topscreen + (count * incinv);
			sint16 dc_yl = ((topscreen + (1 << 16) - 1) >> 16);
			//sint16 dc_yl = ((topscreen + (1 << 15)) >> 16);
			sint16 dc_yh = (bottomscreen - 1) >> 16;
//if (DRW_Skip > 0) kprintf("data %lx textop %ld count %lu top %08lx bottom %08lx dc_yl %ld dc_yh %ld length %ld\n", data, textop, count, topscreen, bottomscreen, dc_yl, dc_yh, (dc_yh - dc_yl + 1));
			if (dc_yh >= height)
				dc_yh = height - 1;

			dword dc_frac;
			byte *dc_dest;
			word dc_length;

			if (dc_yl < 0)
			{
				dc_frac = inc * (-dc_yl);
				dc_yl = 0;
			}
			else
			{
				dc_frac = 0;
			}

			if (dc_yl <= dc_yh)
			{
				dc_dest = dest + 320*dc_yl;
				dc_length = dc_yh - dc_yl + 1;//if (DRW_Skip > 0) kprintf("draw dc_yl %ld dc_yh %ld dc_length %ld\n", dc_yl, dc_yh, dc_length);
				while (dc_length-- > 0) {
					*dc_dest = data[dc_frac >> 16];
					dc_dest += 320;
					dc_frac += inc;
				}
			}

			textop += count;
			data += count;
		}
		c = *data++;
	} while (c > 0);
}

void DRW_DoVerticalDraw1(byte *dest, const byte *data) {
	DRW_DoVerticalDraw(dest, data);
}

void DRW_DoVerticalDraw2(byte *dest, const byte *data) {
	sint32 skip = DRW_Skip << 16;
	int height = DRW_Height;
	dword inc = DRW_DDAInc;
	dword incinv = DRW_DDAIncInv;
	int textop = 0;

	byte c = *data++;
	do {
		word count = c;
		textop += count;
		if ((c = *data++) == 0) {
			break;
		} else {
			word count = c;

			sint32 topscreen = (textop * incinv) - skip;
			sint32 bottomscreen = topscreen + (count * incinv);
			sint16 dc_yl = ((topscreen + (1 << 16) - 1) >> 16);
			sint16 dc_yh = (bottomscreen - 1) >> 16;

			if (dc_yh >= height)
				dc_yh = height - 1;

			dword dc_frac;
			byte *dc_dest;
			word dc_length;

			if (dc_yl < 0)
			{
				dc_frac = inc * (-dc_yl);
				dc_yl = 0;
			}
			else
			{
				dc_frac = 0;
			}

			if (dc_yl <= dc_yh)
			{
				dc_dest = dest + 320*dc_yl;
				dc_length = dc_yh - dc_yl + 1;
				while (dc_length-- > 0) {
					byte color = data[dc_frac >> 16];
					dc_dest[0] = color;
					dc_dest[1] = color;
					dc_dest += 320;
					dc_frac += inc;
				}
			}

			textop += count;
			data += count;
		}
		c = *data++;
	} while (c > 0);
}

void DRW_DoVerticalDraw3(byte *dest, const byte *data) {
	sint32 skip = DRW_Skip << 16;
	int height = DRW_Height;
	dword inc = DRW_DDAInc;
	dword incinv = DRW_DDAIncInv;
	int textop = 0;

	byte c = *data++;
	do {
		word count = c;
		textop += count;
		if ((c = *data++) == 0) {
			break;
		} else {
			word count = c;

			sint32 topscreen = (textop * incinv) - skip;
			sint32 bottomscreen = topscreen + (count * incinv);
			sint16 dc_yl = ((topscreen + (1 << 16) - 1) >> 16);
			sint16 dc_yh = (bottomscreen - 1) >> 16;

			if (dc_yh >= height)
				dc_yh = height - 1;

			dword dc_frac;
			byte *dc_dest;
			word dc_length;

			if (dc_yl < 0)
			{
				dc_frac = inc * (-dc_yl);
				dc_yl = 0;
			}
			else
			{
				dc_frac = 0;
			}

			if (dc_yl <= dc_yh)
			{
				dc_dest = dest + 320*dc_yl;
				dc_length = dc_yh - dc_yl + 1;
				while (dc_length-- > 0) {
					byte color = data[dc_frac >> 16];
					dc_dest[0] = color;
					dc_dest[1] = color;
					dc_dest[2] = color;
					dc_dest += 320;
					dc_frac += inc;
				}
			}

			textop += count;
			data += count;
		}
		c = *data++;
	} while (c > 0);
}

void DRW_DoVerticalDraw4(byte *dest, const byte *data) {
	sint32 skip = DRW_Skip << 16;
	int height = DRW_Height;
	dword inc = DRW_DDAInc;
	dword incinv = DRW_DDAIncInv;
	int textop = 0;

	byte c = *data++;
	do {
		word count = c;
		textop += count;
		if ((c = *data++) == 0) {
			break;
		} else {
			word count = c;

			sint32 topscreen = (textop * incinv) - skip;
			sint32 bottomscreen = topscreen + (count * incinv);
			sint16 dc_yl = ((topscreen + (1 << 16) - 1) >> 16);
			sint16 dc_yh = (bottomscreen - 1) >> 16;

			if (dc_yh >= height)
				dc_yh = height - 1;

			dword dc_frac;
			byte *dc_dest;
			word dc_length;

			if (dc_yl < 0)
			{
				dc_frac = inc * (-dc_yl);
				dc_yl = 0;
			}
			else
			{
				dc_frac = 0;
			}

			if (dc_yl <= dc_yh)
			{
				dc_dest = dest + 320*dc_yl;
				dc_length = dc_yh - dc_yl + 1;
				while (dc_length-- > 0) {
					byte color = data[dc_frac >> 16];
					dc_dest[0] = color;
					dc_dest[1] = color;
					dc_dest[2] = color;
					dc_dest[3] = color;
					dc_dest += 320;
					dc_frac += inc;
				}
			}

			textop += count;
			data += count;
		}
		c = *data++;
	} while (c > 0);
}

void DRW_DoVerticalDraw640(byte *dest, const byte *data) {
	sint32 skip = DRW_Skip << 16;
	int height = DRW_Height;
	dword inc = DRW_DDAInc;
	dword incinv = DRW_DDAIncInv;
	int textop = 0;

	byte c = *data++;
	do {
		word count = c;
		textop += count;
		if ((c = *data++) == 0) {
			break;
		} else {
			word count = c;

			sint32 topscreen = (textop * incinv) - skip;
			sint32 bottomscreen = topscreen + (count * incinv);
			sint16 dc_yl = ((topscreen + (1 << 16) - 1) >> 16);
			sint16 dc_yh = (bottomscreen - 1) >> 16;

			if (dc_yh >= height)
				dc_yh = height - 1;

			dword dc_frac;
			byte *dc_dest;
			word dc_length;

			if (dc_yl < 0)
			{
				dc_frac = inc * (-dc_yl);
				dc_yl = 0;
			}
			else
			{
				dc_frac = 0;
			}

			if (dc_yl <= dc_yh)
			{
				dc_dest = dest + 640*dc_yl;
				dc_length = dc_yh - dc_yl + 1;
				while (dc_length-- > 0) {
					*dc_dest = data[dc_frac >> 16];
					dc_dest += 640;
					dc_frac += inc;
				}
			}

			textop += count;
			data += count;
		}
		c = *data++;
	} while (c > 0);
}

void DRW_DoTransparentDraw(byte *dest, const byte *data) { kprintf("%s(%lx,%lx)\n", __FUNCTION__, dest, data); }

void DRW_DoTransparentDraw640(byte *dest, const byte *data) { kprintf("%s(%lx,%lx)\n", __FUNCTION__, dest, data); }
