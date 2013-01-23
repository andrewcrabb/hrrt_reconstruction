#include "norm_globals.h"
#include <stdio.h>

void plot(const char *basename, const float *y, int size, const char *title)
{
	FILE *fp = fopen(basename, "wb");
	if (fp != NULL)
	{
		if (title != NULL) fprintf(fp, "# %s\n", title);
		for (int x=0; x<size; x++) fprintf(fp, "%d\t%g\n", x+1, y[x]);
		fclose(fp);
	}
}

void plot(const char *basename, const float *y, int count, int size, const char *title)
{
	FILE *fp = fopen(basename, "wb");
	if (fp != NULL)
	{
		if (title != NULL) fprintf(fp, "# %s\n", title);
		for (int x=0; x<size; x++) 
		{
			for (int i=0; i<count; i++)
				fprintf(fp, "\t%g", y[i*size + x]);
			fprintf(fp, "\n");
		}
		fclose(fp);
	}
}
void plot(const char *basename, const float *x, const float *y, int size,
		  const char *title)
{
	FILE *fp = fopen(basename, "wb");
	if (fp != NULL)
	{
		if (title != NULL) fprintf(fp, "# %s\n", title);
		for (int i=0; i<size; i++) fprintf(fp, "%g\t%g\n", x[i], y[i]);
		fclose(fp);
	}
}

void plot(const char *basename,  const float *y1, const float *y2, int count, int size,
		  const char *title)
{
	FILE *fp = fopen(basename, "wb");
	if (fp != NULL)
	{
		if (title != NULL) fprintf(fp, "# %s\n", title);
		for (int x=0; x<size; x++) 
		{
			for (int i=0; i<count; i++)
				fprintf(fp, "\t%g\t%g", y1[i*size + x], y2[i*size + x] );
			fprintf(fp, "\n");
		}
		fclose(fp);
	}
}
