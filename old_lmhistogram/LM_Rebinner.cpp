/*
   Component       : HRRT

   Name            : LM_Rebinner.cpp - Listmode Reader thread interface
   Authors         : Merence Sibomana
   Language        : C++
   Platform        : Microsoft Windows.
   Description     : 
		 *	void lm_rebinner(void *arg=NULL): 64-bit listmode rebinner thread
		 *  The argument is not used and may be NULL
		 *  The thread has 2 states:
		 *  1. The producer thread (lm64_reader) is busy; waits for ready packets and sorts them.
		 *     It returns if no packet has been ready after MAX_TRY*TIME_SLICE.
		 *  2. Sorts all remaining packets.
					  
   Copyright (C) CPS Innovations 2004 All Rights Reserved.

   Creation Date   : 04/25/2004
   Modification history:
					12/11/2004: Added max_rebinners variable for initial number of rebinners
					12/21/2004: The first frame may be done before all rebinner are intialized,
								Use Sleep and check for L32 container initialization rather than
								lm64_consumer_wait which may timeout because of first frame writing.
          20-MAY-2009: Use a single fast LUT rebinner
          04-AUG-2009: Bug fix segment computation was wrong (segment -1 empty)
          25-MAY-2010: Bug fix crystal_depth initialized to LSO/LYSO 1cm
*/
#include "stdafx.h"
#include "LM_Writer.h"
#include "LM_Rebinner.h"
#include "histogram.h"
#include "gantryinfo.h"
#include <iostream>
#include <math.h>
#include <stdlib.h>
#include <gen_delays_lib/lor_sinogram_map.h>
#include <gen_delays_lib/segment_info.h>
#include <gen_delays_lib/geometry_info.h>
#include <gen_delays_lib/gen_delays.h>

using namespace std;
using namespace cps;
int model_number = MODEL_HRRT;
const char *rebinner_lut_file=NULL;

enum { LSO_LSO=0,LSO_NAI=1, LSO_ONLY=2, LSO_GSO=3, LSO_LYSO=4 } HeadType;
int tx_span=21;
static int em_span=9;

void lm64_rebinner(void *)
{
	int next64_pos=0, next32_pos=0;
	int num_try = 0;

	//
	// Initialize 32-bit destination packets
	//

  try 
  {
    L32EventPacket::packet_size = L64EventPacket::packet_size;
    l32_container = new L32EventPacket[num_packets];
  }
  catch (const char *err_msg)
  {
    cerr <<  err_msg << endl;
    delete[] l32_container;
    return;
  }

	if (verbose) cout << "lm64_rebinner " << " started" << endl;
	next64_pos = next32_pos = 0;
	L32EventPacket::file_status |= L32EventPacket::ready; // notify consumer
	while (L64EventPacket::file_status&L64EventPacket::ready ||
		l64_container[next64_pos].status != L64EventPacket::empty) 
	{
		while (l64_container[next64_pos].status == L64EventPacket::empty)
		{ 
			// Terminate this thread if lm64_reader has terminatated.
			// When multiple threads, lm64_reader can be terminated while 
			// this thread is in this loop.
			if ((L64EventPacket::file_status&L64EventPacket::ready) == 0)
			{
        // notify consumer: set done status and signal container ok
        L32EventPacket::file_status = L32EventPacket::done; 
        SetEvent(L32EventPacket::container_ok);
				if (verbose) cout <<"Rebinner "  " done" << endl;
				return;
			}
			if ((L64EventPacket::file_status&L64EventPacket::hold) == 0)
			{
				if (verbose) cout << "lm64_rebinner " << ": L64 container empty, wait at " << next64_pos << endl;
				if (lm64_consumer_wait() != WAIT_OBJECT_0) 
				{
					// check if not holding and continue if on hold
					if ((L64EventPacket::file_status&L64EventPacket::hold) != 0) continue;
					if (verbose) cout << "lm64_rebinner " << ": No producer at " << next64_pos  << endl;
//					return;
				}
			} 
			else
			{
				if (verbose) cout << "lm64_rebinner " << " hold at " << next64_pos << endl;
				WaitForSingleObject(sinowrite_ok, MAX_FILEWRITE_WAIT);
			}
		}

		// find next L32 empty packet
		while (l32_container[next32_pos].status != L32EventPacket::empty)
		{
			if ((L32EventPacket::file_status&L32EventPacket::hold)== 0)
			{
				if (verbose) cout << "lm64_rebinner " << " : L32 container full, wait at " << next32_pos << endl;
				if (lm32_producer_wait() != WAIT_OBJECT_0) 
				{
					if (verbose) cout << "lm64_rebinner: No consumer  at " << next32_pos << endl;
//					return;
				}
			}
			else
			{ 
				if (verbose) cout << "L32EventPacket::hold" << endl;
				WaitForSingleObject(sinowrite_ok, MAX_FILEWRITE_WAIT);
			}
		}

		// Sort 64 packet into found empty lm32 packet
		if (verbose) cout <<"lm64_rebinner " << ": sort L64-packet " << next64_pos << " status=" << l64_container[next64_pos].status <<
			" into L32-packet " << next32_pos << endl;
		rebin_packet(l64_container[next64_pos], l32_container[next32_pos]);
		if (verbose) cout <<"lm64_rebinner " << ": L32 packet " << next32_pos <<
			" #events = " << l32_container[next32_pos].num_events << endl;

		// Mark current packet used and notify l32 consumer if waiting
		l32_container[next32_pos].status = L32EventPacket::used;
		lm32_consumer_ready(next32_pos);

		// Mark l64 packet empty and
		// notify producer if waiting and enough containers are made available
		l64_container[next64_pos].status = L64EventPacket::empty;
		lm64_producer_ready(next64_pos);
		next64_pos = (next64_pos+1)% num_packets;
		next32_pos = (next32_pos+1)% num_packets;
	}
	if (l32_container[next32_pos].num_events > 0) // Mark current packet used if not empty
		l32_container[next32_pos].status = L32EventPacket::used;

	// notify consumer: set done status and signal container ok
  L32EventPacket::file_status = L32EventPacket::done; 
  SetEvent(L32EventPacket::container_ok);
	if (verbose) cout <<"Rebinner " << " done" << endl;
	return;
}


