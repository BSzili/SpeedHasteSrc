#include <polygon.h>

#include <clib/debug_protos.h>

sint32 POLY_MinX, POLY_MinY, POLY_MaxX, POLY_MaxY;

//#define DELTA_CACHE
#define DELTA_FASTDTX
//#define DELTA_FASTDTY // causes some warping on the side of buildings
//#define DELTA_FASTDTL // this causes artifacts
//#define TEXCOORD_SCALE64
//#define TEX_HALFWIDTH
#define TRACE_SKIPHALF

const sint32 *POLY_DivTable;

#if 0
#include <clib/debug_protos.h>
static char *va(char *format, ...) {
	va_list argptr;
	static char string[240];

	va_start(argptr, format);
	vsnprintf(string, sizeof(string), format, argptr);
	va_end(argptr);

	return string;
}

// 40000 * 64  = 0x1000000
// 1 << 26     = 0x4000000
#define DBL(x) ((double)(x) / (double)0x40000000)
#define TC(x) ((double)(x) / (double)(0x1000000))
#endif

#define SHADE_SHIFT (16)
#define TEXTURE_SHIFT (18)
#ifdef TEXCOORD_SCALE64
#define TEXTURE_SCALESHIFT (6)
#define TEXTURE_PRESHIFT (TEXTURE_SHIFT - TEXTURE_SCALESHIFT)
#endif

static inline float fastinv(float number)
{
  union {
    float    f;
    uint32_t i;
  } conv;
  conv.f = number;
#if 0
  conv.i  = 0x7EF311C2 - conv.i;
#else
  conv.i  = 0x7EF311C3 - conv.i;
  conv.f = conv.f * (2 - conv.f * number);
#endif
  return conv.f;
}
#if 1
#define DIV(a, b) ((a) / (b))
#else
#define DIV(a, b) ((a) * fastinv((b)))
#endif

void POLY_TraceEdge(POLY_TFullEdge *edge, const POLY_TFullVertex *v0, const POLY_TFullVertex *v1, int clipend)
{
	//kprintf(va("%s(%p, %p, %p, %d)\n", __FUNCTION__, edge, v0, v1, clipend));
	sint32 dx = v1->x - v0->x; // eax
	sint32 dy = v1->y - v0->y; // ecx
	////kprintf(va("%s:%d dx (%d-%d) = %d, dy (%d - %d) = %d\n", __FUNCTION__, __LINE__, v1->x, v0->x, dx, v1->y, v0->y, dy));
	/*if (dy < 64 && dx < 64 && dx >= -64) {
		//ebp = eax+64 << 16 / ecx 
		dx = POLY_DivTable[((dx + 64) << 6) + dy];
	} else*/ {
		dx = (dx << 16) / dy;
	}
////kprintf(va("%s:%d dx %f\n", __FUNCTION__, __LINE__, DBL(dx)));
	sint32 ptx = v0->x << 16;
	sint32 skip = v0->y - POLY_MinY;
	////kprintf(va("%s:%d ptx %f skip %d\n", __FUNCTION__, __LINE__, DBL(ptx), skip));
	if (skip < 0) {
		ptx += -skip*dx;
		dy -= -skip;
		////kprintf(va("%s:%d ptx %d dy %d\n", __FUNCTION__, __LINE__, ptx, dy));
	}

	dy -= clipend;
	//kprintf("%s:%ld %lx dy %ld clipend %ld\n", __FUNCTION__, __LINE__, edge, dy, clipend);
	if (dy <= 0) {
		// shouldn't happen
		return;
	}

#ifndef TRACE_SKIPHALF
	// Add 0.5 of the delta.
	ptx += (dx >> 1);
#endif
	////kprintf(va("%s:%d ptx %f\n", __FUNCTION__, __LINE__, DBL(ptx)));
	LOOP(i, dy) {
		// this can be x0 or x1 with POLY_FullEdgeBufRight
		edge->x0 = ptx >> 16;
//kprintf(va("%s:%d %d/%d edge x %d\n", __FUNCTION__, __LINE__, i, dy, edge->x0));
		edge++;
		ptx += dx;
	}
}

