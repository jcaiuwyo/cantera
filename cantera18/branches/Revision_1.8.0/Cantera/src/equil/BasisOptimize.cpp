/**
 * @file BasisOptimize.cpp
 *     Functions which calculation optimized basis of the 
 *     stoichiometric coefficient matrix (see /ref equil functions)
 */

/*
 * $Id$
 */

#include "ct_defs.h"
#include "global.h"
#include "ThermoPhase.h"
#include "MultiPhase.h"

#include <string.h>

using namespace Cantera;
using namespace std;

#ifdef DEBUG_MODE
namespace Cantera {
  int BasisOptimize_print_lvl = 0;
}
static void print_stringTrunc(const char *str, int space, int alignment);
#endif

static int amax(double *x, int j, int n);
static void switch_pos(vector_int &orderVector, int jr, int kspec);
static int mlequ(double *c, int idem, int n, double *b, int m);

//@{
#ifndef MIN
#define MIN(x,y) (( (x) < (y) ) ? (x) : (y))
#endif
//@}

/*
 * Choose the optimum basis for the calculations. This is done by 
 * choosing the species with the largest mole fraction 
 * not currently a linear combination of the previous components. 
 * Then, calculate the stoichiometric coefficient matrix for that 
 * basis. 
 *
 * Calculates the identity of the component species in the mechanism. 
 * Rearranges the solution data to put the component data at the 
 * front of the species list. 
 *
 * Then, calculates SC(J,I) the formation reactions for all noncomponent 
 * species in the mechanism. 
 *
 * Input 
 * --------- 
 * mphase          Pointer to the multiphase object. Contains the 
 *                 species mole fractions, which are used to pick the
 *                 current optimal species component basis.
 * orderVectorElement
 *                 Order vector for the elements. The element rows
 *                 in the formula matrix are
 *                 rearranged according to this vector.
 * orderVectorSpecies
 *                 Order vector for the species. The species are
 *                 rearranged according to this formula. The first
 *                 nCompoments of this vector contain the calculated
 *                 species components on exit.
 * doFormRxn       If true, the routine calculates the formation
 *                 reaction matrix based on the calculated 
 *                 component species. If false, this step is skipped.
 * 
 * Output 
 * --------- 
 * usedZeroedSpecies = If true, then a species with a zero concentration
 *                     was used as a component. The problem may be
 *                     converged.
 * formRxnMatrix 
 *
 * Return
 * --------------
 * returns the number of components.
 *
 *
 */
