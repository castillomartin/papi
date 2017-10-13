/****************************/
/* THIS IS OPEN SOURCE CODE */
/****************************/

/**
* @file     papi_hl.c
* @author   Frank Winkler
*           frank.winkler@icl.utk.edu
* @author   Philip Mucci
*           mucci@cs.utk.edu
* @author   Kevin London
*           london@cs.utk.edu
* @author   dan terpstra
*           terpstra@cs.utk.edu
* @brief This file contains the 'high level' interface to PAPI.
*  BASIC is a high level language. ;-) */

#include "papi.h"
#include "papi_internal.h"
#include "papi_memory.h"
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <search.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <error.h>
#include <time.h>


/* high level papi functions*/

/** \internal 
 * This is stored globally.
 */
char **event_names = NULL;       /**< Array of event names for PAPI*/
char **all_event_names = NULL;   /**< Array of all event names*/
char default_events[4][32];      /**< Maximal number of default events*/
int event_number = 0;            /**< Number of events provided by PAPI specified by user + defaults*/
int total_event_number = 0;      /**< total events including specific metrics like real and proc time*/
int generate_output = 0;         /**< Generate output file if value=1 */
int events_determined = 0;       /**< Check if events are determined after initialization*/

typedef struct
{
   long long offset;
   long long total;
} value_t;

typedef struct events
{
   char *region;              /**< Region name */
   struct events *next;
   value_t values[];          /**< Array of values */
} events_t;

typedef struct
{
   unsigned long key;         /**< Thread ID */
   events_t *value;           /**< List of recorded events */
} events_map_t;

int _internal_hl_map_compar(const void *l, const void *r)
{
   const events_map_t *lm = l;
   const events_map_t *lr = r;
   return lm->key - lr->key;
}

//root of events map
static void *root = 0;

//rank of the process
static int rank = -1;

//measurement directory
static char output_absolute_path[256];

//thread local
__thread int _EventSet = PAPI_NULL;
__thread long long *_values;  /**< Array of values per thread based on event names */

void _internal_onetime_library_init(void);

static int _internal_checkCounter ( char* counter );
void _internal_determine_rank();
char *_internal_remove_spaces( char *str );
int _internal_hl_read_events();
void _internal_hl_add_values_to_list(events_t *node, long long *values,
                                     long long cycles, short offset );
events_t* _internal_hl_new_list_node(const char *region);
int _internal_hl_store_values_in_map( unsigned long tid, const char *region,
                                      long long *values, long long cycles, short offset);
static void internal_mkdir(const char *dir);
void _internal_hl_write_output();

void _internal_onetime_library_init(void)
{
   static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
   static int done = 0;
   int retval;

   /*  failure means already we've already initialized or attempted! */
   if (pthread_mutex_trylock(&mutex) == 0) {
   APIDBG("Initializing...\n");
   if ((retval = PAPI_library_init(PAPI_VER_CURRENT)) != PAPI_VER_CURRENT)
      error_at_line(0, retval, __FILE__ ,__LINE__, "PAPI_library_init"); 
   if ((retval = PAPI_multiplex_init()) != PAPI_OK)
      error_at_line(0, retval, __FILE__ ,__LINE__, "PAPI_multiplex_init"); 
   if ((retval = PAPI_thread_init(&pthread_self)) != PAPI_OK)
      error_at_line(0, retval, __FILE__ ,__LINE__, "PAPI_thread_init"); 
   if ((retval = PAPI_set_debug(PAPI_VERB_ECONT)) != PAPI_OK)
      error_at_line(0, retval, __FILE__ ,__LINE__, "PAPI_set_debug");
   if ((retval = PAPI_register_thread()) != PAPI_OK)
      error_at_line(0, retval, __FILE__ ,__LINE__, "PAPI_register_thread"); 
   done++;
   APIDBG("Done!\n");
   } else {
   /* Wait if done still is 0, if not just register thread */
   while(!done) {
      APIDBG("Waiting!\n");
      usleep(1);
   }

   APIDBG("Done waiting!\n");
   if ((retval = PAPI_register_thread()) != PAPI_OK)
      error_at_line(0, retval, __FILE__ ,__LINE__, "PAPI_register_thread"); 
   }
}