void POLY_TraceTexture(POLY_TFullEdge *edge, const POLY_TFullVertex *v0, const POLY_TFullVertex *v1, int clipend)
{
	//kprintf(va("%s(%p, %p, %p, %d)\n", __FUNCTION__, edge, v0, v1, clipend));
	sint32 dy = v1->y - v0->y;
#ifdef TEXCOORD_SCALE64
	sint32 tx0 = (v0->tx >> TEXTURE_PRESHIFT)/* & 4032*/;
	sint32 tx1 = (v1->tx >> TEXTURE_PRESHIFT)/* & 4032*/;
	sint32 ty0 = (v0->ty >> TEXTURE_PRESHIFT)/* & 4032*/;
	sint32 ty1 = (v1->ty >> TEXTURE_PRESHIFT)/* & 4032*/;
#else
	sint32 tx0 = v0->tx;
	sint32 tx1 = v1->tx;
	sint32 ty0 = v0->ty;
	sint32 ty1 = v1->ty;
#endif

	sint32 dtx = (tx1 - tx0) / dy;
	sint32 dty = (ty1 - ty0) / dy;
//kprintf("y0 %3lu y1 %3lu x %08lx %08lx %08lx y %08lx %08lx %08lx\n",  v1->y, v0->y, tx0, tx1, dtx, ty0, ty1, dty);
	dword ptx = tx0;
	dword pty = ty0;
//kprintf(va("y0 %3u y1 %3u tx0 %4.2f tx1 %4.2f dtx %4.2f ty0 %4.2f ty1 %4.2f dty %4.2f\n", v0->y, v1->y, TC(tx0), TC(tx1), TC(dtx), TC(ty0), TC(ty1), TC(dty)));
//kprintf(va("v0 %p (%3u,%3u) (%4.2f,%4.2f) -> v1 %p (%3u,%3u) (%4.2f,%4.2f)\n", v0, v0->x, v0->y, TC(tx0), TC(ty0), v1, v1->x, v1->y, TC(tx1), TC(ty1)));
	sint32 skip = v0->y - POLY_MinY;
	if (skip < 0) {
		ptx += -skip*dtx;
		pty += -skip*dty;
		dy -= -skip;
	}

	dy -= clipend;
	if (dy <= 0) {
		// shouldn't happen
		return;
	}

#ifndef TRACE_SKIPHALF
	ptx += (dtx >> 1);
	pty += (dty >> 1);
#endif
	LOOP(i, dy) {
		edge->tx0 = ptx;
		edge->ty0 = pty;
		edge++;
		ptx += dtx;
		pty += dty;
	}
}

void POLY_TraceShade(POLY_TFullEdge *edge, const POLY_TFullVertex *v0, const POLY_TFullVertex *v1, int clipend)
{
	//kprintf(va("%s(%p, %p, %p, %d)\n", __FUNCTION__, edge, v0, v1, clipend));
	sint32 dy = v1->y - v0->y;
	sint32 dl = (v1->l - v0->l) / dy;
//kprintf(va("%s %p v1->l %f v0->l %f dl %f\n", __FUNCTION__, edge, DBL(v1->l), DBL(v0->l), DBL(dl)));
	sint32 ptx = v0->l;
	sint32 skip = v0->y - POLY_MinY;
//kprintf("%s %lx v1->l %lx v0->l %lx dl %lx\n", __FUNCTION__, edge, v1->l, v0->l, dl);
	if (skip < 0) {
		ptx += -skip*dl;
		dy -= -skip;
	}

	dy -= clipend;
	if (dy <= 0) {
		// shouldn't happen
		return;
	}

#ifndef TRACE_SKIPHALF
	ptx += (dl >> 1);
#endif
	LOOP(i, dy) {
		edge->l0 = ptx;
		edge++;
		ptx += dl;
	}
}

// unused?
void POLY_TraceZ(POLY_TFullEdge *edge,
                        const POLY_TFullVertex *v0,
                        const POLY_TFullVertex *v1,
                        int clipend) { kprintf("%s()\n", __FUNCTION__); }

// ------------------------------------------

#if 1
#define DUMP_DEBUG(color) //for (int i = 0; i < nscans; i++) { kprintf("%s %ld/%ld x0 %ld x1 %ld\n", __FUNCTION__, i, nscans, edge->x0, edge->x1); }
#else
#define DUMP_DEBUG(color) for (int i = 0; i < nscans; i++) { memset(dest + edge[i].x0, color, edge[i].x1 - edge[i].x0 + 1); }
#endif
//extern dword t0, t1, t2, t3, t4, t5, t6, t7, t8, t9; extern dword getdelta(void);
void POLY_SolidDump(byte *dest, POLY_TFullEdge *edge, int nscans, int width, int color)
{//getdelta();
	//byte rgb[3]; VGA_GetPalette(rgb, color, 1); kprintf("%s(%lx,%lx,%ld,%ld,%ld) (%u, %u, %u)\n", __FUNCTION__, dest, edge, nscans, width, color, rgb[0], rgb[1], rgb[2]);
	LOOP(i, nscans) {
		if (edge->x0 < POLY_MaxX && edge->x1 > POLY_MinX) {
			sint32 x0 = edge->x0;
			sint32 x1 = edge->x1;
			sint32 dx = x1 - x0;
			if (dx > 0) {
				// clip left edge
				if (x0 < POLY_MinX) {
					//sint32 skip = POLY_MinX - x0;
					x0 = POLY_MinX;
				}
				// clip right edge
				if (x1 > POLY_MaxX) {
					x1 = POLY_MaxX;
				}
				byte *d = dest + x0;
				dx = x1 - x0;
				LOOP(j, dx) {
					*d++ = color;
				}
			}
		}
		edge++;
		dest += width;
	}//t0 += getdelta();
}

