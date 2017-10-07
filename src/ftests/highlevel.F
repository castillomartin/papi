#include "fpapi_test.h"
C
C      program highlevel
C      implicit integer (p)
C
C      integer*8 values(10)
C      integer events(2)
C      integer eventnum
C      integer availcounters
C      integer retval
C      integer tests_quiet, get_quiet
C      external get_quiet
C      character*PAPI_MAX_STR_LEN name
C      integer last_char, n
C      external last_char
C
C      tests_quiet = get_quiet()
C
C      eventnum = 2
C      call PAPIf_num_counters(availcounters)
C      if (eventnum .GT. availcounters) then
C         print *, "Not enough hardware counters!"
C         stop
C      end if
C
C      retval = PAPI_VER_CURRENT
C      call PAPIf_library_init(retval)
C      if ( retval.NE.PAPI_VER_CURRENT) then
C        call ftest_fail(__FILE__, __LINE__,
C     . 'PAPI_library_init', retval)
C      end if
C
C      call PAPIf_query_event(PAPI_FP_INS, retval)
C      if (retval .NE. PAPI_OK) then
C        events(1)=PAPI_TOT_INS
C      else
C        events(1)=PAPI_FP_INS
C      end if
C      events(2)=PAPI_TOT_CYC
C
C      call PAPIf_start_counters(events, eventnum, retval)
C      if ( retval .NE. PAPI_OK ) then
C        call ftest_fail(__FILE__, __LINE__,
C     . 'PAPIf_start_counters', 
C     *retval)
C      end if
C
C      call fdo_flops(NUM_FLOPS)
C
C      call PAPIf_read_counters(values(1), eventnum, retval)
C      if ( retval .NE. PAPI_OK ) then
C        call ftest_fail(__FILE__, __LINE__,
C     . 'PAPIf_read_counters', 
C     *retval)
C      end if
C
C      call fdo_flops(NUM_FLOPS)
C
C      call PAPIf_stop_counters(values(3), eventnum, retval)
C      if ( retval .NE. PAPI_OK ) then
C        call ftest_fail(__FILE__, __LINE__,
C     . 'PAPIf_stop_counters', 
C     *retval)
C      end if
C      
C      if (tests_quiet .EQ. 0) then
C        call PAPIf_event_code_to_name (events(1), name, retval)
C        if ( retval.NE.PAPI_OK) then
C          call ftest_fail(__FILE__, __LINE__, 
C     * 'PAPIf_event_code_to_name', retval)
C        end if
C        n=last_char(name)
C        print *, "Test case highlevel: Test of high-level APIs."
C        print *, "---------------------------------------------",
C     *    "---------------------"
C        write (*,100) "Test type", 1, 2
C        write (*,100) name(1:n), values(1), values(3)
C        write (*,100) "PAPI_TOT_CYC", values(2), values(4)
C 100    format(a15, ":", i12, i12)
C        print *, "---------------------------------------------",
C     * "---------------------"
C      end if
C
C      call ftests_pass(__FILE__)
C      End