static int
_internal_checkCounter ( char* counter )
{
   int EventSet = PAPI_NULL;
   int eventcode;
   int retval;

   if ( ( retval = PAPI_create_eventset( &EventSet ) ) != PAPI_OK )
   return ( retval );

   if ( ( retval = PAPI_event_name_to_code( counter, &eventcode ) ) != PAPI_OK )
   return ( retval );

   if ( ( retval = PAPI_add_event (EventSet, eventcode) ) != PAPI_OK )
   return ( retval );

   if ( ( retval = PAPI_cleanup_eventset (EventSet) ) != PAPI_OK )
   return ( retval );

   if ( ( retval = PAPI_destroy_eventset (&EventSet) ) != PAPI_OK )
   return ( retval );

   return ( PAPI_OK );
}

void _internal_determine_rank()
{
   //check environment variables for rank identification
   //'OMPI_COMM_WORLD_RANK','ALPS_APP_PE','PMI_RANK',’SLURM_PROCID’

   if ( getenv("OMPI_COMM_WORLD_RANK") != NULL )
      rank = atoi(getenv("OMPI_COMM_WORLD_RANK"));
   else if ( getenv("ALPS_APP_PE") != NULL )
      rank = atoi(getenv("ALPS_APP_PE"));
   else if ( getenv("PMI_RANK") != NULL )
      rank = atoi(getenv("PMI_RANK"));
   else if ( getenv("SLURM_PROCID") != NULL )
      rank = atoi(getenv("SLURM_PROCID"));
}

char *_internal_remove_spaces( char *str )
{
   char *out = str, *put = str;
   for(; *str != '\0'; ++str) {
      if(*str != ' ')
         *put++ = *str;
   }
   *put = '\0';
   return out;
}

int _internal_hl_read_events()
{
   char *perf_counters_from_env = NULL; //content of environment variable PAPI_EVENTS
   char **event_names_from_env = NULL;
   const char *separator; //separator for events
   int pc_array_len = 1;
   int pc_list_len = 0; //number of counter names in string
   const char *position = NULL; //current position in processed string
   char *token;
   int default_number = 0; // number of default PAPI counters
   int i;
   char *default_option = NULL;
   int no_default = 0;

   //check if default counters is disabled
   if ( getenv("PAPI_DEFAULT_NONE") != NULL ) {
      default_option = strdup( getenv("PAPI_DEFAULT_NONE") );
      printf("PAPI_DEFAULT_NONE=%s\n", default_option);
      no_default = 1;
   }

   //we need at least one counter
   strcpy(default_events[0], "perf::TASK-CLOCK");
   default_number = 1;

   if ( no_default == 0 )
   {
      strcpy(default_events[1], "PAPI_TOT_INS");
      strcpy(default_events[2], "PAPI_TOT_CYC");
      default_number = 3;

      //check for FLOPS
      if ( _internal_checkCounter( "PAPI_FP_OPS" ) == PAPI_OK )
      {
         strcpy(default_events[3], "PAPI_FP_OPS");
         default_number = 4;
      } else
         printf("PAPI: Default counter PAPI_FP_OPS not available on this machine!\n");
   }

   //check if environment variable is set
   if ( getenv("PAPI_EVENTS") != NULL ) {

      separator=",";
      //get value from environment variable PAPI_EVENTS
      perf_counters_from_env = strdup( getenv("PAPI_EVENTS") );
      
      //check if environment variable is not empty
      if ( strlen( perf_counters_from_env ) > 0 ) {
         //count number of separator characters
         position = perf_counters_from_env;
         while ( *position ) {
            if ( strchr( separator, *position ) ) {
               pc_array_len++;
            }
               position++;
         }
         //allocate memory for event array
         event_names_from_env = calloc( pc_array_len, sizeof( char* ) );

         //parse list of counter names
         token = strtok( perf_counters_from_env, separator );
         while ( token ) {
            //skip default values
            if ( strcmp(_internal_remove_spaces(token), "perf::TASK-CLOCK") == 0 )
            {
               token = strtok( NULL, separator );
               continue;
            }
            if ( ( no_default == 0 ) && 
                  ( strcmp(_internal_remove_spaces(token), "PAPI_TOT_INS") == 0 ||
                     strcmp(_internal_remove_spaces(token), "PAPI_TOT_CYC") == 0 ||
                     strcmp(_internal_remove_spaces(token), "PAPI_FP_OPS") == 0 ) )
            {
               token = strtok( NULL, separator );
               continue;
            }
            if ( pc_list_len >= pc_array_len ){
               //more entries as in the first run
               return PAPI_EINVAL;
            }
            //printf("TOKEN: %s\n", _internal_remove_spaces(token));
            event_names_from_env[ pc_list_len ] = _internal_remove_spaces(token);
            //debug
            //printf("#%s#\n", perf_counters[ pc_list_len ]);
            token = strtok( NULL, separator );
            pc_list_len++;
         }
      }
   }

   //check availibilty of user events
   if ( pc_list_len > 0 )
   {
      for ( i = 0; i < pc_list_len; i++ ) {
         if ( _internal_checkCounter( event_names_from_env[i] ) != PAPI_OK ) {
            printf("PAPI Error: Counter %s not available on this machine!\n", event_names_from_env[i]);
            exit(EXIT_FAILURE);
         }
      }
   }

   //generate final event set
   if ( event_names_from_env != NULL )
   {
      //combine default and user events
      event_names = calloc( pc_list_len + default_number, sizeof( char* ) );
      for ( i = 0; i < default_number; i++ )
         event_names[i] = default_events[i];
      for ( i = default_number; i < pc_list_len + default_number; i++ )
         event_names[i] = event_names_from_env[i - default_number];
      event_number = pc_list_len + default_number;
   }
   else
   {
      //use only default events
      event_names = calloc( default_number, sizeof( char* ) );
      for ( i = 0; i < default_number; i++ )
         event_names[i] = default_events[i];
      event_number = default_number;
   }

   free(event_names_from_env);

   //increase total event number for region counter and CPU cycles
   total_event_number = event_number + 2;

   //generate array of all events including region count and CPU cycles for output
   all_event_names = calloc( total_event_number, sizeof( char* ) );
   all_event_names[0] = "REGION_COUNT";
   all_event_names[1] = "CYCLES";
   for ( i = 0; i < event_number; i++ ) {
      all_event_names[i + 2] = event_names[i];
   }

   events_determined = 1;
   return ( PAPI_OK );
}

