/**
 *  @file ShomatePoly.h
 *  Header for a single-species standard state object derived
 *  from \link Cantera::SpeciesThermoInterpType SpeciesThermoInterpType\endlink  based 
 *  on the Shomate temperature polynomial form applied to one temperature region
 *  (see \ref spthermo and class \link Cantera::ShomatePoly ShomatePoly\endlink and
 *   \link Cantera::ShomatePoly2 ShomatePoly2\endlink).
 *    Shomate polynomial expressions.
 */
/*
 * $Author$
 * $Revision$
 * $Date$
 */

// Copyright 2001  California Institute of Technology


#ifndef CT_SHOMATEPOLY1_H
#define CT_SHOMATEPOLY1_H

#include "SpeciesThermoInterpType.h"

namespace Cantera {

  
  //! The Shomate polynomial parameterization for one temperature range 
  //! for one species
  /*! 
   *
   * Seven coefficients \f$(A,\dots,G)\f$ are used to represent
   * \f$ c_p^0(T)\f$, \f$ h^0(T)\f$, and \f$ s^0(T) \f$ as 
   * polynomials in the temperature, \f$ T \f$ : 
   * 
   * \f[
   * \tilde{c}_p^0(T) = A + B t + C t^2 + D t^3 + \frac{E}{t^2}
   * \f]
   * \f[
   * \tilde{h}^0(T) = A t + \frac{B t^2}{2} + \frac{C t^3}{3} 
   + \frac{D t^4}{4}  - \frac{E}{t}  + F.
   * \f]
   * \f[
   * \tilde{s}^0(T) = A\ln t + B t + \frac{C t^2}{2}  
   + \frac{D t^3}{3} - \frac{E}{2t^2}  + G.
   * \f]
   *
   * In the above expressions, the thermodynamic polynomials are expressed
   * in dimensional units, but the temperature,\f$ t \f$, is divided by 1000. The
   * following dimensions are assumed in the above expressions:
   *
   *    - \f$ \tilde{c}_p^0(T)\f$ = Heat Capacity (J/gmol*K)
   *    - \f$ \tilde{h}^0(T) \f$ = standard Enthalpy (kJ/gmol)
   *    - \f$ \tilde{s}^0(T) \f$= standard Entropy (J/gmol*K)
   *    - \f$ t \f$= temperature (K) / 1000.
   *
   *  For more information about Shomate polynomials, see the NIST website,
   *  http://webbook.nist.gov/
   *
   * Before being used within Cantera, the dimensions must be adjusted to those
   * used by Cantera (i.e., Joules and kmol).
   *

   * @ingroup spthermo
   */
  class ShomatePoly : public SpeciesThermoInterpType {

  public:

    //! Empty constructor
    ShomatePoly() 
      : m_lowT(0.0), m_highT (0.0),
	m_Pref(0.0), m_index (0) {}

    //! Constructor used in templated instantiations
    /*!
     * @param n            Species index
     * @param tlow         Minimum temperature
     * @param thigh        Maximum temperature
     * @param pref         reference pressure (Pa).
     * @param coeffs       Vector of coefficients used to set the
     *                     parameters for the standard state for species n.
     *                     There are 7 coefficients for the Shomate polynomial:
     *         -   c[0] = \f$ A \f$
     *         -   c[1] = \f$ B \f$
     *         -   c[2] = \f$ C \f$
     *         -   c[3] = \f$ D \f$
     *         -   c[4] = \f$ E \f$
     *         -   c[5] = \f$ F \f$
     *         -   c[6] = \f$ G \f$
     *
     *  See the class description for the polynomial representation of the 
     *  thermo functions in terms of \f$ A, \dots, G \f$.
     */
    ShomatePoly(int n, doublereal tlow, doublereal thigh, doublereal pref,
		const doublereal* coeffs) :
      m_lowT      (tlow),
      m_highT     (thigh),
      m_Pref      (pref),
      m_index     (n) {
      m_coeff.resize(7);
      std::copy(coeffs, coeffs + 7, m_coeff.begin());
    }