/*
 * Gets  configuration values from GantryModel and calls init_sort3d_hrrt with read or default values.
 * When span=0 (TX mode), span is set to the value from gm328.ini (key rebTxLUTMode0Span) or 21 if key not found,
 * max_rd is set to (span-1)/2.
 * Returns 1 if OK and 0 if gm328.ini not found or if a key is not found.
 */
int init_rebinner(int &span, int &max_rd)
{
	int i=0,  uniform_flag=-1;
	int *head_type = (int*)calloc(NHEADS, sizeof(int));
	float radius=23.45f;
	float def_depth = 1.0f; // default crystal depth
	int ret=1, tx_flag=0;
  int nplanes=0;

  em_span = span;
  head_crystal_depth = (float*)calloc(NHEADS, sizeof(float));
  for (i=0; i<NHEADS; i++) head_crystal_depth[i] = def_depth;
	if (GantryInfo::load(model_number)>0) 
  {
//		GantryInfo::get("interactionDepth", def_depth);
		GantryInfo::get("crystalRadius", radius);
		for (i=0; i<NHEADS; i++)
		{
			if (!GantryInfo::get("headInfo[%d].type", i, head_type[i])) break;

			if (head_type[i] != LSO_LSO && head_type[i] != LSO_GSO && head_type[i] != LSO_LYSO) {
				cerr << "Invalid head " << i << " type = " << head_type[i] << endl;
				ret=0;
				break;
			}
			if (i==1) {
				if (head_type[0]==head_type[1]) uniform_flag=1;
				else uniform_flag = 0;
			} else if (head_type[i]!=head_type[0]) uniform_flag = 0;
				
		}
		if (i==NHEADS) { // all heads defines
			for (i=0; i<NHEADS; i++)
			{
				if (head_type[i] == LSO_LYSO) head_crystal_depth[i] = def_depth;
				else head_crystal_depth[i] = 0.75; //LSO_LSO or LSO_GSO
			}
			if (uniform_flag) cout << "Layer thickness for all heads = " << head_crystal_depth[0] << endl;
			else {
				cout << "Layer thickness per head = ";
				for (i=0; i<NHEADS; i++) 
					cout << " " <<head_crystal_depth[i];
				cout << endl;
			}
		}
		else 
		{
			cout << "Layer thickness for all heads = " << def_depth << endl;
		}
		free(head_type);
		if (span==0)
		{ // Transmission mode
      tx_flag = 1;
			if (GantryInfo::get("rebTxLUTMode0Span", span))
			{
				cout << "Tramsission mode: using span " << span << " from configuration file" << endl; 
			}
			else
			{
				span = 21;
				cout << "Tramsission mode: using default span " << span << endl; 
			}
			tx_span = span;
			if (LR_type >0 )
			{
				span = span/2+1;  // 21==>11; 9==>5
				cout << "Low Resolution mode: span changed to " << span << endl; 
			}
			max_rd = (span-1)/2;
		}
  }
	else
	{
		cerr << "Unable to open gm328.ini configuration file, using default values:" << endl;
  	cerr << "Layer thickness for all heads = " << def_depth << "cm" << endl;
		cerr << "Crystal radius = " << radius << "cm" << endl; 
		span = 21;
		cout << "Tramsission mode: using default span " << span << endl; 
		ret=0;
	}
  init_geometry_hrrt();
  init_segment_info(&m_nsegs,&nplanes,&m_d_tan_theta,maxrd,span,NYCRYS,m_crystal_radius,m_plane_sep);
	if ((rebinner_lut_file=hrrt_rebinner_lut_path(tx_flag))==NULL)
  {
    fprintf(stdout,"Rebinner LUT file not found\n");		
    exit(1);
  }
  if (!tx_flag) init_lut_sol(rebinner_lut_file, m_segzoffset);
  else init_lut_sol_tx(rebinner_lut_file);
  m_d_tan_theta= (float)(tx_span*m_plane_sep/m_crystal_radius);
	return ret;
}

