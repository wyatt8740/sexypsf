/*  Pcsx - Pc Psx Emulator
 *  Copyright (C) 1999-2002  Pcsx Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include "driver.h"
#include "Linux.h"

#include <ncurses.h>

static int volPrime=128;
int *volPrimePt=&volPrime;

int main(int argc, char *argv[]) {

  newterm(NULL, stdout, stdin);
  noecho();
  refresh();
	PSFINFO *pi;

	SetupSound();
        int i=1;
         while(i<argc)// only last argument can be filename
        {
          if(strncmp(argv[i],"-v",3)==0)
          {
            i++;
              extern int* volPt;
              *volPt=atoi(argv[i]);
              *volPrimePt=atoi(argv[i]);
/*              printw("vol: %s (%d)\n",argv[i], *volPt);
                refresh();*/
          }
          i++;
        }
        printw("Opening '%s'.\n",argv[argc-1]);
	if(!(pi=sexy_load(argv[argc-1])))
	{
          endwin();
          printf("Error loading PSF %s\n",argv[argc-1]);
	 return(-1);
	}

/*	printw("Game:\t%s\nTitle:\t%s\nArtist:\t%s\nYear:\t%s\nGenre:\t%s\nPSF By:\t%s\nCopyright:\t%s\n",
		pi->game,pi->title,pi->artist,pi->year,pi->genre,pi->psfby,pi->copyright);
        refresh();*/
	{
	 PSFTAG *recur=pi->tags;
 	 while(recur)
	 {
	  printw("%s:\t%s\n",recur->key,recur->value);
	  recur=recur->next;
	 }
        }
         refresh();
	sexy_execute();
        endwin();
	return 0;
}
