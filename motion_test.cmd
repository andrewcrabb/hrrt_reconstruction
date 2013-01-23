#!/bin/csh

set HRRT=/home/ahc/BIN/src/hrrt_codes_Arman
foreach dir (lmhistogram_mp libraries TX_TV3DReg je_hrrt_osem3d gen_delays_main motion_correction if2e7)
    if ( -d ${HRRT}/${dir} ) set path=($path:q ${HRRT}/${dir})
end

set E7_TOOLS=${HRRT}/e7_tools_HRRT
foreach dir (e7_atten_prj e7_fwd_prj e7_sino_prj)
    if ( -d ${E7_TOOLS}/${dir} ) set path=($path:q ${E7_TOOLS}/${dir})
end

# Always needed for proper scatter calculation, etc. It defined geometry of the HRRT:
setenv GMINI '/home/rahmim/ForAndy/gm328_HRRT'
set SUBJ=/tmp/SMALL_SHAWN_100630_162257
set in_dir=${SUBJ}
set em=SMALL-SHAWN-22777-2010.6.30.16.22.57_EM  # Without .l64
set blank=Scan-Blank-8882-2010.6.30.6.50.19_TX  # Without .s
set TX=SMALL-SHAWN-28955-2010.6.30.16.54.29_TX  # Without .s
set norm=/data/recon/norm/norm_090605_090917_9.n
set ErgRatio=14 #Andy: please make sure calibration_factor.txt file exists with correct scaling value (scaled down by 3.10 for this span 9 recon)
set f=0  # Start frame (index starts from 0)
set out_dir=${SUBJ}/temp
set maxframe_n=2 # End frame # to reconstruct

mkdir $out_dir
cd $out_dir
mkdir log
mkdir QC

set step_randoms=0 # Always keep 0; Because in new release, random correction is done inside recon (saves time and space).

# u9 type recon for 256 and 128 size images (the latter is used for motion estimation)
set step_histogram=1
set step_mu_map=1
set step_TX_TV=1
set step_atten=1
set step_sensitivity_image=1
set step_scatter=1
set step_recon=1
set step_if2e7=1
# Motion estimation followed by motion correction 
set motion_correct=1
set final_motion_comparison=1

echo 'ErgRatio is:' $ErgRatio
echo $em
  
if ($step_histogram == 1) then
    lmhistogram_mp ${in_dir}/$em.l64 -span 9 -PR -l log/ -o $em.s
endif

# Creat mu-map

#if ( $phantom == 1) then
#.... Condition to be added later for phantoms
#endif

if ( $step_mu_map == 1) then
    echo 'performing mu_map estimation...'
    e7_atten_u --model 328 --ucut -b ${in_dir}/$blank.s -t ${in_dir}/$TX.s --txblr 1.0 --uzoom 2,1 -w 128 --force --ou $TX.i --ualgo 40,1.5,30,0.,0 -l 73,log -q QC --txsc -0.1,1.18
    echo 'Done performing mu_map estimation...'
    cp $TX.i $TX"_before_TV3D".i
    cp $TX.h33 $TX.i.hdr
endif

if ( $step_TX_TV == 1) then
    TX_TV3DReg -i ${TX}_before_TV3D.i -o $TX.i
    ##echo 'finished with mu-map'
    cp $TX.h33 $TX.i.hdr  # Later programs refer to .i.hdr: .h33 is produced to we need to copy
    ll $TX.i
endif

## Create attenuation map
if ( $step_atten == 1) then
    echo 'generating atten map ...'
    e7_fwd_u --model 328 -u $TX.i  -w 128 --oa $TX.a --span 9 --mrd 67 --prj ifore --force -l 73,log
endif

# Sensitivity calculation: this step is new because Inki Linux code is slightly different than windows code in proper generation of sensitivity maps
set normfac256="normfac256.i"
set normfac128="normfac128.i"
set normfac128_noAtten="normfac128_noAtten.i"

if ( $step_sensitivity_image == 1) then
    echo 'computing sensitivity image...'
    rm $normfac256
    je_hrrt_osem3d -t $em"_frame"$f.s -n $norm -a $TX.a -o "test.i" -W 2 -I 1 -S 16 -m 9,67 -B 0,0,0 -T 4 -v 8 -X 256 -K $normfac256
