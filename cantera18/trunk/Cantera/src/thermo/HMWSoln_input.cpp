/**
 *  @file HMWSoln_input.cpp
 *    Definitions for the %HMWSoln ThermoPhase object, which models concentrated
 *    electrolyte solutions
 *    (see \ref thermoprops and \link Cantera::HMWSoln HMWSoln \endlink) .
 *
 * This file contains definitions for reading in the interaction terms
 * in the formulation.
 */
/*
 * Copywrite (2006) Sandia Corporation. Under the terms of 
 * Contract DE-AC04-94AL85000 with Sandia Corporation, the
 * U.S. Government retains certain rights in this software.
 */
/*
 * $Id$
 */

#include "HMWSoln.h"
#include "ThermoFactory.h"
#include "WaterProps.h"
#include "PDSS_Water.h"
#include <cstring>
#include <cstdlib>

using namespace std;

namespace Cantera {


  //! utility function to assign an integer value from a string
  //! for the ElectrolyteSpeciesType field.
  /*!
   *  @param estString string name of the electrolyte species type
   */
  int HMWSoln::interp_est(std::string estString) {
    const char *cc = estString.c_str();
    string lcs = lowercase(estString);
    const char *ccl = lcs.c_str();
    if (!strcmp(ccl, "solvent")) {
      return cEST_solvent;
    } else if (!strcmp(ccl, "chargedspecies")) {
      return cEST_chargedSpecies;
    } else if (!strcmp(ccl, "weakacidassociated")) {
      return cEST_weakAcidAssociated;
    } else if (!strcmp(ccl, "strongacidassociated")) {
      return cEST_strongAcidAssociated;
    } else if (!strcmp(ccl, "polarneutral")) {
      return cEST_polarNeutral;
    } else if (!strcmp(ccl, "nonpolarneutral")) {
      return cEST_nonpolarNeutral;
    }
    int retn, rval;
    if ((retn = sscanf(cc, "%d", &rval)) != 1) {
      return -1;
    }
    return rval;
  }

  /*
   * Process an XML node called "SimpleSaltParameters. 
   * This node contains all of the parameters necessary to describe
   * the Pitzer model for that particular binary salt.
   * This function reads the XML file and writes the coefficients
   * it finds to an internal data structures.
   */
  void HMWSoln::readXMLBinarySalt(XML_Node &BinSalt) {
    string xname = BinSalt.name();
    if (xname != "binarySaltParameters") {
      throw CanteraError("HMWSoln::readXMLBinarySalt",
			 "Incorrect name for processing this routine: " + xname);
    }
    double *charge = DATA_PTR(m_speciesCharge);
    string stemp;
    int nParamsFound, i;
    vector_fp vParams;
    string iName = BinSalt.attrib("cation");
    if (iName == "") {
      throw CanteraError("HMWSoln::readXMLBinarySalt", "no cation attrib");
    }
    string jName = BinSalt.attrib("anion");
    if (jName == "") {
      throw CanteraError("HMWSoln::readXMLBinarySalt", "no anion attrib");
    }
    /*
     * Find the index of the species in the current phase. It's not
     * an error to not find the species
     */
    int iSpecies = speciesIndex(iName);
    if (iSpecies < 0) {
      return;
    }
    string ispName = speciesName(iSpecies);
    if (charge[iSpecies] <= 0) {
      throw CanteraError("HMWSoln::readXMLBinarySalt", "cation charge problem");
    }
    int jSpecies = speciesIndex(jName);
    if (jSpecies < 0) {
      return;
    }
    string jspName = speciesName(jSpecies);
    if (charge[jSpecies] >= 0) {
      throw CanteraError("HMWSoln::readXMLBinarySalt", "anion charge problem");
    }
	
    int n = iSpecies * m_kk + jSpecies;
    int counter = m_CounterIJ[n];
    int num = BinSalt.nChildren();
    for (int iChild = 0; iChild < num; iChild++) {
      XML_Node &xmlChild = BinSalt.child(iChild);
      stemp = xmlChild.name();
      string nodeName = lowercase(stemp);
      /*
       * Process the binary salt child elements
       */
      if (nodeName == "beta0") {
	/*
	 * Get the string containing all of the values
	 */
	getFloatArray(xmlChild, vParams, false, "", "beta0");
	nParamsFound = vParams.size();
	if (m_formPitzerTemp == PITZER_TEMP_CONSTANT) {
	  if (nParamsFound != 1) {
	    throw CanteraError("HMWSoln::readXMLBinarySalt::beta0 for " + ispName 
			       + "::" + jspName,
			       "wrong number of params found");
	  }
	  m_Beta0MX_ij[counter] = vParams[0];
	  m_Beta0MX_ij_coeff(0,counter) = m_Beta0MX_ij[counter];
	} else  if (m_formPitzerTemp == PITZER_TEMP_LINEAR) {
	  if (nParamsFound != 2) {
	    throw CanteraError("HMWSoln::readXMLBinarySalt::beta0 for " + ispName 
			       + "::" + jspName,
			       "wrong number of params found");
	  }
	  m_Beta0MX_ij_coeff(0,counter) = vParams[0];
	  m_Beta0MX_ij_coeff(1,counter) = vParams[1];
	  m_Beta0MX_ij[counter] = vParams[0];
	} else  if (m_formPitzerTemp == PITZER_TEMP_COMPLEX1) {
	  if (nParamsFound != 5) {
	    throw CanteraError("HMWSoln::readXMLBinarySalt::beta0 for " + ispName 
			       + "::" + jspName,
			       "wrong number of params found");
	  }
	  for (i = 0; i < nParamsFound; i++) {
	    m_Beta0MX_ij_coeff(i, counter) = vParams[i];
	  }
	  m_Beta0MX_ij[counter] = vParams[0];
	}
      }
      if (nodeName == "beta1") {

	/*
	 * Get the string containing all of the values
	 */
	getFloatArray(xmlChild, vParams, false, "", "beta1");
	nParamsFound = vParams.size();
	if (m_formPitzerTemp == PITZER_TEMP_CONSTANT) {
	  if (nParamsFound != 1) {
	    throw CanteraError("HMWSoln::readXMLBinarySalt::beta1 for " + ispName 
			       + "::" + jspName,
			       "wrong number of params found");
	  }
	  m_Beta1MX_ij[counter] = vParams[0];
	  m_Beta1MX_ij_coeff(0,counter) = m_Beta1MX_ij[counter];
	} else  if (m_formPitzerTemp == PITZER_TEMP_LINEAR) {
	  if (nParamsFound != 2) {
	    throw CanteraError("HMWSoln::readXMLBinarySalt::beta1 for " + ispName 
			       + "::" + jspName,
			       "wrong number of params found");
	  }
	  m_Beta1MX_ij_coeff(0,counter) = vParams[0];
	  m_Beta1MX_ij_coeff(1,counter) = vParams[1];
	  m_Beta1MX_ij[counter] = vParams[0];
	} else  if (m_formPitzerTemp == PITZER_TEMP_COMPLEX1) {
	  if (nParamsFound != 5) {
	    throw CanteraError("HMWSoln::readXMLBinarySalt::beta1 for " + ispName 
			       + "::" + jspName,
			       "wrong number of params found");
	  }
	  for (i = 0; i < nParamsFound; i++) {
	    m_Beta1MX_ij_coeff(i, counter) = vParams[i];
	  }
	  m_Beta1MX_ij[counter] = vParams[0];
	}
      }
      if (nodeName == "beta2") {
	getFloatArray(xmlChild, vParams, false, "", "beta2");
	nParamsFound = vParams.size();
	if (m_formPitzerTemp == PITZER_TEMP_CONSTANT) {
	  if (nParamsFound != 1) {
	    throw CanteraError("HMWSoln::readXMLBinarySalt::beta2 for " + ispName 
			       + "::" + jspName,
			       "wrong number of params found");
	  }
	  m_Beta2MX_ij[counter] = vParams[0];
	  m_Beta2MX_ij_coeff(0,counter) = m_Beta2MX_ij[counter];
	} else  if (m_formPitzerTemp == PITZER_TEMP_LINEAR) {
	  if (nParamsFound != 2) {
	    throw CanteraError("HMWSoln::readXMLBinarySalt::beta2 for " + ispName 
			       + "::" + jspName,
			       "wrong number of params found");
	  }
	  m_Beta2MX_ij_coeff(0,counter) = vParams[0];
	  m_Beta2MX_ij_coeff(1,counter) = vParams[1];
	  m_Beta2MX_ij[counter] = vParams[0];
	} else  if (m_formPitzerTemp == PITZER_TEMP_COMPLEX1) {
	  if (nParamsFound != 5) {
	    throw CanteraError("HMWSoln::readXMLBinarySalt::beta2 for " + ispName 
			       + "::" + jspName,
			       "wrong number of params found");
	  }
	  for (i = 0; i < nParamsFound; i++) {
	    m_Beta2MX_ij_coeff(i, counter) = vParams[i];
	  }
	  m_Beta2MX_ij[counter] = vParams[0];
	}

      }
      if (nodeName == "cphi") {
	/*
	 * Get the string containing all of the values
	 */
	getFloatArray(xmlChild, vParams, false, "", "Cphi");
	nParamsFound = vParams.size();
	if (m_formPitzerTemp == PITZER_TEMP_CONSTANT) {
	  if (nParamsFound != 1) {
	    throw CanteraError("HMWSoln::readXMLBinarySalt::Cphi for " + ispName 
			       + "::" + jspName,
			       "wrong number of params found");
	  }
	  m_CphiMX_ij[counter] = vParams[0];
	  m_CphiMX_ij_coeff(0,counter) = m_CphiMX_ij[counter];
	} else  if (m_formPitzerTemp == PITZER_TEMP_LINEAR) {
	  if (nParamsFound != 2) {
	    throw CanteraError("HMWSoln::readXMLBinarySalt::Cphi for " + ispName 
			       + "::" + jspName,
			       "wrong number of params found");
	  }
	  m_CphiMX_ij_coeff(0,counter) = vParams[0];
	  m_CphiMX_ij_coeff(1,counter) = vParams[1];
	  m_CphiMX_ij[counter] = vParams[0];
	} else  if (m_formPitzerTemp == PITZER_TEMP_COMPLEX1) {
	  if (nParamsFound != 5) {
	    throw CanteraError("HMWSoln::readXMLBinarySalt::Cphi for " + ispName 
			       + "::" + jspName,
			       "wrong number of params found");
	  }
	  for (i = 0; i < nParamsFound; i++) {
	    m_CphiMX_ij_coeff(i, counter) = vParams[i];
	  }
	  m_CphiMX_ij[counter] = vParams[0];
	}
      }

      if (nodeName == "alpha1") {
	stemp = xmlChild.value();
	m_Alpha1MX_ij[counter] = atofCheck(stemp.c_str());
      }

      if (nodeName == "alpha2") {
	stemp = xmlChild.value();
	m_Alpha2MX_ij[counter] = atofCheck(stemp.c_str());
      }
    }
  }

