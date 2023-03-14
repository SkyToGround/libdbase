/*
 * libdbasei.c: Internal libdbase source
 *
 * Copyright (c) 2011, 2012 Peder Kock <peder.kock@med.lu.se>
 * Lund University
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <errno.h>  /* error codes */
#include <stdio.h>  /* printf(), fprintf(), ... */
#include <stdlib.h> /* malloc(), calloc(), free(), ... */
#include <string.h> /* memcpy() */

/* libdbase non-public header */
#include "libdbasei.h"

/*
  Endianess (defined in libdbase.c)
*/
extern int big_endian_test;

/*
  These four status msgs are written to dbase during init()
  Use GCC's compact notation for sparse arrays:
  omitted elements are initialized to zero.

  const unsigned const char _STAT[ 4 ][ 80 ] = {
   first status msg 0-79
  {[1]=177,[3]=12,[4]=13,[6]=48,[7]=32,[13]=10,[17]=128,[18]=226,[19]=174,[21]=146,[22]=24,[41]=255,[42]=3,[43]=88,[44]=2,[57]=103,[58]=1,[59]=74,[60]=1,[61]=45,[62]=1,[63]=200,[64]=127,[65]=255,[66]=255,[67]=103,[68]=1,[69]=74,[70]=1,[71]=45,[72]=1,[73]=64,[77]=16,[78]=192,[79]=24},
   second status msg 80-159
  {[1]=49,[3]=12,[4]=13,[6]=48,[7]=32,[13]=10,[17]=128,[18]=226,[19]=46,[21]=146,[22]=24,[41]=255,[42]=3,[43]=88,[44]=2,[57]=103,[58]=1,[59]=74,[60]=1,[61]=45,[62]=1,[63]=200,[64]=127,[65]=255,[66]=127,[67]=103,[68]=1,[69]=74,[70]=1,[71]=45,[72]=1,[73]=64,[78]=192,[79]=24},
   third status msg 160-239
  {[1]=49,[3]=12,[4]=13,[6]=48,[7]=32,[13]=10,[17]=128,[18]=226,[19]=46,[21]=146,[22]=24,[41]=255,[42]=3,[43]=88,[44]=2,[57]=103,[58]=1,[59]=74,[60]=1,[61]=45,[62]=1,[63]=200,[64]=127,[65]=255,[66]=127,[67]=103,[68]=1,[69]=74,[70]=1,[71]=45,[72]=1,[73]=64,[77]=1,[78]=192,[79]=24},
   fourth status msg 240-299
  {[1]=49,[3]=12,[4]=13,[6]=48,[7]=32,[13]=10,[17]=128,[18]=226,[19]=46,[21]=146,[22]=24,[41]=255,[42]=3,[43]=88,[44]=2,[57]=103,[58]=1,[59]=74,[60]=1,[61]=45,[62]=1,[63]=200,[64]=127,[65]=255,[66]=127,[67]=103,[68]=1,[69]=74,[70]=1,[71]=45,[72]=1,[73]=64,[78]=192,[79]=24}
  };

   v. 0.2
   We'll use this shorter status array instead.
   Only the two first msg's differ significantly, the rest are set in init2()
*/
unsigned const char _STAT[2][81] = {
    // first status msg 0-79
    {[1] = 0xbd,  [3] = 0x0c,  [6] = 0x30,  [7] = 0x20,  [8] = 0x03,
     [13] = 0x0a, [18] = 0xb5, [19] = 0xa9, [21] = 0xb0, [22] = 0x1e,
     [41] = 0xff, [42] = 0x03, [43] = 0x58, [44] = 0x02, [57] = 0xfa,
     [58] = 0x01, [59] = 0xd5, [60] = 0x01, [61] = 0xb0, [62] = 0x01, // toggle
     [66] = 0x80, [67] = 0xfa, [68] = 0x01, [69] = 0xd5, [70] = 0x01,
     [71] = 0xb0, [72] = 0x01, [73] = 0x40, [77] = 0x10, [79] = 0x2e,
     [80] = 0x0b},
    // second status msg 80-159
    {[1] = 0x3d,  [3] = 0x0c,  [6] = 0x30,  [7] = 0x20,  [8] = 0x03,
     [13] = 0x0a, [18] = 0xb5, [19] = 0x29, [21] = 0xb0, [22] = 0x1e,
     [41] = 0xff, [42] = 0x03, [43] = 0x58, [44] = 0x02, [57] = 0xfa,
     [58] = 0x01, [59] = 0xd5, [60] = 0x01, [61] = 0xb0, [62] = 0x01,
     [67] = 0xfa, [68] = 0x01, [69] = 0xd5, [70] = 0x01, [71] = 0xb0,
     [72] = 0x01, [73] = 0x40, [79] = 0x2e, [80] = 0x0b}};

