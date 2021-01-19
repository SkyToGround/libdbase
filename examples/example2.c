/*
  Example code on how to use libdbase:

  Open a connection and read some spectra
  then make a clean exit.

  Note: no error handling in this example,
  except for det_init() function.

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
  if ((det = libdbase_init(-1)) == NULL) {
    printf("E: main when locating detector\n");
    return -1;
  } else
    printf("Found detector:\n");

  /* Print a status message */
  libdbase_print_status(det);

  /* Check sanity of HV setting */
  int hv = det->status.HVT;
  if (hv > (int)50 / 1.2f && hv < (int)900 / 1.2f) {
    /* Enable HV and start dbase */
    printf("Enabling HV\n");
    libdbase_hv_on(det);

    printf("Sleeping 2s to stabilize HV\n");
    sleep(2);

    printf("Starting detector\n");
    libdbase_start(det);
  } else {
    printf("Error. Aborting example due to incorrect HV\n");
    libdbase_close(det);
    return 1;
  }

  /* Clear spectrum */
  libdbase_clear_spectrum(det);

  /* Print a couple of spectra */
  int k;
  for (k = 0; k < 5; k++) {
    /* Collect spectrum */
    sleep(1);
    if (libdbase_get_spectrum(det) < 0)
      break;
    /* Print some info */
    printf("Spectrum (%d/%d): sleeping 1000ms\n", k + 1, 5);
    /* Print spectrum to stdout */
    libdbase_print_spectrum(det);
  }

  printf("Stopping dbase\n");
  libdbase_stop(det);

  printf("Disabling HV\n");
  libdbase_hv_off(det);

  /* Close dbase connection and free memory */
  printf("Closing connection ... ");
  libdbase_close(det);

  printf("Done.\n");

  return 0;
}
