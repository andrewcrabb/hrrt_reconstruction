#include <stdio.h>
#include <iostream>

/* get_scan_duration  
read: sinogram header and sets scan_duration global variable.
Returns 1 on success and 0 on failure.
*/
bool get_scan_duration(const char *sino_fname) {
  char hdr_fname[_MAX_PATH];
  char line[256], *p=NULL;
  bool found = false;
  FILE *fp;
  sprintf(hdr_fname, "%s.hdr", sino_fname);

  if ((fp=fopen(hdr_fname,"rt")) != NULL) {
    while (!found && fgets(line, sizeof(line), fp) != NULL)
      if ((p=strstr(line,"image duration")) != NULL) {
        if ((p=strstr(p,":=")) != NULL) 
          if (sscanf(p+2,"%g",&scan_duration) == 1) 
          	found = true;
      }
      fclose(fp);
  }
  return found;
}

int file_open(char *fn,char *mode){
	int ret;
	if(mode[0]=='r' && mode[1]=='b'){
		ret= open(fn,O_RDONLY|O_DIRECT); //|O_DIRECT);
		return ret;
	}
}

void file_close(int fp){
	close(fp);
}

int get_num_frames(const char *em_file) {
  int frame=0;
  char *ext;
  strcpy(em_file_prefix, em_file);
  if ((ext=strstr(em_file_prefix,".dyn")) == NULL) return 0;
  *ext = '\0';
  strcpy(em_file_postfix,".tr");
  // allocate strings for frame dependent sinogram and image file names
  true_file = (char*)calloc(_MAX_PATH, 1);
  out_img_file = (char*)calloc(_MAX_PATH, 1);
  sprintf(true_file,"%s_frame%d%s.s",em_file_prefix,frame,em_file_postfix);
  if (access(true_file,R_OK) != 0) {// try without postfix
    em_file_postfix[0] = '\0'; 
    sprintf(true_file,"%s_frame%d%s.s",em_file_prefix,frame,em_file_postfix);
  }
  while (access(true_file,R_OK) == 0) {
    frame++;
    sprintf(true_file,"%s_frame%d%s.s",em_file_prefix,frame,em_file_postfix);
  }
  if (frame>0) // allocate frame dependent image file name
    out_img_file = (char*)calloc(_MAX_PATH, 1);
  else  // restore sinogram file name
    strcpy(true_file, em_file);
  return frame;
}