int rebin_event_tx( int mp, int alayer, int ax, int ay, int blayer, int bx, 
                      int by)
{
  double dx,dy,dz,d;
  float deta[3], detb[3], tan_theta, z;
  int plane, seg, addr=-1, sino_addr;
  int axx, bxx;

  int ahead = hrrt_mpairs[mp][0];
  int bhead = hrrt_mpairs[mp][1];
  det_to_phy( ahead, alayer, ax, ay, deta);
  det_to_phy( bhead, blayer, bx, by, detb);
  dz = detb[2]-deta[2];
  dy = deta[1]-detb[1];
  dx = deta[0]-detb[0];
  d = sqrt(dx*dx + dy*dy);
  tan_theta = (float)(dz / d);
  z = (float)(deta[2]+(deta[0]*dx+deta[1]*dy)*dz/(d*d));
  seg =  ((int)(tan_theta/m_d_tan_theta+1024.5))-1024;  
  plane = (int)(0.5+z/m_plane_sep);

  axx=ax+NXCRYS*alayer;
  bxx=bx+NXCRYS*blayer;
  if (blayer == 7) 
  {
    sino_addr=m_solution_tx[0][mp][axx][bx].nsino; // b is TX source
    z = m_solution_tx[0][mp][axx][bx].z;
    d = m_solution_tx[0][mp][axx][bx].d;
  }
  else
  {
    sino_addr=m_solution_tx[1][mp][bxx][ax].nsino; // a is TX source
    z = m_solution_tx[1][mp][bxx][ax].z;
    d = m_solution_tx[1][mp][bxx][ax].d;
  }
  if (sino_addr>=0)
  {
    if (seg == 0) 
    {
       addr = plane*m_nprojs*m_nviews + sino_addr;
    }
  }
  return addr;
}

int rebin_event( int mp, int alayer, int ax, int ay, int blayer, int bx, 
                      int by/*, unsigned delayed_flag*/)
{
  float cay, dz2, d, z;
  int plane, segnum, addr, sino_addr, offset;
  int axx, bxx;
  float seg;

  axx=ax+NXCRYS*alayer;
  bxx=bx+NXCRYS*blayer;
  if ((sino_addr=m_solution[mp][axx][bxx].nsino)==-1) return -1;
  cay = m_c_zpos2[ay];
  dz2 = m_c_zpos[by]-m_c_zpos[ay];
  z = m_solution[mp][axx][bxx].z;
  d = m_solution[mp][axx][bxx].d;
  plane = (int)(cay+z*dz2);
  seg = (float)(0.5+dz2*d);
  segnum = (int)seg;
  int rd = abs(by-ay);
  if(seg<0) segnum=1-(segnum<<1);
  else      segnum=segnum<<1;
  if (segnum >= m_nsegs) return -1;
  if(m_segplane[segnum][plane]!=-1)
  {
    offset = m_segzoffset[segnum];
    addr = (plane+offset)*m_nprojs*m_nviews + sino_addr;
  }
  else addr = -1;
  return addr;
}