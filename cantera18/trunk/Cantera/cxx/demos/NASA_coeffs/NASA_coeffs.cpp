
#include <cantera/Cantera.h>
#include <cantera/IdealGasMix.h>    // defines class IdealGasMix

#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <numeric>
#include <string>
#include <algorithm>

using namespace std; 
using namespace Cantera;


// The program is put into a function so that error handling code can
// be conveniently put around the whole thing. See main() below.

void demoprog() {

    printf("\n\n**** Testing modifying NASA polynomial coefficients ****\n\n");

    IdealGasMix gas("h2o2.cti","ohmech");
    int nsp = gas.nSpecies();

    int type;
    doublereal c[15];
    doublereal minTemp, maxTemp, refPressure;

    // get a reference to the species thermo property manager
    SpeciesThermo& sp = gas.speciesThermo();

    int n, j;

    // these constants define the location of coefficient "a6" in the
    // cofficient array c. The c array contains Tmid in the first
    // location, followed by the 7 low-temperature coefficients, then
    // the seven high-temperature ones.
    const int LOW_A6 = 6;
    //const int HIGH_A6 = 13;

    for (n = 0; n < nsp; n++) {

        printf("\n\n %s (original):", gas.speciesName(n).c_str());

        // get the NASA coefficients in array c
        sp.reportParams(n, type, c, minTemp, maxTemp, refPressure);

        // print the unmodified NASA coefficients
        printf("\n      ");
        for (j = 1; j < 8; j++) printf("     A%d     ", j);
        printf("\n  low:");
        for (j = 1; j < 8; j++) printf(" %10.4E ", c[j]);

        printf("\n high:");
        for (j = 8; j < 15; j++) printf(" %10.4E ", c[j]);
        printf("\n      ");

        // modify coefficient A6 of the low-temperature polynomial
        // Note that since we are not modifying the high-temperature
        // polynomial, a warning will be printed about a discontinuity
        // in enthalpy at Tmid.
        c[LOW_A6] += 1.0e4;
        sp.modifyParams(n, c);

        sp.reportParams(n, type, c, minTemp, maxTemp, refPressure);

        // print the modified NASA coefficients
        printf("\n\n %s (modified):", gas.speciesName(n).c_str());
        printf("\n      ");
        for (j = 1; j < 8; j++) printf("     A%d     ", j);
        printf("\n  low:");
        for (j = 1; j < 8; j++) printf(" %10.4E ", c[j]);

        printf("\n high:");
        for (j = 8; j < 15; j++) printf(" %10.4E ", c[j]);
        printf("\n      ");
    }
}
     

 
int main() {

    try {
        demoprog();
    }
    catch (CanteraError) {
        showErrors(cout);
    }
}

