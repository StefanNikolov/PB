/* ///////////////////////////////////////////////////////////////////////////
/// APBS -- Adaptive Poisson-Boltzmann Solver
///
///  Nathan A. Baker (nbaker@wasabi.ucsd.edu)
///  Dept. of Chemistry and Biochemistry
///  Dept. of Mathematics, Scientific Computing Group
///  University of California, San Diego 
///
///  Additional contributing authors listed in the code documentation.
///
/// Copyright � 1999. The Regents of the University of California (Regents).
/// All Rights Reserved. 
/// 
/// Permission to use, copy, modify, and distribute this software and its
/// documentation for educational, research, and not-for-profit purposes,
/// without fee and without a signed licensing agreement, is hereby granted,
/// provided that the above copyright notice, this paragraph and the
/// following two paragraphs appear in all copies, modifications, and
/// distributions.
/// 
/// IN NO EVENT SHALL REGENTS BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT,
/// SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS,
/// ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF
/// REGENTS HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  
/// 
/// REGENTS SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT
/// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
/// PARTICULAR PURPOSE.  THE SOFTWARE AND ACCOMPANYING DOCUMENTATION, IF
/// ANY, PROVIDED HEREUNDER IS PROVIDED "AS IS".  REGENTS HAS NO OBLIGATION
/// TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
/// MODIFICATIONS. 
//////////////////////////////////////////////////////////////////////////// 
/// rcsid="$Id$"
//////////////////////////////////////////////////////////////////////////// */

/* ///////////////////////////////////////////////////////////////////////////
// Picking out numbers of vertices that fit the multigrid algorithm can tax my
// math abilities.  Note that PMG uses about 20 N (usually 160 B/vert) memory.
//
// Author: Nathan Baker
/////////////////////////////////////////////////////////////////////////// */

#include <math.h>
#include <stdio.h>

int main(int argc, char **argv) {

    int i, lev;
    int maxvert = 700;
    int minlev = 3;
    double newval, oldval;

    printf("\n\nThis program determines the acceptable meshpoint number/level\n");
    printf("combinations for the PMG multigrid libraries and %d or more.\n",
      minlev);
    printf("levels in the mesh (because you typically use one less than the\n");
    printf("max number of levels\n\n\n");

    for (i=2; i<maxvert; i++) { 
        /* the number of times it's divisible. */
        lev = 0;
        newval = (double)(i-1);
        oldval = (double)(i-1);
        while (1) {
           oldval = newval;
           newval = newval/2.0;
           if ((floor(newval) != newval) || (ceil(newval) != newval)) break;
           lev++;
        } 
        if (lev >= minlev) {
            printf("%4d verts/direction => %d levels\n", i, lev);
            printf("                        %d verts on coarsest level\n",
              (int)oldval); 
            printf("                        ~%g MB memory (for %d^3 mesh)\n",
              (double)(i*i*i)*160.0/1024.0/1024.0, i);
        }
    }

#if 0
    int i, maxmult = 30;
    int j, maxlev = 5;
    double log2 = log(2.0);
    double x;

    for (i=0; i<maxlev; i++) {
        for (j=0; j<maxmult; j++) {
            printf("%g ", j*pow(2,i+1) + 1);
        }
        printf("\n");
    }
#endif
   
    return 0;
}