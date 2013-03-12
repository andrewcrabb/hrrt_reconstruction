#ifndef HISTOGRAMDLL_H
#define HISTOGRAMDLL_H

#ifndef HISTOGRAMDLLIMPEXP
#define HISTOGRAMDLLIMPEXP __declspec(dllimport)
#endif

HISTOGRAMDLLIMPEXP int HISTInit (HIST_stats*, CCallback*, CSinoInfo*) ;
HISTOGRAMDLLIMPEXP int HISTUnconfig();
HISTOGRAMDLLIMPEXP int HISTSetAttributes(ULONG nprojs, ULONG nviews, ULONG n_em_sinos, ULONG n_tx_sinos);
HISTOGRAMDLLIMPEXP int HISTDefineFrames(ULONG nframes, ULONG frame_criteria, ULONG preset_value);
HISTOGRAMDLLIMPEXP int HISTSinoMode(int em_sinm, int tx_sinm, int acq_mode);
HISTOGRAMDLLIMPEXP int HISTConfig(char *histFname, char *listFname=NULL);
HISTOGRAMDLLIMPEXP int HISTStart();
HISTOGRAMDLLIMPEXP int HISTStop();
HISTOGRAMDLLIMPEXP int HISTAbort();
HISTOGRAMDLLIMPEXP int HISTWait();

#endif