int Cantera::BasisOptimize(int *usedZeroedSpecies, bool doFormRxn,
               MultiPhase *mphase, vector_int & orderVectorSpecies,
               vector_int & orderVectorElements, 
               vector_fp & formRxnMatrix) {

  int  j, jj, k=0, kk, l, i, jl, ml;
  bool lindep;
  std::string ename;
  std::string sname;
  /*
   * Get the total number of elements defined in the multiphase object
   */
  int ne = mphase->nElements();
  /*
   * Get the total number of species in the multiphase object
   */
  int nspecies = mphase->nSpecies();
  doublereal tmp;
  doublereal const USEDBEFORE = -1;
 
  /*
   * Perhaps, initialize the element ordering
   */
  if ((int) orderVectorElements.size() < ne) {
    orderVectorElements.resize(ne);
    for (j = 0; j < ne; j++) {
      orderVectorElements[j] = j;
    }
  }

  /*
   * Perhaps, initialize the species ordering
   */
  if ((int) orderVectorSpecies.size() != nspecies) {
    orderVectorSpecies.resize(nspecies);
    for (k = 0; k < nspecies; k++) {
      orderVectorSpecies[k] = k;
    }
  }
 
#ifdef DEBUG_MODE
  double molSave = 0.0;
  if (BasisOptimize_print_lvl >= 1) {
    writelog("   "); for(i=0; i<77; i++) writelog("-"); writelog("\n");
    writelog("   --- Subroutine BASOPT called to ");
    writelog("calculate the number of components and ");
    writelog("evaluate the formation matrix\n");
    if (BasisOptimize_print_lvl > 0) {
      writelog("   ---\n");
      
      writelog("   ---      Formula Matrix used in BASOPT calculation\n");
      writelog("   ---      Species | Order | ");
      for (j = 0; j < ne; j++) {
    jj = orderVectorElements[j];
    writelog(" ");
    ename = mphase->elementName(jj);
    print_stringTrunc(ename.c_str(), 4, 1);
    writelogf("(%1d)", j); 
      }
      writelog("\n");
      for (k = 0; k < nspecies; k++) {
    kk = orderVectorSpecies[k];
    writelog("   --- ");
    sname = mphase->speciesName(kk);
    print_stringTrunc(sname.c_str(), 11, 1);
    writelogf(" |   %4d |", k); 
    for (j = 0; j < ne; j++) {
      jj = orderVectorElements[j]; 
      double num = mphase->nAtoms(kk,jj);
      writelogf("%6.1g  ", num); 
    }
    writelog("\n");
      }
      writelog("   --- \n");
    }
  }
#endif
   
  /*
   *  Calculate the maximum value of the number of components possible
   *     It's equal to the minimum of the number of elements and the
   *     number of total species.
   */
  int nComponents = MIN(ne, nspecies);
  int nNonComponents = nspecies - nComponents;
  /*
   * Set this return variable to false
   */
  *usedZeroedSpecies = false;

  /*
   * Create an array of mole numbers
   */
  vector_fp molNum(nspecies,0.0);
  mphase->getMoles(DATA_PTR(molNum));

  /*
   * Other workspace
   */
  vector_fp sm(ne*ne, 0.0);
  vector_fp ss(ne, 0.0);
  vector_fp sa(ne, 0.0);
  if ((int) formRxnMatrix.size() < nspecies*ne) {
    formRxnMatrix.resize(nspecies*ne, 0.0);
  }

#ifdef DEBUG_MODE
  /*
   * For debugging purposes keep an unmodified copy of the array.
   */
  vector_fp molNumBase(molNum);
#endif
  

  int jr = -1;
  /*
   *   Top of a loop of some sort based on the index JR. JR is the 
   *   current number of component species found. 
   */
  do {
    ++jr;
    /* - Top of another loop point based on finding a linearly */
    /* - independent species */
    do {
      /*
       *    Search the remaining part of the mole number vector, molNum
       *    for the largest remaining species. Return its identity. 
       *    kk is the raw number. k is the orderVectorSpecies index.
       */
      kk = amax(DATA_PTR(molNum), 0, nspecies);
      for (j = 0; j < nspecies; j++) {
    if (orderVectorSpecies[j] == kk) {
      k = j;
      break;
    }
      }
      if (j == nspecies) {
    throw CanteraError("BasisOptimize", "orderVectorSpecies contains an error");
      }

      if (molNum[kk] == 0.0) *usedZeroedSpecies = true;
      /*
       * If the largest molNum is negative, then we are done.
       */
      if (molNum[kk] == USEDBEFORE) {
    nComponents = jr;
    nNonComponents = nspecies - nComponents;
    goto L_END_LOOP;
      }
      /*
       *  Assign a small negative number to the component that we have
       *  just found, in order to take it out of further consideration.
       */
#ifdef DEBUG_MODE
      molSave = molNum[kk];
#endif
      molNum[kk] = USEDBEFORE;

      /* *********************************************************** */
      /* **** CHECK LINEAR INDEPENDENCE WITH PREVIOUS SPECIES ****** */
      /* *********************************************************** */
      /*    
       *          Modified Gram-Schmidt Method, p. 202 Dalquist 
       *          QR factorization of a matrix without row pivoting. 
       */
      jl = jr;
      for (j = 0; j < ne; ++j) {
    jj = orderVectorElements[j];
    sm[j + jr*ne] = mphase->nAtoms(kk,jj);
      }
      if (jl > 0) {
    /*
     *         Compute the coefficients of JA column of the 
     *         the upper triangular R matrix, SS(J) = R_J_JR 
     *         (this is slightly different than Dalquist) 
     *         R_JA_JA = 1 
     */
    for (j = 0; j < jl; ++j) {
      ss[j] = 0.0;
      for (i = 0; i < ne; ++i) {
        ss[j] += sm[i + jr*ne] * sm[i + j*ne];
      }
      ss[j] /= sa[j];
    }
    /* 
     *     Now make the new column, (*,JR), orthogonal to the 
     *     previous columns
     */
    for (j = 0; j < jl; ++j) {
      for (l = 0; l < ne; ++l) {
        sm[l + jr*ne] -= ss[j] * sm[l + j*ne];
      }
    }
      }
      /*
       *        Find the new length of the new column in Q. 
       *        It will be used in the denominator in future row calcs. 
       */
      sa[jr] = 0.0;
      for (ml = 0; ml < ne; ++ml) {
    tmp = sm[ml + jr*ne];
    sa[jr] += tmp * tmp;
      }
      /* **************************************************** */
      /* **** IF NORM OF NEW ROW  .LT. 1E-3 REJECT ********** */
      /* **************************************************** */
      if (sa[jr] < 1.0e-6)  lindep = true;
      else                  lindep = false;
    } while(lindep);
    /* ****************************************** */
    /* **** REARRANGE THE DATA ****************** */
    /* ****************************************** */
    if (jr != k) {
#ifdef DEBUG_MODE
      if (BasisOptimize_print_lvl >= 1) {
    kk = orderVectorSpecies[k];
    sname = mphase->speciesName(kk);
    writelogf("   ---   %-12.12s", sname.c_str()); 
    jj = orderVectorSpecies[jr];
    ename = mphase->speciesName(jj);
    writelogf("(%9.2g) replaces %-12.12s", molSave, ename.c_str());
    writelogf("(%9.2g) as component %3d\n", molNum[jj], jr);
      }
#endif
      switch_pos(orderVectorSpecies, jr, k);
    }
    /* - entry point from up above */
  L_END_LOOP: ;
    /*
     *      If we haven't found enough components, go back 
     *      and find some more. (nc -1 is used below, because
     *      jr is counted from 0, via the C convention.
     */
  } while (jr < (nComponents-1));
   

 if (! doFormRxn) return nComponents;

  /* ****************************************************** */
  /* **** EVALUATE THE STOICHIOMETRY ********************** */
  /* ****************************************************** */
  /*
   *  Formulate the matrix problem for the stoichiometric
   *  coefficients. CX + B = 0
   *      C will be an nc x nc matrix made up of the formula 
   * vectors for the components. Each component's formular
   * vector is a column. The rows are the elements.
   *      n rhs's will be solved for. Thus, B is an nc x n
   * matrix. 
   *
   * BIG PROBLEM 1/21/99:
   *
   *    This algorithm makes the assumption that the
   * first nc rows of the formula matrix aren't rank deficient.
   * However, this might not be the case. For example, assume
   * that the first element in FormulaMatrix[] is argon. Assume that
   * no species in the matrix problem actually includes argon.
   * Then, the first row in sm[], below will be indentically
   * zero. bleh. 
   *    What needs to be done is to perform a rearrangement
   * of the ELEMENTS -> i.e. rearrange, FormulaMatrix, sp, and gai, such
   * that the first nc elements form in combination with the
   * nc components create an invertible sm[]. not a small
   * project, but very doable.
   *    An alternative would be to turn the matrix problem
   * below into an ne x nc problem, and do QR elimination instead
   * of Gauss-Jordon elimination.
   *    Note the rearrangement of elements need only be done once
   * in the problem. It's actually very similar to the top of 
   * this program with ne being the species and nc being the
   * elements!!
   */
  for (k = 0; k < nComponents; ++k) {
    kk = orderVectorSpecies[k];
    for (j = 0; j < nComponents; ++j) {
      jj = orderVectorElements[j];
      sm[j + k*ne] = mphase->nAtoms(kk, jj);
    }
  }

  for (i = 0; i < nNonComponents; ++i) {
    k = nComponents + i;
    kk = orderVectorSpecies[k];
    for (j = 0; j < nComponents; ++j) {
      jj = orderVectorElements[j];
      formRxnMatrix[j + i * ne] = mphase->nAtoms(kk, jj);
    }
  }
  /*
   *     Use Gauss-Jordon block elimination to calculate
   *     the reaction matrix 
   */
  j = mlequ(DATA_PTR(sm), ne, nComponents, DATA_PTR(formRxnMatrix), nNonComponents);
  if (j == 1) {
    writelog("ERROR: mlequ returned an error condition\n");
    throw CanteraError("basopt", "mlequ returned an error condition");
  }
    
#ifdef DEBUG_MODE
  if (Cantera::BasisOptimize_print_lvl >= 1) {
    writelog("   ---\n");
    writelogf("   ---  Number of Components = %d\n", nComponents);
    writelog("   ---  Formula Matrix:\n");
    writelog("   ---                      Components:    ");
    for (k = 0; k < nComponents; k++) {
      kk = orderVectorSpecies[k];
      writelogf(" %3d (%3d) ", k, kk);
    }
    writelog("\n   ---                Components Moles:       ");
    for (k = 0; k < nComponents; k++) {
      kk = orderVectorSpecies[k];
      writelogf("%-11.3g", molNumBase[kk]); 
    }
    writelog("\n   ---        NonComponent |   Moles  |       ");
    for (i = 0; i < nComponents; i++) {
      kk = orderVectorSpecies[i];
      sname = mphase->speciesName(kk);
      writelogf("%-11.10s", sname.c_str());
    }
    writelog("\n");
  
    for (i = 0; i < nNonComponents; i++) {
      k = i + nComponents;
      kk = orderVectorSpecies[k];
      writelogf("   --- %3d (%3d) ", k, kk);
      sname = mphase->speciesName(kk);
      writelogf("%-10.10s", sname.c_str()); 
      writelogf("|%10.3g|", molNumBase[kk]); 
      /*
       * Print the negative of formRxnMatrix[]; it's easier to interpret.
       */
      for (j = 0; j < nComponents; j++) {
    writelogf("     %6.2f", - formRxnMatrix[j + i * ne]);
      }
      writelog("\n");
    }
    writelog("   "); for (i=0; i<77; i++) writelog("-"); writelog("\n");
  }
#endif

  return nComponents;
} /* basopt() ************************************************************/



