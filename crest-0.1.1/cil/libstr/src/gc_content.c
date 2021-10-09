/* Calculates GC content of a sequence character string
 * (c) 1999 Karl T. Diedrich
 * http://deodas.sourceforge.net/
 */

/*
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */


/* portability header*/
#include <config.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define DEBUG 1

float gc_content (char *seq)
{
  unsigned int len = strlen (seq), gc = 0;
  
  while (*seq) {
    if ((toupper (*seq) == 'G') || (toupper (*seq) == 'C')) gc++; 
    *seq++;
  }

#if DEBUG
  printf("The length of the sequence is %u.\n", len);
  printf("Total GC = %u.\n", gc);
#endif
  
  return (float) gc/ (float) len * 100;
}
