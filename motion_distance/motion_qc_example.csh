set dir="./"
set em=$dir"M_EM"
set no=$dir"norm_jan_09_no_bb.n"
set im=$em"_na"
set smo=6
set im_smo=$im"_"$smo"mm"
set nframes = 12
echo $dir 
#
# Reconstruct all frames and save images in ECAT format (single file)
# There is a bug in linux multi-threaded version when using -N or -X 128, one thread is used.
#
#hrrt_osem3d -t $em.dyn -o $im.v -n $no  -W 0 -X 128 -I 4 -S 8 -N -T 1
#
# Smooth all frames in ECAT file
#
gsmooth $im.v $smo
#
# Use AIR to align all frames to frame 1 and create transformer files
set frame = 1
while (${frame} < ${nframes})
	@ frame++
	ecat_alignlinear $im_smo".v,"$frame",1,1" $im_smo".v,1,1,1" $frame"to1.air" -m 6 -t1 2000 -t2 2000
	echo frame $frame alignment done
end
#
# print motion amplitude for each transformer file
#
set frame = 1
while (${frame} < ${nframes})
	@ frame++
	motion_distance -a $frame"to1.air"
end
