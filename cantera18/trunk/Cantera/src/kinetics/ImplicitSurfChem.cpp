/**
 *  @file ImplicitSurfChem.cpp
 * Definitions for the implicit integration of surface site density equations
 *  (see \ref  kineticsmgr and class
 *  \link Cantera::ImplicitSurfChem ImplicitSurfChem\endlink).
 */

/*
 * $Author$
 * $Revision$
 * $Date$
 */

// Copyright 2001  California Institute of Technology


#ifdef WIN32
#pragma warning(disable:4786)
#pragma warning(disable:4503)
#endif

#include "ImplicitSurfChem.h"
#include "Integrator.h"
#include "solveSP.h"

using namespace std;

namespace Cantera {
  
  // Constructor
  ImplicitSurfChem::ImplicitSurfChem(vector<InterfaceKinetics*> k) :
    FuncEval(),
    m_nsurf(0),
    m_nv(0),
    m_numBulkPhases(0),
    m_numTotalBulkSpecies(0),
    m_numTotalSpecies(0),
    m_integ(0), 
    m_atol(1.e-14),
    m_rtol(1.e-7), 
    m_maxstep(0.0),
    m_mediumSpeciesStart(-1),
    m_bulkSpeciesStart(-1),
    m_surfSpeciesStart(-1),
    m_surfSolver(0),
    m_commonTempPressForPhases(true),
    m_ioFlag(0)
  {
    m_nsurf = static_cast<int>(k.size());
    int ns, nsp;
    int nt, ntmax = 0;
    int kinSpIndex = 0;
    // Loop over the number of surface kinetics objects
    for (int n = 0; n < m_nsurf; n++) {
      InterfaceKinetics *kinPtr = k[n];
      m_vecKinPtrs.push_back(kinPtr);
      ns = k[n]->surfacePhaseIndex();
      if (ns < 0) 
	throw CanteraError("ImplicitSurfChem",
			   "kinetics manager contains no surface phase");
      m_surfindex.push_back(ns);
      m_surf.push_back((SurfPhase*)&k[n]->thermo(ns));
      nsp = m_surf.back()->nSpecies();
      m_nsp.push_back(nsp);
      m_nv += m_nsp.back();
      nt = k[n]->nTotalSpecies();
      if (nt > ntmax) ntmax = nt;
      m_specStartIndex.push_back(kinSpIndex);
      kinSpIndex += nsp;

      int nPhases = kinPtr->nPhases();
      vector_int pLocTmp(nPhases);
      int imatch = -1;
      for (int ip = 0; ip < nPhases; ip++) {
	if (ip != ns) {
	  ThermoPhase *thPtr = & kinPtr->thermo(ip);
	  if ((imatch = checkMatch(m_bulkPhases, thPtr)) < 0) {
	    m_bulkPhases.push_back(thPtr);
	    m_numBulkPhases++;
	    nsp = thPtr->nSpecies();
	    m_nspBulkPhases.push_back(nsp);
	    m_numTotalBulkSpecies += nsp;
	    imatch = m_bulkPhases.size() - 1;
	  } 
	  pLocTmp[ip] = imatch;
	} else {
	  pLocTmp[ip] = -n;
	}
      }
      pLocVec.push_back(pLocTmp);
     
    }
    m_numTotalSpecies = m_nv + m_numTotalBulkSpecies;
    m_concSpecies.resize(m_numTotalSpecies, 0.0);
    m_concSpeciesSave.resize(m_numTotalSpecies, 0.0);
 
    m_integ = newIntegrator("CVODE");



    // use backward differencing, with a full Jacobian computed
    // numerically, and use a Newton linear iterator

    m_integ->setMethod(BDF_Method);
    m_integ->setProblemType(DENSE + NOJAC);
    m_integ->setIterator(Newton_Iter);
    m_work.resize(ntmax);
  }

  int ImplicitSurfChem::checkMatch(std::vector<ThermoPhase *> m_vec, ThermoPhase *thPtr) {
    int retn = -1;
    for (int i = 0; i < (int) m_vec.size(); i++) {
      ThermoPhase *th = m_vec[i];
      if (th == thPtr) {
	return i;
      }
    }
    return retn;
  }

  /*
   * Destructor. Deletes the integrator.
   */
  ImplicitSurfChem::~ImplicitSurfChem(){
    if (m_integ) {
      delete m_integ;
    } 
    if (m_surfSolver) {
      delete m_surfSolver;
    }  
  }

