/*
   * EcatX software is a set of routines and utility programs
   * to access ECAT(TM) data.
   *
   * Copyright (C) 2008 Merence Sibomana
   *
   * Author: Merence Sibomana <sibomana@gmail.com>
   *
   * ECAT(TM) is a registered trademark of CTI, Inc.
   * This software has been written from documentation on the ECAT
   * data format provided by CTI to customers. CTI or its legal successors
   * should not be held responsible for the accuracy of this software.
   * CTI, hereby disclaims all copyright interest in this software.
   * In no event CTI shall be liable for any claim, or any special indirect or 
   * consequential damage whatsoever resulting from the use of this software.
   *
   * This is a free software; you can redistribute it and/or
   * modify it under the terms of the GNU Lesser General Public License
   * as published by the Free Software Foundation; either version 2
   * of the License, or any later version.
   *
   * This software is distributed in the hope that it will be useful,
   * but  WITHOUT ANY WARRANTY; without even the implied warranty of
   * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   * GNU Lesser General Public License for more details.

   * You should have received a copy of the GNU Lesser General Public License
   * along with this software; if not, write to the Free Software
   * Foundation, 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
#include <assert.h>
#include <malloc.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include "ecat_matrix.hpp"
#include "isotope_info.h"

static void usage() {
	fprintf (stderr,"fix_mh %s %s \n", __DATE__, __TIME__);
	fprintf (stderr,
    "usage : \n"
    "fix_mh -i input_file  -c ecf -w patient_weight -h patient_height -s patient_sex\n"
    "       -d dose -D delay -I isotope_name -R radiopharmacentical -t tilt -f n_frames\n"
    "       -p n_planes -A anonymity -v\n");
	fprintf (stderr," Updates main header ... \n");
	fprintf (stderr," -c ecat_calibration_factor, sets calibration units to Bq/ml\n");
	fprintf (stderr," -w patient_weight [kg]\n");
	fprintf (stderr," -h patient height [cm]\n");
	fprintf (stderr," -d injected_dose [mCi]\n");
	fprintf (stderr," -D delay between injection and emission scan (hh:mm:ss)\n");
	fprintf (stderr," -t tilt [degrees]\n");
	fprintf (stderr," -I Isotope name chosen amongst the following: \n");
	fprintf (stderr,"	Br-75 C-11 Cu-62 Cu-64 F-18 Fe-52 Ga-68 N-13\n");
	fprintf (stderr,"	O-14 O-15 Rb-82 Na-22 Zn-62 Br-76 K-38\n");
	fprintf (stderr," -R Radiopharmaceutical: eg. FDG \n");
	fprintf (stderr," -f number of frames \n");
	fprintf (stderr," -F Facility name \n");
	fprintf (stderr," -p number of planes \n");
	fprintf (stderr," -B initial_bed_pos\n");
	fprintf (stderr," -b reference_file for initial_bed_pos and offsets\n");
	fprintf (stderr," -s patient sex :M,F or U\n");
	fprintf (stderr," -M Modality(CT|MR|NM|PET)\n");
	fprintf (stderr," -N patient_name\n");
	fprintf (stderr," -S scan_start_time (dd/mm/yyyy hh:mm:ss)\n");
	fprintf (stderr," -A anonymity sets patient_id, patient_name, study_name and original_file_name to provided argument\n");
	fprintf (stderr,"Version 8 : Nov 07, 2005\n");
	exit(1);
}
int main( argc, argv)
  int argc;
  char** argv;
{
	char *input_file=NULL;
	char *reference_file=NULL;
	char *patient_sex=NULL;
	char *sex_code=NULL;
	char *scan_time=NULL;
	MatrixFile *mptr, *mptr1;
	Main_header* mh;
  char facility_name[20];
	float ecf,weight,height,dose,branching_fraction,tilt;
	int i,nframes, nplanes;
	struct IsotopeInfoItem* isotope_info=NULL;
	char* radiopharmaceutical=NULL;
	char *modality=NULL;
	char *patient_name=NULL, *anonymity=NULL;
	int stype = 0, update_stype=0;
	int hh=0,mm=0,ss=0;
	time_t t;
	int c, j = 0, verbose = 0, calibrate = 0, update_weight = 0, update_height = 0, update_dose = 0, update_delay = 0, update_sex=0; 
	int update_isotope = 0, update_radiopharmaceutical = 0, update_tilt = 0, update_nframes = 0, update_nplanes = 0, update_bed = 0, update_modality=0;
	int update_bed_pos=0;
	int count,count1;
	float *bed_offsets, *bed_offsets1, bed_pos=0;
	extern int optind, opterr;
	extern char *optarg;
	extern int getopt(int argc, char **argv, char *opts);

    memset(facility_name,0,sizeof(facility_name));

    while ((c = getopt (argc, argv, "i:c:w:h:d:D:t:I:M:N:R:f:F:p:b:s:S:B:A:T:v")) != EOF) {

	switch (c) {
		case 'i' :
			input_file = optarg;
			break;
		case 'c' :
			if (sscanf(optarg,"%g",&ecf) != 1)
				crash("error decoding -c %s\n",optarg);
			calibrate = 1;
			break;
		case 'w' :
			if (sscanf(optarg,"%g",&weight) != 1)
				crash("error decoding -w %s\n",optarg);
			update_weight = 1;
			break;
		case 'h' :
			if (sscanf(optarg,"%g",&height) != 1)
				crash("error decoding -h %s\n",optarg);
			update_height = 1;
			break;
		case 'd' :
			if (sscanf(optarg,"%g",&dose) != 1)
				crash("error decoding -d %s\n",optarg);
			update_dose = 1;
			break;
		case 'D' :
			if (sscanf(optarg,"%d:%d:%d",&hh,&mm,&ss) != 3)
				crash("error decoding -D %s\n",optarg);
			update_delay = 1;
			break;
		case 't' :
			if (sscanf(optarg,"%g",&tilt) != 1)
				crash("error decoding -t %s\n",optarg);
			update_tilt = 1;
			break;
		case 'T' :
			if (sscanf(optarg,"%d",&stype) != 1)
				crash("error decoding -T %s\n",optarg);
			update_stype = 1;
			break;

		case 'I' :
      if ( (isotope_info = get_isotope_info(optarg)) == NULL) {
        fprintf(stderr, "ERROR: illegal isotope name: -i %s\n\n",optarg);
        return 1;
      }
      printf("isotope name: %s; isotope halflife: %g sec\n",
              isotope_info->name, isotope_info->halflife);
			update_isotope = 1;
			break;
		case 'M' :
			modality = optarg;
			update_modality = 1;
			break;
		case 'A' :
			anonymity = optarg;
			break;
		case 'N' :
			patient_name = optarg;
			break;
		case 'R' :
			radiopharmaceutical = optarg;
			update_radiopharmaceutical = 1;
			break;
		case 'f' :
			if (sscanf(optarg,"%d",&nframes) != 1)
				crash("error decoding -f %s\n",optarg);
			update_nframes = 1;
			break;
        case 'F': 
            strncpy(facility_name, optarg, sizeof(facility_name)-1);
            break;
		case 'p' :
			if (sscanf(optarg,"%d",&nplanes) != 1)
				crash("error decoding -p %s\n",optarg);
			update_nplanes = 1;
			break;
		case 'b' :
			reference_file = optarg;
			update_bed = 1;
			break;
		case 'B' :
			if (sscanf(optarg,"%g",&bed_pos) != 1)
				crash("error decoding -d %s\n",optarg);
			update_bed_pos = 1;
			break;
		case 'S' :
			scan_time = optarg;
			break;
		case 's' :
			sex_code = optarg;
			update_sex = 1;
			break;
		case 'v':
			verbose = 1;
			break;
		default:
			usage();
		}
	}

	if (input_file==NULL) usage();

	if (verbose) printf(" Input file : %s\n",input_file);

	mptr = matrix_open(input_file, ecat_matrix::MatrixFileAccessMode::OPEN_EXISTING, ecat_matrix::MatrixFileType_64::UNKNOWN_FTYPE);
	if (!mptr) crash( "%s: can't open file '%s'\n", argv[0], input_file);
	mh = mptr->mhptr;
	if( calibrate ) {
    if (verbose) {
      printf("  old calibration factor : %g\n",mh->calibration_factor);
      printf("  old calibration data units : %s\n",mh->data_units);
      printf("  old calibration units : %d\n",mh->calibration_units);
      printf("  old calibration units_label : %d\n",mh->calibration_units_label);
		}
    mh->calibration_factor = ecf;
		mh->calibration_units = 1;
		mh->calibration_units_label = 1;
    strcpy(mh->data_units,"Bq/ml");
	}
	if( update_dose ) {
	   	if (verbose) printf("  old dose : %g\n",mh->dosage);
	     	mh->dosage = dose;
	}
	if( update_weight ) {
	   	if (verbose) printf("  old patient weight : %g\n",mh->patient_weight);
	     	mh->patient_weight = weight;
	}
    if (strlen(facility_name)>0) {
	   	if (verbose) printf("  old facility name : %s\n",mh->facility_name);
        strncpy(mh->facility_name, facility_name, sizeof(mh->facility_name)-1);
    }

	if( update_sex ) {
	   	if (verbose) {
			if (mh->patient_sex[0]==0 || mh->patient_sex[0]=='M')
				printf("  old patient sex : M\n");
			else if (mh->patient_sex[0]==1 || mh->patient_sex[0]=='F')
				 printf("  old patient sex : F\n");
			else  printf("  old patient sex : U\n");
		}
		mh->patient_sex[0] = sex_code[0];
	}
	if( update_height ) {
	   	if (verbose) printf("  old patient height : %g\n",mh->patient_height);
	     	mh->patient_height = height;
	}
	if( update_tilt ) {
	   	if (verbose) printf("  old tilt : %g\n",mh->intrinsic_tilt);
	     	mh->intrinsic_tilt = tilt;
	}
	if( update_isotope ) {
    if (verbose) printf("  old Isotope : %s\n",mh->isotope_code);
    strncpy(mh->isotope_code,isotope_info->name,sizeof(mh->isotope_code)-1);
    mh->isotope_halflife = isotope_info->halflife;
    mh->branching_fraction = isotope_info->branch_ratio;
	}
	if( update_modality ) {
		char *modality_pos = mh->magic_number+sizeof(mh->magic_number)-4;
	   	if (verbose) printf("  old modality : %s\n",modality_pos);
		strncpy(modality_pos, modality, 3);
	}
	if (patient_name) {
		if (verbose) printf("  old patient name : %s\n", mh->patient_name);
		strncpy(mh->patient_name, patient_name, sizeof(mh->patient_name));
	}
	if (anonymity) {
		strncpy(mh->patient_id, anonymity, sizeof(mh->patient_id)-1);
		strncpy(mh->patient_name, anonymity, sizeof(mh->patient_name)-1);
		strncpy(mh->study_name, anonymity, sizeof(mh->study_name)-1);
		strncpy(mh->original_file_name, anonymity,
			sizeof(mh->original_file_name)-1);
	}
	if( update_radiopharmaceutical ) {
	   	if (verbose) printf("  old radiopharmaceutical : %s\n",mh->radiopharmaceutical);
		strncpy(mh->radiopharmaceutical,radiopharmaceutical,32);
	}
	if( update_nframes ) {
	   	if (verbose) printf("  old number of frames : %d\n",mh->num_frames);
	     	mh->num_frames = nframes;
	}
	if( update_nplanes ) {
	   	if (verbose) printf("  old number of planes : %d\n",mh->num_planes);
	     	mh->num_planes = nplanes;
	}
	if (scan_time) {
		struct tm tm;
		int y, d, M, h, m, s=0;
		if (sscanf(scan_time, "%d/%d/%d %d:%d:%d", &d, &M, &y, &h, &m, &s) < 5)
		 crash ("bad time format : %s\n",scan_time);
		assert(d>0 && d<=31);
		assert(M>0 && M<=12);
		assert(y>1999 && y<=2098);
		assert(h>0 && h<=24);
		assert(m>=0 && m<60);
		assert(s>=0 && s<60);
		tm.tm_year = y - 1900;
		tm.tm_mon = M - 1;		/* 0-11 */
		tm.tm_mday = d;
		tm.tm_hour = h;
		tm.tm_min = m;
		tm.tm_sec = s;
		tm.tm_isdst = -1;
