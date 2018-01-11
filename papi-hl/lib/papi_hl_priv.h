/****************************/
/* THIS IS OPEN SOURCE CODE */
/****************************/

/**
* @file     papi_hl_priv.h
* @author   Frank Winkler
*           frank.winkler@icl.utk.edu
* @author   Philip Mucci
*           mucci@icl.utk.edu
* @brief    This file contains private, library internal definitons for the high level interface to PAPI. 
*/

#ifndef PAPI_HL_PRIV_H
#define PAPI_HL_PRIV_H

#ifdef CONFIG_PAPIHLLIB_DEBUG
#define APIDBG(format, args...) fprintf(stderr, format, ## args)
#endif

void _papi_hwi_shutdown_highlevel(  );

#endif
