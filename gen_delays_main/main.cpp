// gen_delays.cpp : Defines the entry point for the console application.
/* Authors: Inki Hong, Dsaint31, Merence Sibomana
  Creation 08/2004
  Modification history: Merence Sibomana
	10-DEC-2007: Modifications for compatibility with windelays.
  02-DEC-2008: Bug fix in searching hrrt_rebinner.lut path (library gen_delay_lib)
  02-DEC-2008: Added sw_version 1.0.1
  02-Jun-2009: Changed sw_version to 1.1
  21-JUL-2010: Bug fix in gen_delays_lib
  21-JUL-2010: Change sw_version to 1.2

*/
#include "gen_delays.h"
static const char *sw_version = "HRRT_U 1.2";
int main(int argc, char* argv[])
{
	printf("\ngen_delays, Version %s, build %s %s\n\n",sw_version, __DATE__,__TIME__);
	gen_delays(argc-1, argv+1, 0, 0.0f, NULL, NULL, NULL);
	return 0;
}