  /**
   * Process an XML node called "thetaAnion". 
   * This node contains all of the parameters necessary to describe
   * the binary interactions between two anions.
   */
  void HMWSoln::readXMLThetaAnion(XML_Node &BinSalt) {
    string xname = BinSalt.name();
    vector_fp vParams;
    int nParamsFound = 0;
    if (xname != "thetaAnion") {
      throw CanteraError("HMWSoln::readXMLThetaAnion",
			 "Incorrect name for processing this routine: " + xname);
    }
    double *charge = DATA_PTR(m_speciesCharge);
    string stemp;
    string ispName = BinSalt.attrib("anion1");
    if (ispName == "") {
      throw CanteraError("HMWSoln::readXMLThetaAnion", "no anion1 attrib");
    }
    string jspName = BinSalt.attrib("anion2");
    if (jspName == "") {
      throw CanteraError("HMWSoln::readXMLThetaAnion", "no anion2 attrib");
    }
    /*
     * Find the index of the species in the current phase. It's not
     * an error to not find the species
     */
    int iSpecies = speciesIndex(ispName);
    if (iSpecies < 0) {
      return;
    }
    if (charge[iSpecies] >= 0) {
      throw CanteraError("HMWSoln::readXMLThetaAnion", "anion1 charge problem");
    }
    int jSpecies = speciesIndex(jspName);
    if (jSpecies < 0) {
      return;
    }
    if (charge[jSpecies] >= 0) {
      throw CanteraError("HMWSoln::readXMLThetaAnion", "anion2 charge problem");
    }
	
    int n = iSpecies * m_kk + jSpecies;
    int counter = m_CounterIJ[n];
    int num = BinSalt.nChildren();
    for (int i = 0; i < num; i++) {
      XML_Node &xmlChild = BinSalt.child(i);
      stemp = xmlChild.name();
      string nodeName = lowercase(stemp);
      if (nodeName == "theta") {
	getFloatArray(xmlChild, vParams, false, "", stemp);
	nParamsFound = vParams.size();
	if (m_formPitzerTemp == PITZER_TEMP_CONSTANT) {
	  if (nParamsFound != 1) {
	    throw CanteraError("HMWSoln::readXMLThetaAnion::Theta for " + ispName 
			       + "::" + jspName,
			       "wrong number of params found");
	  }
	  m_Theta_ij_coeff(0,counter) = vParams[0];
	  m_Theta_ij[counter] = vParams[0];
	} else  if (m_formPitzerTemp == PITZER_TEMP_LINEAR) {
	  if (nParamsFound != 2) {
	    throw CanteraError("HMWSoln::readXMLThetaAnion::Theta for " + ispName
			       + "::" + jspName,
			       "wrong number of params found");
	  }
	  m_Theta_ij_coeff(0,counter) = vParams[0];
	  m_Theta_ij_coeff(1,counter) = vParams[1];
	  m_Theta_ij[counter] = vParams[0];
	} else  if (m_formPitzerTemp == PITZER_TEMP_COMPLEX1) {
	  if (nParamsFound == 1) {
	    vParams.resize(5, 0.0);
	    nParamsFound = 5;
	  } else if (nParamsFound != 5) {
	    throw CanteraError("HMWSoln::readXMLThetaAnion::Theta for " + ispName
			       + "::" + jspName,
			       "wrong number of params found");
	  }
	  for (i = 0; i < nParamsFound; i++) {
	    m_Theta_ij_coeff(i, counter) = vParams[i];
	  }
	  m_Theta_ij[counter] = vParams[0];
	}
      }
    }
  } 

  /**
   * Process an XML node called "thetaCation". 
   * This node contains all of the parameters necessary to describe
   * the binary interactions between two cation.
   */
  void HMWSoln::readXMLThetaCation(XML_Node &BinSalt) {
    string xname = BinSalt.name();
    vector_fp vParams;
    int nParamsFound = 0;
    if (xname != "thetaCation") {
      throw CanteraError("HMWSoln::readXMLThetaCation",
			 "Incorrect name for processing this routine: " + xname);
    }
    double *charge = DATA_PTR(m_speciesCharge);
    string stemp;
    string ispName = BinSalt.attrib("cation1");
    if (ispName == "") {
      throw CanteraError("HMWSoln::readXMLThetaCation", "no cation1 attrib");
    }
    string jspName = BinSalt.attrib("cation2");
    if (jspName == "") {
      throw CanteraError("HMWSoln::readXMLThetaCation", "no cation2 attrib");
    }
    /*
     * Find the index of the species in the current phase. It's not
     * an error to not find the species
     */
    int iSpecies = speciesIndex(ispName);
    if (iSpecies < 0) {
      return;
    }
    if (charge[iSpecies] <= 0) {
      throw CanteraError("HMWSoln::readXMLThetaCation", "cation1 charge problem");
    }
    int jSpecies = speciesIndex(jspName);
    if (jSpecies < 0) {
      return;
    }
    if (charge[jSpecies] <= 0) {
      throw CanteraError("HMWSoln::readXMLThetaCation", "cation2 charge problem");
    }
	
    int n = iSpecies * m_kk + jSpecies;
    int counter = m_CounterIJ[n];
    int num = BinSalt.nChildren();
    for (int i = 0; i < num; i++) {
      XML_Node &xmlChild = BinSalt.child(i);
      stemp = xmlChild.name();
      string nodeName = lowercase(stemp);
      if (nodeName == "theta") {
	getFloatArray(xmlChild, vParams, false, "", stemp);
	nParamsFound = vParams.size();
	if (m_formPitzerTemp == PITZER_TEMP_CONSTANT) {
	  if (nParamsFound != 1) {
	    throw CanteraError("HMWSoln::readXMLThetaCation::Theta for " + ispName 
			       + "::" + jspName,
			       "wrong number of params found");
	  }
	  m_Theta_ij_coeff(0,counter) = vParams[0];
	  m_Theta_ij[counter] = vParams[0];
	} else  if (m_formPitzerTemp == PITZER_TEMP_LINEAR) {
	  if (nParamsFound != 2) {
	    throw CanteraError("HMWSoln::readXMLThetaCation::Theta for " + ispName
			       + "::" + jspName,
			       "wrong number of params found");
	  }
	  m_Theta_ij_coeff(0,counter) = vParams[0];
	  m_Theta_ij_coeff(1,counter) = vParams[1];
	  m_Theta_ij[counter] = vParams[0];
	} else  if (m_formPitzerTemp == PITZER_TEMP_COMPLEX1) {
	  if (nParamsFound == 1) {
	    vParams.resize(5, 0.0);
	    nParamsFound = 5;
	  } else if (nParamsFound != 5) {
	    throw CanteraError("HMWSoln::readXMLThetaCation::Theta for " + ispName
			       + "::" + jspName,
			       "wrong number of params found");
	  }
	  for (i = 0; i < nParamsFound; i++) {
	    m_Theta_ij_coeff(i, counter) = vParams[i];
	  }
	  m_Theta_ij[counter] = vParams[0];
	}
      }
    }
  }