  // overloaded method of FuncEval. Called by the integrator to
  // get the initial conditions.
  void ImplicitSurfChem::getInitialConditions(doublereal t0, size_t lenc, 
					      doublereal * c) 
  {
    int loc = 0;
    for (int n = 0; n < m_nsurf; n++) {
      m_surf[n]->getCoverages(c + loc);
      loc += m_nsp[n];
    }
  }


  /*
   *  Must be called before calling method 'advance'
   */
  void ImplicitSurfChem::initialize(doublereal t0) {
    m_integ->setTolerances(m_rtol, m_atol);
    m_integ->initialize(t0, *this);
  }

  // Integrate from t0 to t1. The integrator is reinitialized first.
  /*
   *   This routine does a time accurate solve from t = t0 to t = t1.
   *   of the surface problem.
   *
   *  @param t0  Initial Time -> this is an input
   *  @param t1  Final Time -> This is an input
   */
  void ImplicitSurfChem::integrate(doublereal t0, doublereal t1) {
    m_integ->initialize(t0, *this);
    m_integ->setMaxStepSize(t1 - t0);
    m_integ->integrate(t1);
    updateState(m_integ->solution());
  }
  
  // Integrate from t0 to t1 without reinitializing the integrator. 
  /*
   *  Use when the coverages have not changed from
   *  their values on return from the last call to integrate or
   *  integrate0.
   *
   *  @param t0  Initial Time -> this is an input
   *  @param t1  Final Time -> This is an input
   */
  void ImplicitSurfChem::integrate0(doublereal t0, doublereal t1) {
    m_integ->integrate(t1);
    updateState(m_integ->solution());
  }

  void ImplicitSurfChem::updateState(doublereal* c) {
    int loc = 0;
    for (int n = 0; n < m_nsurf; n++) {
      m_surf[n]->setCoverages(c + loc);
      loc += m_nsp[n];
    }
  }

  /*
   * Called by the integrator to evaluate ydot given y at time 'time'.
   */
  void ImplicitSurfChem::eval(doublereal time, doublereal* y, 
			      doublereal* ydot, doublereal* p) 
  {
    int n;
    updateState(y);   // synchronize the surface state(s) with y
    doublereal rs0, sum;
    int loc, k, kstart;
    for (n = 0; n < m_nsurf; n++) {
      rs0 = 1.0/m_surf[n]->siteDensity();
      m_vecKinPtrs[n]->getNetProductionRates(DATA_PTR(m_work));
      kstart = m_vecKinPtrs[n]->kineticsSpeciesIndex(0,m_surfindex[n]);
      sum = 0.0;
      loc = 0;
      for (k = 1; k < m_nsp[n]; k++) {
	ydot[k + loc] = m_work[kstart + k] * rs0 * m_surf[n]->size(k);
	sum -= ydot[k];
      }
      ydot[loc] = sum;
      loc += m_nsp[n];
    }
  }