void _internal_hl_add_values_to_list( events_t *node, long long *values,
                                      long long cycles, short offset )
{
   int i;
   int region_count = 1;

   if ( offset == 1 ) {
      //set first fixed counters
      node->values[0].offset = region_count;
      node->values[1].offset = cycles;
      for ( i = 0; i < event_number; i++ )
         node->values[i + 2].offset = values[i];
   } else {
      //determine difference of current value and offset and add
      //previous total value
      node->values[0].total += node->values[0].offset;
      node->values[1].total += cycles - node->values[1].offset;
      for ( i = 0; i < event_number; i++ )
         node->values[i + 2].total += values[i] - node->values[i + 2].offset;
   }
}

events_t* _internal_hl_new_list_node(const char *region )
{
   events_t *new_node;
   int i;
   new_node = malloc(sizeof(events_t) + total_event_number * sizeof(value_t));
   new_node->region = (char *)malloc((strlen(region) + 1) * sizeof(char));
   strcpy(new_node->region, region);
   for ( i = 0; i < total_event_number; i++ )
      new_node->values[i].total = 0;
   return new_node;
}

int _internal_hl_store_values_in_map( unsigned long tid, const char *region,
                                      long long *values, long long cycles, short offset )
{
   //offset = 1 --> region_begin
   //offset = 0 --> region end
   int i;

   APIDBG("tid=%lu, region=%s, cycles=%lld, offset=%d\n",
      tid, region, cycles, offset);
   for ( i = 0; i < event_number; i++ )
      APIDBG("event=%s, value=%lld\n", event_names[i], values[i]);

   //check if thread is already stored in events map
   events_map_t *find_thread = malloc(sizeof(events_map_t));
   find_thread->key = tid;
   void *r = tfind(find_thread, &root, _internal_hl_map_compar); /* read */
   if ( r != NULL )
   {
      //find node for current region in list
      events_t *current = (*(events_map_t**)r)->value;
      while (current != NULL) {
         if ( strcmp(current->region, region) == 0 ) {
            //if region already exists, add values to current node
            _internal_hl_add_values_to_list(current, values, cycles, offset);
            return PAPI_OK;
         }
         current = current->next;
      }
      //create new node for current region (only if offset is set) and store offset
      if ( offset == 1 )
      {
         //create new node
         events_t *new_node = _internal_hl_new_list_node(region);
         //add values to new node
         _internal_hl_add_values_to_list(new_node, values, cycles, offset);
         new_node->next = (*(events_map_t**)r)->value;
         (*(events_map_t**)r)->value = new_node;
            return PAPI_OK;
      }
   }
      
   //if current thread id is not stored in map, we only get offset values
   if ( offset == 1 )
   {
      //create new map item for current thread id
      events_map_t *new_thread = malloc(sizeof(events_map_t));
      new_thread->key = tid;
      new_thread->value = NULL;
      //create new node and save offset values
      events_t *new_node = _internal_hl_new_list_node(region);
      //add values to new node
      _internal_hl_add_values_to_list(new_node, values, cycles, offset);
      new_node->next = new_thread->value;
      new_thread->value = new_node;
      //add new item to map
      tsearch(new_thread, &root, _internal_hl_map_compar); /* insert */

      return PAPI_OK;
   }
   else
      return PAPI_EINVAL;
}

