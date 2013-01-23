/*----------------------------------------------------------------------------------------------

  TV 3D regularization of HRRT TX.i's (unsegmented)

  By Sune Høgild Keller & Merence Sibomana, HRRT Users Community, Rigshospitalet, Copenhagen
  24-09-2008
  Modification History
  27-oct-2008: Changed THRESHOLD from 0.08 to 0.07
  07-Sept-2010: Display removed for linux compile (for Arman Rahmim)

  ------------------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <CImg.h>
#define THRESHOLD 0.07f
#define MU_TISSUE 0.099f

using namespace cimg_library;

inline float sqr(float x) {return x*x;}

int main(int argc,char **argv) {
  cimg_usage("3DTV for HRRT TX");
  const char* file_in = cimg_option("-i",(char*)0,"Input image file");
  const char* file_out = cimg_option("-o",(char*)0,"Output image file");

  double lam = cimg_option("-lam",0.5, "Lambda is smoothing weight");
  int fpiters = cimg_option("-fpi",2,"number of outer fixed point iterations");
  int riters = cimg_option("-ri",3,"number of inner relaxation iterations");
	
  // Display out: bool display = cimg_option("-display",0,"Display option, 1 for on, 0 for off");

  static float voxel[1];

  double thres = 1e-7;			// Convergence threshold
  int conv_count = 1;				// Counts how many times Conv. has been reached
  float eps2 = sqr(0.01);	// Origo differentiability reg., prevents division by 0

  unsigned nx = 128, ny = 128, nz = 207; //HRRT TX dimensions

  // Reading TX.i file into u0 
  CImg<float> u0 = CImg<float>(nx,ny,nz,1,0.0);
  if (file_in == NULL) return 1;
  FILE *fp1 = fopen(file_in,"rb");
  cimg_forXYZ(u0,x,y,z){
    fread(voxel, 1, sizeof(float), fp1);
    u0(x,y,z) = voxel[0];
  }
  fclose(fp1);
		
  CImg<float> u = u0;		// input data is also initialization of output data

  // Replace underestimated tissue pixels by tissue mu-value
  cimg_forXYZ(u0,x,y,z){
    if (u0(x,y,z)>THRESHOLD && u0(x,y,z)<MU_TISSUE ) u(x,y,z) = MU_TISSUE;
    /* 
       if (u0(x,y,z)>THRESHOLD && u0(x,y,z)<MU_TISSUE )
       {
       float avg9 = (
       u0(x-1,y,z) + u0(x,y,z) + u0(x+1,y,z) +  // current line
       u0(x-1,y-1,z) + u0(x,y-1,z) + u0(x+1,y-1,z) + // previous line
       u0(x-1,y+1,z) + u0(x,y+1,z) + u0(x+1,y+1,z) // next line
       )/ 9.0f;
       if (avg9>THRESHOLD) u(x,y,z) = MU_TISSUE;
       }
    */
  }
  u0 = u;


  CImg<float> Be(nx, ny, nz,1);//fixed coeff. east  --> CImg loops will handle boundary conditions
  CImg<float> Bw(nx, ny, nz,1);//fixed coeff. west
  CImg<float> Bn(nx, ny, nz,1);//fixed coeff. north
  CImg<float> Bs(nx, ny, nz,1);//fixed coeff. south
  CImg<float> Bb(nx, ny, nz,1);//fixed coeff. before
  CImg<float> Ba(nx, ny, nz,1);//fixed coeff. after


  // Display for the result and progress, initalized with the input image
  // Display out: CImgDisplay main_disp(u0,"Restored image",3,3,0,1);
  // Display out: CImg<unsigned char> p_im(400,200,1,3,0);
  // Display out: CImgDisplay p_disp(p_im, "Progress",3,3,0,1);
  // Display out: unsigned char white[3]={255,255,255};
  // Display out: unsigned char red[3]={255,0,0};
  // Display out: if(display){
  // Display out: 	main_disp.show();
  // Display out: 	p_disp.show();
  // Display out: 	p_disp.move(500,100);
  // Display out: }
  //if(!display){
  //	main_disp.close();
  //	p_disp.close();
  //}


  // Defining the loop
  CImg_3x3x3(I,float);

  // fixed point outer loop
  for (int i=1; i<=fpiters; i++){
    // looping the image to calculate fixed nonlinear coefficients
    cimg_for3x3x3(u,x,y,z,0,I){


      //	uc  = u(x,y,z); Iccc		ub  = u(x,y,z-1);Iccp			ua  = u(x,y,z+1);Iccn
      //	uw  = u(x-1,y,z); Ipcc		ubw = u(x-1,y,z-1);Ipcp			uaw = u(x-1,y,z+1);Ipcn
      //	ue  = u(x+1,y,z);Incc		ube = u(x+1,y,z-1);Incp			uae = u(x+1,y,z+1);Incn
      //	us  = u(x,y-1,z);Icnc		ubs = u(x,y-1,z-1);Icnp			uas = u(x,y-1,z+1);Icnn
      //	un  = u(x,y+1,z);Icpc		ubn = u(x,y+1,z-1);Icpp			uan = u(x,y+1,z+1);Icpn
      //		
      //	usw = u(x-1,y-1,z);Ipnc
      //	unw = u(x-1,y+1,z);Ippc
      //	use = u(x+1,y-1,z);Innc
      //	une = u(x+1,y+1,z);Inpc
      //		
      //		Bw(x,y,z) = 1/sqrt(sqr(uw-uc) + 0.0625*sqr(un-us+unw-usw) + 0.0625*sqr(ua-ub+uaw-ubw) + eps2);
      //		Be(x,y,z) = 1/sqrt(sqr(ue-uc) + 0.0625*sqr(un-us+une-use) + 0.0625*sqr(ua-ub+uae-ube) + eps2);
      //		
      //		Bs(x,y,z) = 1/sqrt(sqr(us-uc) + 0.0625*sqr(ue-uw+use-usw) + 0.0625*sqr(ua-ub+uas-ubs) + eps2);
      //		Bn(x,y,z) = 1/sqrt(sqr(un-uc) + 0.0625*sqr(ue-uw+une-unw) + 0.0625*sqr(ua-ub+uan-ubn) + eps2);

      //		Bb(x,y,z) = 1/sqrt(sqr(ub-uc) + 0.0625*sqr(ue-uw+ube-ubw) + 0.0625*sqr(un-us+ubn-ubs) + eps2);
      //		Ba(x,y,z) = 1/sqrt(sqr(ua-uc) + 0.0625*sqr(ue-uw+uae-uaw) + 0.0625*sqr(un-us+uan-uas) + eps2);

      Bw(x,y,z) = 1.0/sqrt(sqr(Ipcc-Iccc) + 0.0625*sqr(Icpc-Icnc+Ippc-Ipnc) + 0.0625*sqr(Iccn-Iccp+Ipcn-Ipcp) + eps2);
      Be(x,y,z) = 1.0/sqrt(sqr(Incc-Iccc) + 0.0625*sqr(Icpc-Icnc+Inpc-Innc) + 0.0625*sqr(Iccn-Iccp+Incn-Incp) + eps2);
			
      Bs(x,y,z) = 1.0/sqrt(sqr(Icnc-Iccc) + 0.0625*sqr(Incc-Ipcc+Innc-Ipnc) + 0.0625*sqr(Iccn-Iccp+Icnn-Icnp) + eps2);
      Bn(x,y,z) = 1.0/sqrt(sqr(Icpc-Iccc) + 0.0625*sqr(Incc-Ipcc+Inpc-Ippc) + 0.0625*sqr(Iccn-Iccp+Icpn-Icpp) + eps2);

      Bb(x,y,z) = 1.0/sqrt(sqr(Iccp-Iccc) + 0.0625*sqr(Incc-Ipcc+Incp-Ipcp) + 0.0625*sqr(Icpc-Icnc+Icpp-Icnp) + eps2);
      Ba(x,y,z) = 1.0/sqrt(sqr(Iccn-Iccc) + 0.0625*sqr(Incc-Ipcc+Incn-Ipcn) + 0.0625*sqr(Icpc-Icnc+Icpn-Icnn) + eps2);
    }

    // inner realxation loops
    for (int j = 1; j<=riters; j++){
			
      double residual = 0;

      cimg_for3x3x3(u,x,y,z,0,I){
        // compute the updated value
        // u(x,y,z) = (lam*(bw*uw + be*ue + bs*us + bn*un + bb*ub + ba*ua) +u0c)/(1+ lam*(bw+be+bs+bn+ba+bb));
        float unew = (lam*(Bw(x,y,z)*Ipcc + Be(x,y,z)*Incc + Bs(x,y,z)*Icnc + Bn(x,y,z)*Icpc + Bb(x,y,z)*Iccp + Ba(x,y,z)*Iccn) +u0(x,y,z))/(1+ lam*(Bw(x,y,z)+Be(x,y,z)+Bs(x,y,z)+Bn(x,y,z)+Ba(x,y,z)+Bb(x,y,z)));
        // accumulation for convergence check
        residual += sqr(u(x,y,z)-unew);
        // okay to store updated value now
        u(x,y,z) = unew;
			
      }
      // display result and progress
      // Display out: if(display){
      // Display out: 	u.display(main_disp);
      // Display out: 	p_im.fill(0).draw_text(10,40,white,0,12,1,"Fixed point = %i",i);
      // Display out: 	p_im.draw_text(10,54,white,0,12,1,"Relax = %i",j);
      // Display out: 	p_im.draw_text(10,10,white,0,12,1,"Regularizing the file:");
      // Display out: 	p_im.draw_text(10,22,white,0,12,1,file_in);
      // Display out: 	p_im.display(p_disp);
      // Display out: }
      // compute and check residual error to see if we have convergence
      double r = sqrt(residual)/(nx*ny*nz);
      if (r < thres) {
        // Display out: 	if(display){
        // Display out: 		p_im.draw_text(10,70,red,0,12,1,"Conv. threshold reached %i", conv_count);				
        // Display out: 		p_im.draw_text(10,82,red,0,12,1,"times");				
        // Display out: 		p_im.display(p_disp);
        // Display out: 	}
        conv_count++;
        break;}
    }
  }

  if(file_out == NULL){
    file_out = file_in;
    // Should create saying name from file_in and parameter settings
    // And make renamed copy of the *.i.hdr file
  }
  FILE *fp2 = fopen(file_out,"wb");
  cimg_forXYZ(u0,x,y,z){
    voxel[0] = u(x,y,z);
    fwrite(voxel, 1, sizeof(float), fp2);
  }
  fclose(fp1);


  // Display out: if(display){
  // Display out: 	p_im.draw_text(10,100,white,0,12,1,"Output saved as:");
  // Display out: 	p_im.draw_text(10,112,white,0,12,1,file_out);
  //p_im.draw_text(10,124,white,0,12,1,"Remember to copy and rename image header.");
  // Display out: 	p_im.display(p_disp);
  // Display out: 	p_disp.wait(5000);
  // Display out: }

	

  return 0;
}