/*
   Read bytes from EP (in)
*/
int dbase_read(libusb_device_handle *dev, unsigned char *bytes, int len,
               int *read) {
  if (dev == NULL) {
    fprintf(stderr, "E: dbase_read() device handle is null pointer\n");
    return -1;
  }

  /* Temporary buffer to avoid overflow errors */
  const int iread_buf = 2 * (DBASE_LEN + 1) * sizeof(int32_t); /* 8192 */
  unsigned char *buf = (unsigned char *)malloc(iread_buf);
  if (buf == NULL) {
    fprintf(stderr, "E: unable to allocate memory in dbase_read()\n");
    return -ENOMEM;
  }

  /* Read data from device */
  int err = libusb_bulk_transfer(dev, EP_IN, buf, iread_buf, read, S_TIMEOUT);

  /* Copy data: buf to bytes */
  /*
    2012-02-13, should only copy read bytes
    was: memcpy(bytes, buf, len);
    Added additional check of len
  */
  if (len < *read) {
    fprintf(stderr, "E: dbase_read() read %d bytes, buffer can only hold %d\n",
            *read, len);
    free(buf);
    return -1;
  }
  memcpy(bytes, buf, *read);

  if (_DEBUG > 0) {
    if (*read == 1)
      printf("Read 0x%02x from device\n", (unsigned char)buf[0]);
    else
      printf("Read %d bytes from device\n", *read);
  }

  /* Free temporary buffer */
  free(buf);

  return err;
}

/*
   Write bytes to EP (out)
*/
int dbase_write(libusb_device_handle *dev, unsigned char *bytes, int len,
                int *written) {
  if (dev == NULL) {
    fprintf(stderr, "E: dbase_write() device handle is null pointer\n");
    return -1;
  }
  if (bytes == NULL && len > 0) {
    fprintf(stderr,
            "E: dbase_write() trying to write null pointer, but len was %d\n",
            len);
    return -1;
  }

  /* Write data to device */
  int err = libusb_bulk_transfer(dev, EP_OUT, bytes, len, written, L_TIMEOUT);

  if (err < 0)
    err_str("dbase_write()", err);
  if (_DEBUG > 0)
    printf("Wrote %d bytes to device\n", written[0]);
  return err;
}

/*
   Write one byte to EP (out)
*/
int dbase_write_one(libusb_device_handle *dev, unsigned char byte,
                    int *written) {
  if (_DEBUG > 0)
    printf("Writing 0x%02x to device\n", byte);
  return dbase_write(dev, &byte, 1, written);
}

/*
   Check response from device
*/
int dbase_checkready(libusb_device_handle *dev) {
  /* Sometimes we get status back instead of 1... */
  unsigned char resp[sizeof(status_msg)];
  int err, read;
  err = dbase_read(dev, resp, sizeof(status_msg), &read);
  if (err < 0 || read != 1) {
    if (_DEBUG > 0) {
      printf("check_ready() got %d bytes back\n", read);
      if (read != sizeof(status_msg))
        err_str("dbase_checkready()", err);
    }
    /* 80 bytes (status) might actually have been requested */
    if (read == sizeof(status_msg)) {
      return 1;
    }
    return -1;
  }
  /*
     0 indicate init needed
     1 indicate OK
  */
  if (resp[0] == 0 || resp[0] == 1)
    return resp[0];
  return -1;
}

/*
   Clear the internal spectrum of MCB
*/
int dbase_send_clear_spectrum(libusb_device_handle *dev) {
  int err, written;

  /* use calloc to initialize to zeros... */
  unsigned char *buf = calloc(4097, 1);
  if (buf == NULL) {
    fprintf(stderr,
            "E: Unable to allocate buffer in dbase_send_clear_spectrum()\n");
    return -ENOMEM;
  }
  /* prepend 0x02 to spectrum */
  buf[0] = (unsigned char)2;

  /* Write det->status.LEN int32_t zeros to dbase */
  err = dbase_write(dev, buf, 4097, &written);

  /* Free memory */
  free(buf);
  buf = NULL;

  if (err < 0 || written != 4097) {
    fprintf(stderr, "E: unable to send clear spectrum (sent %d)\n", written);
    err_str("dbase_send_clear_spectrum()", err);
    return err < 0 ? err : -EIO;
  }
  return dbase_checkready(dev);
}

