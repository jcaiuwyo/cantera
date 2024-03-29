// An open Rankine cycle

#include <string>
#include <map>

#include <cantera/Cantera.h>
#include <cantera/PureFluid.h>    // defines class Water

using namespace Cantera;
using namespace std;

map<string,double> h;
map<string,double> s;
map<string,double> T;
map<string,double> P;
map<string,double> x;
vector<string> states;

template<class F>
void saveState(F& fluid, string name) {
    h[name] = fluid.enthalpy_mass();
    s[name] = fluid.entropy_mass();
    T[name] = fluid.temperature();
    P[name] = fluid.pressure();
    x[name] = fluid.vaporFraction();
    states.push_back(name);
}

void printStates() {
    string name;
    int n;
    int nStates = states.size();
    for (n = 0; n < nStates; n++) {
        name = states[n];
        printf(" %5s %10.6g %10.6g  %12.6g %12.6g %5.2g \n", 
            name.c_str(), T[name], P[name], h[name], s[name], x[name]);
    }
}

int openRankine(int np, void* p) {

    double etap = 0.6;     // pump isentropic efficiency
    double etat = 0.8;     // turbine isentropic efficiency
    double phigh = 8.0e5;  // high pressure

    Water w;

    // begin with water at 300 K, 1 atm
    w.setState_TP(300.0, OneAtm);
    saveState(w,"1");

    // pump water to 0.8 MPa
    w.setState_SP(s["1"], phigh);
    saveState(w,"2s");
    double h2 = (h["2s"] - h["1"])/etap + h["1"];
    w.setState_HP(h2, phigh);
    saveState(w,"2");

    // heat to saturated vapor
    w.setState_Psat(phigh, 1.0);
    saveState(w,"3");

    // expand to 1 atm
    w.setState_SP(s["3"], OneAtm);
    saveState(w,"4s");
    double work_s = h["3"] - h["4s"];
    double work = etat*work_s;
    w.setState_HP(h["3"] - work, OneAtm);
    saveState(w,"4");

    printStates();

    double heat_in = h["3"] - h["2"];
    double efficiency = work/heat_in;

    cout << "efficiency = " << efficiency << endl;
#ifdef WIN32
#ifndef CXX_DEMO
    cout << "press any key to end" << endl;
    char ch;
    cin >> ch;
#endif
#endif
    return 0;
}


#ifndef CXX_DEMO
int main() {
    try {
        return openRankine(0, 0);
    }
    catch (CanteraError) {
        showErrors(cout);
        return -1;
    }
}
#endif

