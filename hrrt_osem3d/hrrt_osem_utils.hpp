#pragma once

bool get_scan_duration(const char *sino_fname);
int file_open(char *fn,char *mode);
void file_close(int fp);
int get_num_frames(const char *em_file);