/*
   Send status message
*/
int dbase_write_status(libusb_device_handle *dev, status_msg *stat) {
  int err, written;

  /* Create temporary buffer to hold status struct + 1 */
  unsigned char buf[sizeof(status_msg) + 1];
  /* Prepend zero at beginning of buffer */
  buf[0] = 0;

  /* Copy status msg to buffer */
  memcpy(buf + 1, stat, sizeof(status_msg));

  /* 2012-02-16
     Moved down after memcpy(),
     changed to buf - we shouldn't touch stat!
  */
  if (IS_BIG_ENDIAN()) {
    dbase_byte_swap_status_struct((status_msg *)(buf + 1));
  }

  /* Send status_msg */
  err = dbase_write(dev, buf, sizeof(status_msg) + 1, &written);

  /* Check nbr of sent bytes */
  if (err < 0 || written != (sizeof(status_msg) + 1)) {
    printf("E: unable to send whole status_msg ( sent=%d)\n", written);
    err_str("dbase_write_status()", err);
    return err < 0 ? err : -EIO;
  }
  /* Success? */
  return dbase_checkready(dev);
}

/*
   Read status bytes from device
*/
int dbase_get_status(libusb_device_handle *dev, status_msg *stat) {
  /* Temporary buffer */
  status_msg tmp_stat;
  int err, io;

  /* Request status from device */
  err = dbase_write_one(dev, STATUS, &io);
  if (err < 0 || io != 1) {
    fprintf(stderr, "E: When writing status request (written=%d)\n", io);
    err_str("dbase_get_status()", err);
  }
  /* Read answer */
  err = dbase_read(dev, (unsigned char *)&tmp_stat, sizeof(status_msg), &io);

  /* repack big endian struct */
  if (IS_BIG_ENDIAN()) {
    dbase_byte_swap_status_struct(&tmp_stat);
  }
  /* Sanity check */
  if (err == 0 &&                            /* no libusb error */
      io == sizeof(status_msg) &&            /* whole message recieved? */
      tmp_stat.LEN == (uint16_t)DBASE_LEN && /* this is constant on digiBASE */
      tmp_stat.HVT > (uint16_t)MIN_HV)       /* this also seems sane */
  {
    /* Status seems ok - update pointer */
    *stat = tmp_stat;
  } else if (err < 0) {
    err_str("dbase_get_status()", err);
  } else {
    fprintf(stderr, "W: sanity check failed on recieved status data\n");
  }

  return err;
}

/*
   Read spectrum from device
*/
int dbase_get_spectrum(libusb_device_handle *dev, int32_t *chans,
                       int32_t *last) {
  int err, io;
  const int len = (DBASE_LEN + 1);
  const int len_bytes = len * sizeof(int32_t);
  unsigned char onflag = 1;

  /* Temporary buffer */
  int32_t tmp[DBASE_LEN + 1];

  /* Request spectrum */
  err = dbase_write_one(dev, SPECTRUM, &io);
  if (err < 0 || io != 1) {
    fprintf(stderr, "E: get_spectrum (written=%d)\n", io);
    err_str("dbase_get_spectrum()", err);
    return err < 0 ? err : -EIO;
  }

  /* Read spectrum bytes into temporary buffer */
  err = dbase_read(dev, (unsigned char *)tmp, len_bytes, &io);

  //  if(err < 0 || io != 4096){
  if (err < 0 || io != len_bytes) {
    fprintf(stderr,
            "E: possible incomplete read of spectral data (read %d bytes)\n",
            io);
    err_str("dbase_get_spectrum()", err);
    return err;
  }

  /* Read ok; parse int32, calc changes since last spectrum and update spectrum
   */
  for (io = 0; io < len; io++) {
    /* Check for first msmt after clear */
    if (onflag) {
      /* spec = all zeros; first measurement  */
      onflag &= (chans[io] == 0);
    }
    /*
      Watch out for byte order
    */
    if (IS_BIG_ENDIAN()) {
      BYTESWAP(tmp[io]);
    }
    last[io] = tmp[io] - chans[io];
    chans[io] = tmp[io];
  }

  /* If onflag is true, set all diff spectra to zeros */
  if (onflag) {
    for (io = 0; io < len; io++)
      last[io] = 0;
  }

  return err;
}

