/**
 * @file PDSS_ConstVol.cpp
 * Implementation of a pressure dependent standard state 
 * virtual function.
 */
/*
 * Copywrite (2006) Sandia Corporation. Under the terms of
 * Contract DE-AC04-94AL85000 with Sandia Corporation, the
 * U.S. Government retains certain rights in this software.
 */
/*
 * $Id$
 */

#include "ct_defs.h"
#include "xml.h"
#include "ctml.h"
#include "PDSS_ConstVol.h"
#include "ThermoFactory.h"

#include "VPStandardStateTP.h"

using namespace std;

namespace Cantera {
  /**
   * Basic list of constructors and duplicators
   */

  PDSS_ConstVol::PDSS_ConstVol(VPStandardStateTP *tp, int spindex) :
    PDSS(tp, spindex)
  {
    m_pdssType = cPDSS_CONSTVOL;
  }


  PDSS_ConstVol::PDSS_ConstVol(VPStandardStateTP *tp, int spindex, std::string inputFile, std::string id) :
    PDSS(tp, spindex)
  {
    m_pdssType = cPDSS_CONSTVOL;
    constructPDSSFile(tp, spindex, inputFile, id);
  }

  PDSS_ConstVol::PDSS_ConstVol(VPStandardStateTP *tp, int spindex,
			       const XML_Node& speciesNode, 
			       const XML_Node& phaseRoot, 
			       bool spInstalled) :
    PDSS(tp, spindex)
  {
    m_pdssType = cPDSS_CONSTVOL;
    constructPDSSXML(tp, spindex, speciesNode,  phaseRoot, spInstalled) ;
  }


  PDSS_ConstVol::PDSS_ConstVol(const PDSS_ConstVol &b) :
    PDSS(b)
  {
    /*
     * Use the assignment operator to do the brunt
     * of the work for the copy construtor.
     */
    *this = b;
  }

  /*
   * Assignment operator
   */
  PDSS_ConstVol& PDSS_ConstVol::operator=(const PDSS_ConstVol&b) {
    if (&b == this) return *this;
    PDSS::operator=(b);
    m_constMolarVolume      = b.m_constMolarVolume;
    return *this;
  }

  PDSS_ConstVol::~PDSS_ConstVol() { 
  }
  
  // Duplicator
  PDSS* PDSS_ConstVol::duplMyselfAsPDSS() const {
    PDSS_ConstVol * idg = new PDSS_ConstVol(*this);
    return (PDSS *) idg;
  }

  /*
   * constructPDSSXML:
   *
   * Initialization of a PDSS_ConstVol object using an
   * xml file.
   *
   * This routine is a precursor to initThermo(XML_Node*)
   * routine, which does most of the work.
   *
   * @param infile XML file containing the description of the
   *        phase
   *
   * @param id  Optional parameter identifying the name of the
   *            phase. If none is given, the first XML
   *            phase element will be used.
   */
  void PDSS_ConstVol::constructPDSSXML(VPStandardStateTP *tp, int spindex,
				       const XML_Node& speciesNode, 
				       const XML_Node& phaseNode, bool spInstalled) {
    PDSS::initThermo();
    SpeciesThermo &sp = m_tp->speciesThermo();
    m_p0 = sp.refPressure(m_spindex);

    if (!spInstalled) {
      throw CanteraError("PDSS_ConstVol::constructPDSSXML", "spInstalled false not handled");
    }

    const XML_Node *ss = speciesNode.findByName("standardState");
    if (!ss) {
      throw CanteraError("PDSS_ConstVol::constructPDSSXML",
			 "no standardState Node for species " + speciesNode.name());
    }
    std::string model = (*ss)["model"];
    if (model != "constant_incompressible") {
      throw CanteraError("PDSS_ConstVol::initThermoXML",
			 "standardState model for species isn't constant_incompressible: " + speciesNode.name());
    }
  
    m_constMolarVolume = ctml::getFloat(*ss, "molarVolume", "toSI");
  
    std::string id = "";
    // initThermoXML(phaseNode, id);
  }

   
  /*
   * constructPDSSFile():
   *
   * Initialization of a PDSS_ConstVol object using an
   * xml file.
   *
   * This routine is a precursor to initThermo(XML_Node*)
   * routine, which does most of the work.
   *
   * @param infile XML file containing the description of the
   *        phase
   *
   * @param id  Optional parameter identifying the name of the
   *            phase. If none is given, the first XML
   *            phase element will be used.
   */
  void PDSS_ConstVol::constructPDSSFile(VPStandardStateTP *tp, int spindex,
					std::string inputFile, std::string id) {

    if (inputFile.size() == 0) {
      throw CanteraError("PDSS_ConstVol::initThermo",
			 "input file is null");
    }
    std::string path = findInputFile(inputFile);
    ifstream fin(path.c_str());
    if (!fin) {
      throw CanteraError("PDSS_ConstVol::initThermo","could not open "
			 +path+" for reading.");
    }
    /*
     * The phase object automatically constructs an XML object.
     * Use this object to store information.
     */

    XML_Node *fxml = new XML_Node();
    fxml->build(fin);
    XML_Node *fxml_phase = findXMLPhase(fxml, id);
    if (!fxml_phase) {
      throw CanteraError("PDSS_ConstVol::initThermo",
			 "ERROR: Can not find phase named " +
			 id + " in file named " + inputFile);
    }

    XML_Node& speciesList = fxml_phase->child("speciesArray");
    XML_Node* speciesDB = get_XML_NameID("speciesData", speciesList["datasrc"],
					 &(fxml_phase->root()));
    const vector<string>&sss = tp->speciesNames();
    const XML_Node* s =  speciesDB->findByAttr("name", sss[spindex]);

    constructPDSSXML(tp, spindex, *s, *fxml_phase, true);
    delete fxml;
  }

