// GantryInfo : Defines the entry point for the console application.
//
/*[ gantryinfo.h
  ------------------------------------------------

  Component       : HRRT

  Name            : gantryinfo.h - GantryInfo class interface
  Authors         : Merence Sibomana
  Language        : C++

  Creation Date   : 11/25/2003
  Description     :  GantryInfo class loads configuration table "GM328.INI" in memory
  and provides methods to get the values.

  Copyright (C) CPS Innovations 2003 All Rights Reserved.

  ---------------------------------------------------------------------*/

#include "gantryinfo.hpp"
#include "histogram_mp.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <iostream>

#define LINE_SIZE 256

using namespace std;
using namespace cps;

int GantryInfo::state = 0;

typedef map <string,string> InfoMap;
static InfoMap gantry_info;
static int current_model = -1;

/**
 * Loads the gmXXX.ini file from the directory specified by GMINI environment variable,
 * where XXX is the model_number.
 * Returns the table size on success fails, otherwise set the state to fail (-1) and returns -1.
 */
int GantryInfo::load(int model_number) {
  char filename[FILENAME_MAX] = "";
  // char msg[1000];
  char line[LINE_SIZE];
  const char *gmini_dir=NULL;
  FILE *fp=NULL;

  if (state < 0) 
    return state;
  if (state>0) {
    if (model_number == current_model) 
      return gantry_info.size();
    else
      gantry_info.clear();
  }
  if ((gmini_dir = getenv("GMINI")) != NULL) {
#ifdef unix
    sprintf(filename, "%s/gm%d.ini", gmini_dir, model_number);
#else
    sprintf(filename, "%s\\gm%d.ini", gmini_dir, model_number);
#endif
    cerr << "Using GMINI file " << filename << endl;
    fp = fopen(filename,"rb");
  } else {
    cerr << "ERROR: GantryInfo::load(): Envt var GMINI not set!" << endl;
    exit(-1);
  }
  if (fp == NULL) {
    cerr << "ERROR: GantryInfo::load(): Cannot read filename '" << filename << "'" << endl;
    state = -1;
    return state;
  }
  while (fgets(line,LINE_SIZE,fp) != NULL) {
    line[strlen(line)] = '\0'; //clear endl
    char *end = line + strlen(line);
    char *w1 = line;
    char *sep = strchr(line,'=');
    if (sep == NULL) 
      continue;
    while (isspace(*w1) && w1<sep) 
      w1++;
    if (w1==sep) 
      continue;
    char *p = sep-1;
    while (isspace(*p)) 
      *p-- = '\0';
    *sep = '\0';
    char *w2 = sep+1;
    while (isspace(*w2) && w2<end) 
      w2++;
    if (w2==end) 
      continue;
    end--;
    while (isspace(*end)) 
      *end-- = '\0';
    string key = w1, value = w2;
    gantry_info.insert(make_pair(key,value));
  }
  fclose(fp);
  state = 1;
  return gantry_info.size();
}

/**
 *	Get a string value
 * Returns 1 if found or 0 otherwise
 */
int GantryInfo::get(const char *key, string &value)
{
  string skey = key;
  InfoMap::iterator i = gantry_info.find(skey);
  if (i != gantry_info.end()) {
    value = i->second;
    return 1;
  }
  return 0;
}

/**
 *	Get an integer value
 *  Returns 1 if found or 0 otherwise
 */
int GantryInfo::get(const char *key, int &ivalue)
{
  string value;
  if (get(key, value)) {
    if (sscanf(value.c_str(),"%d",&ivalue) == 1) 
      return 1;
  }
  return 0;
}

/**
 *	Get an integer value
 *  Returns 1 if found or 0 otherwise
 */
int GantryInfo::get(const char *key, float &fvalue)
{
  string value;
  if (get(key, value)) {
    if (sscanf(value.c_str(),"%g",&fvalue) == 1) 
      return 1;
  }
  return 0;
}

int GantryInfo::get(const char *format, int index, string &value)
{
  char key[LINE_SIZE];
  sprintf(key, format, index);
  return get(key, value);
}

int GantryInfo::get(const char *format, int index, float &value)
{
  char key[LINE_SIZE];
  sprintf(key, format, index);
  return get(key, value);
}

int GantryInfo::get(const char *format, int index, int &value)
{
  char key[LINE_SIZE];
  sprintf(key, format, index);
  return get(key, value);
}