/*
  Print spectrum to file handle
*/
void dbase_print_spectrum_file(const int32_t *spec, int len, FILE *fh) {
  if (spec == NULL) {
    fprintf(stderr, "E: dbase_print_spectrum_file(), *spec was NULL\n");
    return;
  }
  if (fh == NULL) {
    fprintf(stderr, "E: dbase_print_spectrum_file(), *fh was NULL\n");
    return;
  }
  if (len <= 0) {
    fprintf(stderr, "E: dbase_print_spectrum_file(), len was %d\n", len);
    return;
  }
  /* 2012-02-14
     Used to call fprintf 'len' times,
     print to memory string first...
  */
  int k, left, w = 0;
  /* allocate 1 char per chan (including space) */
  const int n = 2048;
  char *buf = (char *)malloc(n);
  if (buf == NULL) {
    fprintf(stderr,
            "E: dbase_print_spectrum_file() unable to allocate memory\n");
    return;
  }

  for (k = 0; k < len; k++) {
    left = n - w;
    w += snprintf(buf + w, left, "%d ", spec[k]);
    /* Check for overflows in buffer -> flush to fh */
    if (left < 10) {
      fprintf(fh, "%s", buf);
      w = 0; /* reset written to 0 */
    }
  }
  fprintf(fh, "%s\n", buf);
  if (_DEBUG)
    fprintf(fh, "%d chars since buffer was cleared\n", w);
  free(buf);

  /* flush stream */
  if (fflush(fh) < 0)
    fprintf(stderr, "E: dbase_print_spectrum_file() when flushing stream\n");

  /* old code
  for(k=0;k<len;k++)
     fprintf(fh, "%d ", spec[k]);
  fprintf(fh, "\n"); */
}

/*
   Actual binary print method
*/
void dbase_print_file_spectrum_binary(const int32_t *arr, int len, FILE *fh) {
  if (fh != NULL && arr != NULL) {
    int wtn;
    wtn = fwrite(arr, sizeof(int32_t), len, fh);
    if (wtn != len) {
      fprintf(stderr,
              "E: dbase_print_file_spectrum_binary() written %d should be %d, "
              "errno=%d\n",
              wtn, len, errno);
    }
    if (fflush(fh) < 0)
      fprintf(stderr,
              "E: dbase_print_file_spectrum_binary() when flushing stream\n");
  } else if (arr != NULL)
    fprintf(stderr,
            "E: dbase_print_file_spectrum_binary(): FILE handle was NULL\n");
  else
    fprintf(
        stderr,
        "E: dbase_print_file_spectrum_binary(): spectrum handle was NULL\n");
}

/*
   digibase init-sequence
*/
int dbase_init(libusb_device_handle *dev) {
  int err, written;

  /* PHASE I: */
  if (_DEBUG > 0)
    printf("\n\nInit - PHASE I:\n");

  /* ?? Don't know if these are neccessary... ?? */
  {
    err = libusb_clear_halt(dev, EP_IN);
    if (err < 0) {
      fprintf(stderr, "W: libusb_clear_halt() (EP IN:  %d) failed\n", EP_IN);
      err_str("dbase_init()", err);
    }
    err = libusb_clear_halt(dev, EP_OUT);
    if (err < 0) {
      fprintf(stderr, "W: libusb_clear_halt() (EP OUT: %d) failed\n", EP_OUT);
      err_str("dbase_init()", err);
    }
  }

  /* Already awake?
     - If dbase is awake and initialized it will
     respond to status request with a 0x01
     if this read fails - START will work...
  */
  /*  err = dbase_write_one(dev, STATUS, &written);
  if(err == 0 && dbase_checkready(dev) == 1){
    if(_DEBUG > 0)
      printf("dbase_init() - Got response at first try!\n\n");
    return 1;
    }*/

  /* Send START command */
  err = dbase_write_one(dev, START, &written);
  if (err < 0 || written != 1) {
    fprintf(stderr, "E: When writing start command (err=%d, written=%d)\n", err,
            written);
    err_str("dbase_init()", err);
  }

  /* New Connection? */
  err = dbase_checkready(dev);
  if (err == 0) {
    if (_DEBUG > 0)
      printf("Device uninitialized - Starting init2():\n");
    return dbase_init2(dev);
  }
  /* Already awake - init done */
  else if (err == 1) {
    if (_DEBUG > 0)
      printf("dbase_init() - Device already awake.\n\n");
    return err;
  }
  /* Something's not right */
  return -1;
}