endif

while ($f <= $maxframe_n)
    set EMF = $em"_frame"$f
    if ( $step_scatter == 1) then   
        e7_sino_u -a $TX.a -u $TX.i -w 128 -e $EMF.tr.s -n $norm  --lber $ErgRatio --force --os $EMF"_sc.s" --os2d --gf --model 328 --skip 2 --mrd 67 --span 9 -l 73,log -q QC --ssf 0.25,2 --athr 1.03,4
    endif

    # Random estimation (no need to run, as this is done inside hrrt_osem3d below)
    if ( $step_randoms == 1) then
        echo 'random estimation...'
        gen_delays -s 9,67 -h $EMF.ch -O $EMF"_ra_smo.s" -t 1 -S crytal_singles.dat -v
    endif

    if ( $step_recon == 1) then
        # PLEASE NOTE: With -B option, -T 8 does not work well. So keep -T to 4. 
        set I=10
        set I2=6 #For 128 recon

        # 256 with atten/scatter
        je_hrrt_osem3d -p $EMF.s -n $norm -a $TX.a -s ${EMF}_sc.s -o ${EMF}_HRRTcode_MAP0_u9B.i -W 3 -d $EMF.ch -I $I -S 16 -m 9,67 -B 0,0,0 -T 4 -v 8 -X 256 -K $normfac256

        # 128 without atten/scatter
        rm normfac.i # Needs to recalculate this inside (not from before!) and also with -T 4 not -T 2
        je_hrrt_osem3d -p $EMF.s -n $norm -o ${EMF}_HRRTcode_MAP0_u9B_noAttennoScatter128B.i -W 3 -d $EMF.ch -I $I2 -S 16 -m 9,67 -B 0,0,0 -T 4 -v 8 -X 128 #-K $normfac128_noAtten
    endif
    # Do not use any smoothing for PSF reconstructions
    @ f++
end

if ( $step_if2e7 == 1) then
    echo 'if2e7 ...'
    # calibration_factor.txt should be in local folder for now
    if2e7 -v -e 6.67E-6 -u Bq/ml -g 0 -s ${in_dir}/calibration_factor.txt $em"_frame0_HRRTcode_MAP0_u9B".i
    if2e7 -v -e 6.67E-6 -u Bq/ml -g 0 -s ${in_dir}/calibration_factor.txt $em"_frame0_HRRTcode_MAP0_u9B_noAttennoScatter128B".i
    echo '... Done with if2e7'
endif

if ( $motion_correct == 1) then
    #set ref_frame=17
    set v_file_name_128=$em"_HRRTcode_MAP0_u9B_noAttennoScatter128B".v
    echo 'Starting motion qc'
    motion_qc $v_file_name_128 -v -O -R 0 #-a 1.03,4 #-r $ref_frame

    set dyn_file_name=$em.dyn
    ls - $dyn_file_name
    echo 'lber value is:' $ErgRatio
    echo 'Starting motion correction'
    # Before invoking this, motion_qc above must be invoked above
    motion_correct_recon $dyn_file_name -n $norm -u $TX.i -v -L $ErgRatio -P -E $v_file_name_128 -a 1.03,4 -K $normfac256 -O -s ${in_dir}/calibration_factor.txt #-t  #-r $ref_frame

    # Step below already done in program above
    ####if2e7 -e 6.67E-6 -u Bq/ml -g 0 -s ${in_dir}/calibration_factor.txt $em"_frame0_3D_ATX".i
endif

if ( $final_motion_comparison == 1) then
    # Post-recon motion correction (the not-so-good way, but commonly used by all)
    set v_file_name=$em"_HRRTcode_MAP0_u9B".v
    motion_qc $v_file_name -v -O -R 1

    # Checking that post-recon MC kind of works:
    set v_file_name=$em"_HRRTcode_MAP0_u9B_rsl".v
    motion_qc $v_file_name -v -O -R 0 

    # Checking that inter-recon MC (the right way) works:
    set v_file_name=$em"_3D_ATX_rsl".v
    motion_qc $v_file_name -v -O -R 0
endif
