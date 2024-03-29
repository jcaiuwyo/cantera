/**
 *  @file Mu0Poly.cpp
 *  Definitions for a single-species standard state object derived
 *  from \link Cantera::SpeciesThermoInterpType SpeciesThermoInterpType\endlink  based 
 *  on a piecewise constant mu0 interpolation
 *  (see \ref spthermo and class \link Cantera::Mu0Poly Mu0Poly\endlink).
 */
/*
 * $Author$
 * $Revision$
 * $Date$
 */


#include "Mu0Poly.h"
#include "ctexceptions.h"
#include "speciesThermoTypes.h"
#include "SpeciesThermo.h"
#include "xml.h"
#include "ctml.h"

using namespace std;
using namespace ctml;

namespace Cantera {

 
  Mu0Poly::Mu0Poly() : m_numIntervals(0),
		       m_H298(0.0),
		       m_lowT(0.0),
		       m_highT(0.0),
		       m_Pref(0.0),
		       m_index(0) {
  }

  /*
   * Mu0Poly():
   *
   * In the constructor, we calculate and store the
   * piecewise linear approximation to the thermodynamic
   * functions.
   *
   *  coeffs[0] = number of points (integer)
   *         1  = H298(J/kmol)
   *         2  = T1  (Kelvin)
   *         3  = mu1 (J/kmol) 
   *         4  = T2  (Kelvin)
   *         5  = mu2 (J/kmol) 
   *         6  = T3  (Kelvin)
   *         7  = mu3 (J/kmol)
   *         ........
   */
  Mu0Poly::Mu0Poly(int n, doublereal tlow, doublereal thigh, 
		   doublereal pref,
		   const doublereal* coeffs) :
    m_numIntervals(0),
    m_H298(0.0),
    m_lowT      (tlow),
    m_highT     (thigh),
    m_Pref      (pref),
    m_index     (n) {

    processCoeffs(coeffs);
  }


  Mu0Poly::Mu0Poly(const Mu0Poly &b)
    : m_numIntervals (b.m_numIntervals),
      m_H298         (b.m_H298),
      m_t0_int       (b.m_t0_int),
      m_mu0_R_int    (b.m_mu0_R_int),
      m_h0_R_int     (b.m_h0_R_int),
      m_s0_R_int     (b.m_s0_R_int),
      m_cp0_R_int    (b.m_cp0_R_int),
      m_lowT         (b.m_lowT),
      m_highT        (b.m_highT),
      m_Pref         (b.m_Pref),
      m_index        (b.m_index) {
  }

  Mu0Poly& Mu0Poly::operator=(const Mu0Poly& b) {
    if (&b != this) {
      m_numIntervals = b.m_numIntervals;
      m_H298         = b.m_H298;
      m_t0_int       = b.m_t0_int;
      m_mu0_R_int    = b.m_mu0_R_int;
      m_h0_R_int     = b.m_h0_R_int;
      m_s0_R_int     = b.m_s0_R_int;
      m_cp0_R_int    = b.m_cp0_R_int;
      m_lowT         = b.m_lowT;
      m_highT        = b.m_highT;
      m_Pref         = b.m_Pref;
      m_index        = b.m_index;
    }
    return *this;
  }
    
  /*
   * Destructor:
   */
  Mu0Poly::~Mu0Poly(){
  }
    
  SpeciesThermoInterpType *
  Mu0Poly::duplMyselfAsSpeciesThermoInterpType() const {
    Mu0Poly* mp = new Mu0Poly(*this);
    return (SpeciesThermoInterpType *) mp;
  }
	
  doublereal Mu0Poly::minTemp() const { return m_lowT;}
  doublereal  Mu0Poly::maxTemp() const { return m_highT;}
  doublereal Mu0Poly::refPressure() const { return m_Pref; }
    
  /*
   *  updateProperties is the main workhorse program. 
   *  Given a temperature (*tt), it calculates the thermodynamic
   *  functions H/RT, S_R, and cp_R, and returns the answer.
   *
   *  Note, it returns an answer by inserting the values into the
   *  index position, m_index in vectors of  H/RT, S_R, and cp_R.
   *
   *
   *   Input
   *  -------
   *       *tt = Temperature (Kelvin)
   *      
   */
  void  Mu0Poly::
  updateProperties(const doublereal* tt,  doublereal* cp_R,
		   doublereal* h_RT, doublereal* s_R) const {
    int j = m_numIntervals;
    double T = *tt;
    for (int i = 0; i < m_numIntervals; i++) {
      double T2 =  m_t0_int[i+1];
      if (T <=T2) {
	j = i;
	break;
      }
    }
    double T1 = m_t0_int[j];
    double cp_Rj = m_cp0_R_int[j];

    doublereal rt = 1.0/T;
    cp_R[m_index] = cp_Rj;
    h_RT[m_index] = rt*(m_h0_R_int[j] + (T - T1) * cp_Rj);
    s_R[m_index]  = m_s0_R_int[j] + cp_Rj * (log(T/T1));
  }