  // Solve for the pseudo steady-state of the surface problem
  /*
   * Solve for the steady state of the surface problem. 
   * This is the same thing as the advanceCoverages() function,
   * but at infinite times.
   *
   * Note, a direct solve is carried out under the hood here,
   * to reduce the computational time.
   */
  void ImplicitSurfChem::solvePseudoSteadyStateProblem(int ifuncOverride,
						       doublereal timeScaleOverride) {
 
    int ifunc;
    /*
     * set bulkFunc
     *    -> We assume that the bulk concentrations are constant.
     */
    int bulkFunc = BULK_ETCH;
    /*
     * time scale - time over which to integrate equations
     */
    doublereal time_scale = timeScaleOverride;
    /*
     *
     */
    if (!m_surfSolver) {
      m_surfSolver = new solveSP(this, bulkFunc);
      /*
       * set ifunc, which sets the algorithm.
       */
      ifunc = SFLUX_INITIALIZE;
    } else {
      ifunc = SFLUX_RESIDUAL;
    }
    
    // Possibly override the ifunc value
    if (ifuncOverride >= 0) {
      ifunc = ifuncOverride;
    }

    /*
     * Get the specifications for the problem from the values
     * in the ThermoPhase objects for all phases.
     *
     *  1) concentrations of all species in all phases, m_concSpecies[]
     *  2) Temperature and pressure
     */
    getConcSpecies(DATA_PTR(m_concSpecies));
    InterfaceKinetics *ik = m_vecKinPtrs[0];
    ThermoPhase &tp = ik->thermo(0);
    doublereal TKelvin = tp.temperature();
    doublereal PGas  = tp.pressure();
    /*
     * Make sure that there is a common temperature and 
     * pressure for all ThermoPhase objects belonging to the
     * interfacial kinetics object, if it is required by
     * the problem statement.
     */
    if (m_commonTempPressForPhases) {
      setCommonState_TP(TKelvin, PGas);
    }

    doublereal reltol = 1.0E-6;
    doublereal atol = 1.0E-20;

    /*
     * Install a filter for negative concentrations. One of the 
     * few ways solvess can fail is if concentrations on input
     * are below zero.
     */
    bool rset = false;
    for (int k = 0; k < m_nv; k++) {
      if (m_concSpecies[k] < 0.0) {
	rset = true;
	m_concSpecies[k] = 0.0;
      }
    }
    if (rset) {
      setConcSpecies(DATA_PTR(m_concSpecies));
    }

    m_surfSolver->m_ioflag = m_ioFlag; 

    // Save the current solution
    copy(m_concSpecies.begin(), m_concSpecies.end(), m_concSpeciesSave.begin());

 
    int retn = m_surfSolver->solveSurfProb(ifunc, time_scale, TKelvin, PGas,
					   reltol, atol);
    if (retn != 1) {
      // reset the concentrations
      copy(m_concSpeciesSave.begin(), m_concSpeciesSave.end(), m_concSpecies.begin());
      setConcSpecies(DATA_PTR(m_concSpeciesSave));
      ifunc = SFLUX_INITIALIZE;
      retn = m_surfSolver->solveSurfProb(ifunc, time_scale, TKelvin, PGas,
					 reltol, atol);
	  
      if (retn != 1) { 
	throw CanteraError("ImplicitSurfChem::solvePseudoSteadyStateProblem",
			   "solveSP return an error condition!");
      }
    }
  }



  /*
   * getConcSpecies():
   *
   * Fills the local concentration vector, m_concSpecies for all of the
   * species in all of the phases that are unknowns in the surface
   * problem.
   *
   * m_concSpecies[]
   */
  void ImplicitSurfChem::getConcSpecies(doublereal * const vecConcSpecies) const {
    int kstart;
    for (int ip = 0; ip < m_nsurf; ip++) {
      ThermoPhase * TP_ptr = m_surf[ip];
      kstart = m_specStartIndex[ip];
      TP_ptr->getConcentrations(vecConcSpecies + kstart);
    }
    kstart = m_nv;
    for (int ip = 0; ip <  m_numBulkPhases; ip++) {
      ThermoPhase * TP_ptr = m_bulkPhases[ip];
      int nsp = TP_ptr->nSpecies();
      TP_ptr->getConcentrations(vecConcSpecies + kstart);
      kstart += nsp;
    }
  }

  /*
   * setConcSpecies():
   *
   * Fills the local concentration vector, m_concSpecies for all of the
   * species in all of the phases that are unknowns in the surface
   * problem.
   *
   * m_concSpecies[]
   */
  void ImplicitSurfChem::setConcSpecies(const doublereal * const vecConcSpecies) {
    int kstart;
    for (int ip = 0; ip < m_nsurf; ip++) {
      ThermoPhase * TP_ptr = m_surf[ip];
      kstart = m_specStartIndex[ip];
      TP_ptr->setConcentrations(vecConcSpecies + kstart);
    }
    kstart = m_nv;
    for (int ip = 0; ip <  m_numBulkPhases; ip++) {
      ThermoPhase * TP_ptr = m_bulkPhases[ip];
      int nsp = TP_ptr->nSpecies();
      TP_ptr->setConcentrations(vecConcSpecies + kstart);
      kstart += nsp;
    }
  }

  /*
   * setCommonState_TP():
   *
   *  Sets a common temperature and pressure amongst the 
   *  thermodynamic objects in the interfacial kinetics object.
   *
   *  Units Temperature = Kelvin
   *        Pressure    = Pascal
   */
  void ImplicitSurfChem::
  setCommonState_TP(doublereal TKelvin, doublereal PresPa) {
    int nphases = m_nsurf;
    for (int ip = 0; ip < nphases; ip++) {
      ThermoPhase *TP_ptr = m_surf[ip];
      TP_ptr->setState_TP(TKelvin, PresPa);
    }
    for (int ip = 0; ip < m_numBulkPhases; ip++) {
      ThermoPhase *TP_ptr = m_bulkPhases[ip];
      TP_ptr->setState_TP(TKelvin, PresPa);
    }
  }

}