    //! copy constructor
    /*!
     * @param b object to be copied
     */
    ShomatePoly(const ShomatePoly& b) :
      m_lowT      (b.m_lowT),
      m_highT     (b.m_highT),
      m_Pref      (b.m_Pref),
      m_coeff     (array_fp(7)),
      m_index     (b.m_index) {
      std::copy(b.m_coeff.begin(),
		b.m_coeff.begin() + 7,
		m_coeff.begin());
    }

    //! Assignment operator
    /*!
     * @param  b   
     */
    ShomatePoly& operator=(const ShomatePoly& b) {
      if (&b != this) {
	m_lowT   = b.m_lowT;
	m_highT  = b.m_highT;
	m_Pref   = b.m_Pref;
	m_index  = b.m_index;
	m_coeff.resize(7);
	std::copy(b.m_coeff.begin(),
		  b.m_coeff.begin() + 7,
		  m_coeff.begin());
      }
      return *this;
    }
    
    //! Destructor
    virtual ~ShomatePoly(){}

    //! Duplicator from the base class
    virtual SpeciesThermoInterpType *
    duplMyselfAsSpeciesThermoInterpType() const {
      ShomatePoly* sp = new ShomatePoly(*this);
      return (SpeciesThermoInterpType *) sp;
    }

    //! Returns the minimum temperature that the thermo
    //! parameterization is valid
    virtual doublereal minTemp() const { return m_lowT;}

    //! Returns the maximum temperature that the thermo
    //! parameterization is valid
    virtual doublereal maxTemp() const { return m_highT;}

    //! Returns the reference pressure (Pa)
    virtual doublereal refPressure() const { return m_Pref; }

    //! Returns an integer representing the type of parameterization
    virtual int reportType() const { return SHOMATE; }

    //! Returns an integer representing the species index
    virtual int speciesIndex() const { return m_index; }
  
         
    //! Update the properties for this species, given a temperature polynomial
    /*!
     * This method is called with a pointer to an array containing the functions of
     * temperature needed by this  parameterization, and three pointers to arrays where the
     * computed property values should be written. This method updates only one value in
     * each array.
     *
     *  tt is T/1000. 
     *  m_t[0] = tt;
     *  m_t[1] = tt*tt;
     *  m_t[2] = m_t[1]*tt;
     *  m_t[3] = 1.0/m_t[1];
     *  m_t[4] = log(tt);
     *  m_t[5] = 1.0/GasConstant;
     *  m_t[6] = 1.0/(GasConstant * T);
     *
     * @param tt      Vector of temperature polynomials
     * @param cp_R    Vector of Dimensionless heat capacities.
     *                (length m_kk).
     * @param h_RT    Vector of Dimensionless enthalpies.
     *                (length m_kk).
     * @param s_R     Vector of Dimensionless entropies.
     *                (length m_kk).
     */
    virtual void updateProperties(const doublereal* tt, 
			  doublereal* cp_R, doublereal* h_RT, 
			  doublereal* s_R) const {

      doublereal A      = m_coeff[0]; 
      doublereal Bt     = m_coeff[1]*tt[0];    
      doublereal Ct2    = m_coeff[2]*tt[1];    
      doublereal Dt3    = m_coeff[3]*tt[2]; 
      doublereal Etm2   = m_coeff[4]*tt[3]; 
      doublereal F      = m_coeff[5];
      doublereal G      = m_coeff[6];

      doublereal cp, h, s;
      cp = A + Bt + Ct2 + Dt3 + Etm2;
      h = tt[0]*(A + 0.5*Bt + OneThird*Ct2 + 0.25*Dt3 - Etm2) + F;
      s = A*tt[4] + Bt + 0.5*Ct2 + OneThird*Dt3 - 0.5*Etm2 + G; 

      /*
       *  Shomate polynomials parameterizes assuming units of
       *  J/(gmol*K) for cp_r and s_R and kJ/(gmol) for h.
       *  However, Cantera assumes default MKS units of 
       *  J/(kmol*K). This requires us to multiply cp and s
       *  by 1.e3 and h by 1.e6, before we then nondimensionlize
       *  the results by dividing by (GasConstant * T),
       *  where GasConstant has units of J/(kmol * K).
       */
      cp_R[m_index] = 1.e3 * cp * tt[5];
      h_RT[m_index] = 1.e6 * h  * tt[6];
      s_R[m_index]  = 1.e3 * s  * tt[5];
    }