  /*
   * Process an XML node called "readXMLPsiCommonCation". 
   * This node contains all of the parameters necessary to describe
   * the binary interactions between two anions and one common cation.
   */
  void HMWSoln::readXMLPsiCommonCation(XML_Node &BinSalt) {
    string xname = BinSalt.name();
    if (xname != "psiCommonCation") {
      throw CanteraError("HMWSoln::readXMLPsiCommonCation",
			 "Incorrect name for processing this routine: " + xname);
    }
    double *charge = DATA_PTR(m_speciesCharge);
    string stemp;
    vector_fp vParams;
    int nParamsFound = 0;
    string kName = BinSalt.attrib("cation");
    if (kName == "") {
      throw CanteraError("HMWSoln::readXMLPsiCommonCation", "no cation attrib");
    }
    string iName = BinSalt.attrib("anion1");
    if (iName == "") {
      throw CanteraError("HMWSoln::readXMLPsiCommonCation", "no anion1 attrib");
    }
    string jName = BinSalt.attrib("anion2");
    if (jName == "") {
      throw CanteraError("HMWSoln::readXMLPsiCommonCation", "no anion2 attrib");
    }
    /*
     * Find the index of the species in the current phase. It's not
     * an error to not find the species
     */
    int kSpecies = speciesIndex(kName);
    if (kSpecies < 0) {
      return;
    }
    if (charge[kSpecies] <= 0) {
      throw CanteraError("HMWSoln::readXMLPsiCommonCation", 
			 "cation charge problem");
    }
    int iSpecies = speciesIndex(iName);
    if (iSpecies < 0) {
      return;
    }
    if (charge[iSpecies] >= 0) {
      throw CanteraError("HMWSoln::readXMLPsiCommonCation", 
			 "anion1 charge problem");
    }
    int jSpecies = speciesIndex(jName);
    if (jSpecies < 0) {
      return;
    }
    if (charge[jSpecies] >= 0) {
      throw CanteraError("HMWSoln::readXMLPsiCommonCation", 
			 "anion2 charge problem");
    }
    
    int n = iSpecies * m_kk + jSpecies;
    int counter = m_CounterIJ[n];
    int num = BinSalt.nChildren();
    for (int i = 0; i < num; i++) {
      XML_Node &xmlChild = BinSalt.child(i);
      stemp = xmlChild.name();
      string nodeName = lowercase(stemp);
      if (nodeName == "theta") {
	stemp = xmlChild.value();
	double old = m_Theta_ij[counter];
	m_Theta_ij[counter] = atofCheck(stemp.c_str());
	if (old != 0.0) {
	  if (old != m_Theta_ij[counter]) {
	    throw CanteraError("HMWSoln::readXMLPsiCommonCation",
			       "conflicting values");
	  }
	}
      }
      if (nodeName == "psi") {
	getFloatArray(xmlChild, vParams, false, "", stemp);
	nParamsFound = vParams.size();
	n = iSpecies * m_kk *m_kk + jSpecies * m_kk + kSpecies ;

	if (m_formPitzerTemp == PITZER_TEMP_CONSTANT) {
	  if (nParamsFound != 1) {
	    throw CanteraError("HMWSoln::readXMLPsiCommonCation::Psi for "
			       + kName + "::" + iName + "::" + jName,
			       "wrong number of params found");
	  }
	  m_Psi_ijk_coeff(0,n) = vParams[0];
	  m_Psi_ijk[n] = vParams[0];
	} else  if (m_formPitzerTemp == PITZER_TEMP_LINEAR) {
	  if (nParamsFound != 2) {
	    throw CanteraError("HMWSoln::readXMLPsiCation::Psi for "
			       + kName + "::" + iName + "::" + jName,
			       "wrong number of params found");
	  }
	  m_Psi_ijk_coeff(0,n) = vParams[0];
	  m_Psi_ijk_coeff(1,n) = vParams[1];
	  m_Psi_ijk[n]         = vParams[0];
	} else  if (m_formPitzerTemp == PITZER_TEMP_COMPLEX1) {
	  if (nParamsFound == 1) {
	    vParams.resize(5, 0.0);
	    nParamsFound = 5;
	  } else if (nParamsFound != 5) {
	    throw CanteraError("HMWSoln::readXMLPsiCation::Psi for "
			       + kName + "::" + iName + "::" + jName,
			       "wrong number of params found");
	  }
	  for (i = 0; i < nParamsFound; i++) {
	    m_Psi_ijk_coeff(i, n) = vParams[i];
	  }
	  m_Psi_ijk[n] = vParams[0];
	}


	// fill in the duplicate entries
	n = iSpecies * m_kk *m_kk + kSpecies * m_kk + jSpecies ;
	for (i = 0; i < nParamsFound; i++) {
	  m_Psi_ijk_coeff(i, n) = vParams[i];
	}
	m_Psi_ijk[n] = vParams[0];

	n = jSpecies * m_kk *m_kk + iSpecies * m_kk + kSpecies ;
	for (i = 0; i < nParamsFound; i++) {
	  m_Psi_ijk_coeff(i, n) = vParams[i];
	}
	m_Psi_ijk[n] = vParams[0];

	n = jSpecies * m_kk *m_kk + kSpecies * m_kk + iSpecies ;
	for (i = 0; i < nParamsFound; i++) {
	  m_Psi_ijk_coeff(i, n) = vParams[i];
	}
	m_Psi_ijk[n] = vParams[0];

	n = kSpecies * m_kk *m_kk + jSpecies * m_kk + iSpecies ;
	for (i = 0; i < nParamsFound; i++) {
	  m_Psi_ijk_coeff(i, n) = vParams[i];
	}
	m_Psi_ijk[n] = vParams[0];

	n = kSpecies * m_kk *m_kk + iSpecies * m_kk + jSpecies ;
	for (i = 0; i < nParamsFound; i++) {
	  m_Psi_ijk_coeff(i, n) = vParams[i];
	}
	m_Psi_ijk[n] = vParams[0];
      }
    }
  }
   
