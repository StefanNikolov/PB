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
// File:     vpee.c
//
// Purpose:  Class Vpee: methods.
//
// Author:   Nathan Baker
/////////////////////////////////////////////////////////////////////////// */

#include "apbs/vpee.h"
VEXTERNC double Alg_estNonlinResid(Alg *thee, SS *sm, int u, int ud, int f);
VEXTERNC double Alg_estDualProblem(Alg *thee, SS *sm, int u, int ud, int f);
VEXTERNC double Alg_estLocalProblem(Alg *thee, SS *sm, int u, int ud, int f);


VEMBED(rcsid="$Id$")

/* ///////////////////////////////////////////////////////////////////////////
// Class Vpee: Non-inlineable methods
/////////////////////////////////////////////////////////////////////////// */

/* ///////////////////////////////////////////////////////////////////////////
// Routine:  Vpee_ctor
//
// Purpose:  Construct the parallel error estimator
//     
// Args:     GM              a pointer to an initialized and partitioned mesh
//           localPartID     the local partition ID
//           killFlag        a flag to indicate how error esimates are to be 
//                           attenuated outside the local partition.
//                           0 => no attenuation
//                           1 => all error outside the local partition set to
//                                zero
//                           2 => all error is set to zero outside a sphere of 
//                                radius (killParam*partRadius), where
//                                partRadius is the radius of the sphere
//                                circumscribing the local partition
//                           3 => all error is set to zero except for the local
//                                partition and its immediate neighbors
//
// Author:   Nathan Baker
/////////////////////////////////////////////////////////////////////////// */
VPUBLIC Vpee* Vpee_ctor(Vgm *gm, int localPartID, int killFlag, double
  killParam) {

    Vpee *thee = VNULL;

    /* Set up the structure */
    thee = Vmem_malloc(VNULL, 1, sizeof(Vpee) );
    VASSERT( thee != VNULL);
    VASSERT( Vpee_ctor2(thee, gm, localPartID, killFlag, killParam));

    return thee;
}

