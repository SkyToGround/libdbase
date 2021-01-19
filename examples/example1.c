/*
  Example code on how to use libdbase:

  Open a connection to all connected dbases
  and print some info, then make a clean exit.

*/
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h> /* usleep(), ... */

/* libdbase public header */
#include <libdbase.h>

int main(int argc, char **argv) {

  if (sizeof(status_msg) != 80) {
    fprintf(stderr, "E: invalid size of status_msg: %d\n",
            (int)sizeof(status_msg));
    return -1;
  }

  /* dbase list */
  detector **det;
  int found;

  /* populate list with dbases */
  if (libdbase_get_list(&det, &found) < 0 || det == NULL) {
    printf("Error, unable to populate detector pointer list\n");
    return -1;
  }

  /* print a human-readable status msg */
  int k = 0;
  while (det[k] != NULL) {
    // for(k=0; k < found; k++){
    printf("Requesting status from detector (%d/%d)\n", k + 1, found);
    /* Print a status message */
    libdbase_print_status(det[k]);
    printf("\n");
    k++;
  }

  /* close dbase connection and free memory */
  if (libdbase_free_list(&det) < 0) {
    printf("Error when calling det_free_list\n");
    return -1;
  }

  printf("Example1 done.\n");
  return 0;
}