/*
   New Connection - send firmware packages
*/
int dbase_init2(libusb_device_handle *dev) {
  int k, err, written;

  /* PHASE II */
  if (_DEBUG > 0)
    printf("\n\nInit - PHASE II:\n");

  /* START2 command */
  err = dbase_write_one(dev, START2, &written);
  if (err < 0 || written != 1 || dbase_checkready(dev) != 0x01) {
    fprintf(stderr, "Error sending START2 command to device - aborting!\n");
    err_str("dbase_init2(START(2))", err);
    return -1;
  }

  /* Get filename:
     defined though gcc compiler option (-DPACK_PATH ...)
     in Makefile
  */
#ifndef PACK_PATH
  /* if no path is given, assume firmware in . */
  char str[] = "digiBase.rbf";
#else
#define STRLEN(s) (sizeof(s)/sizeof(s[0]))
  char str[STRLEN(PACK_PATH)];
  snprintf(str, sizeof(str), "%s", PACK_PATH);
#endif

  /* Write packs 1,2,NULL,3 */
  for (k = 0; k < 3; k++) {

    /* Get binary data from file */
    int len;
    unsigned char *buf = dbase_get_firmware_pack(str, k, &len);
    if (buf == NULL) {
      fprintf(stderr, "E: unable to read firmware - aborting\n");
      return -EIO;
    }

    /* Write pack k to dbase */
    err = dbase_write(dev, buf, len, &written);

    /* Free memory */
    free(buf);

    if (err < 0) {
      err_str("dbase_init2(), when writing package", err);
      return err;
    }
    if (written != len) {
      fprintf(stderr, "E: written=%d, read from file=%d\n", written, len);
      return -EIO;
    }
    if (k != 1 && dbase_checkready(dev) != 0x01) {
      fprintf(stderr, "E: sending pack%d to device - aborting!\n", k + 1);
      return -EIO;
    }

    if (k == 1) {
      /* Write dummy pack (NULL) */
      dbase_write(dev, NULL, 0, &written);
      if (err < 0 || dbase_checkready(dev) != 0x01) {
        err_str("dbase_init2(), when writing dummy package", err);
        fprintf(stderr, "E: sending dummy pack to device - aborting!\n");
        return -EIO;
      }
      if (_DEBUG > 0)
        printf("Wrote dummy package to device.\n");
    }
  }

  /* Write start */
  err = dbase_write_one(dev, START, &written);
  /* Confirm */
  if (err < 0 || dbase_checkready(dev) != 0x01) {
    fprintf(stderr, "E: sending start(2) command to device - aborting!\n");
    err_str("dbase_inti2(), when writing second START command", err);
    return -EIO;
  }

  /* PHASE III: */
  if (_DEBUG > 0)
    printf("\n\nInit - PHASE III:\n");

  /*
     Write status 1-5;
      - two from predefined _STAT
      - two to clear counters,
      - one to disable ltp/rtp at start
  */
  unsigned char *_stat = (unsigned char *)_STAT;
  const int sz = (int)sizeof(status_msg) + 1;
  unsigned char buf[81]; /* status_msg buffer */

  for (k = 0; k <= 4; k++) {
    memcpy(buf, _stat, sz); // _STAT is const
    if (k == 2) {
      if (_DEBUG)
        printf("setting clr bit\n");
      buf[77] |= (uint8_t)0x01;
    }
    if (k == 3) {
      if (_DEBUG)
        printf("clearing clr bit\n");
      buf[77] &= (uint8_t)0xfe;
    }
    if (k == 4) {
      /* 2012-02-27: clr l/r tp bits */
      if (_DEBUG)
        printf("clearing ltp/rtp in ctrl byte\n");
      buf[1] &= (uint8_t)LRTP_OFF_MASK;
    }
    // Write status
    err = dbase_write(dev, buf, sz, &written);
    if (k == 0) {
      _stat += sz; // Move pointer to next status_msg in _STAT3
    }
    if (err < 0 || written != sz) {
      fprintf(stderr, "E: writing status package %d, written = %d\n", k + 1,
              written);
      err_str("dbase_init2(), when writing status package", err);
      return err < 0 ? err : -EIO;
    }
    if (dbase_checkready(dev) != 0x01) {
      fprintf(stderr, "E: when sending pack%d command to device - aborting!\n",
              k + 1);
      return -EIO;
    }
  }

  /* Finally, clear spectrum */
  err = dbase_send_clear_spectrum(dev);
  if (err < 0) {
    fprintf(stderr, "E: failed to send clear spectrum in init()\n");
    err_str("dbase_init2(), when writing clear spectrum", err);
    return err;
  }

  /* init done */
  return 0;
}