    //! Compute the reference-state property of one species
    /*!
     * Given temperature T in K, this method updates the values of
     * the non-dimensional heat capacity at constant pressure,
     * enthalpy, and entropy, at the reference pressure, Pref
     * of one of the species. The species index is used
     * to reference into the cp_R, h_RT, and s_R arrays.
     *
     * @param temp    Temperature (Kelvin)
     * @param cp_R    Vector of Dimensionless heat capacities.
     *                (length m_kk).
     * @param h_RT    Vector of Dimensionless enthalpies.
     *                (length m_kk).
     * @param s_R     Vector of Dimensionless entropies.
     *                (length m_kk).
     */
    virtual void updatePropertiesTemp(const doublereal temp, 
			      doublereal* cp_R, doublereal* h_RT, 
			      doublereal* s_R) const {
      double tPoly[7];
      doublereal tt = 1.e-3*temp;
      tPoly[0] = tt;
      tPoly[1] = tt * tt;
      tPoly[2] = tPoly[1] * tt;
      tPoly[3] = 1.0/tPoly[1];
      tPoly[4] = std::log(tt);
      tPoly[5] = 1.0/GasConstant;
      tPoly[6] = 1.0/(GasConstant * temp);
      updateProperties(tPoly, cp_R, h_RT, s_R);
    }

    //!This utility function reports back the type of 
    //! parameterization and all of the parameters for the 
    //! species, index.
    /*!
     * All parameters are output variables
     *
     * @param n         Species index
     * @param type      Integer type of the standard type
     * @param tlow      output - Minimum temperature
     * @param thigh     output - Maximum temperature
     * @param pref      output - reference pressure (Pa).
     * @param coeffs    Vector of coefficients used to set the
     *                  parameters for the standard state.
     */
    virtual void reportParameters(int &n, int &type,
			  doublereal &tlow, doublereal &thigh,
			  doublereal &pref,
			  doublereal* const coeffs) const {
      n = m_index;
      type = SHOMATE;
      tlow = m_lowT;
      thigh = m_highT;
      pref = m_Pref;
      for (int i = 0; i < 7; i++) {
	coeffs[i] = m_coeff[i];
      }
    }

    //! Modify parameters for the standard state
    /*!
     * @param coeffs   Vector of coefficients used to set the
     *                 parameters for the standard state.
     */
    virtual void modifyParameters(doublereal* coeffs) {
      if (m_coeff.size() != 7) {
	throw CanteraError("modifyParameters", 
			   "modifying something that hasn't been initialized");
      }
      std::copy(coeffs, coeffs + 7, m_coeff.begin());
    }

#ifdef H298MODIFY_CAPABILITY

    //! Report the 298 K Heat of Formation of the standard state of one species (J kmol-1)
    /*!
     *   The 298K Heat of Formation is defined as the enthalpy change to create the standard state
     *   of the species from its constituent elements in their standard states at 298 K and 1 bar.
     *
     *   @param h298 If this is nonnull,  the current value of the Heat of Formation at 298K and 1 bar for
     *               species m_index is returned in h298[m_index].
     *   @return     Returns the current value of the Heat of Formation at 298K and 1 bar for 
     *               species m_index.
     */
    virtual doublereal reportHf298(doublereal* const h298 = 0) const {

      double tPoly[4];
      doublereal tt = 1.e-3*298.15;
      tPoly[0] = tt;
      tPoly[1] = tt * tt;
      tPoly[2] = tPoly[1] * tt;
      tPoly[3] = 1.0/tPoly[1];
    
      doublereal A      = m_coeff[0]; 
      doublereal Bt     = m_coeff[1]*tPoly[0];    
      doublereal Ct2    = m_coeff[2]*tPoly[1];    
      doublereal Dt3    = m_coeff[3]*tPoly[2]; 
      doublereal Etm2   = m_coeff[4]*tPoly[3]; 
      doublereal F      = m_coeff[5];
 
      doublereal h = tPoly[0]*(A + 0.5*Bt + OneThird*Ct2 + 0.25*Dt3 - Etm2) + F;
  
      double hh =  1.e6 * h;
      if (h298) {
	h298[m_index] = 1.e6 * h;
      }
      return hh;
    }