/* ///////////////////////////////////////////////////////////////////////////
// Routine:  Vpee_ctor2
//
// Purpose:  Construct the parallel error estimator
//
// Author:   Nathan Baker
/////////////////////////////////////////////////////////////////////////// */
VPUBLIC int Vpee_ctor2(Vpee *thee, Vgm *gm, int localPartID, int killFlag,
  double killParam) {

    int *usedVert;
    int ivert, isimp, nLocalVerts;
    SS *simp;
    VV *vert;
    double radius, dx, dy, dz;

    VASSERT(thee != VNULL);

    thee->gm = gm;
    thee->localPartID = localPartID;
    thee->killFlag = killFlag;
    thee->killParam = killParam;
    thee->mem = Vmem_ctor("APBS::VPEE");

    /* Now, figure out the center of geometry for the local partition.  We 
     * need to keep track of vertices so we don't double count them. */
    usedVert = (int *)Vmem_malloc(thee->mem, Vgm_numVV(thee->gm), sizeof(int));
    for (ivert=0; ivert<Vgm_numVV(thee->gm); ivert++) usedVert[ivert] = 0;
    thee->localPartCenter[0] = 0.0;
    thee->localPartCenter[1] = 0.0;
    thee->localPartCenter[2] = 0.0;
    nLocalVerts = 0;
    for (isimp=0; isimp<Vgm_numSS(thee->gm); isimp++) {
        simp = Vgm_SS(thee->gm, isimp);
        if (SS_chart(simp) == thee->localPartID) {
            for (ivert=0; ivert<Vgm_dimVV(thee->gm); ivert++) {
                vert = SS_vertex(simp, ivert);
                if (usedVert[VV_id(vert)] == 0) {
                    usedVert[VV_id(vert)] = 1;
                    thee->localPartCenter[0] += VV_coord(vert, 0);
                    thee->localPartCenter[1] += VV_coord(vert, 0);
                    thee->localPartCenter[2] += VV_coord(vert, 0);
                    nLocalVerts++;
                }
            }
        }
    }
    VASSERT(nLocalVerts > 0);
    thee->localPartCenter[0] =
      thee->localPartCenter[0]/((double)(nLocalVerts));
    thee->localPartCenter[1] =
      thee->localPartCenter[1]/((double)(nLocalVerts));
    thee->localPartCenter[2] =
      thee->localPartCenter[2]/((double)(nLocalVerts));
    Vnm_print(2, "Vpee_ctor2: Part %d centered at (%4.3f, %4.3f, %4.3f)\n",
      thee->localPartID, thee->localPartCenter[0], thee->localPartCenter[1],
      thee->localPartCenter[2]);


    /* Now, figure out the radius of the sphere circumscribing the local
     * partition.  We need to keep track of vertices so we don't double count 
     * them. */
    for (ivert=0; ivert<Vgm_numVV(thee->gm); ivert++) usedVert[ivert] = 0;
    thee->localPartRadius = 0.0;
    for (isimp=0; isimp<Vgm_numSS(thee->gm); isimp++) {
        simp = Vgm_SS(thee->gm, isimp);
        if (SS_chart(simp) == thee->localPartID) {
            for (ivert=0; ivert<Vgm_dimVV(thee->gm); ivert++) {
                vert = SS_vertex(simp, ivert);
                if (usedVert[VV_id(vert)] == 0) {
                    usedVert[VV_id(vert)] = 1;
                    dx = thee->localPartCenter[0] - VV_coord(vert, 0);
                    dy = thee->localPartCenter[1] - VV_coord(vert, 1);
                    dz = thee->localPartCenter[2] - VV_coord(vert, 2);
                    radius = dx*dx + dy*dy + dz*dz;
                    if (radius > thee->localPartRadius) thee->localPartRadius =
                      radius;
                }
            }
        }
    }
    thee->localPartRadius = VSQRT(thee->localPartRadius);
    Vnm_print(2, "Vpee_ctor2: Part %d has circumscribing sphere of radius %4.3f\n", 
      thee->localPartID, thee->localPartRadius);
    Vmem_free(thee->mem, Vgm_numVV(thee->gm), sizeof(int), 
      (void **)&usedVert);

    return 1;
}

/* ///////////////////////////////////////////////////////////////////////////
// Routine:  Vpee_dtor
//
// Purpose:  Clean up
//
// Author:   Nathan Baker
/////////////////////////////////////////////////////////////////////////// */
VPUBLIC void Vpee_dtor(Vpee **thee) {
    
    if ((*thee) != VNULL) {
        Vpee_dtor2(*thee);
        Vmem_free(VNULL, 1, sizeof(Vpee), (void **)thee);
        (*thee) = VNULL;
    }

}

/* ///////////////////////////////////////////////////////////////////////////
// Routine:  Vpee_dtor2
//
// Purpose:  Clean up
//
// Author:   Nathan Baker
/////////////////////////////////////////////////////////////////////////// */
VPUBLIC void Vpee_dtor2(Vpee *thee) { Vmem_dtor(&(thee->mem)); }

