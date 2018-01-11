/****************************/
/* THIS IS OPEN SOURCE CODE */
/****************************/

/**
* @file     papi_hl.c
* @author   Frank Winkler
*           frank.winkler@icl.utk.edu
* @author   Philip Mucci
*           mucci@icl.utk.edu
* @brief    Return codes and api definitions for the high level API.
*/

#ifndef PAPI_HL_H
#define PAPI_HL_H

/** \internal
  @defgroup high_api  The High Level API 

   The simple interface implemented by the following two routines
   allows the user to access and count specific hardware events from
   both C and Fortran.
	@{ */

   int PAPI_region_begin(const char* region); /**< begin a new region for reading hardware events */
   int PAPI_region_end(const char* region); /**< end region and store difference of hardware events in map */
/** @} */

#endif

