# pragma once

#include <string.h>
typedef enum {UnknownModality=0, CT, MR, NM, PET} ImageModality;
static ImageModality modality_code(const char *s)
{
	if (strncasecmp(s,"CT",2)==0) return CT;
	if (strncasecmp(s,"MR",2)==0) return MR;
	if (strncasecmp(s,"NM",2)==0) return NM;
	if (strncasecmp(s,"PT",2)==0) return PET;
	if (strncasecmp(s,"PET",3)==0) return PET;
	return UnknownModality;
}