  void PDSS_ConstVol::initThermoXML(const XML_Node& phaseNode, std::string& id) {
    PDSS::initThermoXML(phaseNode, id);
    m_minTemp = m_spthermo->minTemp(m_spindex);
    m_maxTemp = m_spthermo->maxTemp(m_spindex);
    m_p0 = m_spthermo->refPressure(m_spindex);
    m_mw = m_tp->molecularWeight(m_spindex);
  }

  void PDSS_ConstVol::initThermo() {
    PDSS::initThermo();
    SpeciesThermo &sp = m_tp->speciesThermo();
    m_p0 = sp.refPressure(m_spindex);
    m_V0_ptr[m_spindex] = m_constMolarVolume;
    m_Vss_ptr[m_spindex] = m_constMolarVolume;
  }

  doublereal 
  PDSS_ConstVol::enthalpy_mole() const {
    doublereal val = enthalpy_RT();
    doublereal RT = GasConstant * m_temp;
    return (val * RT);
  }

  doublereal 
  PDSS_ConstVol::enthalpy_RT() const {
    doublereal val = m_hss_RT_ptr[m_spindex];
    return (val);
  }


  doublereal 
  PDSS_ConstVol::intEnergy_mole() const {
    doublereal pVRT = (m_pres * m_Vss_ptr[m_spindex]) / (GasConstant * m_temp);
    doublereal val = m_h0_RT_ptr[m_spindex] - pVRT;
    doublereal RT = GasConstant * m_temp;
    return (val * RT);
  }


  doublereal
  PDSS_ConstVol::entropy_mole() const {
    doublereal val = entropy_R();
    return (val * GasConstant);
  }

  doublereal
  PDSS_ConstVol::entropy_R() const {
    doublereal val = m_sss_R_ptr[m_spindex];
    return (val);
  }

  /*
   * Calculate the Gibbs free energy in mks units of
   * J kmol-1 K-1.
   */
  doublereal
  PDSS_ConstVol::gibbs_mole() const {
    doublereal val = gibbs_RT();
    doublereal RT = GasConstant * m_temp;
    return (val * RT);
  }

  doublereal
  PDSS_ConstVol::gibbs_RT() const {
    doublereal val = m_gss_RT_ptr[m_spindex];
    return (val);
  }

  doublereal 
  PDSS_ConstVol::cp_mole() const {
    doublereal val = m_cpss_R_ptr[m_spindex];
    return (val * GasConstant);
  }