/* Get firmware pack 0 <= n <= 2 */
unsigned char *dbase_get_firmware_pack(const char *file, int n, int *len) {
  if (file == NULL || n > 2 || n < 0)
    return NULL;

  /* fw packet sizes */
  int const pzs[3] = {61438, 61439, 44088};

  /* Open fw file */
  FILE *fh = fopen(file, "rb");
  if (fh == NULL) {
    fprintf(stderr,
            "E: dbase_get_firmware_pack() Unable to open digiBase.rbf file; %s "
            "(%s)\n%s\n",
            file, strerror(errno),
            "\tYou should set the right path in Makefile and recompile!");
    return NULL;
  }

  /* Read in packet n */
  int k, io, pos = 0;
  for (k = 0; k < n; k++)
    pos += pzs[k];
  unsigned char *buf = (unsigned char *)malloc(1024 * 62);
  if (buf == NULL) {
    fprintf(stderr, "E: get_firmware_pack(): Out of memory()\n");
    fclose(fh);
    return NULL;
  }
  /* copy bytes inp -> buf */
  fseek(fh, pos, SEEK_SET);
  io = fread(buf + 1, 1, pzs[n], fh);
  /* insert 0x05 at beginning of blob */
  buf[0] = 0x05;
  if (io != pzs[n]) {
    fprintf(stderr,
            "E: dbase_get_firmware_pack(): Unable to read file %s, only read "
            "%d bytes (%s)\n",
            file, io, strerror(errno));
    fclose(fh);
    return NULL;
  }
  /* Set length */
  *len = io + 1;
  if (_DEBUG > 0) {
    fprintf(stderr, "Read in package (%d/3): %d bytes read\n", k + 1, io);
  }

  /* success */
  fclose(fh);
  return buf;
}

/*
  Locate digiBASE on usb bus and return device handle

  - serial will be assigned with dbase's serial number
  - given_serial can be -1 for first device, otherwise
    the device with serial number 'given_serial' will be
    returned.
*/
libusb_device_handle *dbase_locate(int *serial, int given_serial,
                                   libusb_context *cntx) {
  libusb_device **list, *found = NULL; /* temp list and device */
  ssize_t cnt = libusb_get_device_list(cntx, &list);
  if (cnt < 0)
    return NULL;
  if (_DEBUG > 0)
    printf("Found %d usb devices\n", (int)cnt);
  ssize_t i;
  int err;

  struct libusb_device_descriptor desc; /* device descriptor struct */
  libusb_device_handle *handle = NULL;  /* digibase handle */

  /* Iterate over found usb devices until digibase is found */
  for (i = 0; i < cnt; i++) {
    libusb_device *device = list[i];
    err = libusb_get_device_descriptor(device, &desc);
    /* Is the device a dbase? */
    if (err == 0 && desc.idVendor == VENDOR_ID && desc.idProduct == PROD_ID) {
      if (_DEBUG > 0) {
        printf("Device %d was digibase\n", (int)i);
        printf("Opening device for serial no aquisition\n");
      }
      /* Open libusb connection: assign handle */
      if ((err = libusb_open(device, &handle)) < 0) {
        fprintf(stderr, "E: dbase_get_serial() Error opening device\n");
        libusb_free_device_list(list, 1);
        return NULL;
      }
      /* Return first dbase */
      if (given_serial == -1) {
        found = device;
        err = dbase_get_serial(found, handle, desc, serial);
        if (err < 0)
          fprintf(stderr, "W: dbase_get_serial returned %d\n", err);
        break;
      }
      /* Check if this dbase has the correct serial no */
      else {
        int tmp_serial = -1;
        err = dbase_get_serial(device, handle, desc, &tmp_serial);
        if (err < 0) {
          /* bail */
          libusb_close(handle);
          libusb_free_device_list(list, 1);
          return NULL;
        }
        if (given_serial == tmp_serial) {
          found = device;
          *serial = tmp_serial;
          break;
        }
      }
      /*
         Not the correct dbase
         close connection and check next device
      */
      libusb_close(handle);
    } else if (err < 0)
      err_str("libusb_get_device_descriptor()", err);
  }

  /* free libusb list, tell libusb to unref devices */
  libusb_free_device_list(list, 1);
  /* return dbase handle */
  return handle;
}

/* Returns the serial number for a located digibase */
int dbase_get_serial(libusb_device *dev, libusb_device_handle *handle,
                     struct libusb_device_descriptor desc, int *serial) {
  int err;
  unsigned char str[16];
  /* Get iSerialNumber string descriptor */
  err = libusb_get_string_descriptor_ascii(handle, desc.iSerialNumber, str,
                                           sizeof(str));
  /* Actually the len of str here... */
  if (err > 0) {
    *serial = atoi((char *)str);
    if (_DEBUG > 0)
      printf("Got serial: %d\n", *serial);
  } else if (err < 0) {
    fprintf(stderr, "E: dbase_get_serial(): unable to aquire serial no\n");
    err_str("dbase_get_serial()", err);
    return err;
  }
  return 0;
}