void POLY_ShadeDump(byte *dest, POLY_TFullEdge *edge, int nscans, int width, int color, const byte *ltable)
{//getdelta();
#ifdef DELTA_CACHE
	sint32 odx = -1, odl = -1, prevdl = 0;
#endif
	LOOP(i, nscans) {
		if (edge->x0 < POLY_MaxX && edge->x1 > POLY_MinX) {
			sint32 x0 = edge->x0;
			sint32 x1 = edge->x1;
			sint32 dx = x1 - x0;
#ifdef DELTA_CACHE
			int match = (dx == odx);
			odx = dx;
#endif
			if (dx > 0) {
				sint32 l0 = edge->l0;
				sint32 l1 = edge->l1;
				sint32 ndl = (l1 - l0);
#ifdef DELTA_CACHE
				sint32 dl;
#ifdef DELTA_FASTDTL
				if (match) {
#else
				if (match && ((ndl >> SHADE_SHIFT) == (odl >> SHADE_SHIFT))) {
#endif
					dl = prevdl;
				} else {
					odl = ndl;
					dl = ndl / dx;
					prevdl = dl;
				}
#else
				sint32 dl = DIV((l1 - l0), dx);
#endif
				// clip left edge
				if (x0 < POLY_MinX) {
					sint32 skip = POLY_MinX - x0;
					l0 += dl * skip;
					x0 = POLY_MinX;
				}
				// clip right edge
				if (x1 > POLY_MaxX) {
					x1 = POLY_MaxX;
				}
				byte *d = dest + x0;
				dx = x1 - x0;
				if (!(ndl >> SHADE_SHIFT)) {
					byte c = ltable[((byte)(l0 >> SHADE_SHIFT) << 8) + color];
					LOOP(j, dx) {
						*d++ = c;
					}
				} else {
					LOOP(j, dx) {
						*d++ = ltable[((byte)(l0 >> SHADE_SHIFT) << 8) + color];
						l0 += dl;
					}
				}
			}
		}
		edge++;
		dest += width;
	}//t1 += getdelta();
}

void POLY_GouraudDump(byte *dest, POLY_TFullEdge *edge, int nscans, int width) { kprintf("%s()\n", __FUNCTION__); }

void POLY_TextureDump(byte *dest, POLY_TFullEdge *edge, int nscans, int width, const byte *texture)
{
	//kprintf("%s()\n", __FUNCTION__);
	DUMP_DEBUG(texture[0]);
#if 0
	POLY_SolidDump(dest, edge, nscans, width, *texture);
	return;
#endif
//getdelta();
#ifdef DELTA_CACHE
	sint32 odx = -1, odtx = -1, prevdtx = 0, odty = -1, prevdty = 0;
#endif
	LOOP(i, nscans) {
		if (edge->x0 < POLY_MaxX && edge->x1 > POLY_MinX) {
			sint32 x0 = edge->x0;
			sint32 x1 = edge->x1;
			sint32 dx = x1 - x0;
#ifdef DELTA_CACHE
			int match = (dx == odx);
			odx = dx;
#endif
			if (dx > 0) {
				sint32 tx0 = edge->tx0;
				sint32 tx1 = edge->tx1;
				sint32 ty0 = edge->ty0;
				sint32 ty1 = edge->ty1;
#ifdef DELTA_CACHE
				sint32 ndtx = (tx1 - tx0);
				sint32 dtx;
#ifdef DELTA_FASTDTX
				if (match) {
#else
				if (match && (ndtx == odtx)) {
#endif
					dtx = prevdtx;
				} else {
					odtx = ndtx;
					dtx = ndtx / dx;
					prevdtx = dtx;
				}
				sint32 ndty = (ty1 - ty0);
				sint32 dty;
#ifdef DELTA_FASTDTY
				if (match) {
#else
				if (match && (ndty == odty)) {
#endif
					dty = prevdty;
				} else {
					odty = ndty;
					dty = ndty / dx;
					prevdty = dty;
				}
#else
				sint32 dtx = DIV((tx1 - tx0), dx);
				sint32 dty = DIV((ty1 - ty0), dx);
#endif

				// clip left edge
				if (x0 < POLY_MinX) {
					sint32 skip = POLY_MinX - x0;
					tx0 += dtx * skip;
					ty0 += dty * skip;
					x0 = POLY_MinX;
				}
				// clip right edge
				if (x1 > POLY_MaxX) {
					x1 = POLY_MaxX;
				}

				byte *d = dest + x0;
				dx = x1 - x0;
#ifdef TEX_HALFWIDTH
				sint32 hdx = dx >> 1
				sint32 ddtx = dtx << 1;
				sint32 ddty = dty << 1;

				LOOP(j, hdx) {
					sint32 xcoord = (tx0 >> TEXTURE_SHIFT) & 63;
					sint32 ycoord = (ty0 >> TEXTURE_SHIFT) & 63;
					byte color = texture[xcoord + (ycoord << 6)];
					*d++ = color;
					*d++ = color;
					tx0 += ddtx;
					ty0 += ddty;
				}
				if (dx & 1) {
					sint32 xcoord = (tx0 >> TEXTURE_SHIFT) & 63;
					sint32 ycoord = (ty0 >> TEXTURE_SHIFT) & 63;
					byte color = texture[xcoord + (ycoord << 6)];
					*d = color;
				}

#else
				LOOP(j, dx) {
#ifdef TEXCOORD_SCALE64
					byte color = texture[((tx0 >> 6) & 63) + (ty0 & 4032)];
#else
					sint32 xcoord = (tx0 >> TEXTURE_SHIFT) & 63;
					sint32 ycoord = (ty0 >> TEXTURE_SHIFT) & 63;
					byte color = texture[xcoord + (ycoord << 6)];
#endif
					//if (dtx < (1 << (TEXTURE_SHIFT -1))) color = rand();
					*d++ = /*match ? rand() :*/ color;
					tx0 += dtx;
					ty0 += dty;
				}
#endif
			}
		}
		edge++;
		dest += width;
	}//t2 += getdelta();
}

void POLY_TextureDump256(byte *dest, POLY_TFullEdge *edge, int nscans, int width, const byte *texture) { kprintf("%s()\n", __FUNCTION__); }

void POLY_HoleTexDump(byte *dest, POLY_TFullEdge *edge, int nscans, int width, const byte *texture) { kprintf("%s()\n", __FUNCTION__); }

void POLY_LightTexDump(byte *dest, POLY_TFullEdge *edge, int nscans, int width, const byte *texture, const byte *ltable) { kprintf("%s()\n", __FUNCTION__); }


void POLY_ShadeTexDump(byte *dest, POLY_TFullEdge *edge, int nscans, int width, const byte *texture, word ShadeTableSel)
{
	//kprintf("%s()\n", __FUNCTION__);
	DUMP_DEBUG(texture[0]);
#if 0
	POLY_ShadeDump(dest, edge, nscans, width, *texture, POLY_ShadeTable);
	return;
#endif
//getdelta();
	const byte *ltable = POLY_ShadeTable;
#ifdef DELTA_CACHE
	sint32 odx = -1, odtx = -1, prevdtx = 0, odty = -1, prevdty = 0, odl = -1, prevdl = 0;
#endif
	LOOP(i, nscans) {
		if (edge->x0 < POLY_MaxX && edge->x1 > POLY_MinX) {
			sint32 x0 = edge->x0;
			sint32 x1 = edge->x1;
			sint32 dx = x1 - x0;
#ifdef DELTA_CACHE
			int match = (dx == odx);
			odx = dx;
#endif
			if (dx > 0) {
				sint32 tx0 = edge->tx0;
				sint32 tx1 = edge->tx1;
				sint32 ty0 = edge->ty0;
				sint32 ty1 = edge->ty1;
				sint32 l0 = edge->l0;
				sint32 l1 = edge->l1;
#ifdef DELTA_CACHE
				sint32 ndtx = (tx1 - tx0);
				sint32 dtx;
#ifdef DELTA_FASTDTX
				if (match) {
#else
				if (match && (ndtx == odtx)) {
#endif
					dtx = prevdtx;
				} else {
					odtx = ndtx;
					dtx = ndtx / dx;
					prevdtx = dtx;
				}
				sint32 ndty = (ty1 - ty0);
				sint32 dty;
#ifdef DELTA_FASTDTY
				if (match) {
#else
				if (match && (ndty == odty)) {
#endif
					dty = prevdty;
				} else {
					odty = ndty;
					dty = ndty / dx;
					prevdty = dty;
				}
				sint32 ndl = (l1 - l0);
				sint32 dl;
#ifdef DELTA_FASTDTL
				if (match) {
#else
				if (match && ((ndl >> SHADE_SHIFT) == (odl >> SHADE_SHIFT))) {
#endif
					dl = prevdl;
				} else {
					odl = ndl;
					dl = ndl / dx;
					prevdl = dl;
				}
#else
				sint32 dtx = DIV((tx1 - tx0), dx);
				sint32 dty = DIV((ty1 - ty0), dx);
				sint32 dl = DIV((l1 - l0), dx);
#endif

				// clip left edge
				if (x0 < POLY_MinX) {
					sint32 skip = POLY_MinX - x0;
					tx0 += dtx * skip;
					ty0 += dty * skip;
					l0 += dl * skip;
					x0 = POLY_MinX;
				}
				// clip right edge
				if (x1 > POLY_MaxX) {
					x1 = POLY_MaxX;
				}

				byte *d = dest + x0;
				dx = x1 - x0;

#ifdef TEX_HALFWIDTH

				sint32 hdx = dx >> 1;
				sint32 ddtx = dtx << 1;
				sint32 ddty = dty << 1;
				sint32 ddl = dl << 1;

				LOOP(j, hdx) {
					sint32 xcoord = (tx0 >> TEXTURE_SHIFT) & 63;
					sint32 ycoord = (ty0 >> TEXTURE_SHIFT) & 63;
					byte color = texture[xcoord + (ycoord << 6)];
					color = ltable[256*(byte)(l0 >> SHADE_SHIFT) + color];
					*d++ = color;
					*d++ = color;
					tx0 += ddtx;
					ty0 += ddty;
					l0 += ddl;
				}
				if (dx & 1) {
					sint32 xcoord = (tx0 >> TEXTURE_SHIFT) & 63;
					sint32 ycoord = (ty0 >> TEXTURE_SHIFT) & 63;
					byte color = texture[xcoord + (ycoord << 6)];
					color = ltable[256*(byte)(l0 >> SHADE_SHIFT) + color];
					*d = color;
				}

#else

				LOOP(j, dx) {
#ifdef TEXCOORD_SCALE64
					byte color = texture[((tx0 >> 6) & 63) + (ty0 & 4032)];
#else
					sint32 xcoord = (tx0 >> TEXTURE_SHIFT) & 63;
					sint32 ycoord = (ty0 >> TEXTURE_SHIFT) & 63;
					byte color = texture[xcoord + (ycoord << 6)];
#endif
					*d++ = ltable[((byte)(l0 >> SHADE_SHIFT) << 8) + color];
					tx0 += dtx;
					ty0 += dty;
					l0 += dl;
				}

#endif
			}
		}
		edge++;
		dest += width;
	}//t3 += getdelta();
}

void POLY_ShadeTexDump256(byte *dest, POLY_TFullEdge *edge, int nscans, int width, const byte *texture, word ShadeTableSel) { kprintf("%s()\n", __FUNCTION__); }

void POLY_TransDump(byte *dest, POLY_TFullEdge *edge, int nscans, int width, const byte *ltable)
{//getdelta();
	DUMP_DEBUG(rand());
	LOOP(i, nscans) {
		if (edge->x0 < POLY_MaxX && edge->x1 > POLY_MinX) {
			sint32 x0 = edge->x0;
			sint32 x1 = edge->x1;
			sint32 dx = x1 - x0;
			if (dx > 0) {
				// clip left edge
				if (x0 < POLY_MinX) {
					x0 = POLY_MinX;
				}
				// clip right edge
				if (x1 > POLY_MaxX) {
					x1 = POLY_MaxX;
				}
				byte *d = dest + x0;
				dx = x1 - x0;
				LOOP(j, dx) {
					byte color = *d;
					*d++ = ltable[color];
				}
			}
		}
		edge++;
		dest += width;
	}//t4 += getdelta();
}

void POLY_ZTextureDump(byte *dest, POLY_TFullEdge *edge, int nscans, int width, const byte *texture) { kprintf("%s()\n", __FUNCTION__); }


// STUB, only used by POLY_ZTextureDumpC 
byte *PutTexByte(byte *dest, const byte *tex, sint32 tx, sint32 ty) { kprintf("%s()\n", __FUNCTION__); return NULL; }