    //! Modify the value of the 298 K Heat of Formation of one species in the phase (J kmol-1)
    /*!
     *   The 298K heat of formation is defined as the enthalpy change to create the standard state
     *   of the species from its constituent elements in their standard states at 298 K and 1 bar.
     *
     *   @param  k           Species k
     *   @param  Hf298New    Specify the new value of the Heat of Formation at 298K and 1 bar                      
     */
    virtual void modifyOneHf298(const int k, const doublereal Hf298New) {
      doublereal hnow = reportHf298();
      doublereal delH = Hf298New - hnow;
      m_coeff[5] += delH / 1.0E6;
    }

#endif

  protected:
    //! Minimum temperature for which the parameterization is valid (Kelvin)    
    doublereal m_lowT;
    //! Maximum temperature for which the parameterization is valid (Kelvin)
    doublereal m_highT;
    //! Reference pressure (Pa)
    doublereal m_Pref;
    //! Array of coeffcients
    array_fp m_coeff;
    //! Species Index
    int m_index;
    
  private:

  };

  //! The Shomate polynomial parameterization for two temperature ranges 
  //! for one species
  /*! 
   *
   * Seven coefficients \f$(A,\dots,G)\f$ are used to represent
   * \f$ c_p^0(T)\f$, \f$ h^0(T)\f$, and \f$ s^0(T) \f$ as 
   * polynomials in the temperature, \f$ T \f$, in one temperature region: 
   * 
   * \f[
   * \tilde{c}_p^0(T) = A + B t + C t^2 + D t^3 + \frac{E}{t^2}
   * \f]
   * \f[
   * \tilde{h}^0(T) = A t + \frac{B t^2}{2} + \frac{C t^3}{3} 
   + \frac{D t^4}{4}  - \frac{E}{t}  + F.
   * \f]
   * \f[
   * \tilde{s}^0(T) = A\ln t + B t + \frac{C t^2}{2}  
   + \frac{D t^3}{3} - \frac{E}{2t^2}  + G.
   * \f]
   *
   * In the above expressions, the thermodynamic polynomials are expressed
   * in dimensional units, but the temperature,\f$ t \f$, is divided by 1000. The
   * following dimensions are assumed in the above expressions:
   *
   *    - \f$ \tilde{c}_p^0(T)\f$ = Heat Capacity (J/gmol*K)
   *    - \f$ \tilde{h}^0(T) \f$ = standard Enthalpy (kJ/gmol)
   *    - \f$ \tilde{s}^0(T) \f$= standard Entropy (J/gmol*K)
   *    - \f$ t \f$= temperature (K) / 1000.
   *
   *  For more information about Shomate polynomials, see the NIST website,
   *  http://webbook.nist.gov/
   *
   * Before being used within Cantera, the dimensions must be adjusted to those
   * used by Cantera (i.e., Joules and kmol).
   *
   * This function uses two temperature regions, each with a Shomate polynomial
   * representation to represent the thermo functions. There are 15 coefficients,
   * therefore, in this representation. The first coefficient is the midrange
   * temperature.
   *
   *
   * @ingroup spthermo
   */
  class ShomatePoly2 : public SpeciesThermoInterpType {
  public:

    //! Empty constructor
    ShomatePoly2() 
      : m_lowT(0.0),
	m_midT(0.0), 
	m_highT (0.0),
	m_Pref(0.0),
	msp_low(0),
	msp_high(0),
	m_index(0) {
      m_coeff.resize(15);
    }

    //! Constructor used in templated instantiations
    /*!
     * @param n            Species index
     * @param tlow         Minimum temperature
     * @param thigh        Maximum temperature
     * @param pref         reference pressure (Pa).
     * @param coeffs       Vector of coefficients used to set the
     *                     parameters for the standard state.
     *                     There are 15 coefficients for the 2-zone Shomate polynomial.
     *                     The first coefficient is the value of Tmid. The next 7 
     *                     coefficients are the low temperature range Shomate coefficients.
     *                     The last 7 are the high temperature range Shomate coefficients.
     */
    ShomatePoly2(int n, doublereal tlow, doublereal thigh, doublereal pref,
		 const doublereal* coeffs) :
      m_lowT      (tlow),
      m_midT(0.0),
      m_highT     (thigh),
      m_Pref      (pref),
      msp_low(0),
      msp_high(0),
      m_index     (n)  {
      m_coeff.resize(15);
      std::copy(coeffs, coeffs + 15, m_coeff.begin());
      m_midT = coeffs[0];
      msp_low  = new ShomatePoly(n, tlow, m_midT, pref, coeffs+1);
      msp_high = new ShomatePoly(n, m_midT, thigh, pref, coeffs+8);
    }