  void  Mu0Poly::
  updatePropertiesTemp(const doublereal T,
		       doublereal* cp_R,
		       doublereal* h_RT,
		       doublereal* s_R) const {
    updateProperties(&T, cp_R, h_RT, s_R);
  }

  /*
   * report all of the parameters that make up this 
   * interpolation.
   *
   * 
   */
  void Mu0Poly::reportParameters(int &n, int &type,
				 doublereal &tlow, doublereal &thigh,
				 doublereal &pref,
				 doublereal* const coeffs) const {
    n = m_index;
    type = MU0_INTERP;
    tlow = m_lowT;
    thigh = m_highT;
    pref = m_Pref;
    coeffs[0] = m_numIntervals+1;
    coeffs[1] = m_H298 * GasConstant;
    int j = 2;
    for (int i = 0; i < m_numIntervals+1; i++) {
      coeffs[j] = m_t0_int[i];
      coeffs[j+1] = m_mu0_R_int[i] * GasConstant;
      j += 2;
    }
  }

  void Mu0Poly::modifyParameters(doublereal* coeffs) {
    processCoeffs(coeffs);
  }

  /*
   * Install a Mu0 polynomial thermodynamic reference state property
   * parameterization for species k into a SpeciesThermo instance,
   * getting the information from an XML database.
   */
  void installMu0ThermoFromXML(std::string speciesName,
			       SpeciesThermo& sp, int k, 
			       const XML_Node* Mu0Node_ptr) {

    doublereal tmin, tmax;
    bool dimensionlessMu0Values = false;
    const XML_Node& Mu0Node = *Mu0Node_ptr;
  
    tmin = fpValue(Mu0Node["Tmin"]);
    tmax = fpValue(Mu0Node["Tmax"]);
    doublereal pref = fpValue(Mu0Node["Pref"]);

    doublereal h298 = 0.0;
    if (Mu0Node.hasChild("H298")) {
      h298 = getFloat(Mu0Node, "H298", "actEnergy");
    }

    int numPoints = 1;
    if (Mu0Node.hasChild("numPoints")) {
      numPoints = getInteger(Mu0Node, "numPoints");
    }

    vector_fp cValues(numPoints);
    const XML_Node *valNode_ptr = 
      getByTitle(const_cast<XML_Node&>(Mu0Node), "Mu0Values");
    if (!valNode_ptr) {
      throw CanteraError("installMu0ThermoFromXML",
			 "missing required while processing "
			 + speciesName);
    }
    getFloatArray(*valNode_ptr, cValues, true, "actEnergy");
    /*
     * Check to see whether the Mu0's were input in a dimensionless
     * form. If they were, then the assumed temperature needs to be
     * adjusted from the assumed T = 273.15
     */
    string uuu = (*valNode_ptr)["units"];
    if (uuu == "Dimensionless") {
      dimensionlessMu0Values = true;
    }
    int ns = cValues.size();
    if (ns != numPoints) {
      throw CanteraError("installMu0ThermoFromXML",
			 "numPoints inconsistent while processing "
			 + speciesName);
    }
	
    vector_fp cTemperatures(numPoints);
    const XML_Node *tempNode_ptr = 
      getByTitle(const_cast<XML_Node&>(Mu0Node), "Mu0Temperatures");
    if (!tempNode_ptr) {
      throw CanteraError("installMu0ThermoFromXML",
			 "missing required while processing + "
			 + speciesName);
    }
    getFloatArray(*tempNode_ptr, cTemperatures, false);
    ns = cTemperatures.size();
    if (ns != numPoints) {
      throw CanteraError("installMu0ThermoFromXML",
			 "numPoints inconsistent while processing "
			 + speciesName);
    }
	
    /*
     * Fix up dimensionless Mu0 values if input
     */
    if (dimensionlessMu0Values) {
      for (int i = 0; i < numPoints; i++) {
	cValues[i] *= cTemperatures[i] / 273.15;
      }
    }


    vector_fp c(2 + 2 * numPoints);
     
    c[0] = numPoints;
    c[1] = h298;
    for (int i = 0; i < numPoints; i++) {
      c[2+i*2]   = cTemperatures[i];
      c[2+i*2+1] = cValues[i];
    }
      
    sp.install(speciesName, k, MU0_INTERP, &c[0], tmin, tmax, pref);
  }