static void internal_mkdir(const char *dir)
{
   char tmp[256];
   char *p = NULL;
   size_t len;

   snprintf(tmp, sizeof(tmp),"%s",dir);
   len = strlen(tmp);
   if(tmp[len - 1] == '/')
      tmp[len - 1] = 0;
   for(p = tmp + 1; *p; p++)
   {
      if(*p == '/')
      {
         *p = 0;
         mkdir(tmp, S_IRWXU);
         *p = '/';
      }
   }
   mkdir(tmp, S_IRWXU);
}

void _internal_hl_write_output()
{
   //first thread creates measurement directory
   if ( generate_output == 0 )
   {
      _papi_hwi_lock( HIGHLEVEL_LOCK );
      if ( generate_output == 0 ) {
         //create directory for output files (+timestamp to keep it unique)

         //check if PAPI_OUTPUT_DIRECTORY is set
         char *output_prefix = NULL;
         if ( getenv("PAPI_OUTPUT_DIRECTORY") != NULL )
            output_prefix = strdup( getenv("PAPI_OUTPUT_DIRECTORY") );
         else
            output_prefix = strdup( get_current_dir_name() );
         
         //get date and time for measurement directory
         //time_t t = time(NULL);
         //struct tm tm = *localtime(&t);
         //char m_time[128];
         //sprintf(m_time, "%d%d%dT%d%d%d", tm.tm_year+1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

         //generate absolute path including measurement directory
         //sprintf(output_absolute_path, "%s/papi-%s", output_prefix, m_time);
         sprintf(output_absolute_path, "%s/papi", output_prefix);
         internal_mkdir(output_absolute_path);
         generate_output = 1;
      }
      _papi_hwi_unlock( HIGHLEVEL_LOCK );
   }

   if ( generate_output == 1 )
   {
      unsigned long *tids = NULL;
      int i, j, number;
      FILE *output_file;
      short output_file_generated = 0;
      //current CPU frequency in Hz
      int cpu_freq;

      //determine rank for output file
      _internal_determine_rank();

      //determine current cpu frequency
      cpu_freq = PAPI_get_opt( PAPI_CLOCKRATE, NULL );

      char output_file_path[128];

      if ( rank < 0 )
      {
         //generate unique rank number
         sprintf(output_file_path, "%s/rank_XXXXXX", output_absolute_path);
         mkstemp(output_file_path);
      }
      else
      {
         sprintf(output_file_path, "%s/rank_%d", output_absolute_path, rank);
      }

      output_file = fopen(output_file_path, "w");

      if ( output_file == NULL )
      {
         printf("Error: Cannot create output file %s!\n", output_file_path);
      }
      else
         output_file_generated = 1;

      if ( output_file_generated == 1 )
      {
         //list all threads
         PAPI_list_threads( tids, &number );
         tids=malloc( number * sizeof(unsigned long) );
         PAPI_list_threads( tids, &number );

         // Thread,list<Region:list<Event:Value>>
         // 1,<"calc_1":<"PAPI_TOT_INS":57258,"PAPI_TOT_CYC":39439>,"calc_2":<"PAPI_TOT_INS":57258,"PAPI_TOT_CYC":39439>>

         //print current CPU frequency for each rank
         fprintf(output_file, "CPU in MHz:%d\n", cpu_freq);
         fprintf(output_file, "Thread,list<Region:list<Event:Value>>");
         for ( i = 0; i < number; i++ )
         {
            APIDBG("Thread %lu\n", tids[i]);
            //find values of current thread in global map and dump in file
            events_map_t *find_thread = malloc(sizeof(events_map_t));
            find_thread->key = tids[i];
            void *r = tfind(find_thread, &root, _internal_hl_map_compar); /* read */
            if ( r != NULL ) {
               //print thread
               //do we really need the exact thread id?
               //fprintf(output_file, "\n%lu,<", (*(events_map_t**)r)->key);
               //use of iterator id
               fprintf(output_file, "\n%d,<", i);

               //iterate over values list
               events_t *current = (*(events_map_t**)r)->value;
               while (current != NULL) {
                  //print region
                  fprintf(output_file, "\"%s\":<", current->region);
                  for ( j = 0; j < total_event_number; j++ )
                     if ( j == ( total_event_number - 1 ) )
                        fprintf(output_file, "\"%s\":\"%lld\">", all_event_names[j], current->values[j].total);
                     else
                        fprintf(output_file, "\"%s\":\"%lld\",", all_event_names[j], current->values[j].total);
                  current = current->next;
                  if (current == NULL )
                     fprintf(output_file, ">");
                  else
                     fprintf(output_file, ",");
               }
            }
         }
         fprintf(output_file, "\n");
         fclose(output_file);
      }
   }
}