  /**
   * Process an XML node called "PsiCommonAnion". 
   * This node contains all of the parameters necessary to describe
   * the binary interactions between two cations and one common anion.
   */
  void HMWSoln::readXMLPsiCommonAnion(XML_Node &BinSalt) {
    string xname = BinSalt.name();
    if (xname != "psiCommonAnion") {
      throw CanteraError("HMWSoln::readXMLPsiCommonAnion",
			 "Incorrect name for processing this routine: " + xname);
    }
    double *charge = DATA_PTR(m_speciesCharge);
    string stemp;
    vector_fp vParams;
    int nParamsFound = 0;
    string kName = BinSalt.attrib("anion");
    if (kName == "") {
      throw CanteraError("HMWSoln::readXMLPsiCommonAnion", "no anion attrib");
    }
    string iName = BinSalt.attrib("cation1");
    if (iName == "") {
      throw CanteraError("HMWSoln::readXMLPsiCommonAnion", "no cation1 attrib");
    }
    string jName = BinSalt.attrib("cation2");
    if (jName == "") {
      throw CanteraError("HMWSoln::readXMLPsiCommonAnion", "no cation2 attrib");
    }
    /*
     * Find the index of the species in the current phase. It's not
     * an error to not find the species
     */
    int kSpecies = speciesIndex(kName);
    if (kSpecies < 0) {
      return;
    }
    if (charge[kSpecies] >= 0) {
      throw CanteraError("HMWSoln::readXMLPsiCommonAnion", "anion charge problem");
    }
    int iSpecies = speciesIndex(iName);
    if (iSpecies < 0) {
      return;
    }
    if (charge[iSpecies] <= 0) {
      throw CanteraError("HMWSoln::readXMLPsiCommonAnion", 
			 "cation1 charge problem");
    }
    int jSpecies = speciesIndex(jName);
    if (jSpecies < 0) {
      return;
    }
    if (charge[jSpecies] <= 0) {
      throw CanteraError("HMWSoln::readXMLPsiCommonAnion", 
			 "cation2 charge problem");
    }
	
    int n = iSpecies * m_kk + jSpecies;
    int counter = m_CounterIJ[n];
    int num = BinSalt.nChildren();
    for (int i = 0; i < num; i++) {
      XML_Node &xmlChild = BinSalt.child(i);
      stemp = xmlChild.name();
      string nodeName = lowercase(stemp);
      if (nodeName == "theta") {
	stemp = xmlChild.value();
	double old = m_Theta_ij[counter];
	m_Theta_ij[counter] = atofCheck(stemp.c_str());
	if (old != 0.0) {
	  if (old != m_Theta_ij[counter]) {
	    throw CanteraError("HMWSoln::readXMLPsiCommonAnion",
			       "conflicting values");
	  }
	}
      }
      if (nodeName == "psi") {

	getFloatArray(xmlChild, vParams, false, "", stemp);
	nParamsFound = vParams.size();
	n = iSpecies * m_kk *m_kk + jSpecies * m_kk + kSpecies ;

	if (m_formPitzerTemp == PITZER_TEMP_CONSTANT) {
	  if (nParamsFound != 1) {
	    throw CanteraError("HMWSoln::readXMLPsiCommonAnion::Psi for "
			       + kName + "::" + iName + "::" + jName,
			       "wrong number of params found");
	  }
	  m_Psi_ijk_coeff(0,n) = vParams[0];
	  m_Psi_ijk[n] = vParams[0];
	} else  if (m_formPitzerTemp == PITZER_TEMP_LINEAR) {
	  if (nParamsFound != 2) {
	    throw CanteraError("HMWSoln::readXMLPsiAnion::Psi for "
			       + kName + "::" + iName + "::" + jName,
			       "wrong number of params found");
	  }
	  m_Psi_ijk_coeff(0,n) = vParams[0];
	  m_Psi_ijk_coeff(1,n) = vParams[1];
	  m_Psi_ijk[n]         = vParams[0];
	} else  if (m_formPitzerTemp == PITZER_TEMP_COMPLEX1) {
	  if (nParamsFound == 1) {
	    vParams.resize(5, 0.0);
	    nParamsFound = 5;
	  } else if (nParamsFound != 5) {
	    throw CanteraError("HMWSoln::readXMLPsiAnion::Psi for "
			       + kName + "::" + iName + "::" + jName,
			       "wrong number of params found");
	  }
	  for (i = 0; i < nParamsFound; i++) {
	    m_Psi_ijk_coeff(i, n) = vParams[i];
	  }
	  m_Psi_ijk[n] = vParams[0];
	}


	// fill in the duplicate entries
	n = iSpecies * m_kk *m_kk + kSpecies * m_kk + jSpecies ;
	for (i = 0; i < nParamsFound; i++) {
	  m_Psi_ijk_coeff(i, n) = vParams[i];
	}
	m_Psi_ijk[n] = vParams[0];

	n = jSpecies * m_kk *m_kk + iSpecies * m_kk + kSpecies ;
	for (i = 0; i < nParamsFound; i++) {
	  m_Psi_ijk_coeff(i, n) = vParams[i];
	}
	m_Psi_ijk[n] = vParams[0];

	n = jSpecies * m_kk *m_kk + kSpecies * m_kk + iSpecies ;
	for (i = 0; i < nParamsFound; i++) {
	  m_Psi_ijk_coeff(i, n) = vParams[i];
	}
	m_Psi_ijk[n] = vParams[0];

	n = kSpecies * m_kk *m_kk + jSpecies * m_kk + iSpecies ;
	for (i = 0; i < nParamsFound; i++) {
	  m_Psi_ijk_coeff(i, n) = vParams[i];
	}
	m_Psi_ijk[n] = vParams[0];

	n = kSpecies * m_kk *m_kk + iSpecies * m_kk + jSpecies ;
	for (i = 0; i < nParamsFound; i++) {
	  m_Psi_ijk_coeff(i, n) = vParams[i];
	}
	m_Psi_ijk[n] = vParams[0];

      }
    }
  }


   
  /**
   * Process an XML node called "LambdaNeutral". 
   * This node contains all of the parameters necessary to describe
   * the binary interactions between one neutral species and
   * any other species (neutral or otherwise) in the mechanism.
   */
  void HMWSoln::readXMLLambdaNeutral(XML_Node &BinSalt) {
    string xname = BinSalt.name();
    vector_fp vParams;
    int nParamsFound;
    if (xname != "lambdaNeutral") {
      throw CanteraError("HMWSoln::readXMLLanbdaNeutral",
			 "Incorrect name for processing this routine: " + xname);
    }
    double *charge = DATA_PTR(m_speciesCharge);
    string stemp;
    string iName = BinSalt.attrib("species1");
    if (iName == "") {
      throw CanteraError("HMWSoln::readXMLLambdaNeutral", "no species1 attrib");
    }
    string jName = BinSalt.attrib("species2");
    if (jName == "") {
      throw CanteraError("HMWSoln::readXMLLambdaNeutral", "no species2 attrib");
    }
    /*
     * Find the index of the species in the current phase. It's not
     * an error to not find the species
     */
    int iSpecies = speciesIndex(iName);
    if (iSpecies < 0) {
      return;
    }
    if (charge[iSpecies] != 0) {
      throw CanteraError("HMWSoln::readXMLLambdaNeutral", 
			 "neutral charge problem");
    }
    int jSpecies = speciesIndex(jName);
    if (jSpecies < 0) {
      return;
    }
	
    int num = BinSalt.nChildren();
    for (int i = 0; i < num; i++) {
      XML_Node &xmlChild = BinSalt.child(i);
      stemp = xmlChild.name();
      string nodeName = lowercase(stemp);
      if (nodeName == "lambda") {
	int nCount = iSpecies*m_kk + jSpecies;
	getFloatArray(xmlChild, vParams, false, "", stemp);
	nParamsFound = vParams.size();
	if (m_formPitzerTemp == PITZER_TEMP_CONSTANT) {
	  if (nParamsFound != 1) {
	    throw CanteraError("HMWSoln::readXMLLambdaNeutral::Lambda for " + iName 
			       + "::" + jName,
			       "wrong number of params found");
	  }
	  m_Lambda_nj_coeff(0,nCount) = vParams[0];
	  m_Lambda_nj(iSpecies,jSpecies) = vParams[0];

	} else  if (m_formPitzerTemp == PITZER_TEMP_LINEAR) {
	  if (nParamsFound != 2) {
	    throw CanteraError("HMWSoln::readXMLLambdaNeutral::Lambda for " + iName
			       + "::" + jName,
			       "wrong number of params found");
	  }
	  m_Lambda_nj_coeff(0,nCount) = vParams[0];
	  m_Lambda_nj_coeff(1,nCount) = vParams[1];
	  m_Lambda_nj(iSpecies, jSpecies) = vParams[0];

	} else  if (m_formPitzerTemp == PITZER_TEMP_COMPLEX1) {
	  if (nParamsFound == 1) {
	    vParams.resize(5, 0.0);
	    nParamsFound = 5;
	  } else if (nParamsFound != 5) {
	    throw CanteraError("HMWSoln::readXMLLambdaNeutral::Lambda for " + iName
			       + "::" + jName,
			       "wrong number of params found");
	  }
	  for (i = 0; i < nParamsFound; i++) {
	    m_Lambda_nj_coeff(i,nCount) = vParams[i];
	  }
	  m_Lambda_nj(iSpecies, jSpecies) = vParams[0];
	}
      }
    }
  }

