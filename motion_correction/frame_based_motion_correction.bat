REM
REM Frame-based motion correction reconstruction 
REM Author Merence Sibomana
REM Author Sune H. Keller
REM Example: 36 frames scan with frame 16 as reference frame
REM 
set em=hal-2009.12.03.12.02.59_em
set /A ref=16
set /A nf=36
set TX=hal-2009_12_03_11_34_44_TX
set no=D:\SCS_SCANS\norm\Norm_2010-02-03_CBN_Span9.n
REM SK: added v 1.2. osem3d
set hrrt_osem3d=hrrt_osem3d_v1.2_x64.exe

REM FAST UW-OSEM reconstruction(10sec/frame on dual quadcore computer)  of uncorrected images in 128x128 of all frames 
hrrt_osem3d_v1.2_x64 -t %em%.dyn -n %no% -X 128 -o %em%_na.v -W 0 -I 4 -S 8 -N -L osem3d.log


REM Gaussian smoothing of all frames
REM echo gsmooth_u  %em%_na.v 6
REM SK: assuming echo just write and doesn't envoke:
gsmooth_u  %em%_na.v 6

REM Rescale smoothed images to same unscaled intensity
REM AIR uses unscaled short data matrix, threshold with 7000 by default
set im_smo=%em%_na_6mm
mat_reslice -i %im_smo% -o %im_smo%_rescaled.v -u %TX%.i -t 7000 -T 0.03
set im_smo=%im_smo%_rescaled.v 


REM AIR alignment of smoothed frames to reference frame (first frame with more than 10M trues)

set im_smo=%em%_na_6mm
set /A f=1


:NEXT1
if %f%==%ref% (goto REF_FRAME1)
REM SK: assuming echo just write and doesn't envoke, remove REM:
REM echo ecat_alignlinear %im_smo%.v,%ref%,1,1 %im_smo%.v,%f%,1,1 %im_smo%_fr%f%.air -m 6 
ecat_alignlinear  %im_smo%.v,%ref%,1,1 %im_smo%.v,%f%,1,1 %im_smo%_fr%f%.air -m 6 
REM SK: assuming echo just write and doesn't envoke, remove REM:
REM echo ecat_reslice %im_smo%_fr%f%.air %im_smo%_rsl.v,%f%,1,1 -k -o
ecat_reslice %im_smo%_fr%f%.air %im_smo%_rsl.v,%f%,1,1 -k -o
if %f%==%nf% goto END1
set /A f=f+1
goto NEXT1

:REF_FRAME1
REM echo matcopy -i %im_smo%.v,%ref%,1,1 -o %im_smo%_rsl.v,%ref%,1,1
REM SK: assuming echo just write and doesn't envoke, remove REM:
matcopy -i %im_smo%.v,%ref%,1,1 -o %im_smo%_rsl.v,%ref%,1,1
REM SK: correction? Not goto END but goto END1:
if %f%==%nf% (goto END1)
set /A f=f+1
goto NEXT1

:END1
echo alignment done

REM Sum aligned frames reference and up 
REM echo sum_frames %im_smo%.v %ref%
REM SK: assuming echo just write and doesn't envoke, remove REM:
sum_frames %im_smo%.v %ref%

REM Use Vinci to align mu-map with sum image
REM Not much motion between mu and reference frame; use measured mu-map.

REM Reconstruction of corrected images using aligned mu-map
set /A f=1
set /A i=0
set LB=12
REM LBER is 6 for sept 09 norm...

:NEXT2
if %f%==%ref% (goto REF_FRAME2)
set TXA=%TX%_fr%f%.air
set TXF=%TX%_fr%f%
set EMF=%em%_frame%i%
REM SK: invert_air to ecat_invert_air:
REM echo ecat_invert_air %im_smo%_fr%f%.air %TXA% y
ecat_invert_air %im_smo%_fr%f%.air %TXA%  y
REM SK: add -o and -k arguments as in csh version:
REM echo ecat_reslice %TXA% %TXF%.i  -a %TX%.i -o -k
ecat_reslice %TXA% %TXF%.i  -a %TX%.i -o -k
goto recon
:REF_FRAME2
set TX_F=%TX%

:recon
REM MAKE sure QC and log directories exist -- create them manually if they are not there!
REM SK: keeping -w 128 in both e7_fwd_u and e7_sino_u (both removed in csh-version) correct?:
e7_fwd_u --model 328 -u %TXF%.i -w 128 --oa %TXF%.a --span 9 --mrd 67 --prj ifore --force -l 53,log
e7_sino_u -a %TXF%.a -u %TXF%.i -w 128 -e %EMF%.tr.s -n %no% --lber %LB% --force --os %EMF%_sc.s --os2d --gf --model 328 --skip 2 --mrd 67 --span 9 --ssf 0.25,2 -l 73,log -q QC
REM SK: Adding 4 lines for QC plot as in csh-version, ok?:
cd QC
pgnuplot scatter_qc_00.plt
move scatter_qc_00.ps %EMF%_sc_qc.ps
cd ..


hrrt_osem3d_v1.2_x64 -p %EMF%.s -n %no% -a %TXF%.a -s %EMF%_sc.s -o %EMF%.i -W 3 -d %EMF%.ch -I 10 -S 16 -m 9,67 -B 0,0,0  
if %f%==%nf% goto END2
set /A f=f+1
set /A i=i+1
goto NEXT2

:END2

REM Conversion to ECAT
if2e7 -v -e 7.3E-6 -u Bq/ml -S C:\CPS\calibration_files %em%_frame0.i

REM SK: smoothing as in csh-version and new im_smo:
REM Smooth reconstructed images
gsmooth_u %em%.v 6
set im_smo = %em%_6mm


REM Alignment of reconstructed images
set /A f=1

:NEXT3
if %f%==%ref% (goto REF_FRAME3)
REM SK: add ecat_alignlinear as in csh-version:
ecat_alignlinear %im_smo%.v,%ref%,1,1 %im_smo%.v,%f%,1,1 %im_smo%_fr%f%.air -m 6 
REM SK: Adding -k and -o options as in csh-version:
ecat_reslice %im_smo%_fr%f%.air %em%_rsl.v,%f%,1,1 -a %em%.v,%f%,1,1 -k -o
if %f%==%nf% goto END
set /A f=f+1
goto NEXT3

:REF_FRAME3
echo matcopy -i %em%.v,%ref%,1,1 -o  %em%_rsl.v,%ref%,1,1
matcopy -i %em%.v,%ref%,1,1 -o  %em%_rsl.v,%ref%,1,1
if %f%==%nf% (goto END)
set /A f=f+1
goto NEXT3

:END


