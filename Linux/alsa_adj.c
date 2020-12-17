/***************************************************************************
                            alsa.c  -  description
                             -------------------
    begin                : Mon Aug 24 2020
    copyright            : (C) 2020 by Wyatt Ward
    email                : wyattfward@gmail.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version. See also the license.txt file for *
 *   additional informations.                                              *
 *                                                                         *
 ***************************************************************************/

/*
 * This is essentially just my "baby's first ALSA project" - the first
 * thing I have ever written using ALSA at all. It is borrowing heavily from
 * an old Linux Journal article ( https://www.linuxjournal.com/article/6735 )
 * and I wrote it because I was getting slightly annoyed with having to wrap
 * the OSS version of this program, since XMMS 1.x has been dead for quite a
 * while now.
 *
 * I also tried to follow the flow of the `oss.c` code as much as I could.
 * I have never written OSS code either.
 *
 * It should be a drop-in replacement for oss.c, if you link to libasound.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sched.h>

#include <termios.h>

#include <ncurses.h>

/* alsa globals */
#include <alsa/asoundlib.h>

/* sexy_stop defined elsewhere (../)*/
#include <driver.h>

#define PCM_DEVICE "default"
snd_pcm_t *alsa_pcm_handle;
snd_pcm_uframes_t alsa_periods;          /* Number of fragments/periods */
snd_pcm_hw_params_t *alsa_params;
int alsa_channels=2;
unsigned int alsa_rate=44100;
unsigned int alsa_pcm;
unsigned int alsa_tmp;

/* func signatures */
void SetupSound(void);
void RemoveSound(void);
void sexyd_update(unsigned char* pSound, long lBytes);


void SetupSound(void)
{
  if( (alsa_pcm = snd_pcm_open(&alsa_pcm_handle, PCM_DEVICE, SND_PCM_STREAM_PLAYBACK, 0))
      < 0)
  {
    /* error opening handle */
    fprintf(stderr,"Error opening a handle to ALSA device: %s\n",snd_strerror(alsa_pcm));
  }
  snd_pcm_hw_params_alloca(&alsa_params);
  snd_pcm_hw_params_any(alsa_pcm_handle, alsa_params);
  if ((alsa_pcm = snd_pcm_hw_params_set_access(alsa_pcm_handle, alsa_params,
                                              SND_PCM_ACCESS_RW_INTERLEAVED))
      < 0)
  {
    fprintf(stderr,"Error setting interleaved mode: %s\n", snd_strerror(alsa_pcm));
  }
  if ((alsa_pcm = snd_pcm_hw_params_set_format(alsa_pcm_handle, alsa_params,
                                               SND_PCM_FORMAT_S16)) /* LE or BE autodetermined */
      < 0)
  {
    fprintf(stderr, "Error setting PCM format: %s\n", snd_strerror(alsa_pcm));
  }
  if ((alsa_pcm = snd_pcm_hw_params_set_channels(alsa_pcm_handle, alsa_params,
                                                 2))
      < 0)
  {
    fprintf(stderr, "Error setting channel count: %s\n", snd_strerror(alsa_pcm));
  }
  /* rate 44100hz */
  if ((alsa_pcm = snd_pcm_hw_params_set_rate_near(alsa_pcm_handle,
                                                   alsa_params,
                                                   &alsa_rate,
                                                  0))
      < 0)
  {
    fprintf(stderr,"Error setting sample rate: %s\n", snd_strerror(alsa_pcm));
  }
  if ((alsa_pcm = snd_pcm_hw_params(alsa_pcm_handle, alsa_params))
      < 0)
  {
    fprintf(stderr,"Error setting PCM hardware parameters: %s\n", snd_strerror(alsa_pcm));
  }
  printw("Using PCM:\t'%s'\n", snd_pcm_name(alsa_pcm_handle));
  snd_pcm_hw_params_get_channels(alsa_params, &alsa_tmp);
  printw("PCM channels:\t%i ", alsa_tmp);
  if (alsa_tmp == 1) {
    printw("(mono)\n");
  }
  else if (alsa_tmp == 2) {
    printw("(stereo)\n");
  }
  snd_pcm_hw_params_get_rate(alsa_params, &alsa_tmp, 0);
  printw("Sample rate:\t%d Hz\n", alsa_tmp);
  snd_pcm_hw_params_get_period_size(alsa_params, &alsa_periods, 0);
}

void sexyd_update(unsigned char* pSound, long lBytes)
{
  int check;
  if(!alsa_pcm_handle) {
    return;
  }
  /* dividing by four because it seems that's what alsa wants, as opposed to
     OSS. I know nothing about either system, honestly.
     My guess though is that the four is derived from two bytes per sample
     and two bytes per channel = four bytes total.
  */
  if ((alsa_pcm = snd_pcm_writei(alsa_pcm_handle, pSound, lBytes / 4) == -EPIPE)) {
    printw("XRUN.\n");
    snd_pcm_prepare(alsa_pcm_handle);
  } else if (alsa_pcm < 0) {
    printw("ERROR. Can't write to PCM device. %s\n", snd_strerror(alsa_pcm));
  }
  snd_pcm_hw_params_get_period_time(alsa_params, &alsa_tmp, NULL);
  /* quit if stdin immediately contains data, and starts with a 'q'
     (user requests quit) */
  #ifndef NONINTERACTIVE
  extern int* volPt; /* for volume control, defined in spu.c */
  if(!ioctl(fileno(stdin),FIONREAD,&check))
  {
    if(check)
    {
      int curpos_y, curpos_x;
      int buf[256];
   /* char buf[256];
      fgets(buf,256,stdin);*/
      refresh();
      buf[0]=wgetch(stdscr);
      refresh();
      switch(buf[0])
      {
      case 'q':
        sexy_stop();
        break;
      case '/':
      case 'd':
//        printw("volPt: %d -> ",*volPt);
        /* volume down, min 0 */
        if(*volPt >= 1)
        {
          *volPt = *volPt - 1;
        }
        getyx(stdscr,curpos_y,curpos_x);
        move(curpos_y, 0);
        clrtoeol();
        printw("Volume: %d",*volPt);
        refresh();
        break;
      case '*':
      case 'u':
//        printw("volPt: %d -> ",*volPt);
        /* volume up, max 128 */
        if(*volPt <= 127)
        {
          *volPt = *volPt + 1;
        }
        getyx(stdscr,curpos_y,curpos_x);
        move(curpos_y, 0);
        clrtoeol();
        printw("Volume: %d",*volPt);
        refresh();
        break;
      }
    }
  }
  #endif
}

void RemoveSound(void)
{
  /* end */
  if(alsa_pcm_handle)
  {
    snd_pcm_drain(alsa_pcm_handle);
    snd_pcm_close(alsa_pcm_handle);
    snd_pcm_close(alsa_pcm_handle);
    alsa_pcm_handle=NULL;
  }
}