  /**
   * Process an XML node called "MunnnNeutral". 
   * This node contains all of the parameters necessary to describe
   * the self-ternary interactions for one neutral species.
   */
  void HMWSoln::readXMLMunnnNeutral(XML_Node &BinSalt) {
    string xname = BinSalt.name();
    vector_fp vParams;
    int nParamsFound;
    if (xname != "MunnnNeutral") {
      throw CanteraError("HMWSoln::readXMLMunnnNeutral",
			 "Incorrect name for processing this routine: " + xname);
    }
    double *charge = DATA_PTR(m_speciesCharge);
    string stemp;
    string iName = BinSalt.attrib("species1");
    if (iName == "") {
      throw CanteraError("HMWSoln::readXMLMunnnNeutral", "no species1 attrib");
    }
 
    /*
     * Find the index of the species in the current phase. It's not
     * an error to not find the species
     */
    int iSpecies = speciesIndex(iName);
    if (iSpecies < 0) {
      return;
    }
    if (charge[iSpecies] != 0) {
      throw CanteraError("HMWSoln::readXMLMunnnNeutral", 
			 "neutral charge problem");
    }
 
	
    int num = BinSalt.nChildren();
    for (int i = 0; i < num; i++) {
      XML_Node &xmlChild = BinSalt.child(i);
      stemp = xmlChild.name();
      string nodeName = lowercase(stemp);
      if (nodeName == "munnn") {
	getFloatArray(xmlChild, vParams, false, "", "Munnn");
	nParamsFound = vParams.size();
	if (m_formPitzerTemp == PITZER_TEMP_CONSTANT) {
	  if (nParamsFound != 1) {
	    throw CanteraError("HMWSoln::readXMLMunnnNeutral::Munnn for " + iName,	  
			       "wrong number of params found");
	  }
	  m_Mu_nnn_coeff(0,iSpecies) = vParams[0];
	  m_Mu_nnn[iSpecies] = vParams[0];

	} else  if (m_formPitzerTemp == PITZER_TEMP_LINEAR) {
	  if (nParamsFound != 2) {
	    throw CanteraError("HMWSoln::readXMLMunnnNeutral::Munnn for " + iName,
			       "wrong number of params found");
	  }
	  m_Mu_nnn_coeff(0, iSpecies) = vParams[0];
	  m_Mu_nnn_coeff(1, iSpecies) = vParams[1];
	  m_Mu_nnn[iSpecies] = vParams[0];

	} else  if (m_formPitzerTemp == PITZER_TEMP_COMPLEX1) {
	  if (nParamsFound == 1) {
	    vParams.resize(5, 0.0);
	    nParamsFound = 5;
	  } else if (nParamsFound != 5) {
	    throw CanteraError("HMWSoln::readXMLMunnnNeutral::Munnn for " + iName,
			       "wrong number of params found");
	  }
	  for (i = 0; i < nParamsFound; i++) {
	    m_Mu_nnn_coeff(i, iSpecies) = vParams[i];
	  }
	  m_Mu_nnn[iSpecies] = vParams[0];
	}
      }
    }
  }

  /*
   * Process an XML node called "readXMLZetaCation". 
   * This node contains all of the parameters necessary to describe
   * the ternary interactions between a neutral, a cation and an anion
   */
  void HMWSoln::readXMLZetaCation(const XML_Node &BinSalt) {
    string xname = BinSalt.name();
    if (xname != "zetaCation") {
      throw CanteraError("HMWSoln::readXMLZetaCation",
			 "Incorrect name for processing this routine: " + xname);
    }
    double *charge = DATA_PTR(m_speciesCharge);
    string stemp;
    vector_fp vParams;
    int nParamsFound = 0;
    
    string iName = BinSalt.attrib("neutral");
    if (iName == "") {
      throw CanteraError("HMWSoln::readXMLZetaCation", "no neutral attrib");
    }
  
    string jName = BinSalt.attrib("cation1");
    if (jName == "") {
      throw CanteraError("HMWSoln::readXMLZetaCation", "no cation1 attrib");
    }

    string kName = BinSalt.attrib("anion1");
    if (kName == "") {
      throw CanteraError("HMWSoln::readXMLZetaCation", "no anion1 attrib");
    }
    /*
     * Find the index of the species in the current phase. It's not
     * an error to not find the species
     */
    int iSpecies = speciesIndex(iName);
    if (iSpecies < 0) {
      return;
    }
    if (charge[iSpecies] != 0.0) {
      throw CanteraError("HMWSoln::readXMLZetaCation",  "neutral charge problem");
    }
 
    int jSpecies = speciesIndex(jName);
    if (jSpecies < 0) {
      return;
    }
    if (charge[jSpecies] <= 0.0) {
      throw CanteraError("HMWSoln::readXLZetaCation", "cation1 charge problem");
    }

    int kSpecies = speciesIndex(kName);
    if (kSpecies < 0) {
      return;
    }
    if (charge[kSpecies] >= 0.0) {
      throw CanteraError("HMWSoln::readXMLZetaCation", "anion1 charge problem");
    }
 
    int num = BinSalt.nChildren();
    for (int i = 0; i < num; i++) {
      XML_Node &xmlChild = BinSalt.child(i);
      stemp = xmlChild.name();
      string nodeName = lowercase(stemp);
      if (nodeName == "zeta") {
	getFloatArray(xmlChild, vParams, false, "", "zeta");
	nParamsFound = vParams.size();
	int n = iSpecies * m_kk *m_kk + jSpecies * m_kk + kSpecies ;

	if (m_formPitzerTemp == PITZER_TEMP_CONSTANT) {
	  if (nParamsFound != 1) {
	    throw CanteraError("HMWSoln::readXMLZetaCation::Zeta for "
			       + iName + "::" + jName + "::" + kName,
			       "wrong number of params found");
	  }
	  m_Psi_ijk_coeff(0,n) = vParams[0];
	  m_Psi_ijk[n] = vParams[0];
	} else  if (m_formPitzerTemp == PITZER_TEMP_LINEAR) {
	  if (nParamsFound != 2) {
	    throw CanteraError("HMWSoln::readXMLZetaCation::Zeta for "
			       + iName + "::" + jName + "::" + kName,
			       "wrong number of params found");
	  }
	  m_Psi_ijk_coeff(0,n) = vParams[0];
	  m_Psi_ijk_coeff(1,n) = vParams[1];
	  m_Psi_ijk[n]         = vParams[0];
	} else  if (m_formPitzerTemp == PITZER_TEMP_COMPLEX1) {
	  if (nParamsFound == 1) {
	    vParams.resize(5, 0.0);
	    nParamsFound = 5;
	  } else if (nParamsFound != 5) {
	    throw CanteraError("HMWSoln::readXMLZetaCation::Zeta for "
			       + iName + "::" + jName + "::" + kName,
			       "wrong number of params found");
	  }
	  for (i = 0; i < nParamsFound; i++) {
	    m_Psi_ijk_coeff(i, n) = vParams[i];
	  }
	  m_Psi_ijk[n] = vParams[0];
	}

	// There are no duplicate entries
      }
    }
  }
   
  // Process an XML node called "croppingCoefficients"
  // for the cropping coefficients values
  /*
   * @param acNode Activity Coefficient XML Node
   */
  void HMWSoln::readXMLCroppingCoefficients(const XML_Node &acNode) {

    if (acNode.hasChild("croppingCoefficients")) {
      XML_Node &cropNode = acNode.child("croppingCoefficients");
      if (cropNode.hasChild("ln_gamma_k_min")) {
	XML_Node &gkminNode = cropNode.child("ln_gamma_k_min");
	getOptionalFloat(gkminNode, "pureSolventValue", CROP_ln_gamma_k_min);
      }
      if (cropNode.hasChild("ln_gamma_k_max")) {
	XML_Node &gkmaxNode = cropNode.child("ln_gamma_k_max");
	getOptionalFloat(gkmaxNode, "pureSolventValue", CROP_ln_gamma_k_max);
      }

      if (cropNode.hasChild("ln_gamma_o_min")) {
	XML_Node &gominNode = cropNode.child("ln_gamma_o_min");
	getOptionalFloat(gominNode, "pureSolventValue", CROP_ln_gamma_o_min);
      }

      if (cropNode.hasChild("ln_gamma_o_max")) {
	XML_Node &gomaxNode = cropNode.child("ln_gamma_o_max");
	getOptionalFloat(gomaxNode, "pureSolventValue", CROP_ln_gamma_o_max);
      } 
    }
  }

  /*
   *  Initialization routine for a HMWSoln phase.
   *
   * This is a virtual routine. This routine will call initThermo()
   * for the parent class as well.
   */
  void HMWSoln::initThermo() {
    MolalityVPSSTP::initThermo();
    initLengths();
  }

