/*
  Example code on how to use libdbase:

  Open a connection and put digibase in list mode
  read amplitudes, pulses and times for 10s, then exit.

  MAC users: this example might fail due                                                                     
  to sleep()... see BUGLIST        
*/

/* printf() */
#include <stdio.h> 
 /* sleep() */
#include <unistd.h> 
/* libdbase public header */
#include "libdbase.h"

int main(int argc, char **argv) {

  /* dbase handle */
  detector *det;

  /* Find and open connection to dbase (first one) */
  if( (det = libdbase_init(-1)) == NULL){
    printf("E: main when finding detector\n");
    return 1;
  }
  printf("Found detector:\n");

  /* Enable HV if it's off */
  int hv_was_on = 1;
  /* Use status macro: IS_HV_ON */
  if( !IS_HV_ON(det) ){
    hv_was_on = 0;
    /* Check sanity of HV setting */
    int hv = det->status.HVT;
    if(hv > 50 && hv < 1200){
      /* Enable HV and start dbase */
      printf("Enabling HV\n");
      libdbase_hv_on(det);
      printf("Sleeping 5s to stabilize HV\n");
      sleep(5);
    }
    else {
      printf("Error. Aborting example due to incorrect HV\n");
      libdbase_close(det);
      return 1;
    }
  }

  /* Stop detector */
  libdbase_stop(det);

  /* Put detector in list mode */
  libdbase_set_list_mode(det);

  /* Start it */
  libdbase_start(det);

  /* Data buffer */
  pulse data[1024];
  int k, i, pulses;
  uint32_t time = 0;
  /* 
     Read cnt, amp & times for 10s @ 20 Hz
  */
  for(k=0; k < 200; k++){
     /* collect data for 50 ms */
    usleep(1000 * 50);
    
    /* read data */
    libdbase_read_lm_packets(det, data, sizeof(data)/sizeof(pulse), &pulses, &time);

    /* Print data */
    printf("Read %d pulses:\n", pulses);
    for(i=0; i < pulses; i++)
      printf("\tAt t=%d\tamp.\t%d\n", data[i].time, data[i].amp);
    /* 
       No need to clear buffer, since we know how many new events we
       got through 'pulses'.
    */
  }

  /* Stop it */
  libdbase_stop(det);

  /* Reset dbase into PHA mode */
  libdbase_set_pha_mode(det);

  /* Reset detector to it's previous state */
  if( !hv_was_on ){
    printf("Disabling HV\n");
    libdbase_hv_off(det);
  }

  /* Close dbase connection and free memory */
  printf("Closing connection ... ");
  libdbase_close(det);

  printf("Done.\n");

  return 0;
}