/*
  Basic consistency check
*/
int check_detector(const detector *det, const char *func) {
  if (det == NULL) {
    fprintf(stderr, "E: %s(): detector pointer was null\n",
            func == NULL ? "" : func);
    return -1;
  }
  if (det->dev == NULL) {
    fprintf(stderr, "E: %s(): detector->libusb_device_handle was null\n",
            func == NULL ? "" : func);
    return -1;
  }
  return 0;
}

/*
  getline(3) needed below isn't on MAC OS, define it here...

  This code is public domain -- Will Hartung 4/9/09
*/
//#ifdef __APPLE__
size_t my_getline(char **lineptr, size_t *n, FILE *stream) {
  char *bufptr = NULL;
  char *p = bufptr;
  size_t size;
  int c;

  if (lineptr == NULL || stream == NULL || n == NULL) {
    return -1;
  }
  bufptr = *lineptr;
  size = *n;

  c = fgetc(stream);
  if (c == EOF) {
    return -1;
  }
  if (bufptr == NULL) {
    bufptr = malloc(128);
    if (bufptr == NULL) {
      return -1;
    }
    size = 128;
  }
  p = bufptr;
  while (c != EOF) {
    if ((p - bufptr) > (size - 1)) {
      size = size + 128;
      bufptr = realloc(bufptr, size);
      if (bufptr == NULL) {
        return -1;
      }
    }
    *p++ = c;
    if (c == '\n') {
      break;
    }
    c = fgetc(stream);
  }

  *p++ = '\0';
  *lineptr = bufptr;
  *n = size;

  return p - bufptr - 1;
}
//#endif

/*
   Read in status meassage from text file
*/
int parse_status_lines(status_msg *status, FILE *fh) {
  size_t len = 63;
  char *line = (char *)malloc(len + 1); /* Line buffer */

  float tt, tt2;
  int t, t2, t3, llen, lineno = 0;

  /* sscanf() buffer, risk of overflow in sscanf() later... */
#define buflen_64 64
  char str[buflen_64];

  while ((llen = my_getline(&line, &len, fh)) >= 0 && lineno < 19) {
    /* Check line length against str buffer size */
    if (llen > buflen_64) {
      fprintf(stderr,
              "E: Risk of overflow in parse_status_lines()\n\t buflen=%d "
              "getline() read %d bytes!\n",
              buflen_64, llen);
      free(line);
      return -1; /* bail before corrupting memory */
    }
    switch (lineno) {
    case 4:
      sscanf(line, "RTP on     : %s]", str);
      if (strcmp(str, "Yes") == 0)
        status->CTRL |= (uint8_t)RTP_MASK;
      else
        status->CTRL &= (uint8_t)RTP_OFF_MASK;
      if (_DEBUG)
        printf("Parsed RTP EN to: %s\n", str);
      break;
    case 5:
      sscanf(line, "LTP on     : %s]", str);
      if (strcmp(str, "Yes") == 0)
        status->CTRL |= (uint8_t)LTP_MASK;
      else
        status->CTRL &= (uint8_t)LTP_OFF_MASK;
      if (_DEBUG)
        printf("Parsed LTP EN to: %s\n", str);
      break;
    case 6:
      sscanf(line, "Gain stab. : %s", str);
      if (strcmp(str, "Yes") == 0)
        status->CTRL |= (uint8_t)GS_MASK;
      else
        status->CTRL &= (uint8_t)GS_OFF_MASK;
      if (_DEBUG)
        printf("Parsed GS EN to: %s\n", str);
      break;
    case 7:
      sscanf(line, "Zero stab. : %s", str);
      if (strcmp(str, "Yes") == 0)
        status->CTRL |= (uint8_t)ZS_MASK;
      else
        status->CTRL &= (uint8_t)ZS_OFF_MASK;
      if (_DEBUG)
        printf("Parsed ZS EN to: %s\n", str);
      break;
    case 10:
      sscanf(line, "HV Target  : %d V", &t);
      status->HVT = (int)(t / HV_FACTOR);
      if (_DEBUG)
        printf("Parsed HVT: %d -> %d\n", t, status->HVT);
      break;
    case 11:
      sscanf(line, "Pulse width: %f us", &tt);
      status->PW = GET_INV_PW(tt);
      if (_DEBUG)
        printf("Parsed PW: %0.2f -> %d\n", tt, status->PW);
      break;
    case 12:
      // sscanf(line, "Fine Gain  : %f ", &tt);
      // status->FGN = tt * GAIN_FACTOR;
      sscanf(line, "Fine Gain  : %f (set: %f)\n", &tt, &tt2);
      status->FGN = GET_GAIN_VALUE(tt);
      if (_DEBUG)
        printf("Parsed FG: %0.5f -> %d\n", tt, status->FGN);
      break;
    case 13:
      sscanf(line, "Live Time Preset  : %f s", &tt);
      status->LTP = GET_TICKS(tt);
      if (_DEBUG)
        printf("Parsed LTP: %0.2f -> %d\n", tt, status->LTP);
      break;
    case 15:
      sscanf(line, "Real Time Preset  : %f s", &tt);
      status->RTP = GET_TICKS(tt);
      if (_DEBUG)
        printf("Parsed RTP: %0.2f -> %d\n", tt, status->RTP);
      break;
    case 17:
      sscanf(line, "Gain Stab. chans  : [%d %d %d]", &t, &t2, &t3);
      status->GSL = t;
      status->GSC = t2;
      status->GSU = t3;
      if (_DEBUG)
        printf("Parsed GAIN: %d %d %d\n", status->GSL, status->GSC,
               status->GSU);
      break;
    case 18:
      sscanf(line, "Zero Stab. chans  : [%d %d %d]", &t, &t2, &t3);
      status->ZSL = t;
      status->ZSC = t2;
      status->ZSU = t3;
      if (_DEBUG)
        printf("Parsed GAIN: %d %d %d\n", status->ZSL, status->ZSC,
               status->ZSU);
      break;
    default:
      break;
    }
    lineno++;
  }

  free(line);
  return 0;
}

