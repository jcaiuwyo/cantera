/* ======================================================================= */
/* -------------------------------------------------- */
/* | RCS Head Information on zuzax.pchem.sandia.gov | */
/* -------------------------------------------------- */
/* $RCSfile: vcs_TP.cpp,v $ */
/* $Author$ */
/* $Date$ */
/* $Revision$ */
/* ======================================================================= */

#include "vcs_solve.h"
#include "vcs_internal.h" 
#include "vcs_species_thermo.h"
#include "vcs_VolPhase.h"

#include <cstdio>
#include <cstdlib>
#include <cmath>

namespace VCSnonideal {

  /*****************************************************************************/
  /*****************************************************************************/
  /*****************************************************************************/

  int VCS_SOLVE::vcs_TP(int ipr, int ip1, int maxit, double T_arg, double pres_arg)
   
    /**************************************************************************
     *
     * vcs_TP:
     *
     *      Solve an equilibrium problem at a particular fixed temperature 
     *      and pressure
     *
     *     ipr = 1 -> Print results to standard output 
     *           0 -> don't report on anything 
     *     ip1 = 1 -> Print intermediate results. 
     *     maxit -> Maximum number of iterations for the algorithm 
     *      T = Temperature (Kelvin)
     *    pres = Pressure (pascal)
     *
     * Return Codes
     * ------------------
     *   0 = Equilibrium Achieved
     *   1 = Range space error encountered. The element abundance criteria are
     *       only partially satisfied. Specifically, the first NC= (number of
     *       components) conditions are satisfied. However, the full NE 
     *       (number of elements) conditions are not satisfied. The equilibrirum
     *       condition is returned.
     * -1 = Maximum number of iterations is exceeded. Convergence was not
     *      found.
     ***************************************************************************/
  {
    int retn, iconv;
    /*
     *        Store the temperature and pressure in the private global variables
     */
    m_temperature = T_arg;
    m_pressurePA = pres_arg;
    /*
     *        Evaluate the standard state free energies
     *        at the current temperatures and pressures.
     */
    iconv = vcs_evalSS_TP(ipr, ip1, m_temperature, pres_arg);
   
    /*
     *        Prepare the problem data:
     *          ->nondimensionalize the free energies using
     *            the divisor, R * T
     */
    vcs_nondim_TP();
    /*
     * Prep the fe field
     */
    vcs_fePrep_TP();
    /*
     *      Decide whether we need an initial estimate of the solution
     *      If so, go get one. If not, then
     */
    if (m_doEstimateEquil) {
      retn = vcs_inest_TP();
      if (retn != VCS_SUCCESS) {
	plogf("vcs_inest_TP returned a failure flag\n");	 
      }
    }
    /*
     *        Solve the problem at a fixed Temperature and Pressure
     * (all information concerning Temperature and Pressure has already
     *  been derived. The free energies are now in dimensionless form.)
     */
    iconv  = vcs_solve_TP(ipr, ip1, maxit);
   
    /*
     *        Redimensionalize the free energies using
     *        the reverse of vcs_nondim to add back units.
     */
    vcs_redim_TP();
    /*
     *        Return the convergence success flag.
     */
    return iconv;
  }
  /*****************************************************************************/
  /*****************************************************************************/
  /*****************************************************************************/
  /*ARGSUSED*/
  int VCS_SOLVE::vcs_evalSS_TP(int ipr, int ip1, double Temp, double pres)
   
    /**************************************************************************
     *
     * vcs_evalSS_TP:
     *
     *     IPR = 1 -> Print results to standard output 
     *           0 -> don't report on anything 
     *     IP1 = 1 -> Print intermediate results. 
     *      T = Temperature (Kelvin)
     *    Pres = Pressure (Pascal)
     *
     *    Evaluate the standard state free energies at the current temperature
     *    and pressure. Ideal gas pressure contribution is added in here.
     *
     ***************************************************************************/
  {
    // int i;
    //double  R; 
    /*
     * At this level of the program, we are still using values
     * for the free energies that have units. 
     */
    // R =  vcsUtil_gasConstant(m_VCS_UnitsFormat);
   
    /*
     *  We need to special case VCS_UNITS_UNITLESS, here. 
     *      cpc_ts_GStar_calc() returns units of Kelvin. Also, the temperature
     *      comes into play in calculating the ideal equation of state
     *      contributions, and other equations of state also. Therefore,
     *      we will emulate the VCS_UNITS_KELVIN case, here by chaning
     *      the initial gibbs free energy units to Kelvin before feeding
     *      them to the cpc_ts_GStar_calc() routine. Then, we will revert
     *      them back to unitless at the end of this routine.
     */

   
    /*
     *    Loop over the species calculating the standard state Gibbs free
     *    energies. -> These are energies that only depend upon the Temperature
     *    and possibly on the pressure (i.e., ideal gas, etc).
     */
    // HKM -> We can change this to looks over phases, calling the vcs_VolPhase 
    //        object. Working to get rid of VCS_SPECIES_THERMO object
    //for (i = 0; i < m_numSpeciesTot; ++i) {
    // VCS_SPECIES_THERMO *spt = SpeciesThermo[i];
    // ff[i] = R * spt->GStar_R_calc(i, Temp, pres);
    //}

    for (int iph = 0; iph < m_numPhases; iph++) {
      vcs_VolPhase* vph = m_VolPhaseList[iph];
      vph->setState_TP(m_temperature, m_pressurePA);
      vph->sendToVCS_GStar(VCS_DATA_PTR(m_SSfeSpecies));
    }
   
    if (m_VCS_UnitsFormat == VCS_UNITS_UNITLESS) {
      for (int i = 0; i < m_numSpeciesTot; ++i) {
	m_SSfeSpecies[i]  /= Temp;
      }
    }
    return VCS_SUCCESS;
  } /***************************************************************************/

  /*****************************************************************************/
  /*****************************************************************************/
  /*****************************************************************************/

  void  VCS_SOLVE::vcs_fePrep_TP(void)
   
    /**************************************************************************
     *
     *
     ***************************************************************************/
  {
    int i;
    for (i = 0; i < m_numSpeciesTot; ++i) {
      /*
       *        For single species phases, initialize the chemical
       *        potential with the value of the standard state chemical
       *        potential. This value doesn't change during the calculation
       */
      if (m_SSPhase[i]) {
	m_feSpecies_old[i] = m_SSfeSpecies[i];
	m_feSpecies_new[i] = m_SSfeSpecies[i];
      }
    }    
  } /* vcs_fePrep_TP() ********************************************************/

}