  /*
   * Mu0Poly():
   *
   * In the constructor, we calculate and store the
   * piecewise linear approximation to the thermodynamic
   * functions.
   *
   *  coeffs[0] = number of points (integer)
   *         1  = H298(J/kmol)
   *         2  = T1  (Kelvin)
   *         3  = mu1 (J/kmol) 
   *         4  = T2  (Kelvin)
   *         5  = mu2 (J/kmol) 
   *         6  = T3  (Kelvin)
   *         7  = mu3 (J/kmol)
   *         ........
   */
  void Mu0Poly::processCoeffs(const doublereal* coeffs)  {

    int i, iindex;
    double T1, T2;
    int nPoints = (int) coeffs[0];
    if (nPoints < 2) {
      throw CanteraError("Mu0Poly", 
			 "nPoints must be >= 2");
    }
    m_numIntervals = nPoints - 1;
    m_H298       = coeffs[1] / GasConstant;
    int iT298 = 0;
    /*
     * Resize according to the number of points
     */
    m_t0_int.resize(nPoints);
    m_h0_R_int.resize(nPoints);
    m_s0_R_int.resize(nPoints);
    m_cp0_R_int.resize(nPoints);
    m_mu0_R_int.resize(nPoints);
    /*
     * Calculate the T298 interval and make sure that
     * the temperatures are strictly monotonic.
     * Also distribute the data into the internal arrays.
     */
    bool ifound = false;
    for (i = 0, iindex = 2; i < nPoints; i++) {
      T1 = coeffs[iindex];
      m_t0_int[i] = T1;
      m_mu0_R_int[i] =  coeffs[iindex+1] / GasConstant;
      if (T1 == 298.15) {
	iT298 = i;
	ifound = true;
      }
      if (i < nPoints - 1) {
	T2 = coeffs[iindex+2];
	if (T2 <= T1) {
	  throw CanteraError("Mu0Poly", 
			     "Temperatures are not monotonic increasing");
	}
      }
      iindex += 2;
    }
    if (!ifound) {
      throw CanteraError("Mu0Poly",
			 "One temperature has to be 298.15");
    }

    /*
     * Starting from the interval with T298, we go up
     */
    doublereal mu2, s1, s2, h1, h2, cpi, deltaMu, deltaT;
    T1 = m_t0_int[iT298];
    doublereal mu1 = m_mu0_R_int[iT298];
    m_h0_R_int[iT298] = m_H298;
    m_s0_R_int[iT298] = - (mu1 - m_h0_R_int[iT298]) / T1;
    for (i = iT298; i < m_numIntervals; i++) {
      T1      = m_t0_int[i];
      s1      = m_s0_R_int[i];
      h1      = m_h0_R_int[i];
      mu1     = m_mu0_R_int[i];
      T2      = m_t0_int[i+1];
      mu2     = m_mu0_R_int[i+1];
      deltaMu = mu2 - mu1;
      deltaT  = T2 - T1;
      cpi = (deltaMu - T1 * s1 + T2 * s1) / (deltaT - T2 * log(T2/T1));
      h2 = h1 + cpi * deltaT;
      s2 = s1 + cpi * log(T2/T1);
      m_cp0_R_int[i]   = cpi;
      m_h0_R_int[i+1]  = h2;
      m_s0_R_int[i+1]  = s2;
      m_cp0_R_int[i+1] = cpi;
    }

    /*
     * Starting from the interval with T298, we go down
     */
    if (iT298 > 0) {
      T2 = m_t0_int[iT298];
      mu2 = m_mu0_R_int[iT298];
      m_h0_R_int[iT298] = m_H298;
      m_s0_R_int[iT298] = - (mu2 - m_h0_R_int[iT298]) / T2;
      for (i = iT298 - 1; i >= 0; i--) {
	T1      = m_t0_int[i];
	mu1     = m_mu0_R_int[i];
	T2      = m_t0_int[i+1];
	mu2     = m_mu0_R_int[i+1];
	s2      = m_s0_R_int[i+1];
	h2      = m_h0_R_int[i+1];
	deltaMu = mu2 - mu1;
	deltaT  = T2 - T1;
	cpi = (deltaMu - T1 * s2 + T2 * s2) / (deltaT - T1 * log(T2/T1)); 
	h1 = h2 - cpi * deltaT;
	s1 = s2 - cpi * log(T2/T1);
	m_cp0_R_int[i]   = cpi;
	m_h0_R_int[i]    = h1;
	m_s0_R_int[i]    = s1;
	if (i == (m_numIntervals-1)) {
	  m_cp0_R_int[i+1] = cpi;
	}
      }
    }
#ifdef DEBUG_HKM_NOT
    printf("    Temp     mu0(J/kmol)   cp0(J/kmol/K)   "
	   " h0(J/kmol)   s0(J/kmol/K) \n");
    for (i = 0; i < nPoints; i++) {
      printf("%12.3g %12.5g %12.5g %12.5g %12.5g\n",
	     m_t0_int[i],  m_mu0_R_int[i] * GasConstant,
	     m_cp0_R_int[i]* GasConstant, 
	     m_h0_R_int[i]* GasConstant,
	     m_s0_R_int[i]* GasConstant);
      fflush(stdout);
    }
#endif
  }

}





