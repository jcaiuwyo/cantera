/**
 *  @file ReactorBase.h
 */

/*
 * $Author$
 * $Revision$
 * $Date$
 */

// Copyright 2001  California Institute of Technology

#ifndef CT_REACTORBASE_H
#define CT_REACTORBASE_H

#ifdef WIN32
#pragma warning(disable:4786)
#pragma warning(disable:4503)
#endif

#include "ThermoPhase.h"

/// Namespace for classes implementing zero-dimensional reactor networks. 
namespace CanteraZeroD {

//    typedef Thermo   thermo_t;

    class FlowDevice;
    class Wall;

    const int ReservoirType = 1;
    const int ReactorType = 2;
    const int FlowReactorType = 3;
    const int ConstPressureReactorType = 4;

    /**
     * Base class for stirred reactors. 
     * Allows using any substance model, with arbitrary 
     * inflow, outflow, heat loss/gain, surface chemistry, and
     * volume change. 
     */ 
    class ReactorBase {

    public:

        ReactorBase(std::string name = "(none)");
        virtual ~ReactorBase(){}

        //-----------------------------------------------------

        virtual int type() const { return 0; }
        std::string name() const { return m_name; }
        void setName(std::string name) { m_name = name; }

        /** @name Methods to set up a simulation. */
        //@{


        /** 
         * Set the initial reactor volume. By default, the volume is
         * 1.0 m^3.
         */ 
        void setInitialVolume(doublereal vol) { 
            m_vol = vol; 
            m_vol0 = vol; 
        }

        /**
         * Set initial time. Default = 0.0 s. Restarts integration
         * from this time using the current mixture state as the
         * initial condition.
         */
        void setInitialTime(doublereal time) {
            m_time = time;
            m_init = false;
        }

        /**
         * Specify the mixture contained in the reactor. Note that
         * a pointer to this substance is stored, and as the integration
         * proceeds, the state of the substance is modified.
         */
        void setThermoMgr(Cantera::thermo_t& thermo);

        void addInlet(FlowDevice& inlet);
        void addOutlet(FlowDevice& outlet);
        FlowDevice& inlet(int n = 0);
        FlowDevice& outlet(int n = 0);

        int nInlets() { return m_inlet.size(); }
        int nOutlets() { return m_outlet.size(); }
        int nWalls() { return m_wall.size(); }

        void addWall(Wall& w, int lr);
        Wall& wall(int n);

        /**
         * Initialize the reactor. Must be called after specifying the
         *  (and if necessary the inlet mixture) and before
         * calling advance.
         */
        virtual void initialize(doublereal t0 = 0.0) { tilt(); }

        /**
         * Advance the state of the reactor in time.
         * @param time Time to advance to (s). 
         * Note that this method
         * changes the state of the mixture object.
         */
        virtual void advance(doublereal time) { tilt(); }
        virtual double step(doublereal time) { tilt(); return 0.0; }
        virtual void start() {}

        //@}

        void resetState();

        /// return a reference to the contents.
        Cantera::thermo_t& contents() { return *m_thermo; }

        const Cantera::thermo_t& contents() const { return *m_thermo; }

        doublereal residenceTime();


        /** 
         * @name Solution components. 
         * The values returned are those after the last call to advance 
         * or step. 
         */
        //@{

        /// the current time (s).
        doublereal time() const { return m_time; }


      //! Returns the current volume of the reactor
      /*!
       * @return  Return the volume in m**3
       */
      doublereal volume() const { 
	return m_vol; 
      }
        doublereal density() const { return m_state[1]; }
        doublereal temperature() const { return m_state[0]; }
        doublereal enthalpy_mass() const { return m_enthalpy; }
        doublereal intEnergy_mass() const { return m_intEnergy; }
        doublereal pressure() const { return m_pressure; }
        doublereal mass() const { return m_vol * density(); }
        const doublereal* massFractions() const { return DATA_PTR(m_state) + 2; }
        doublereal massFraction(int k) const { return m_state[k+2]; }

        //@}

        int error(std::string msg) const {
            Cantera::writelog("Error: "+msg);
            return 1;
        }

    protected:
 
      //! Number of homogeneous species in the mixture
      int m_nsp;

        Cantera::thermo_t*  m_thermo;
        doublereal m_time;
        doublereal m_vol, m_vol0;
        bool m_init;
        int m_nInlets, m_nOutlets;
        bool m_open;
        doublereal m_enthalpy;
        doublereal m_intEnergy;
        doublereal m_pressure;
        Cantera::vector_fp m_state;
        std::vector<FlowDevice*> m_inlet, m_outlet;
        std::vector<Wall*> m_wall;
        Cantera::vector_int m_lr;
        int m_nwalls;
        std::string m_name;
        double m_rho0;

    private:

        void tilt(std::string method="") const { 
        throw Cantera::CanteraError("ReactorBase::"+method,
            "ReactorBase method called!"); }
    };
}

#endif

