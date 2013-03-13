#include "stdafx.h"
#include "QuanImage.h"

#include "header.h"

int QuantifyImage(float *img)
{
	CHeader sinoHdr, imgHdr, qnHdr;
	long avgSPB, duration,x, tsize, inc=0;
	float dtc = 1.f, calFactor=1.f, decfac = 1.f,plnEff[207],reco_method = 1.f;
	int err, i, cnt=0;
	char sinoHeaderName[1024], t[1024];

	sprintf(sinoHeaderName, "%s.hdr",m_Em_File);
	err = sinoHdr.OpenFile(sinoHeaderName, 1);
	if (err >=0)
	{
		if(sinoHdr.Readfloat("Dead time correction",&dtc) != 0)
			dtc = 1.f;
		if(sinoHdr.Readlong("image duration",&duration) != 0)
			duration = 1;
		if(sinoHdr.Readlong("average singles per block", &avgSPB) != 0)
			avgSPB = 1;
		if(sinoHdr.Readfloat("decay correction factor",&decfac) != 0)
			decfac = 1.f;

		sinoHdr.CloseFile();
	}
	else
	{
		// Display error
		return err;
	}
	
	err = qnHdr.OpenFile((char *)m_Quan_Header, 1);
	if(err >=0)
	{
		if (span == 3)
			qnHdr.Readfloat("OSEM3D span 3",&reco_method);
		else if (span == 9)
			qnHdr.Readfloat("OSEM3D span 9",&reco_method);	
		}

		if(qnHdr.Readfloat("calibration factor",&calFactor) != 0)
			calFactor = 1.f;
		for (i=0;i<m_izsize;i++)
		{
			sprintf(t,"efficient factor for plane %d",i);
			if(qnHdr.Readfloat(t,&plnEff[i]) != 0)
				plnEff[i] = 1.f;
		}
		qnHdr.CloseFile();
	}
	else
		return err;
	// Calculate 
	if (reco_method <=0)
		reco_method = 1.0;
	if (dtc <= 0)
		dtc = (float)exp((double)lld*(double)avgSPB);
	tsize = m_ixsize * m_iysize * m_izsize ;

	for (x=0;x<tsize;++x)
	{
		img[x] = img[x] * reco_method * dtc*calFactor*plnEff[cnt]/(float)duration;
		inc++;
		if (inc >= m_ixsize * m_iysize)
		{
			cnt++;
			inc = 0;
		}

	}

	return 0;
}

void WriteImageHeader()
{
	char sinoHeaderName[1024], t[1024], ImgHeaderName[1024];
	FILE  *in_file, *out_file;
	std::string tag;
	printf("Writing image header file \n" );
	// write image header
	sprintf(sinoHeaderName, "%s.hdr",emission_filename.c_str());
	sprintf(ImgHeaderName, "%s.hdr",ssr_img_file_name.c_str());
	in_file = fopen(sinoHeaderName,"r");
	out_file = fopen(ImgHeaderName,"w");
	
	if ((in_file != NULL) &&  (out_file != NULL))
	{
		while (fgets(t,1024,in_file) != NULL )
		{		
			tag.assign(t);
			//  Print the data file name
			if(tag.find("name of data file")!=-1 )
				fprintf( out_file, "name of data file := %s\n", ssr_img_file_name.c_str());
			else if(tag.find("scan data type description [1]")!=-1 )
				fprintf( out_file, "scan data type description [1] := corrected trues\n");
			else if(tag.find("matrix size [2]")!=-1 )
				fprintf( out_file, "matrix size [2] := %d\n", m_iysize);		
			else if(tag.find("matrix size [3]")!=-1 )
				fprintf( out_file, "matrix size [3] := %d\n", m_izsize);
			else if(tag.find("data format")!=-1 )
				fprintf( out_file, "data format := image\n");
			else if(tag.find("number format")!=-1 )
				fprintf( out_file, "number format := float\n");
			else if(tag.find("number of bytes per pixel")!=-1 )
				fprintf( out_file, "number of bytes per pixel := 4\n");
			else
				fprintf( out_file, "%s", tag.c_str());
			
		}
	}
	else
	{
		if (out_file != NULL)
		{
			fprintf( out_file, "name of data file := %s\n", ssr_img_file_name.c_str());
			fprintf( out_file, "matrix size [1] := %d\n", m_ixsize);		
			fprintf( out_file, "matrix size [2] := %d\n", m_iysize);		
			fprintf( out_file, "matrix size [3] := %d\n", m_izsize);
			fprintf( out_file, "matrix size [3] := %d\n", m_izsize);
			fprintf( out_file, "data format := image\n");
			fprintf( out_file, "number format := float\n");
			fprintf( out_file, "number of byte per pixel := 4\n");

		}
		
	}
	if (out_file != NULL)
	{
		fprintf( out_file, "normalization file name used := %s\n", norm_file.c_str());
		fprintf( out_file, "3D attenuation file name used := %s\n", atten_file.c_str());
		fprintf( out_file, "emission file name used := %s\n", emission_filename.c_str());
		fprintf( out_file, "scatter file name used := %s\n", Scatter_file.c_str());
		fprintf( out_file, "quantification header file name used := %s\n", Quan_Header_Filename.c_str());

		switch (ssroption)
		{
		case 1:
			fprintf( out_file, "method of reconstruction used := SSR/DIFT\n");
			break;	
		case 2:
			fprintf( out_file, "method of reconstruction used := FORE/DIFT\n");
			break;
		case 3:
			fprintf( out_file, "method of reconstruction used := FORE/OSEM\n");
			break;
		}
		//fprintf( out_file, "calibration factor := %f\n", calFactor);
		
		fclose(out_file);
	}
	if (in_file != NULL)
		fclose(in_file);
	
}