  /*
   *   Import, construct, and initialize a HMWSoln phase 
   *   specification from an XML tree into the current object.
   *
   * This routine is a precursor to constructPhaseXML(XML_Node*)
   * routine, which does most of the work.
   *
   * @param infile XML file containing the description of the
   *        phase
   *
   * @param id  Optional parameter identifying the name of the
   *            phase. If none is given, the first XML
   *            phase element will be used.
   */
  void HMWSoln::constructPhaseFile(std::string inputFile, std::string id) {

    if (inputFile.size() == 0) {
      throw CanteraError("HMWSoln:constructPhaseFile",
			 "input file is null");
    }
    string path = findInputFile(inputFile);
    std::ifstream fin(path.c_str());
    if (!fin) {
      throw CanteraError("HMWSoln:constructPhaseFile","could not open "
			 +path+" for reading.");
    }
    /*
     * The phase object automatically constructs an XML object.
     * Use this object to store information.
     */
    XML_Node &phaseNode_XML = xml();
    XML_Node *fxml = new XML_Node();
    fxml->build(fin);
    XML_Node *fxml_phase = findXMLPhase(fxml, id);
    if (!fxml_phase) {
      throw CanteraError("HMWSoln:constructPhaseFile",
			 "ERROR: Can not find phase named " +
			 id + " in file named " + inputFile);
    }
    fxml_phase->copy(&phaseNode_XML);	
    constructPhaseXML(*fxml_phase, id);
    delete fxml;
  }
  
  /*
   *   Import, construct, and initialize a HMWSoln phase 
   *   specification from an XML tree into the current object.
   *
   *   Most of the work is carried out by the cantera base
   *   routine, importPhase(). That routine imports all of the
   *   species and element data, including the standard states
   *   of the species.
   *
   *   Then, In this routine, we read the information 
   *   particular to the specification of the activity 
   *   coefficient model for the Pitzer parameterization.
   *
   *   We also read information about the molar volumes of the
   *   standard states if present in the XML file.
   *
   * @param phaseNode This object must be the phase node of a
   *             complete XML tree
   *             description of the phase, including all of the
   *             species data. In other words while "phase" must
   *             point to an XML phase object, it must have
   *             sibling nodes "speciesData" that describe
   *             the species in the phase.
   * @param id   ID of the phase. If nonnull, a check is done
   *             to see if phaseNode is pointing to the phase
   *             with the correct id. 
   */
  void HMWSoln::constructPhaseXML(XML_Node& phaseNode, std::string id) {
    string stemp;
    if (id.size() > 0) {
      string idp = phaseNode.id();
      if (idp != id) {
	throw CanteraError("HMWSoln::constructPhaseXML", 
			   "phasenode and Id are incompatible");
      }
    }

    /*
     * Find the Thermo XML node 
     */
    if (!phaseNode.hasChild("thermo")) {
      throw CanteraError("HMWSoln::constructPhaseXML",
			 "no thermo XML node");
    }
    XML_Node& thermoNode = phaseNode.child("thermo");

    /*
     * Possibly change the form of the standard concentrations
     */
    if (thermoNode.hasChild("standardConc")) {
      XML_Node& scNode = thermoNode.child("standardConc");
      m_formGC = 2;
      stemp = scNode.attrib("model");
      string formString = lowercase(stemp);
      if (formString != "") {
	if (formString == "unity") {
	  m_formGC = 0;
	  printf("exit standardConc = unity not done\n");
	  exit(EXIT_FAILURE);
	} else if (formString == "molar_volume") {
	  m_formGC = 1;
	  printf("exit standardConc = molar_volume not done\n");
	  exit(EXIT_FAILURE);
	} else if (formString == "solvent_volume") {
	  m_formGC = 2;
	} else {
	  throw CanteraError("HMWSoln::constructPhaseXML",
			     "Unknown standardConc model: " + formString);
	}
      }
    }
    /*
     * Get the Name of the Solvent:
     *      <solvent> solventName </solvent>
     */
    string solventName = "";
    if (thermoNode.hasChild("solvent")) {
      XML_Node& scNode = thermoNode.child("solvent");
      vector<string> nameSolventa;
      getStringArray(scNode, nameSolventa);
      int nsp = static_cast<int>(nameSolventa.size());
      if (nsp != 1) {
	throw CanteraError("HMWSoln::constructPhaseXML",
			   "badly formed solvent XML node");
      }
      solventName = nameSolventa[0];
    }

    /*
     * Determine the form of the Pitzer model,
     *   We will use this information to size arrays below.
     */
    if (thermoNode.hasChild("activityCoefficients")) {
      XML_Node& scNode = thermoNode.child("activityCoefficients");
      m_formPitzer = m_formPitzer;
      stemp = scNode.attrib("model");
      string formString = lowercase(stemp);
      if (formString != "") {
	if        (formString == "pitzer" || formString == "default") {
	  m_formPitzer = PITZERFORM_BASE;
	} else if (formString == "base") {
	  m_formPitzer = PITZERFORM_BASE;
	} else {
	  throw CanteraError("HMWSoln::constructPhaseXML",
			     "Unknown Pitzer ActivityCoeff model: "
			     + formString);
	}
      }

      /*
       * Determine the form of the temperature dependence
       * of the Pitzer activity coefficient model.
       */
      stemp = scNode.attrib("TempModel");
      formString = lowercase(stemp);
      if (formString != "") {
	if (formString == "constant" || formString == "default") {
	  m_formPitzerTemp = PITZER_TEMP_CONSTANT;
	} else if (formString == "linear") {
	  m_formPitzerTemp = PITZER_TEMP_LINEAR;
	} else if (formString == "complex" || formString == "complex1") {
	  m_formPitzerTemp = PITZER_TEMP_COMPLEX1;
	} else {
	  throw CanteraError("HMWSoln::constructPhaseXML",
			     "Unknown Pitzer ActivityCoeff Temp model: "
			     + formString);
	}
      }

      /*
       * Determine the reference temperature
       * of the Pitzer activity coefficient model's temperature
       * dependence formulation: defaults to 25C
       */
      stemp = scNode.attrib("TempReference");
      formString = lowercase(stemp);
      if (formString != "") {
	m_TempPitzerRef = atofCheck(formString.c_str());
      } else {
	m_TempPitzerRef = 273.15 + 25;
      }
	
    }

    /*
     * Call the Cantera importPhase() function. This will import
     * all of the species into the phase. This will also handle
     * all of the solvent and solute standard states
     */
    bool m_ok = importPhase(phaseNode, this);
    if (!m_ok) {
      throw CanteraError("HMWSoln::constructPhaseXML","importPhase failed "); 
    }
    
  }

