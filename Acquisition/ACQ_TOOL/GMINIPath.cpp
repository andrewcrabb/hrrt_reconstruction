#include <string.h>
#include "stdafx.h"
#include "GMINIPath.h"
int GMINIPath(const char *filename, CString &path)
{
	static char *GMINIDir=NULL;
	if (GMINIDir==NULL)
	{
		const char *v = getenv("GMINI");
		if (v == NULL)
		{
			path = filename;
			return 0; 
		}
		GMINIDir = _strdup(v);
		/* remove trailing '\' */
		if (GMINIDir[strlen(GMINIDir)-1] == '\\') 
			GMINIDir[strlen(GMINIDir)-1] = '\0'; 
  }
	path.Format("%s\\%s", GMINIDir, filename);
	return 1;
}
