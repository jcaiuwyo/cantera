"""A counterflow flame."""

from onedim import *
from Cantera.num import zeros
import math

def erfc(x):
    """The complementary error function."""
    exp = math.exp

    p  =  0.3275911
    a1 =  0.254829592
    a2 = -0.284496736
    a3 =  1.421413741
    a4 = -1.453152027
    a5 =  1.061405429

    t = 1.0 / (1.0 + p*x)
    erfcx = ( (a1 + (a2 + (a3 +
                          (a4 + a5*t)*t)*t)*t)*t ) * exp(-x*x)
    return erfcx

def erf(x):
    """The error function."""
    if x < 0:
        return -(1.0 - erfc(-x))
    else:
        return 1.0 - erfc(x)



class CounterFlame(Stack):
    """A non-premixed counterflow flame."""

    def __init__(self, gas = None, grid = None):
        """
        The domains are::

            [self.fuel_inlet,      # class Inlet,
             self.flame,           # class AxisymmetricFlow,
             self.oxidizer_inlet]  # class Inlet
        """

        self.fuel_inlet = Inlet('fuel inlet')
        self.oxidizer_inlet = Inlet('oxidizer inlet')
        self.gas = gas
        self.fuel_inlet.set(temperature = gas.temperature())
        self.oxidizer_inlet.set(temperature = gas.temperature())
        self.pressure = gas.pressure()
        self.flame = AxisymmetricFlow('flame',gas = gas)
        self.flame.setupGrid(grid)
        Stack.__init__(self, [self.fuel_inlet, self.flame,
                              self.oxidizer_inlet])
        self.setRefineCriteria()


    def init(self, fuel = '', oxidizer = 'O2', stoich = -1.0):
        """Set the initial guess for the solution. The fuel species
        must be specified, and the oxidizer may be

        >>> f.init(fuel='CH4')

        The initial guess is generated by assuming infinitely-fast
        chemistry."""
        self.getInitialSoln()
        gas = self.gas
        nsp = gas.nSpecies()
        wt = gas.molecularWeights()

        # find the fuel and oxidizer species
        iox = gas.speciesIndex(oxidizer)
        ifuel = gas.speciesIndex(fuel)

        # if no stoichiometric ratio was input, compute it
        if stoich < 0.0:
            if oxidizer == 'O2':
                nh = gas.nAtoms(fuel, 'H')
                nc = gas.nAtoms(fuel, 'C')
                stoich = 1.0*nc + 0.25*nh
            else:
                raise CanteraError('oxidizer/fuel stoichiometric ratio must'+
                            ' be specified, since  the oxidizer is not O2')

        s = stoich*wt[iox]/wt[ifuel]
        y0f = self.fuel_inlet.massFraction(ifuel)
        y0ox = self.oxidizer_inlet.massFraction(iox)
        phi = s*y0f/y0ox
        zst = 1.0/(1.0 + phi)

        yin_f = zeros(nsp, 'd')
        yin_o = zeros(nsp, 'd')
        yst = zeros(nsp, 'd')
        for k in range(nsp):
            yin_f[k] = self.fuel_inlet.massFraction(k)
            yin_o[k] = self.oxidizer_inlet.massFraction(k)
            yst[k] = zst*yin_f[k] + (1.0 - zst)*yin_o[k]

        gas.setState_TPY(self.fuel_inlet.temperature(), self.pressure, yin_f)
        mdotf = self.fuel_inlet.mdot()
        u0f = mdotf/gas.density()
        t0f = self.fuel_inlet.temperature()

        gas.setState_TPY(self.oxidizer_inlet.temperature(),
                         self.pressure, yin_o)
        mdoto = self.oxidizer_inlet.mdot()
        u0o = mdoto/gas.density()
        t0o = self.oxidizer_inlet.temperature()

        # get adiabatic flame temperature and composition
        tbar = 0.5*(t0o + t0f)
        gas.setState_TPY(tbar, self.pressure, yst)
        gas.equilibrate('HP')
        teq = gas.temperature()
        yeq = gas.massFractions()

        # estimate strain rate
        zz = self.flame.grid()
        dz = zz[-1] - zz[0]
        a = (u0o + u0f)/dz
        diff = gas.mixDiffCoeffs()
        f = math.sqrt(a/(2.0*diff[iox]))

        x0 = mdotf*dz/(mdotf + mdoto)
        nz = len(zz)

        y = zeros([nz,nsp],'d')
        t = zeros(nz,'d')
        for j in range(nz):
            x = zz[j]
            zeta = f*(x - x0)
            zmix = 0.5*(1.0 - erf(zeta))
            if zmix > zst:
                for k in range(nsp):
                    y[j,k] = yeq[k] + (zmix - zst)*(yin_f[k]
                                                    - yeq[k])/(1.0 - zst)
                t[j] = teq + (t0f - teq)*(zmix - zst)/(1.0 - zst)
            else:
                for k in range(nsp):
                    y[j,k] = yin_o[k] + zmix*(yeq[k] - yin_o[k])/zst
                t[j] = t0o + (teq - t0o)*zmix/zst

        t[0] = t0f
        t[-1] = t0o
        zrel = zz/dz
        self.setProfile('u', [0.0, 1.0], [u0f, -u0o])
        self.setProfile('V', [0.0, x0/dz, 1.0], [0.0, a, 0.0])
        self.setProfile('T', zrel, t)
        for k in range(nsp):
            self.setProfile(gas.speciesName(k), zrel, y[:,k])

        self._initialized = 1


    def solve(self, loglevel = 1, refine_grid = 1):
        if not self._initialized: self.init()
        Stack.solve(self, loglevel = loglevel, refine_grid = refine_grid)


    def setRefineCriteria(self, ratio = 10.0, slope = 0.8, curve = 0.8,
                          prune = 0.0):
        Stack.setRefineCriteria(self, domain = self.flame,
                                ratio = ratio, slope = slope, curve = curve,
                                prune = prune)

    def setGridMin(self, gridmin):
        Stack.setGridMin(self, self.flame, gridmin)

    def setProfile(self, component, locs, vals):
        self._initialized = 1
        Stack.setProfile(self, self.flame, component, locs, vals)

    def set(self, tol = None, energy = '', tol_time = None):
        """Set parameters.

        :param tol:
            (rtol, atol) for steady-state
        :param tol_time:
            (rtol, atol) for time stepping
        :param energy:
            'on' or 'off' to enable or disable the energy equation
        """
        if tol:
            self.flame.setTolerances(default = tol)
        if tol_time:
            self.flame.setTolerances(default = tol_time, time = 1)
        if energy:
            self.flame.set(energy = energy)

    def T(self, point = -1):
        """The temperature [K]"""
        return self.solution('T', point)

    def u(self, point = -1):
        """The axial velocity [m/s]"""
        return self.solution('u', point)

    def V(self, point = -1):
        """The radial velocity divided by radius [s^-1]"""
        return self.solution('V', point)

    def solution(self, component = '', point = -1):
        """The solution for one specified component. If a point number
        is given, return the value of component 'component' at this
        point. Otherwise, return the entire profile for this
        component."""
        if point >= 0: return self.value(self.flame, component, point)
        else: return self.profile(self.flame, component)

    def setGasState(self, j):
        """Set the state of the object representing the gas to the
        current solution at grid point j."""
        nsp = self.gas.nSpecies()
        y = zeros(nsp, 'd')
        for n in range(nsp):
            nm = self.gas.speciesName(n)
            y[n] = self.solution(nm, j)
        self.gas.setState_TPY(self.T(j), self.pressure, y)

fix_docs(CounterFlame)
