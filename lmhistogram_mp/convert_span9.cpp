/*
Modification history:
          20-MAY-2009: Use a single fast LUT rebinner
*/
#include <stdio.h>
#include <stdlib.h>
#include <gen_delays_lib/geometry_info.h>

/*
  int convert_span9(short *sino, int t_maxrd)
  converts in place span3 to span 9 sinogram
*/
int convert_span9(short *sino, int t_maxrd, int nrings) {
  // int iseg = 0
  // int seg = 0, 
  int span3 = 3, span9 = 9;
  int nseg = (2 * t_maxrd + 1) / span3;  // span 3 # egments
  int nplns0 = 2 * nrings - 1;    // Segment 0 #planes
  // int ntotal = nseg * nplns0 - (nseg - 1) * (span3 * (nseg - 1) / 2 + 1);

  int iseg_9 = 0, nseg_9 = (2 * t_maxrd + 1) / span9;
  // int offset = 0;
  int npixels =  m_nprojs * m_nviews;
  int j = 0, nvoxels = npixels * nplns0;
  short *in_p = NULL, *out_p = NULL;
  // short *in_data = NULL;
  short *out_data_neg = NULL;
  short *out_data_pos = NULL;

  int *seg_span9 = (int*)calloc(nseg, sizeof(int)); // destination in span9 mode

  int *nplns_span9 = (int*)calloc(nseg, sizeof(int)); // #planes foreach segment in span9 mode
  int *nplns = (int*)calloc(nseg, sizeof(int));   // #planes foreach segment in span3 mode
  int *move = (int*)calloc(nseg, sizeof(int));     // plane shift foreach segment from span3 to span9

  for (int iseg = 0; iseg < nseg; iseg++)
    seg_span9[iseg] = (iseg + 3) / 6;
  for (int iseg = 0; iseg < nseg; iseg++)
    nplns[iseg] = nplns_span9[iseg] = 207;


  for (int iseg = 0; iseg < nseg; iseg++) {
    int seg = (iseg + 1) / 2;
    if (seg > 0 )
      nplns[iseg] = 2 * nrings - 3 * (2 * seg - 1) - 2; // other elements  initialized to 207
    if (seg > 1)
      nplns_span9[iseg] = 2 * nrings - 9 * (2 * seg_span9[iseg] - 1) - 2; // other elements  initialized to 207
    if (seg > 0 )
      move[iseg] = (nplns_span9[iseg] - nplns[iseg]) / 2; // other elements initialized to 0 with calloc

  }

  // span 9 sino build by pair (-seg,+seg) except for segment 0
  int iseg = 0;
  int span3_pos = 0, span9_pos = 0;
  while (iseg_9 < nseg_9) {
    int out_nplanes = nplns[iseg];
    if (iseg_9 == 0) {
      out_p = sino;  // segment 0
      // Add first negative span3 segment to segment 0
      iseg++;
      span3_pos += nplns[iseg - 1];
      // printf("Add Span3 Segment %d pos=%d into span9 segment %d pos=%d nplanes=%d\n",
      //  -(iseg + 1) / 2, span3_pos, -(iseg_9 + 1) / 2, move[iseg], nplns[iseg]);
      in_p = sino + span3_pos * npixels;
      out_p = sino + move[iseg] * npixels;
      nvoxels = npixels * nplns[iseg];
      for (j = 0; j < nvoxels; j++) out_p[j] += in_p[j];
      // Add first positive span3 segment to segment 0
      iseg++;
      // printf("Add Span3 Segment %d pos=%d into span9 segment %d pos=%d nplanes=%d\n",
      //  (iseg + 1) / 2, span3_pos, -(iseg_9 + 1) / 2, move[iseg], nplns[iseg]);
      span3_pos += nplns[iseg - 1];
      in_p = sino + span3_pos * npixels;
      for (j = 0; j < nvoxels; j++)
        out_p[j] += in_p[j];
      span9_pos = out_nplanes;
      iseg_9 += 1; // next span9 segment
      iseg++;
    } else {
      // Copy first negative span3 segment
      span3_pos += nplns[iseg - 1];
      out_data_neg = sino + span9_pos * npixels;
      // printf("Copy Span3 Segment %d pos=%d into span9 segment %d pos=%d nplanes=%d\n",
      //  -(iseg + 1) / 2, span3_pos, -(iseg_9 + 1) / 2, 0, nplns[iseg]);
      in_p = sino + span3_pos * npixels;
      nvoxels = npixels * nplns[iseg];
      for (j = 0; j < nvoxels; j++)
        out_data_neg[j] = in_p[j];
      iseg++;

      // Copy first positive span3 segment
      span3_pos += nplns[iseg - 1];
      out_data_pos = out_data_neg + out_nplanes * npixels;
      // printf("Copy Span3 Segment %d pos=%d into span9 segment %d pos=%d nplanes=%d\n",
      //  (iseg + 1) / 2, span3_pos, (iseg_9 + 1) / 2, 0, nplns[iseg]);
      in_p = sino + span3_pos * npixels;
      nvoxels = npixels * nplns[iseg];
      for (j = 0; j < nvoxels; j++)
        out_data_pos[j] = in_p[j];
      iseg++;

      // Add next negative span3 segment
      span3_pos += nplns[iseg - 1];
      // printf("Add Span3 Segment %d pos=%d into span9 segment %d pos=%d nplanes=%d\n",
      //  -(iseg + 1) / 2, span3_pos, -(iseg_9 + 1) / 2, move[iseg], nplns[iseg]);
      out_p = out_data_neg + move[iseg] * npixels;
      in_p = sino + span3_pos * npixels;
      nvoxels = npixels * nplns[iseg];
      for (j = 0; j < nvoxels; j++)
        out_p[j] += in_p[j];
      iseg++;

      // Add next positive span3 segment
      span3_pos += nplns[iseg - 1];
      // printf("Add Span3 Segment %d pos=%d into span9 segment %d pos=%d nplanes=%d\n",
      //  (iseg + 1) / 2, span3_pos, (iseg_9 + 1) / 2, span9_pos+out_nplanes+move[iseg], nplns[iseg]);
      out_p = out_data_pos + move[iseg] * npixels;
      in_p = sino + span3_pos * npixels;
      nvoxels = npixels * nplns[iseg];
      for (j = 0; j < nvoxels; j++)
        out_p[j] += in_p[j];
      iseg++;

      // Add next negative span3 segment
      span3_pos += nplns[iseg - 1];
      // printf("Add Span3 Segment %d pos=%d into span9 segment %d pos=%d nplanes=%d\n",
      //  -(iseg + 1) / 2, span3_pos, -(iseg_9 + 1) / 2, span9_pos+move[iseg], nplns[iseg]);
      out_p = out_data_neg + move[iseg] * npixels;
      in_p = sino + span3_pos * npixels;
      nvoxels = npixels * nplns[iseg];
      for (j = 0; j < nvoxels; j++)
        out_p[j] += in_p[j];
      iseg++;

      // Add next positive span3 segment
      span3_pos += nplns[iseg - 1];
      // printf("Add Span3 Segment %d pos=%d into span9 segment %d pos=%d nplanes=%d\n",
      //  (iseg + 1) / 2, span3_pos, (iseg_9 + 1) / 2, span9_pos+out_nplanes+move[iseg], nplns[iseg]);
      out_p = out_data_pos + move[iseg] * npixels;
      in_p = sino + span3_pos * npixels;
      nvoxels = npixels * nplns[iseg];
      for (j = 0; j < nvoxels; j++)
        out_p[j] += in_p[j];
      iseg++;
      span9_pos += 2 * out_nplanes;
      iseg_9 += 2; // next span9 segment
    }
  }
  free(seg_span9);
  free(nplns_span9);
  free(nplns);
  free(move);
  return 1;
}