/*
   Print libusb errors
*/
void err_str(const char *func, enum libusb_error err) {
  const char *err_msg = NULL;
  switch (err) {
  case LIBUSB_SUCCESS:
    if (_DEBUG > 0)
      printf("LUSB: OK (%s)\n", func == NULL ? "" : func);
    return;
  case LIBUSB_ERROR_IO:
    err_msg = "LUSB_E: IO error";
    break;
  case LIBUSB_ERROR_ACCESS:
    err_msg = "LUSB_E: Access error";
    break;
  case LIBUSB_ERROR_NO_DEVICE:
    err_msg = "LUSB_E: No device error";
    break;
  case LIBUSB_ERROR_NOT_FOUND:
    err_msg = "LUSB_E: Not found error";
    break;
  case LIBUSB_ERROR_BUSY:
    err_msg = "LUSB_E: Device busy error";
    break;
  case LIBUSB_ERROR_TIMEOUT:
    err_msg = "LUSB_E: Timeout error";
    break;
  case LIBUSB_ERROR_OVERFLOW:
    err_msg = "LUSB_E: Overflow error";
    break;
  case LIBUSB_ERROR_PIPE:
    err_msg = "LUSB_E: Pipe error";
    break;
  case LIBUSB_ERROR_INTERRUPTED:
    err_msg = "LUSB_E: Interrupted error";
    break;
  case LIBUSB_ERROR_NO_MEM:
    err_msg = "LUSB_E: No memory error";
    break;
  case LIBUSB_ERROR_NOT_SUPPORTED:
    err_msg = "LUSB_E: Not supported error";
    break;
  case LIBUSB_ERROR_OTHER:
    err_msg = "LUSB_E: Other error";
    break;
  default:
    err_msg = "LUSB_E: Unknown error";
    break;
  }
  if (err_msg != NULL) {
    if (func != NULL)
      fprintf(stderr, "LUSB_E in function %s:\n", func);
    fprintf(stderr, "%s\n", err_msg);
  }
}

/*
  Big endian conversion functions

  Swap byte order in status struct
*/
void dbase_byte_swap_status_struct(status_msg *stat) {
  /* 2012-02-27, we'll swap these as well */
  BYTESWAP(stat->TMR);
  BYTESWAP(stat->AFGN);

  BYTESWAP(stat->FGN);
  BYTESWAP(stat->LLD);
  BYTESWAP(stat->LTP);
  BYTESWAP(stat->LT);
  BYTESWAP(stat->RTP);
  BYTESWAP(stat->RT);
  BYTESWAP(stat->LEN);
  BYTESWAP(stat->HVT);
  BYTESWAP(stat->GSU);
  BYTESWAP(stat->GSC);
  BYTESWAP(stat->GSL);
  BYTESWAP(stat->ZSU);
  BYTESWAP(stat->ZSC);
  BYTESWAP(stat->ZSL);
}

/* swap byte-order in b */
void dbase_switch_endian(unsigned char *b, int n) {
  register int i = 0;
  register int j = n - 1;
  char c;
  while (i < j) {
    c = b[i];
    b[i] = b[j];
    b[j] = c;
    i++, j--;
  }
}