int
PAPI_region_begin( const char* region )
{
   int i, event, retval;
   unsigned long tid;
   long long cycles;

   /* Read event names and store in global data structure */
   if ( events_determined == 0 )
   {
      _internal_onetime_library_init();
      _papi_hwi_lock( HIGHLEVEL_LOCK );
      if ( events_determined == 0 )
      {
         _internal_hl_read_events();
         //register the termination function for output
         atexit(_internal_hl_write_output);
      }
      _papi_hwi_unlock( HIGHLEVEL_LOCK );
   }

   if ( _EventSet == PAPI_NULL )
   {
      if ( ( retval = PAPI_create_eventset( &_EventSet ) ) != PAPI_OK )
      return ( retval );

      /* load events to the new EventSet */
      for ( i = 0; i < event_number; i++ ) {
         retval = PAPI_event_name_to_code( event_names[i], &event );
         if ( retval != PAPI_OK ) {
            return ( retval );
         }
         retval = PAPI_add_event( _EventSet, event );
         if ( retval == PAPI_EISRUN )
            return ( retval );
      }

      //allocate memory for values based on EventSet
      _values = calloc(event_number, sizeof(long long));

      if ( ( retval = PAPI_start( _EventSet ) ) != PAPI_OK ) {
         return ( retval );
      }

      //warm up PAPI code paths and data structures
      if ( PAPI_read_ts( _EventSet, _values, &cycles ) != PAPI_OK ) {
         printf("PAPI: Your counter combination does not work on this machine!\n");
         exit(EXIT_FAILURE);
      }
   }

   //read current hardware events
   if ( ( retval = PAPI_read_ts( _EventSet, _values, &cycles ) ) != PAPI_OK ) {
      return ( retval );
   }

   tid = PAPI_thread_id();
   
   APIDBG("Region %s Thread %lu\n", region, tid);

   //store offset values in global map
   _papi_hwi_lock( HIGHLEVEL_LOCK );
   _internal_hl_store_values_in_map( tid, region, _values, cycles, 1);
   _papi_hwi_unlock( HIGHLEVEL_LOCK );

   return ( PAPI_OK );
}

int
PAPI_region_end( const char* region )
{
   int retval;
   unsigned long tid;
   long long cycles;

   if ( _EventSet == PAPI_NULL )
      return ( PAPI_EINVAL );

   //read current hardware events
   if ( ( retval = PAPI_read_ts( _EventSet, _values, &cycles ) ) != PAPI_OK ) {
      return ( retval );
   }

   tid = PAPI_thread_id();

   //store values in global map
   _papi_hwi_lock( HIGHLEVEL_LOCK );
   _internal_hl_store_values_in_map( tid, region, _values, cycles, 0);
   _papi_hwi_unlock( HIGHLEVEL_LOCK );

   return ( PAPI_OK );
}

void
_papi_hwi_shutdown_highlevel(  )
{
   //
}
