# 
# Frame-based motion correction reconstruction csh script
# Author: Merence Sibomana (Sibomana@gmail.com) 
# Example with 5 frames
set em=M_EM
set ref=1
set nf=12
set TX=M_TX
set no=norm_jan_09_no_bb.n
# For Windows
# set cald = C:\CPS\calibration_files
set cald = $HOME/data/calibration_files
#set hrrt_osem3d=hrrt_osem3d_v1.2_x64.exe
set hrrt_osem3d=hrrt_osem3d_v1.2_x64.exe

# FAST UW-OSEM reconstruction(10sec/frame on dual quadcore computer)  of uncorrected images in 128x128 
# of all frames 
rm -f normfac.i
$hrrt_osem3d -t $em.dyn -n $no -X 128 -o $em"_na.v" -W 0 -I 4 -S 8 -L osem3d.log

# Gaussian smoothing of all frames
gsmooth_u  $em"_na.v" 6

# Rescale smoothed images to same unscaled intensity
# AIR uses unscaled short data matrix, threshold with 7000 by default
set im_smo=$em"_na_6mm"
mat_reslice -i $im_smo -o $im_smo"_rescaled.v" -u $TX.i -t 7000 -T 0.03
set im_smo=$im_smo"_rescaled.v" 

# AIR alignment of smoothed frames to reference frame (first frame with more than 10M trues)
set f=1
while ($f <= $nf)
  if ($f != $ref) then
     ecat_alignlinear $im_smo.v,$ref,1,1 $im_smo.v,$f,1,1 $im_smo"_fr"$f.air -m 6 
     ecat_reslice $im_smo"_fr"$f.air $im_smo"_rsl.v,"$f,1,1 -k -o
   else
     matcopy -i $im_smo.v,$ref,1,1 -o $im_smo"_rsl.v,"$ref,1,1
   endif
   @ f++
   echo frame $f done
end
echo alignment done
#Sum aligned frames reference and up  into %im_smo%_sum.v
sum_frames $im_smo.v $ref

# Use Vinci to align mu-map with sum image
# Not much motion between mu and reference frame; use measured mu-map.

# Reconstruction of corrected images using aligned mu-map
set f=1
set i=0
set LB=12
rm -f normfac.i
while ($f <= $nf)
  set TXA = $TX"_fr"$f.air
  set TXF = $TX"_fr"$f
  set EMF = $em"_frame"$i
  if ($f != $ref) then
    ecat_invert_air $im_smo"_fr"$f.air $TXA  y
    ecat_reslice $TXA $TXF.i  -a $TX.i -o -k
  else
    set TXF = $TX
  endif
  e7_fwd_u --model 328 -u $TXF.i --oa $TXF.a --span 9 --mrd 67 --prj ifore --force -l 53,log
  e7_sino_u -a $TXF.a -u $TXF.i  -e $EMF.tr.s -n $no --lber $LB --force --os $EMF"_sc.s" --os2d --gf --model 328 --skip 2 --mrd 67 --span 9 --ssf 0.25,2 -l 73,log -q QC
  cd QC
  pgnuplot scatter_qc_00.plt
  mv scatter_qc_00.ps $EMF"_sc_qc.ps"
  cd ..
  $hrrt_osem3d -p $EMF.s -n $no -a $TXF.a -s $EMF"_sc.s" -o $EMF.i -W 3 -d $EMF.ch -I 10 -S 16 -m 9,67 -B 0,0,0 
  @ f++
  @ i++
end

# Conversion on non-aligned frames to ECAT
if2e7 -v -e 7.3E-6 -u Bq/ml -S $cald $em"_frame0.i"

# Smooth reconstructed images
gsmooth_u $em.v 6
set im_smo = $em"_6mm"

# Alignment of reconstructed images
set f=1
while ($f<= $nf)
  if ($f != $ref) then
 
     ecat_alignlinear $im_smo.v,$ref,1,1 $im_smo.v,$f,1,1 $im_smo"_fr"$f.air -m 6
     ecat_reslice $im_smo"_fr"$f.air $em"_rsl.v,"$f,1,1 -a $em.v,$f,1,1 -k -o
   else
     matcopy -i $em.v,$ref,1,1 -o  $em"_rsl.v,"$ref,1,1
   endif
   @ f++
end
