#ifndef sincos_h
#define sincos_h
#include <math.h>
#ifdef win32

static void sincos(double theta, double *sintheta, double *costheta)
{
	*sintheta = sin(theta);
	*costheta = cos(theta);
}
#endif /* win32 */
#endif /* sincos_h */

