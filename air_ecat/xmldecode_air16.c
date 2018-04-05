/* Copyright 1995 Roger P. Woods, M.D. */
/* Modified: 12/11/95 */

/****************************************************************/
/* int read_air16						*/
/*								*/
/* reads an AIR reslice parameter file into an air struct	*/
/*								*/
/* program will also attempt to convert old 12 parameter	*/
/* AIR files into new 15 parameter files			*/
/* 								*/
/* returns:							*/
/*	1 if successful						*/
/*	0 if unsuccessful					*/
/****************************************************************/

#include <stdio.h>
#include <string.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include "ecat2air.h"
#include <expat/expat.h>

#include	<unistd.h>

typedef enum {CLASS_UNKNOWN=0, CLASS_REFERENCE=1, CLASS_RESLICE} ClassCode;
typedef struct _ClassNameCode { ClassCode code; char *name; } ClassNameCode;
static ClassNameCode class_name_codes[] = 
{
	CLASS_REFERENCE, "Reference",
	CLASS_RESLICE, "Reslice",
	0, NULL
};


static char *magic_number = "<AIR_Transformer>\n";
static char *end_number = "\n</AIR_Transformer>\n";
static int end_tag = '>';
static char current_dir[_MAX_PATH];

static struct AIR_Air16 air;

static unsigned class_id(const char *name)
{
	ClassNameCode *class_name_code = class_name_codes;
	while (class_name_code->name != NULL)
	{
	 	if (strcasecmp(class_name_code->name,name) == 0)
			 return class_name_code->code; 
		 class_name_code++;
	}
	return CLASS_UNKNOWN;
}

static void AIR_end_element(void *UserData, const char *name) {
}

static void AIR_element_data(void *userData, const char* s, int len) { 
}


static const char* get_attr(const char* name,	const char **atts)
{ int i=0;
	for (i=0; atts[i] != NULL; i += 2)
		if (strcmp(name,atts[i]) == 0) return atts[i+1];
	 return NULL;
}

static void AIR_start_element(void *userData, const char* name, const char **atts)
{
  int i,j;
	float a1,a2,a3,a4,a5,a6;
  char filename[_MAX_PATH];
  int f_ok;
	const char *v=NULL, *sv=NULL;

  switch(class_id(name)) 
  {
  case CLASS_REFERENCE:
    if ((v = get_attr("filename", atts))) {
      strcpy(filename,v);
      if ((f_ok=access((char*)filename, F_OK)) == -1) { 
        // not absolute path, try current directory
        sprintf(filename, "%s%c%s",current_dir,DIR_SEPARATOR,v);
        f_ok =  access((char*)filename, F_OK); 
      }
      if (f_ok == -1)  crash("Reference : invalid filename"); 
      strcpy(air.r_file, filename);
    } else crash("Reference: missing filename");
    if ((v = get_attr("dimension", atts))) {
      if (sscanf(v,"%d,%d,%d", &air.s.x_dim,&air.s.y_dim,&air.s.z_dim) != 3)
        crash("Invalid Reference voxelsize: %s", v);
    }
    if ((v = get_attr("voxelsize", atts))) {
      if (sscanf(v,"%g,%g,%g", &air.s.x_size,&air.s.y_size,&air.s.z_size) != 3)
        crash("Invalid Reference voxelsize: %s", v);
    }
    break;
  case CLASS_RESLICE:
    if ((v = get_attr("filename", atts))) {
      strcpy(filename,v);
      if ((f_ok=access((char*)filename, F_OK)) == -1) { 
        // not absolute path, try current directory
        sprintf(filename, "%s%c%s",current_dir,DIR_SEPARATOR,v);
        f_ok =  access((char*)filename, F_OK);
      }
      if (f_ok == -1)  crash("Reslice: invalid filename"); 
        strcpy(air.r_file, filename);
    } else crash("Reslice: missing filename");

    memset(air.comment, 0, sizeof(air.comment));
    if ((v = get_attr("param", atts))) {
      if (sscanf(v,"%g,%g,%g,%g,%g,%g",
        &a1,&a2,&a3,&a4,&a5,&a6) == 6) strcpy(air.comment,v);
      else crash("Invalid Reslice param: %s");
    }
    if ((v = get_attr("matrix", atts))) {
      sv = v;
      for (j=0; j<4; i++) {
        for (i=0; i<4 && sv!=NULL; i++) {
          if (sscanf(sv,"%g",&air.e[i][j]) != 1) 
            crash("Invalid Reslice matrix: %s (%s)", v, sv); 
          if ((sv=strchr(sv,',')) != NULL) sv++;
        }
        if (i !=4 || j!= 4) 
          crash("Invalid Reslice matrix: %s : too short", v); 
      }
    }
    break;	
  default:	// skip unkown tags
    break;
  }
}


struct AIR_Air16 *xmldecode_air16(const char *filename)
{
  char *buf=NULL, *p=NULL;
	FILE *fp=NULL;
	int buf_size=BUFSIZ;
	int done=0;
	int cc, len = (int)strlen(magic_number);
	int num_frames = 0;
	struct stat st;
  XML_Parser parser;

	if (filename == NULL || strlen(filename) == 0)
		crash("Null or blank filename");
	if (stat(filename, &st) < 0)	crash(strerror(errno));
	if (buf_size<(st.st_size+1)) buf_size = st.st_size+1; 
	if ((fp = fopen(filename,"rb")) == NULL) crash(strerror(errno));
	buf = (char*)calloc(1,buf_size);
	if (fread(buf,len, 1, fp) != 1) crash(strerror(errno));
	buf[len+1] = '\0';
	if (strcmp(buf, magic_number) != 0)  crash("Invalid magic number");
	strcpy(current_dir,filename);
  if ((p = strrchr(current_dir, DIR_SEPARATOR)) == NULL) strcpy(current_dir, ".");
  else  *p = '\0';
	parser = XML_ParserCreate(NULL);
	XML_SetElementHandler(parser, AIR_start_element, AIR_end_element); //_end_element);
	XML_SetCharacterDataHandler(parser, AIR_element_data); //_element_data);
	cc = st.st_size;
	do
	{
		len = (int)fread(buf, 1, cc, fp);
		done = (len <= cc);
		buf[len+1] = '\0';
		cc -= len;
		if (!XML_Parse(parser, buf, len, done))
		{
			fclose(fp);
			crash(XML_ErrorString(XML_GetErrorCode(parser)));
		}
	} while (!done);
	fclose(fp);
	free(buf);
	XML_ParserFree(parser);
	return &air;
}