#ifdef DEBUG_MODE
static void print_stringTrunc(const char *str, int space, int alignment)

   /***********************************************************************
    *  vcs_print_stringTrunc():
    *
    *     Print a string within a given space limit. This routine
    *     limits the amount of the string that will be printed to a
    *     maximum of "space" characters.
    *
    *     str = String -> must be null terminated.
    *     space = space limit for the printing.
    *     alignment = 0 centered
    *           1 right aligned
    *           2 left aligned
    ***********************************************************************/
{
  int i, ls=0, rs=0;
  int len = strlen(str);
  if ((len) >= space) {
    for (i = 0; i < space; i++) {
      writelogf("%c", str[i]);
    }
  } else {
    if (alignment == 1) {
      ls = space - len;
    } else if (alignment == 2) {
      rs = space - len;
    } else {
      ls = (space - len) / 2;
      rs = space - len - ls;
    }
    if (ls != 0) {
      for (i = 0; i < ls; i++) writelog(" ");
    }
    writelogf("%s", str);
    if (rs != 0) {
      for (i = 0; i < rs; i++) writelog(" ");
    }
  }
}
#endif

/*
 * Finds the location of the maximum component in a double vector
 * INPUT
 *    x(*) - Vector to search
 *    j <= i < n     : i is the range of indecises to search in X(*)
 *
 * RETURN
 *    return index of the greatest value on X(*) searched
 */
