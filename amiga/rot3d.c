#include <rot3d.h>
#include <sincos.h>

sint32 R3D_FocusX, R3D_FocusY, R3D_CenterX, R3D_CenterY;

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
#define DBL(x) ((double)(x) / (double)0x40000000)
#endif

void R3D_Gen3DMatrix(R3D_PRotMatrix dest, R3D_PAngles angs)
{
/*
 ł      ÚÄ                                                              Äż
 ł      ł  COSc*COSa+SINc*SINb*SINa -COSc*SINa+SINc*SINb*COSa  SINc*COSb ł
 ł RM đ ł  COSb*SINa                 COSb*COSa                -SINb      ł
 ł      ł -SINc*COSa+COSc*SINb*SINa  SINc*SINa+COSc*SINb*COSa  COSc*COSb ł
 ł      ŔÄ                                                              ÄŮ
 */
	sint32 cosA, cosB, cosC, sinA, sinB, sinC, sinBsinA, sinBcosA;

	// Roll(Z)
	cosA = Cos(angs[0]);
	sinA = Sin(angs[0]);

	// Pitch(X)
	cosB = Cos(angs[1]);
	sinB = Sin(angs[1]);

	// Yaw(Y)
	cosC = Cos(angs[2]);
	sinC = Sin(angs[2]);
//kprintf(va("cosC %4.2f sinC %4.2f\n", DBL(cosC), DBL(sinC)));
	sinBsinA = FPMult(sinB, sinA);
	sinBcosA = FPMult(sinB, cosA);

	dest[0] = FPMult(cosC, cosA) + FPMult(sinC, sinBsinA);
	dest[1] = -FPMult(cosC, sinA) + FPMult(sinC, sinBcosA);
	dest[2] = FPMult(sinC, cosB);

	dest[3] = FPMult(cosB, sinA);
	dest[4] = FPMult(cosB, cosA);
	dest[5] = -sinB;

	dest[6] = -FPMult(sinC, cosA) + FPMult(cosC, sinBsinA);
	dest[7] = FPMult(sinC, sinA) + FPMult(cosC, sinBcosA);
	dest[8] = FPMult(cosC, cosB);
//kprintf(va("%s: %4.2f %4.2f %4.2f\n %4.2f %4.2f %4.2f\n %4.2f %4.2f %4.2f\n", __FUNCTION__, DBL(dest[0]), DBL(dest[1]), DBL(dest[2]), DBL(dest[3]), DBL(dest[4]), DBL(dest[5]), DBL(dest[6]), DBL(dest[7]), DBL(dest[8])));
}

void R3D_Rot3DVector(R3D_PPosVector dest, R3D_PRotMatrix m, R3D_PPosVector v, int n, int size)
{
	//kprintf("%s(%lx, %lx, %lx, %ld, %ld)\n", __FUNCTION__, dest, m, v, n, size);
	do {
#if 1
		dest[0] = FPMult(v[0], m[0]) + FPMult(v[1], m[1]) + FPMult(v[2], m[2]);
		dest[1] = FPMult(v[0], m[3]) + FPMult(v[1], m[4]) + FPMult(v[2], m[5]);
		dest[2] = FPMult(v[0], m[6]) + FPMult(v[1], m[7]) + FPMult(v[2], m[8]);
#else
		dest[0] = FPMult(v[0], m[0]) + FPMult(v[0], m[1]) + FPMult(v[0], m[2]);
		dest[1] = FPMult(v[1], m[3]) + FPMult(v[1], m[4]) + FPMult(v[1], m[5]);
		dest[2] = FPMult(v[2], m[6]) + FPMult(v[2], m[7]) + FPMult(v[2], m[8]);
#endif
//kprintf("%s %lx (%ld,%ld,%ld) -> %lx (%ld,%ld,%ld)\n", __FUNCTION__, v, v[0], v[1], v[2], dest, dest[0], dest[1], dest[2]);
		if (--n <= 0) {
			break;
		}

		v = (R3D_PPosVector)((dword)v + size);
		dest = (R3D_PPosVector)((dword)dest + size);
	} while(1);
}

void R3D_Add3DVector(R3D_PPosVector dest, R3D_PPosVector v1, int n, int size)
{
	//if (n == 0) kprintf("%s:%ld\n", __FUNCTION__, __LINE__);
	//kprintf("%s(%lx, %lx, %ld, %ld)\n", __FUNCTION__, dest, v1, n, size);
	do {
		dest[0] += v1[0];
		dest[1] += v1[1];
		dest[2] += v1[2];
//kprintf("%s:%ld %lx (%ld,%ld,%ld) -> %lx (%ld,%ld,%ld)\n", __FUNCTION__, __LINE__, v1, v1[0], v1[1], v1[2], dest, dest[0], dest[1], dest[2]);
		if (--n <= 0) {
			break;
		}

		dest = (R3D_PPosVector)((dword)dest + size);
	} while(1);
}

void R3D_Project3D(R3D_PProjPos dest, R3D_PPosVector v, int n, int size1, int size2)
{
	//if (n == 0) kprintf("%s:%ld\n", __FUNCTION__, __LINE__);
	//kprintf("%s(%lx, %lx, %ld, %ld, %ld)\n", __FUNCTION__, dest, v, n, size1, size2);
	do {
		sint32 ebp = v[2];
		if (ebp < 60) ebp = 60;
		dest[0] =  FPMultDiv(v[0], R3D_FocusX, ebp) + R3D_CenterX;
		dest[1] = -FPMultDiv(v[1], R3D_FocusY, ebp) + R3D_CenterY;
		//kprintf("%s:%ld %lx (%ld,%ld,%ld) -> %lx (%ld,%ld)\n", __FUNCTION__, __LINE__, v, v[0], v[1], v[2], dest, dest[0], dest[1]);
		if (--n <= 0) {
			break;
		}

		v = (R3D_PPosVector)((dword)v + size2);
		dest = (R3D_PPosVector)((dword)dest + size1);
	} while(1);
}