  doublereal 
  PDSS_ConstVol::cp_R() const {
    doublereal val = m_cpss_R_ptr[m_spindex];
    return (val);
  }

  doublereal 
  PDSS_ConstVol::cv_mole() const {
    doublereal val = (cp_mole() -  m_V0_ptr[m_spindex]);
    return (val);
  }

  doublereal 
  PDSS_ConstVol::molarVolume() const {
    doublereal val = m_Vss_ptr[m_spindex];
    return (val);
  }

  doublereal 
  PDSS_ConstVol::density() const {
    doublereal val = m_Vss_ptr[m_spindex];
    return (m_mw/val);
  }

  doublereal
  PDSS_ConstVol::gibbs_RT_ref() const {
    doublereal val = m_g0_RT_ptr[m_spindex];
    return (val);
  }

  doublereal PDSS_ConstVol::enthalpy_RT_ref() const {
    doublereal val = m_h0_RT_ptr[m_spindex];
    return (val);
  }

  doublereal PDSS_ConstVol::entropy_R_ref() const {
    doublereal val = m_s0_R_ptr[m_spindex];
    return (val);
  }

  doublereal PDSS_ConstVol::cp_R_ref() const {
    doublereal val = m_cp0_R_ptr[m_spindex];
    return (val);
  }

  doublereal PDSS_ConstVol::molarVolume_ref() const {
    doublereal val = m_V0_ptr[m_spindex];
    return (val);
  }

 

  // critical temperature 
  doublereal PDSS_ConstVol::critTemperature() const { 
    throw CanteraError("PDSS_ConstVol::critTemperature()", "unimplemented");
    return (0.0);
  }
        
  // critical pressure
  doublereal PDSS_ConstVol::critPressure() const {
    throw CanteraError("PDSS_ConstVol::critPressure()", "unimplemented");
    return (0.0);
  }
        
  // critical density
  doublereal PDSS_ConstVol::critDensity() const {
    throw CanteraError("PDSS_ConstVol::critDensity()", "unimplemented");
    return (0.0);
  }

  void PDSS_ConstVol::setPressure(doublereal p) {
    m_pres = p;
    doublereal del_pRT = (m_pres - m_p0) / (GasConstant * m_temp);
    m_hss_RT_ptr[m_spindex]  = m_h0_RT_ptr[m_spindex] + del_pRT * m_Vss_ptr[m_spindex];
    m_gss_RT_ptr[m_spindex] = m_hss_RT_ptr[m_spindex] - m_sss_R_ptr[m_spindex];
  }
 
  void PDSS_ConstVol::setTemperature(doublereal temp) {
    m_temp = temp;
    m_spthermo->update_one(m_spindex, temp,
                           m_cp0_R_ptr, m_h0_RT_ptr, m_s0_R_ptr);
    m_g0_RT_ptr[m_spindex] =  m_h0_RT_ptr[m_spindex] -  m_s0_R_ptr[m_spindex];

    doublereal del_pRT = (m_pres - m_p0) / (GasConstant * m_temp);
 
    m_hss_RT_ptr[m_spindex]  = m_h0_RT_ptr[m_spindex] + del_pRT * m_Vss_ptr[m_spindex];
    m_cpss_R_ptr[m_spindex]  = m_cp0_R_ptr[m_spindex];
    m_sss_R_ptr[m_spindex] = m_s0_R_ptr[m_spindex];
    m_gss_RT_ptr[m_spindex] = m_hss_RT_ptr[m_spindex] - m_sss_R_ptr[m_spindex];
    
  }


  void PDSS_ConstVol::setState_TP(doublereal temp, doublereal pres) {
    setTemperature(temp);
    setPressure(pres);
  }


  void PDSS_ConstVol::setState_TR(doublereal temp, doublereal rho) {
    doublereal rhoStored = m_mw / m_constMolarVolume;
    if (fabs(rhoStored - rho) / (rhoStored + rho) > 1.0E-4) {
      throw CanteraError("PDSS_ConstVol::setState_TR",
			 "Inconsistent supplied rho");
    }
    setTemperature(temp);
  }

  // saturation pressure
  doublereal PDSS_ConstVol::satPressure(doublereal t){
    return (1.0E-200);
  }
 
}