static int amax(double *x, int j, int n) {
  int i;
  int largest = j;
  double big = x[j];
  for (i = j + 1; i < n; ++i) {
    if (x[i] > big) {
      largest = i;
      big = x[i];
    }
  }
  return largest;
}


 static void switch_pos(vector_int &orderVector, int jr, int kspec) {
   int kcurr = orderVector[jr];
   orderVector[jr] = orderVector[kspec];
   orderVector[kspec] = kcurr;
 }

 /*
  * vcs_mlequ:
  *
  *  Invert an nxn matrix and solve m rhs's
  *
  *    Solve         C X + B = 0;
  *
  * This routine uses Gauss elimination and is optimized for the solution
  * of lots of rhs's.
  * A crude form of row pivoting is used here.
  *
  *
  * c[i+j*idem] = c_i_j = Matrix to be inverted: i = row number
  *                                              j = column number
  * b[i+j*idem] = b_i_j = vectors of rhs's:      i = row number
  *                                              j = column number
  *            (each column is a new rhs)
  * n = number of rows and columns in the matrix
  * m = number of rhs to be solved for
  * idem = first dimension in the calling routine
  *        idem >= n must be true
  *
  * Return Value
  *      1 : Matrix is singluar
  *      0 : solution is OK
  *
  *      The solution is returned in the matrix b.
  */
 static int mlequ(double *c, int idem, int n, double *b, int m) {
   int i, j, k, l;
   double R;

   /*
    * Loop over the rows
    *    -> At the end of each loop, the only nonzero entry in the column
    *       will be on the diagonal. We can therfore just invert the
    *       diagonal at the end of the program to solve the equation system.
    */
   for (i = 0; i < n; ++i) {
     if (c[i + i * idem] == 0.0) {
       /*
    *   Do a simple form of row pivoting to find a non-zero pivot
    */
       for (k = i + 1; k < n; ++k) {
     if (c[k + i * idem] != 0.0) goto FOUND_PIVOT;
       }
#ifdef DEBUG_MODE
       writelogf("vcs_mlequ ERROR: Encountered a zero column: %d\n", i); 
#endif
       return 1;
     FOUND_PIVOT: ;
       for (j = 0; j < n; ++j) c[i + j * idem] += c[k + j * idem];
       for (j = 0; j < m; ++j) b[i + j * idem] += b[k + j * idem];
     }

     for (l = 0; l < n; ++l) {
       if (l != i && c[l + i * idem] != 0.0) {
     R = c[l + i * idem] / c[i + i * idem];
     c[l + i * idem] = 0.0;
     for (j = i+1; j < n; ++j) c[l + j * idem] -= c[i + j * idem] * R;
     for (j = 0; j < m; ++j)   b[l + j * idem] -= b[i + j * idem] * R;
       }
     }
   }
   /*
    *  The negative in the last expression is due to the form of B upon
    *  input
    */
   for (i = 0; i < n; ++i) {
     for (j = 0; j < m; ++j)
       b[i + j * idem] = -b[i + j * idem] / c[i + i*idem];
   }
   return 0;
 } /* mlequ() *************************************************************/