  /**
   * Process the XML file after species are set up.
   *
   *  This gets called from importPhase(). It processes the XML file
   *  after the species are set up. This is the main routine for
   *  reading in activity coefficient parameters.
   *
   * @param phaseNode This object must be the phase node of a
   *             complete XML tree
   *             description of the phase, including all of the
   *             species data. In other words while "phase" must
   *             point to an XML phase object, it must have
   *             sibling nodes "speciesData" that describe
   *             the species in the phase.
   * @param id   ID of the phase. If nonnull, a check is done
   *             to see if phaseNode is pointing to the phase
   *             with the correct id.
   */
  void HMWSoln::
  initThermoXML(XML_Node& phaseNode, std::string id) {
    int k;
    string stemp;
    /*
     * Find the Thermo XML node 
     */
    if (!phaseNode.hasChild("thermo")) {
      throw CanteraError("HMWSoln::initThermoXML",
			 "no thermo XML node");
    }
    XML_Node& thermoNode = phaseNode.child("thermo");

    /*
     * Get the Name of the Solvent:
     *      <solvent> solventName </solvent>
     */
    string solventName = "";
    if (thermoNode.hasChild("solvent")) {
      XML_Node& scNode = thermoNode.child("solvent");
      vector<string> nameSolventa;
      getStringArray(scNode, nameSolventa);
      int nsp = static_cast<int>(nameSolventa.size());
      if (nsp != 1) {
	throw CanteraError("HMWSoln::initThermoXML",
			   "badly formed solvent XML node");
      }
      solventName = nameSolventa[0];
    }

    /*
     * Initialize all of the lengths of arrays in the object
     * now that we know what species are in the phase.
     */
    initLengths();

    /*
     * Reconcile the solvent name and index.
     */
    for (k = 0; k < m_kk; k++) {
      string sname = speciesName(k);
      if (solventName == sname) {
	setSolvent(k);
	if (k != 0) {
	  throw CanteraError("HMWSoln::initThermoXML",
			     "Solvent must be species 0 atm");
	}
	m_indexSolvent = k;
	break;
      }
    }
    if (m_indexSolvent == -1) {
      std::cout << "HMWSoln::initThermo: Solvent Name not found" 
		<< std::endl;
      throw CanteraError("HMWSoln::initThermoXML",
			 "Solvent name not found");
    }
    if (m_indexSolvent != 0) {
      throw CanteraError("HMWSoln::initThermoXML",
			 "Solvent " + solventName +
			 " should be first species");
    }

    /*
     * Now go get the specification of the standard states for
     * species in the solution. This includes the molar volumes
     * data blocks for incompressible species.
     */
    XML_Node& speciesList = phaseNode.child("speciesArray");
    XML_Node* speciesDB =
      get_XML_NameID("speciesData", speciesList["datasrc"],
		     &phaseNode.root());
    const vector<string>&sss = speciesNames();

    for (k = 0; k < m_kk; k++) {
      XML_Node* s =  speciesDB->findByAttr("name", sss[k]);
      if (!s) {
	throw CanteraError("HMWSoln::initThermoXML",
			 "Species Data Base " + sss[k] + " not found");
      }
      XML_Node *ss = s->findByName("standardState");
      if (!ss) {
	throw CanteraError("HMWSoln::initThermoXML",
			 "Species " + sss[k] + 
			   " standardState XML block  not found");
      }
      string modelStringa = ss->attrib("model");
      if (modelStringa == "") {
	throw CanteraError("HMWSoln::initThermoXML",
			   "Species " + sss[k] + 
			   " standardState XML block model attribute not found");
      }
      string modelString = lowercase(modelStringa);
      if (k == 0) {
	if (modelString == "wateriapws" || modelString == "real_water" ||
	    modelString == "waterpdss") {
	  /*
	   * Store a local pointer to the water standard state model.
	   *   -> We've hardcoded it to a PDSS_Water model, so this is ok.
	   */
	  m_waterSS = dynamic_cast<PDSS_Water *>(providePDSS(0)) ;
	  if (!m_waterSS) {
	    throw CanteraError("HMWSoln::initThermoXML",
			       "Dynamic cast to PDSS_Water failed");
	  }
	  /*
	   * Fill in the molar volume of water (m3/kmol)
	   * at standard conditions to fill in the m_speciesSize entry
	   * with something reasonable.
	   */
	  m_waterSS->setState_TP(300., OneAtm);
	  double dens = m_waterSS->density();
	  double mw = m_waterSS->molecularWeight();
	  m_speciesSize[0] = mw / dens;
#ifdef DEBUG_HKM_NOT
	  cout << "Solvent species " << sss[k] << " has volume " <<  
	    m_speciesSize[k] << endl;
#endif
	} else {
	  //  throw CanteraError("HMWSoln::initThermoXML",
	  //	     "Solvent SS Model \"" + modelStringa + 
	  //	     "\" is not allowed, name = " + sss[0]);
	  m_waterSS = providePDSS(0);
	  m_waterSS->setState_TP(300., OneAtm);
	  double dens = m_waterSS->density();
	  double mw = m_waterSS->molecularWeight();
	  m_speciesSize[0] = mw / dens;
	}
      } else {
	if (modelString != "constant_incompressible" && modelString != "hkft") {
	  throw CanteraError("HMWSoln::initThermoXML",
			     "Solute SS Model \"" + modelStringa + 
			     "\" is not known");
	}
	if (modelString ==  "constant_incompressible" ) {
	  m_speciesSize[k] = getFloat(*ss, "molarVolume", "toSI");
#ifdef DEBUG_HKM_NOT
	  cout << "species " << sss[k] << " has volume " <<  
	    m_speciesSize[k] << endl;
#endif
	}
	// HKM Note, have to fill up m_speciesSize[] for HKFT species
      }
    }

    /*
     * Initialize the water property calculator. It will share
     * the internal eos water calculator.
     */
    m_waterProps = new WaterProps(dynamic_cast<PDSS_Water*>(m_waterSS));

    /*
     * Fill in parameters for the calculation of the 
     * stoichiometric Ionic Strength
     *
     * The default is that stoich charge is the same as the
     * regular charge.
     */
    for (k = 0; k < m_kk; k++) {
      m_speciesCharge_Stoich[k] = m_speciesCharge[k];
    }

    /*
     * Go get all of the coefficients and factors in the
     * activityCoefficients XML block
     */
    XML_Node *acNodePtr = 0;
    if (thermoNode.hasChild("activityCoefficients")) {
      XML_Node& acNode = thermoNode.child("activityCoefficients");
      acNodePtr = &acNode;
      /*
       * Look for parameters for A_Debye
       */
      if (acNode.hasChild("A_Debye")) {
	XML_Node &ADebye = acNode.child("A_Debye");
	m_form_A_Debye = A_DEBYE_CONST;
	stemp = "model";
	if (ADebye.hasAttrib(stemp)) {
	  string atemp = ADebye.attrib(stemp);
	  stemp = lowercase(atemp);
	  if (stemp == "water") {
	    m_form_A_Debye = A_DEBYE_WATER;
	  }
	}
	if (m_form_A_Debye == A_DEBYE_CONST) {
	  m_A_Debye = getFloat(acNode, "A_Debye");
	}
#ifdef DEBUG_HKM_NOT
	cout << "A_Debye = " << m_A_Debye << endl;
#endif
      }

      /*
       * Look for Parameters for the Maximum Ionic Strength
       */
      if (acNode.hasChild("maxIonicStrength")) {
	m_maxIionicStrength = getFloat(acNode, "maxIonicStrength");
#ifdef DEBUG_HKM_NOT
	cout << "m_maxIionicStrength = "
	     <<m_maxIionicStrength << endl;
#endif
      }


      /*
       * Look for parameters for the Ionic radius
       */
      if (acNode.hasChild("ionicRadius")) {
	XML_Node& irNode = acNode.child("ionicRadius");

	string Aunits = "";
	double Afactor = 1.0;
	if (irNode.hasAttrib("units")) {
	  string Aunits = irNode.attrib("units");
	  Afactor = toSI(Aunits); 
	}

	if (irNode.hasAttrib("default")) {
	  string ads = irNode.attrib("default");
	  double ad = fpValue(ads);
	  for (int k = 0; k < m_kk; k++) {
	    m_Aionic[k] = ad * Afactor;
	  }
	}

      }

      /*
       * First look at the species database.
       *  -> Look for the subelement "stoichIsMods"
       *     in each of the species SS databases.
       */
      std::vector<const XML_Node *> xspecies = speciesData();
   
      string kname, jname;
      int jj = xspecies.size();
      for (k = 0; k < m_kk; k++) {
	int jmap = -1;
	kname = speciesName(k);
	for (int j = 0; j < jj; j++) {
	  const XML_Node& sp = *xspecies[j];
	  jname = sp["name"];
	  if (jname == kname) {
	    jmap = j;
	    break;
	  }
	}
	if (jmap > -1) {
	  const XML_Node& sp = *xspecies[jmap];
	  getOptionalFloat(sp, "stoichIsMods",  m_speciesCharge_Stoich[k]);
	  // if (sp.hasChild("stoichIsMods")) {
	  // double val = getFloat(sp, "stoichIsMods");
	  //m_speciesCharge_Stoich[k] = val;
	  //}
	}
      }
   
      /*
       * Now look at the activity coefficient database
       */
      if (acNodePtr) {
	if (acNodePtr->hasChild("stoichIsMods")) {
	  XML_Node& sIsNode = acNodePtr->child("stoichIsMods");

	  map<string, string> msIs;
	  getMap(sIsNode, msIs);
	  map<string,string>::const_iterator _b = msIs.begin();
	  for (; _b != msIs.end(); ++_b) {
	    int kk = speciesIndex(_b->first);
	    if (kk < 0) {
	      //throw CanteraError(
	      //   "HMWSoln::initThermo error",
	      //   "no species match was found"
	      //   );
	    } else {
	      double val = fpValue(_b->second);
	      m_speciesCharge_Stoich[kk] = val;
	    }
	  }
	}
      }
      
    
      /*
       * Loop through the children getting multiple instances of
       * parameters
       */
      if (acNodePtr) {
	int n = acNodePtr->nChildren();
	for (int i = 0; i < n; i++) {
	  XML_Node &xmlACChild = acNodePtr->child(i);
	  stemp = xmlACChild.name();
	  string nodeName = lowercase(stemp);
	  /*
	   * Process a binary salt field, or any of the other XML fields
	   * that make up the Pitzer Database. Entries will be ignored
	   * if any of the species in the entry isn't in the solution.
	   */
	  if (nodeName == "binarysaltparameters") {
	    readXMLBinarySalt(xmlACChild);
	  } else if (nodeName == "thetaanion") {
	    readXMLThetaAnion(xmlACChild);
	  } else if (nodeName == "thetacation") {
	    readXMLThetaCation(xmlACChild);
	  } else if (nodeName == "psicommonanion") { 
	    readXMLPsiCommonAnion(xmlACChild);
	  } else if (nodeName == "psicommoncation") { 
	    readXMLPsiCommonCation(xmlACChild);
	  } else if (nodeName == "lambdaneutral") {
	    readXMLLambdaNeutral(xmlACChild);
	  } else if (nodeName == "zetacation") {
	    readXMLZetaCation(xmlACChild);
	  }
	}
      }

      // Go look up the optional Cropping parameters
      readXMLCroppingCoefficients(acNode);

    }

    /*
     * Fill in the vector specifying the electrolyte species
     * type
     *
     *   First fill in default values. Everthing is either
     *   a charge species, a nonpolar neutral, or the solvent.
     */
    for (k = 0; k < m_kk; k++) {
      if (fabs(m_speciesCharge[k]) > 0.0001) {
	m_electrolyteSpeciesType[k] = cEST_chargedSpecies;
	if (fabs(m_speciesCharge_Stoich[k] - m_speciesCharge[k])
	    > 0.0001) {
	  m_electrolyteSpeciesType[k] = cEST_weakAcidAssociated;
	}
      } else if (fabs(m_speciesCharge_Stoich[k]) > 0.0001) {
	m_electrolyteSpeciesType[k] = cEST_weakAcidAssociated;
      } else {
	m_electrolyteSpeciesType[k] = cEST_nonpolarNeutral;
      }
    }
    m_electrolyteSpeciesType[m_indexSolvent] = cEST_solvent;
    /*
     * First look at the species database.
     *  -> Look for the subelement "stoichIsMods"
     *     in each of the species SS databases.
     */
    std::vector<const XML_Node *> xspecies = speciesData();
    const XML_Node *spPtr = 0;
    string kname;
    for (k = 0; k < m_kk; k++) {
      kname = speciesName(k);
      spPtr = xspecies[k];
      if (!spPtr) {
	if (spPtr->hasChild("electrolyteSpeciesType")) {
	  string est = getChildValue(*spPtr, "electrolyteSpeciesType");
	  if ((m_electrolyteSpeciesType[k] = interp_est(est)) == -1) {
	    throw CanteraError("HMWSoln::initThermoXML",
			       "Bad electrolyte type: " + est);
	  }
	}
      } 
    }
    /*
     * Then look at the phase thermo specification
     */
    if (acNodePtr) {
      if (acNodePtr->hasChild("electrolyteSpeciesType")) {
	XML_Node& ESTNode = acNodePtr->child("electrolyteSpeciesType");
	map<string, string> msEST;
	getMap(ESTNode, msEST);
	map<string,string>::const_iterator _b = msEST.begin();
	for (; _b != msEST.end(); ++_b) {
	  int kk = speciesIndex(_b->first);
	  if (kk < 0) {
	  } else {
	    string est = _b->second;
	    if ((m_electrolyteSpeciesType[kk] = interp_est(est))  == -1) {
	      throw CanteraError("HMWSoln::initThermoXML",
				 "Bad electrolyte type: " + est);
	    }
	  }
	}
      }
    }

    IMS_typeCutoff_ = 2;
    if (IMS_typeCutoff_ == 2) {
      calcIMSCutoffParams_();
    }
    calcMCCutoffParams_();
    setMoleFSolventMin(1.0E-5);

    MolalityVPSSTP::initThermoXML(phaseNode, id);
    /*
     * Lastly set the state
     */
    //    if (phaseNode.hasChild("state")) {
    // XML_Node& stateNode = phaseNode.child("state");
    // setStateFromXML(stateNode);
    //}

  }

