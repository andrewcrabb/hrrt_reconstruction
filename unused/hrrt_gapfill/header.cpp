#include <fstream>
#include "header.h"
#include <iostream>
#include <string>
using namespace std;

int constructheader(char* inheader, char* outheader, HeaderInfo header)
{
  ifstream in;
  ofstream out;
  string line;

  // Output image
  in.open(inheader, ios::in);
  if (!in.is_open())
  {
    cerr << "Error: could not open file " << inheader << endl;
    return 1;
  }

  // Output image
  out.open(outheader, ios::out);
  if (!out.is_open())
  {
    cerr << "Error: could not open file " << outheader << endl;
    in.close();
    return 1;
  }

  // Construct header
  out << "!INTERFILE" << endl;
  out << "!name of data file := " << header.correctedname << endl;
  out << "data format := sinogram" << endl;
  out << "number format := float" << endl;
  out << "number of bytes per pixel := 4" << endl;
  out << "matrix size [1] := " << header.nelems << endl;
  out << "matrix size [2] := " << header.nangles << endl;
  out << "matrix size [3] := " << header.nplanes << endl;
  out << "prompt file used := " << header.prompt << endl;
  out << "delayed file used := " << header.delayed << endl;
  out << "norm file used := " << header.norm << endl;
  if (strcmp(header.atten,""))
    out << "atten file used := " << header.atten << endl;
  if (strcmp(header.scatter,""))
    out << "scatter file used := " << header.scatter << endl;
  out << "gap filling strategy used := ";
  switch(header.gapfill)
  {
    case 0:
    {
      out << "none" << endl;
    } break;
    case 1:
    {
      out << "linear interpolation" << endl;
    } break;
    case 2:
    {
      out << "constrained Fourier space" << endl;
    } break;
    default:
    {
      out << "unknown method" << endl;
    }
  }
  // Get information from old header
  while(getline(in, line))
  {
  	if(line.compare(0, 10, "!INTERFILE") == 0)
  	{
  		// Do nothing!
  	}
    else if (line.compare(0, 11, "data format") == 0)
    {
  		// Do nothing!
    }
    else if (line.compare(0, 11, "matrix size") == 0)
    {
  		// Do nothing!
    }
    else if (line.compare(0, 25, "number of bytes per pixel") == 0)
    {
  		// Do nothing!
    }
    else if (line.compare(0, 13, "number format") == 0)
    {
  		// Do nothing!
    }
    else if (line.compare(0, 18, "!name of data file") == 0)
    {
  		// Do nothing!
    }
    else
    {
      out << line << endl;
    }
  }

  // Copy entire header
  //out << in.rdbuf();

  // Close files;
  in.close();
  out.close();

  return 0;
}