    //! Copy constructor
    /*!
     * @param b object to be copied.
     */
    ShomatePoly2(const ShomatePoly2& b) :
      m_lowT      (b.m_lowT),
      m_midT      (b.m_midT),
      m_highT     (b.m_highT),
      m_Pref      (b.m_Pref),
      msp_low(0),
      msp_high(0),
      m_coeff     (array_fp(15)),
      m_index     (b.m_index) {
      std::copy(b.m_coeff.begin(),
		b.m_coeff.begin() + 15,
		m_coeff.begin());
      msp_low  = new ShomatePoly(m_index, m_lowT, m_midT, 
				 m_Pref, &m_coeff[1]);
      msp_high = new ShomatePoly(m_index, m_midT, m_highT, 
				 m_Pref, &m_coeff[8]);
    }

    //! Assignment operator
    /*!
     * @param b object to be copied.
     */
    ShomatePoly2& operator=(const ShomatePoly2& b) {
      if (&b != this) {
	m_lowT   = b.m_lowT;
	m_midT   = b.m_midT;
	m_highT  = b.m_highT;
	m_Pref   = b.m_Pref;
	m_index  = b.m_index;
	std::copy(b.m_coeff.begin(),
		  b.m_coeff.begin() + 15,
		  m_coeff.begin());
	if (msp_low) delete msp_low;
	if (msp_high) delete msp_high;
	msp_low  = new ShomatePoly(m_index, m_lowT, m_midT, 
				   m_Pref, &m_coeff[1]);
	msp_high = new ShomatePoly(m_index, m_midT, m_highT, 
				   m_Pref, &m_coeff[8]);
      }
      return *this;
    }

    //! Destructor
    virtual ~ShomatePoly2(){
      delete msp_low;
      delete msp_high;
    }


    //! duplicator
    virtual SpeciesThermoInterpType *
    duplMyselfAsSpeciesThermoInterpType() const {
      ShomatePoly2* sp = new ShomatePoly2(*this);
      return (SpeciesThermoInterpType *) sp;
    }

    //! Returns the minimum temperature that the thermo
    //! parameterization is valid
    virtual doublereal minTemp() const { return m_lowT;}

    //! Returns the maximum temperature that the thermo
    //! parameterization is valid
    virtual doublereal maxTemp() const { return m_highT;}

    //! Returns the reference pressure (Pa)
    virtual doublereal refPressure() const { return m_Pref; }

    //! Returns an integer representing the type of parameterization
    virtual int reportType() const { return SHOMATE2; }

     //! Returns an integer representing the species index
    virtual int speciesIndex() const { return m_index; }
  
    //! Update the properties for this species, given a temperature polynomial
    /*!
     * This method is called with a pointer to an array containing the functions of
     * temperature needed by this  parameterization, and three pointers to arrays where the
     * computed property values should be written. This method updates only one value in
     * each array.
     *
     * Temperature Polynomial:
     *  tt[0] = t;
     *  tt[1] = t*t;
     *  tt[2] = m_t[1]*t;
     *  tt[3] = m_t[2]*t;
     *  tt[4] = 1.0/t;
     *  tt[5] = std::log(t);
     *
     * @param tt      vector of temperature polynomials
     * @param cp_R    Vector of Dimensionless heat capacities.
     *                (length m_kk).
     * @param h_RT    Vector of Dimensionless enthalpies.
     *                (length m_kk).
     * @param s_R     Vector of Dimensionless entropies.
     *                (length m_kk).
     */
    virtual void updateProperties(const doublereal* tt, 
			  doublereal* cp_R, doublereal* h_RT, 
			  doublereal* s_R) const {
      double T = 1000 * tt[0];
      if (T <= m_midT) {
	msp_low->updateProperties(tt, cp_R, h_RT, s_R);
      } else {
	msp_high->updateProperties(tt, cp_R, h_RT, s_R);
      }
	    
    }