  // Precalculate the IMS Cutoff parameters for typeCutoff = 2
  void  HMWSoln::calcIMSCutoffParams_() {
    IMS_afCut_ = 1.0 / (std::exp(1.0) *  IMS_gamma_k_min_);
    IMS_efCut_ = 0.0;
    bool converged = false;
    double oldV = 0.0;
    int its;
    for (its = 0; its < 100 && !converged; its++) {
      oldV = IMS_efCut_;
      IMS_afCut_ = 1.0 / (std::exp(1.0) * IMS_gamma_k_min_)  -IMS_efCut_;
      IMS_bfCut_ = IMS_afCut_ / IMS_cCut_ + IMS_slopefCut_ - 1.0;
      IMS_dfCut_ = ((- IMS_afCut_/IMS_cCut_ + IMS_bfCut_ - IMS_bfCut_*IMS_X_o_cutoff_/IMS_cCut_)
                /
                (IMS_X_o_cutoff_*IMS_X_o_cutoff_/IMS_cCut_ - 2.0 * IMS_X_o_cutoff_));
      double tmp = IMS_afCut_ + IMS_X_o_cutoff_*( IMS_bfCut_ + IMS_dfCut_ *IMS_X_o_cutoff_);
      double eterm = std::exp(-IMS_X_o_cutoff_/IMS_cCut_);
      IMS_efCut_ = - eterm * (tmp);
      if (fabs(IMS_efCut_ - oldV) < 1.0E-14) {
        converged = true;
      }
    }
    if (!converged) {
      throw CanteraError("HMWSoln::calcIMSCutoffParams_()",
                         " failed to converge on the f polynomial");
    }
    converged = false;
    double f_0 = IMS_afCut_ + IMS_efCut_;
    double f_prime_0 = 1.0 - IMS_afCut_ / IMS_cCut_ + IMS_bfCut_;
    IMS_egCut_ = 0.0;
    for (its = 0; its < 100 && !converged; its++) {
      oldV = IMS_egCut_;
      double lng_0 = -log(IMS_gamma_o_min_) -  f_prime_0 / f_0;
      IMS_agCut_ = exp(lng_0) - IMS_egCut_;
      IMS_bgCut_ = IMS_agCut_ / IMS_cCut_ + IMS_slopegCut_ - 1.0;
      IMS_dgCut_ = ((- IMS_agCut_/IMS_cCut_ + IMS_bgCut_ - IMS_bgCut_*IMS_X_o_cutoff_/IMS_cCut_)
                /
                (IMS_X_o_cutoff_*IMS_X_o_cutoff_/IMS_cCut_ - 2.0 * IMS_X_o_cutoff_));
      double tmp = IMS_agCut_ + IMS_X_o_cutoff_*( IMS_bgCut_ + IMS_dgCut_ *IMS_X_o_cutoff_);
      double eterm = std::exp(-IMS_X_o_cutoff_/IMS_cCut_);
      IMS_egCut_ = - eterm * (tmp);
      if (fabs(IMS_egCut_ - oldV) < 1.0E-14) {
        converged = true;
      }
    }
    if (!converged) {
      throw CanteraError("HMWSoln::calcIMSCutoffParams_()",
                         " failed to converge on the g polynomial");
    }
  }

  // Precalculate the MC Cutoff parameters
  void  HMWSoln::calcMCCutoffParams_() {
    MC_X_o_min_ = 0.35;
    MC_X_o_cutoff_ = 0.6;
    MC_slopepCut_ = 0.02;
    MC_cpCut_ = 0.25;

    // Initial starting values
    MC_apCut_ = MC_X_o_min_;
    MC_epCut_ = 0.0;
    bool converged = false;
    double oldV = 0.0;
    int its;
    double damp = 0.5;
    for (its = 0; its < 500 && !converged; its++) {
      oldV = MC_epCut_;
      MC_apCut_ = damp *(MC_X_o_min_ - MC_epCut_) + (1-damp) * MC_apCut_;
      double MC_bpCutNew = MC_apCut_ / MC_cpCut_ + MC_slopepCut_ - 1.0;
      MC_bpCut_ = damp * MC_bpCutNew + (1-damp) * MC_bpCut_;
      double MC_dpCutNew = ((- MC_apCut_/MC_cpCut_ + MC_bpCut_ - MC_bpCut_ * MC_X_o_cutoff_/MC_cpCut_)
		   /
		   (MC_X_o_cutoff_ * MC_X_o_cutoff_/MC_cpCut_ - 2.0 * MC_X_o_cutoff_));
      MC_dpCut_ = damp * MC_dpCutNew + (1-damp) * MC_dpCut_;
      double tmp = MC_apCut_ + MC_X_o_cutoff_*( MC_bpCut_ + MC_dpCut_ * MC_X_o_cutoff_);
      double eterm = std::exp(- MC_X_o_cutoff_ / MC_cpCut_);
      MC_epCut_ = - eterm * (tmp);
      double diff = MC_epCut_ - oldV;
      if (fabs(diff) < 1.0E-14) {
        converged = true;
      }
    }
    if (!converged) {
      throw CanteraError("HMWSoln::calcMCCutoffParams_()",
                         " failed to converge on the p polynomial");
    }
  }

}