/*
 *
 * ElemRearrange:
 *
 *    This subroutine handles the rearrangement of the constraint
 *    equations represented by the Formula Matrix. Rearrangement is only
 *    necessary when the number of components is less than the number of
 *    elements. For this case, some constraints can never be satisfied 
 *    exactly, because the range space represented by the Formula
 *    Matrix of the components can't span the extra space. These 
 *    constraints, which are out of the range space of the component
 *    Formula matrix entries, are migrated to the back of the Formula
 *    matrix.
 *
 *    A prototypical example is an extra element column in 
 *    FormulaMatrix[], 
 *    which is identically zero. For example, let's say that argon is
 *    has an element column in FormulaMatrix[], but no species in the 
 *    mechanism
 *    actually contains argon. Then, nc < ne. Unless the entry for
 *    desired elementabundance vector for Ar is zero, then this
 *    element abundance constraint can never be satisfied. The 
 *    constraint vector is not in the range space of the formula
 *    matrix.
 *    Also, without perturbation
 *    of FormulaMatrix[], BasisOptimize[] would produce a zero pivot 
 *    because the matrix
 *    would be singular (unless the argon element column was already the
 *    last column of  FormulaMatrix[]. 
 *       This routine borrows heavily from BasisOptimize algorithm. It 
 *    finds nc constraints which span the range space of the Component
 *    Formula matrix, and assigns them as the first nc components in the
 *    formular matrix. This guarrantees that BasisOptimize has a
 *    nonsingular matrix to invert.
 */