    //! Compute the reference-state property of one species
    /*!
     * Given temperature T in K, this method updates the values of
     * the non-dimensional heat capacity at constant pressure,
     * enthalpy, and entropy, at the reference pressure, Pref
     * of one of the species. The species index is used
     * to reference into the cp_R, h_RT, and s_R arrays.
     *
     * @param temp    Temperature (Kelvin)
     * @param cp_R    Vector of Dimensionless heat capacities.
     *                (length m_kk).
     * @param h_RT    Vector of Dimensionless enthalpies.
     *                (length m_kk).
     * @param s_R     Vector of Dimensionless entropies.
     *                (length m_kk).
     */
    virtual void updatePropertiesTemp(const doublereal temp, 
			      doublereal* cp_R,
			      doublereal* h_RT, 
			      doublereal* s_R) const {
      if (temp <= m_midT) {
	msp_low->updatePropertiesTemp(temp, cp_R, h_RT, s_R);
      } else {
	msp_high->updatePropertiesTemp(temp, cp_R, h_RT, s_R);
      }
    }

    //!This utility function reports back the type of 
    //! parameterization and all of the parameters for the 
    //! species, index.
    /*!
     * All parameters are output variables
     *
     * @param n         Species index
     * @param type      Integer type of the standard type
     * @param tlow      output - Minimum temperature
     * @param thigh     output - Maximum temperature
     * @param pref      output - reference pressure (Pa).
     * @param coeffs    Vector of coefficients used to set the
     *                  parameters for the standard state.
     */
    virtual void reportParameters(int &n, int &type,
			  doublereal &tlow, doublereal &thigh,
			  doublereal &pref,
			  doublereal* const coeffs) const {
      n = m_index;
      type = SHOMATE2;
      tlow = m_lowT;
      thigh = m_highT;
      pref = m_Pref;
      for (int i = 0; i < 15; i++) {
	coeffs[i] = m_coeff[i];
      }
    }

    //! Modify parameters for the standard state
    /*!
     * Here, we take the tact that we will just regenerate the
     * object.
     *
     * @param coeffs   Vector of coefficients used to set the
     *                 parameters for the standard state.
     */
    virtual void modifyParameters(doublereal* coeffs) {
      delete msp_low;
      delete msp_high;
      std::copy(coeffs, coeffs + 15, m_coeff.begin());
      m_midT = coeffs[0];
      msp_low  = new ShomatePoly(m_index, m_lowT, m_midT,  m_Pref, coeffs+1);
      msp_high = new ShomatePoly(m_index, m_midT, m_highT, m_Pref, coeffs+8);
    }

#ifdef H298MODIFY_CAPABILITY

    virtual doublereal reportHf298(doublereal* const h298 = 0) const {
      doublereal h;
      if (298.15 <= m_midT) {
	h = msp_low->reportHf298(h298);
      } else {
	h = msp_high->reportHf298(h298);
      }
      if (h298) {
	h298[m_index] = h;
      }
      return h;
    }

    virtual void modifyOneHf298(const int k, const doublereal Hf298New) {
      if (k != m_index) return;
 
      doublereal h298now = reportHf298(0);
      doublereal delH = Hf298New - h298now;
      double h = msp_low->reportHf298(0);
      double hnew = h + delH;
      msp_low->modifyOneHf298(k, hnew);
      h  = msp_high->reportHf298(0);
      hnew = h + delH;
      msp_high->modifyOneHf298(k, hnew);
    }

#endif

  protected:
    //! Minimum temperature the representation is valid(kelvin)
    doublereal m_lowT;
    //! Midrange temperature (kelvin)
    doublereal m_midT;
    //! Maximum temperature the representation is valid (kelvin)
    doublereal m_highT;
    //! Reference pressure (Pascal)
    doublereal m_Pref;
    //! Pointer to the Shomate polynomial for the low temperature region.
    ShomatePoly *msp_low;
    //! Pointer to the Shomate polynomial for the high temperature region.
    ShomatePoly *msp_high;
    //! Array of the original coefficients.
    array_fp m_coeff;
    //! Species index
    int m_index;
  };
}

#endif