#if defined(sun) && !defined(__SVR4)
		t = timelocal(&tm);
#else
		t = mktime(&tm);
#endif
		mh->dose_start_time = mh->scan_start_time = t;
		printf("scan_start_time(%d/%d/%d %d:%d:%d) = %s",y, d, M, h, m, s,ctime(&t));
	}

	if( update_bed ) {
		mptr1 = matrix_open(reference_file, ecat_matrix::MatrixFileAccessMode::READ_ONLY, ecat_matrix::MatrixFileType_64::UNKNOWN_FTYPE);
		if (!mptr1) crash( "%s: can't open reference file '%s'\n", argv[0], reference_file);
		count = mptr->mhptr->num_bed_pos;
		bed_offsets = mptr->mhptr->bed_offset;
		count1 = mptr1->mhptr->num_bed_pos;
		bed_offsets1 = mptr1->mhptr->bed_offset;
		if (verbose) {
			printf("Initial bed position %g (old) %g (new)\n",mptr->mhptr->init_bed_position,mptr1->mhptr->init_bed_position);
			for (i=0; i<count; i++)  printf("Position: %d Old offset %g\n",i,bed_offsets[i]);
			for (i=0; i<count1; i++) printf("Position: %d New offset %g\n",i,bed_offsets1[i]);
		}
		mptr->mhptr->num_bed_pos = mptr1->mhptr->num_bed_pos;
		mptr->mhptr->init_bed_position = mptr1->mhptr->init_bed_position;
		memcpy(mptr->mhptr->bed_offset,mptr1->mhptr->bed_offset,sizeof(mptr->mhptr->bed_offset));
	}
	if( update_delay) {
		t = mh->dose_start_time;
	   	if (verbose) {
			printf("  old dose start time : %s",ctime(&t));
			printf("  old delay since injection : %d sec\n",mh->scan_start_time - t);
		}
	    mh->dose_start_time = mh->scan_start_time - (hh*3600+mm*60+ss);
	}
	if( update_bed_pos ) {
	   	if (verbose) printf("  old Initial bed position: %g\n",mh->init_bed_position);
     	mh->init_bed_position = bed_pos;
	}
	if( update_stype ) {
	   	if (verbose) printf("  old system type: %g\n",mh->system_type);
     	mh->system_type = stype;
	}
	mat_write_main_header(mptr->fptr, mh);
	matrix_close(mptr);
	return(0);
}
