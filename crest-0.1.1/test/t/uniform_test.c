/* Copyright (c) 2008, Jacob Burnim (jburnim@cs.berkeley.edu)
 *
 * This file is part of CREST, which is distributed under the revised
 * BSD license.  A copy of this license can be found in the file LICENSE.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See LICENSE
 * for details.
 */

#include <crest.h>
#include <stdio.h>
#include <assert.h>

void check(int a, int b, int c, int d){
        if (a == 5) {
                if (b == 19) {
                        if (c == 7) {
                                if (d == 4) {
                                        fprintf(stderr, "GOAL!\n");
                                }
                        }
                }
        }  
}


int main(void) {
        int a, b, c, d;
        CREST_int(a);
        CREST_int(b);
        CREST_int(c);
        CREST_int(d);

        check(a,b,c,d);

        assert(a > 5);

		__CrestLogPC(3);
        return 0;
}