/* ///////////////////////////////////////////////////////////////////////////
// Routine:  Vpee_markRefine
//
// Purpose:  A wrapper/reimplementation of AM_markRefine that allows for more
//           flexible attenuation of error-based markings outside the local
//           partition.  Specifically, the error in each simplex is estimated
//           using the user-specified error estimator and then modified by the
//           method (see killFlag) specified in the Vpee constructor.  This
//           allows the user to confine refinement to an arbitrary area around
//           the local partition.
//
// Args:     am            A pointer to the AM object from which the
//                         solution/error should be obtained
//           level         The solution level in the AM object to be used
//           akey          The marking method:
//                          -1 => Reset markings  \
//                           0 => Uniform          >--> killFlag has no effect
//                           1 => User defined    /
//                           2 => Nonlinear residual indicator
//                           3 => Local problem indicator
//                           4 => Dual problem indicator
//           rcol          The ID of the main partition on which to mark (or -1
//                           if all partitions should be marked)
//           etol          The error tolerance criterion for marking
//
// Returns:  The number of simplices marked for refinement.
//
// Notes:    This is pretty much a rip-off of AM_markRefine.
//
// Author:   Nathan Baker (and Michael Holst: the author of AM_markRefine, on
//           which this is based)
/////////////////////////////////////////////////////////////////////////// */
VPUBLIC int Vpee_markRefine(Vpee *thee, AM *am, int level, int akey, int rcol, 
  double etol) {

    Alg *alg;
    int markMe, marked = 0;
    int i, ivert, smid, count, currentQ;
    double minError = 0.0;
    double maxError = 0.0;
    double errEst = 0.0;
    double dist, dx, dy, dz;
    SS *sm;


    VASSERT(thee != VNULL);

    /* Get the Alg object from AM */
    VASSERT((level >= am->minLevel) && (level <= am->maxLevel));
    alg = AM_alg(am, level);
    VASSERT(alg != VNULL);

    /* input check and some i/o */
    if ( ! ((-1 <= akey) && (akey <= 4)) ) {
        Vnm_print(0,"Vpee_markRefine: bad refine key; simplices marked = %d\n",
            marked);
        return marked;
    }

    /* For geometry-based markings, we have no effect */
    if ((-1 <= akey) && (akey <= 1)) {
        marked = Vgm_markRefine(thee->gm, akey, rcol);
        return marked;
    }

    /* Check the relevant I/O */
    if (akey == 2) {
        Vnm_print(0,"Vpee_markRefine: using Alg_estNonlinResid().\n");
    } else if (akey == 3) {
        Vnm_print(0,"Vpee_markRefine: using Alg_estLocalProblem().\n");
    } else if (akey == 4) {
        Vnm_print(0,"Vpee_markRefine: using Alg_estDualProblem().\n");
    } else {
        Vnm_print(2,"Vpee_markRefine: bad akey given; simplices marked = %d\n",
            marked);
        return marked;
    }
    if (thee->killFlag == 0) {
        Vnm_print(0, "Vpee_markRefine: No error attenuation -- simplices in all partitions will be marked.\n");
    } else if (thee->killFlag == 1) {
        Vnm_print(0, "Vpee_markRefine: Maximum error attenuation -- only simplices in local partition will be marked.\n");
    } else if (thee->killFlag == 2) {
        Vnm_print(0, "Vpee_markRefine: Spherical error attenutation -- simplices within a sphere of %4.3f times the size of the partition will be marked\n", 
          thee->killParam);
    } else if (thee->killFlag == 2) {
        Vnm_print(0, "Vpee_markRefine: Neighbor-based error attenuation -- simplices in the local and neighboring partitions will be marked [NOT IMPLEMENTED]!\n");
        VASSERT(0);
    } else {    
        Vnm_print(2,"Vpee_markRefine: bogus killFlag given; simplices marked = %d\n",
            marked);
        return marked;
    }

    /* timer */
    Vnm_tstart(30, "error estimation");

    /* count = num generations to produce from marked simplices (minimally) */
    count = 1; /* must be >= 1 */

    /* check the refinement Q for emptyness */
    currentQ = 0;
    if (Vgm_numSQ(thee->gm,currentQ) > 0) {
        Vnm_print(0,"Vpee_markRefine: non-empty refinement Q%d....clearing..",
            currentQ);
        Vgm_resetSQ(thee->gm,currentQ);
        Vnm_print(0,"..done.\n");
    }
    if (Vgm_numSQ(thee->gm,!currentQ) > 0) {
        Vnm_print(0,"Vpee_markRefine: non-empty refinement Q%d....clearing..",
            !currentQ);
        Vgm_resetSQ(thee->gm,!currentQ);
        Vnm_print(0,"..done.\n");
    }
    VASSERT( Vgm_numSQ(thee->gm,currentQ)  == 0 );
    VASSERT( Vgm_numSQ(thee->gm,!currentQ) == 0 );

    /* clear everyone's refinement flags */
    Vnm_print(0,"Vpee_markRefine: clearing all simplex refinement flags..");
    for (i=0; i<Vgm_numSS(thee->gm); i++) {
        if ( (i>0) && (i % VPRTKEY) == 0 ) Vnm_print(0,"[MS:%d]",i);
        sm = Vgm_SS(thee->gm,i);
        SS_setRefineKey(sm,currentQ,0);
        SS_setRefineKey(sm,!currentQ,0);
        SS_setRefinementCount(sm,0); /* refine X many times? */
    }
    Vnm_print(0,"..done.\n");

    /* gerror = global error accumulation */
    alg->gerror = 0.;

    /* traverse the simplices and estimate the error */
    Vnm_print(0,"Vpee_markRefine: estimating error..");
    smid = 0;
    while ( (smid < Vgm_numSS(thee->gm)) && (!Vsig_sigInt()) ) {
        markMe = 0;
        sm = Vgm_SS(thee->gm,smid);

        if ( (smid>0) && (smid % VPRTKEY) == 0 ) Vnm_print(0,"[MS:%d]",smid);

        /* Produce an error estimate for this element */
        if (akey == 2) {
            errEst = Alg_estNonlinResid(alg, sm, W_u, W_ud, W_f);
        } else if (akey == 3) {
            errEst = Alg_estLocalProblem(alg, sm, W_u,W_ud,W_f);
        } else if (akey == 4) {
            errEst = Alg_estDualProblem(alg, sm, W_u,W_ud,W_f);
        }
        VASSERT( errEst >= 0. );

        /* Figure out whether or not the simplex gets marked. */
        if (thee->killFlag == 0) {
            if (errEst > etol) markMe = 1;
        } else if (thee->killFlag == 1) {
            if ( (SS_chart(sm) == rcol) || (rcol < 0)) {
                if (errEst > etol) markMe = 1;
            }
        } else if (thee->killFlag == 2) {
            if (rcol < 0) markMe = 1;
            /* Find the closest distance between this simplex and the center of
             * the local partition and check it against
             * (thee->localPartRadius*thee->killParam) */
            dist = 0;
            for (ivert=0; ivert<SS_dimVV(sm); ivert++) {
                dx = VV_coord(SS_vertex(sm, ivert), 0) -
                  thee->localPartCenter[0];
                dy = VV_coord(SS_vertex(sm, ivert), 1) -
                  thee->localPartCenter[1];
                dz = VV_coord(SS_vertex(sm, ivert), 2) -
                  thee->localPartCenter[2];
                dist = VSQRT((dx*dx + dy*dy + dz*dz));
            }
            if ( (dist < thee->localPartRadius*thee->killParam) && 
                 (errEst > etol) ) {
                markMe = 1;
            }
        } else if (thee->killFlag == 3) {
            VASSERT(0);
        } else VASSERT(0);

        /* If it's supposed to be marked; mark it */
        if (markMe) {
            marked++;
            Vgm_appendSQ(thee->gm,currentQ, sm); /*add to refinement Q*/
            SS_setRefineKey(sm,currentQ,1);      /* note now on refine Q */
            SS_setRefinementCount(sm,count);     /* refine X many times? */
        }
     
        /* Accounting just for our partition */ 
        if ( (SS_chart(sm) == rcol) || (rcol < 0)) {
            /* keep track of min/max errors over the mesh */
            minError = VMIN2( VABS(errEst), minError );
            maxError = VMAX2( VABS(errEst), maxError );

            /* store the estimate */
            Bvec_set( alg->WE[ WE_err ], 0, smid, errEst );

            /* accumlate into the global error */
            alg->gerror += errEst;
        }
    
        smid++;
    }

    /* do some i/o */
    Vnm_print(0,"..done.  [marked=<%d>  gerror=<%g>]\n", marked, alg->gerror);
    Vnm_print(0,"Alg_estRefine: elevel=<%g>  maxError=<%g>  minError=<%g>\n",
        etol, maxError, minError);
    Vnm_tstop(30, "error estimation");

    /* return */
    return marked;

}