int Cantera::ElemRearrange(int nComponents, const vector_fp & elementAbundances,
               MultiPhase *mphase, 
               vector_int & orderVectorSpecies,
               vector_int & orderVectorElements) {
 
  int  j, k, l, i, jl, ml, jr, ielem, jj, kk=0;
 
  bool lindep = false;
  int nelements = mphase->nElements();
  std::string ename;
  /*
   * Get the total number of species in the multiphase object
   */
  int nspecies = mphase->nSpecies();

  double test = -1.0E10;
#ifdef DEBUG_MODE
  if (BasisOptimize_print_lvl > 0) {
    writelog("   "); for(i=0; i<77; i++) writelog("-"); writelog("\n");
    writelog("   --- Subroutine ElemRearrange() called to ");
    writelog("check stoich. coefficent matrix\n");
    writelog("   ---    and to rearrange the element ordering once\n");
  }
#endif

  /*
   * Perhaps, initialize the element ordering
   */
  if ((int) orderVectorElements.size() < nelements) {
    orderVectorElements.resize(nelements);
    for (j = 0; j < nelements; j++) {
      orderVectorElements[j] = j;
    }
  }

  /*
   * Perhaps, initialize the species ordering. However, this is 
   * dangerous, as this ordering is assumed to yield the
   * component species for the problem
   */
  if ((int) orderVectorSpecies.size() != nspecies) {
    orderVectorSpecies.resize(nspecies);
    for (k = 0; k < nspecies; k++) {
      orderVectorSpecies[k] = k;
    }
  }

  /*
   * If the elementAbundances aren't input, just create a fake one
   * based on summing the column of the stoich matrix.
   * This will force elements with zero species to the
   * end of the element ordering.
   */
  vector_fp eAbund(nelements,0.0);
  if ((int) elementAbundances.size() != nelements) {
    for (j = 0; j < nelements; j++) {
      eAbund[j] = 0.0;
      for (k = 0; k < nspecies; k++) {
    eAbund[j] += fabs(mphase->nAtoms(k, j));
      }
    }
  } else {
    copy(elementAbundances.begin(), elementAbundances.end(), 
     eAbund.begin());
  }

  vector_fp sa(nelements,0.0);
  vector_fp ss(nelements,0.0);
  vector_fp sm(nelements*nelements,0.0);
   
  /*
   *        Top of a loop of some sort based on the index JR. JR is the 
   *       current number independent elements found. 
   */
  jr = -1;
  do {
    ++jr;
    /* 
     *     Top of another loop point based on finding a linearly 
     *     independent element
     */
    do {
      /*
       *    Search the element vector. We first locate elements that
       *    are present in any amount. Then, we locate elements that
       *    are not present in any amount.
       *    Return its identity in K. 
       */
      k = nelements;
      for (ielem = jr; ielem < nelements; ielem++) {
    kk = orderVectorElements[ielem];
    if (eAbund[kk] != test && eAbund[kk] > 0.0) {
      k = ielem;
      break;
    }
      }
      for (ielem = jr; ielem < nelements; ielem++) {
    kk = orderVectorElements[ielem];
    if (eAbund[kk] != test) {
      k = ielem;
      break;
    }
      }

      if (k == nelements) {
    // When we are here, there is an error usually.
    // We haven't found the number of elements necessary.
    // This is signalled by returning jr != nComponents.
#ifdef DEBUG_MODE
      if (BasisOptimize_print_lvl > 0) {
    writelogf("Error exit: returning with nComponents = %d\n", jr);
      }
#endif
    return jr;
      }
     
      /*
       *  Assign a large negative number to the element that we have
       *  just found, in order to take it out of further consideration.
       */
      eAbund[kk] = test;
     
      /* *********************************************************** */
      /* **** CHECK LINEAR INDEPENDENCE OF CURRENT FORMULA MATRIX    */
      /* **** LINE WITH PREVIOUS LINES OF THE FORMULA MATRIX  ****** */
      /* *********************************************************** */
      /*    
       *          Modified Gram-Schmidt Method, p. 202 Dalquist 
       *          QR factorization of a matrix without row pivoting. 
       */
      jl = jr;
      /*
       *   Fill in the row for the current element, k, under consideration
       *   The row will contain the Formula matrix value for that element
       *   with respect to the vector of component species.
       *   (note j and k indecises are flipped compared to the previous routine)
       */
      for (j = 0; j < nComponents; ++j) {
    jj = orderVectorSpecies[j];
    kk = orderVectorElements[k];
    sm[j + jr*nComponents] = mphase->nAtoms(jj,kk);
      } 
      if (jl > 0) {
    /*
     *         Compute the coefficients of JA column of the 
     *         the upper triangular R matrix, SS(J) = R_J_JR 
     *         (this is slightly different than Dalquist) 
     *         R_JA_JA = 1 
     */
    for (j = 0; j < jl; ++j) {
      ss[j] = 0.0;
      for (i = 0; i < nComponents; ++i) {
        ss[j] += sm[i + jr*nComponents] * sm[i + j*nComponents];
      }
      ss[j] /= sa[j];
    }
    /* 
     *     Now make the new column, (*,JR), orthogonal to the 
     *     previous columns
     */
    for (j = 0; j < jl; ++j) {
      for (l = 0; l < nComponents; ++l) {
        sm[l + jr*nComponents] -= ss[j] * sm[l + j*nComponents];
      }
    }
      }
     
      /*
       *        Find the new length of the new column in Q. 
       *        It will be used in the denominator in future row calcs. 
       */
      sa[jr] = 0.0;
      for (ml = 0; ml < nComponents; ++ml) {
    double tmp = sm[ml + jr*nComponents];
    sa[jr] += tmp * tmp;
      }
      /* **************************************************** */
      /* **** IF NORM OF NEW ROW  .LT. 1E-6 REJECT ********** */
      /* **************************************************** */
      if (sa[jr] < 1.0e-6)  lindep = true;
      else                  lindep = false;
    } while(lindep);
    /* ****************************************** */
    /* **** REARRANGE THE DATA ****************** */
    /* ****************************************** */
    if (jr != k) {
#ifdef DEBUG_MODE
      if (BasisOptimize_print_lvl > 0) {
	kk = orderVectorElements[k];
	ename = mphase->elementName(kk);
	writelog("   ---   ");
        writelogf("%-2.2s", ename.c_str()); 
	writelog("replaces ");
	kk = orderVectorElements[jr];
	ename = mphase->elementName(kk);
	writelogf("%-2.2s", ename.c_str()); 
	writelogf(" as element %3d\n", jr); 
      }
#endif
      switch_pos(orderVectorElements, jr, k);
    }
  
    /*
     *      If we haven't found enough components, go back 
     *      and find some more. (nc -1 is used below, because
     *      jr is counted from 0, via the C convention.
     */
  } while (jr < (nComponents-1));
  return nComponents;
} /* vcs_elem_rearrange() ****************************************************/

