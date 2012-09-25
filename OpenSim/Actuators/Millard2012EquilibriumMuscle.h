#ifndef OPENSIM_Millard2012EquilibriumMuscle_h__
#define OPENSIM_Millard2012EquilibriumMuscle_h__
/* -------------------------------------------------------------------------- *
 *                  OpenSim:  Millard2012EquilibriumMuscle.h                  *
 * -------------------------------------------------------------------------- *
 * The OpenSim API is a toolkit for musculoskeletal modeling and simulation.  *
 * See http://opensim.stanford.edu and the NOTICE file for more information.  *
 * OpenSim is developed at Stanford University and supported by the US        *
 * National Institutes of Health (U54 GM072970, R24 HD065690) and by DARPA    *
 * through the Warrior Web program.                                           *
 *                                                                            *
 * Copyright (c) 2005-2012 Stanford University and the Authors                *
 * Author(s): Matthew Millard                                                 *
 *                                                                            *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may    *
 * not use this file except in compliance with the License. You may obtain a  *
 * copy of the License at http://www.apache.org/licenses/LICENSE-2.0.         *
 *                                                                            *
 * Unless required by applicable law or agreed to in writing, software        *
 * distributed under the License is distributed on an "AS IS" BASIS,          *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   *
 * See the License for the specific language governing permissions and        *
 * limitations under the License.                                             *
 * -------------------------------------------------------------------------- */


// INCLUDE
#include <OpenSim/Actuators/osimActuatorsDLL.h>

#include <simbody/internal/common.h>
#include <OpenSim/Simulation/Model/Muscle.h>
/*
The parent class, Muscle.h, provides 
    1. max_isometric_force
    2. optimal_fiber_length
    3. tendon_slack_length
    4. pennation_angle_at_optimal
    5. max_contraction_velocity
*/
#include <OpenSim/Simulation/Model/Muscle.h>

//All of the sub-models required for a muscle model:
#include <OpenSim/Actuators/MuscleFirstOrderActivationDynamicModel.h>
#include <OpenSim/Actuators/MuscleFixedWidthPennationModel.h>

#include <OpenSim/Actuators/ActiveForceLengthCurve.h>
#include <OpenSim/Actuators/ForceVelocityCurve.h>
#include <OpenSim/Actuators/ForceVelocityInverseCurve.h>
#include <OpenSim/Actuators/FiberForceLengthCurve.h>
//#include <OpenSim/Actuators/FiberCompressiveForceLengthCurve.h>
//#include <OpenSim/Actuators/FiberCompressiveForceCosPennationCurve.h>
#include <OpenSim/Actuators/TendonForceLengthCurve.h>


#ifdef SWIG
    #ifdef OSIMACTUATORS_API
        #undef OSIMACTUATORS_API
        #define OSIMACTUATORS_API
    #endif
#endif

namespace OpenSim {


/**
This class implements a configurable equilibrium muscle model. An equilibrium 
model assumes that the force generated by the tendon and the force generated by 
the fiber are assumed to be equal and opposite:

\f[
 f_{ISO}\Big(\mathbf{a}(t) \mathbf{f}_L(\hat{l}_{CE}) \mathbf{f}_V(\hat{v}_{CE}) 
+ \mathbf{f}_{PE}(\hat{l}_{CE}) )\Big) \cos \phi
-  f_{ISO}\mathbf{f}_{SE}(\hat{l}_{T}) = 0
\f]

\image html fig_Millard2012EquilibriumMuscle.png

This model can be simulated in a number of different ways by setting using the
following configuration options:

<B>Tendon Configuration</B>

\li ignore_tendon_compliance: when set to true makes the tendon rigid. This 
assumption is usually reasonable for short tendons, and results in a simulation
speedup because fiber length is no longer a state vector. 

<B>Fiber Configuration</B>

\li use_reduced_fiber_dynamics: when set to true, and when the tendon is 
elastic (ie ignore_tendon_compliance = false) the state of the fiber is
estimated by solving a reduced equilibrium equation rather than integrating
fiber state. The equation reduction is achieved by assuming that fiber length
acceleration is zero. Simulation speed ups of between 2.5-20 are possible when 
compared to an elastic tendon simulation with fiber length as a state. Note 
that a reduced muscle with an elastic tendon simulations as fast, or marginally 
faster than a rigid tendon muscle.

\li ignore_activation_dynamics: when set to true, excitation is treated as 
activation. This results in faster simulation times because the state vector
is smaller.

<B>Elastic Tendon, Full Fiber Dynamics,Full Activation Dynamics</B>

The most typical configuration used in the literature is to simulate a muscle
with an elastic tendon, full fiber dynamics, and activation dynamics. The 
resulting formulation suffers from several singularities:
\f$\mathbf{a}(t) \rightarrow 0\f$,\f$\phi \rightarrow 90^\circ\f$, and 
\f$ \mathbf{f}_L(\hat{l}_{CE}) \rightarrow 0 \f$. These singularities are all
mananaged carefully in this model to ensure that this model does not produce
singularities, nor intolerably long simulation times.

These singularities arise from the manner in which the equilibrium equation
is rearranged to yield an ordinary differential equation (ODE). The above 
equation is rearranged to  isolate for \f$ \mathbf{f}_V(\hat{v}_{CE}) \f$, 
which is inverted to solve for \f$ \hat{v}_{CE} \f$,  and then is numerically 
integrated during simulation:

\f[
 \hat{v}_{CE} = \mathbf{f}_V ^{-1} (
 \frac{ ( \mathbf{f}_{SE}(\hat{l}_{T}) ) / 
 \cos \phi
  -  \mathbf{f}_{PE}(\hat{l}_{CE}) }
  { \mathbf{a}(t) \mathbf{f}_L(\hat{l}_{CE})} )
\f]


The above equation becomes numerical stiff when terms in the denominator 
approach zero (when \f$\mathbf{a}(t) \rightarrow 0\f$,\f$\phi 
\rightarrow 90^\circ\f$, or \f$ \mathbf{f}_L(\hat{l}_{CE}) \rightarrow 0 \f$) 
or additionally when the slope of \f$\mathbf{f}_V ^{-1}\f$ is steep (which
occurs at fiber velocities close to the maximum concentric and maximum 
eccentric fiber velocities).

Physically singularity management means that this muscle model is always 
activated (\f$\mathbf{a}(t) > 0\f$), the fiber will stop contracting when a 
pennation angle of 90 degrees is approached (\f$\phi < 90^\circ\f$), and the 
fiber will also stop contracting as its length approaches a lower bound 
(\f$ \hat{l}_{CE} > lowerbound\f$), typically around half the fiber's resting 
length ( to ensure \f$ \mathbf{f}_L(\hat{l}_{CE}) > 0 \f$). The fiber length is
 prevented from achieving either an unphysiological length, or its maximum
 pennation angle, through the use of a unilateral constraint. Additionally, the
 force velocity curve is modified so that it is invertible.

 <B>(Rigid Tendon) and (Elastic Tendon, Reduced Fiber Dynamics)</B>

 Neither of these formulations have any singularities. This allows the lower 
 bound of the active force length curve to be 0 
 (min( \f$ \mathbf{f}_L(\hat{l}_{CE})) = 0 \f$), activation can go to 0, the
 pennation angle can come much closer to \f$90^\circ\f$, and the force velocity
 curve need not be invertible.

 Physically this means that the muscle can be turned off, its fibers can 
 approach high pennation angles, and its characteristic curves do not need
 to be modified (active force length can go to 0, and the force velocity
 curve does not have to be invertible).

 The rigid tendon formulation removes these singularities by ignoring the 
 elasticity of the tendon. This assumption is reasonable for many muscles, but
 is up to the musculoskeletal model designer and user to determine when this 
 assumption can be made. 
 
 The elastic tendon with reduced fiber dynamics removes 
 singularities by assuming that fiber acceleration is 0. This model will produce
 forces that are very similar to an elastic tendon muscle with full fiber 
 dynamics, but will overestimate or underestimate the forces produced by the 
 fiber by ~5% when the fiber length has a large, and non-zero fiber 
 acceleration. Note that this method is still being tested, and so, its 
 accuracy relative to the full fiber dynamic model is not known in all 
 simulations. Additionally note that to use the elastic tendon with reduced 
 fiber dynamics that the ignore_tendon_compliance flag must be set to false.


For more information please see the
doxygen for the properties that are objects themselves ( such as 
MuscleFirstOrderActivationDynamicModel, MuscleFixedWidthPennationModel).


@author Matt Millard
*/
class OSIMACTUATORS_API Millard2012EquilibriumMuscle : public Muscle {
OpenSim_DECLARE_CONCRETE_OBJECT(Millard2012EquilibriumMuscle, Muscle);

//class OSIMACTUATORS_API Millard2012EquilibriumMuscle : public Muscle {

public:
//==============================================================================
// PROPERTIES
//==============================================================================
    /** @name Property declarations 
    These are the serializable properties associated with this class. **/
    /**@{**/

    OpenSim_DECLARE_PROPERTY( use_reduced_fiber_dynamics, bool, 
        "Use reduced equations to compute the fiber"
        " state for an elastic tendon muscle.");


    OpenSim_DECLARE_PROPERTY( default_activation, double,
                       "assumed initial activation level if none is assigned.");

    OpenSim_DECLARE_PROPERTY( default_fiber_length, double,
                       "assumed initial fiber length if none is assigned.");
    

    OpenSim_DECLARE_UNNAMED_PROPERTY( 
                                MuscleFirstOrderActivationDynamicModel,
                                "activation dynamics model with a lower bound");   
    
    OpenSim_DECLARE_UNNAMED_PROPERTY(
                                ActiveForceLengthCurve,
                                "active force length curve");

    OpenSim_DECLARE_UNNAMED_PROPERTY(
                                ForceVelocityInverseCurve,
                                "force velocity inverse curve");

    OpenSim_DECLARE_UNNAMED_PROPERTY(
                                FiberForceLengthCurve,
                                "fiber force length curve");

    OpenSim_DECLARE_UNNAMED_PROPERTY(
                                TendonForceLengthCurve,
                                "Tendon force length curve");
    



//=============================================================================
// Construction
//=============================================================================
    /**Default constructor: produces a non-functional empty muscle*/
    Millard2012EquilibriumMuscle();

    /**Constructs a functional muscle using all of the default curves and
       activation model. The tendon is assumed to be elastic, full fiber
       dynamics are solved, and activation dynamics are included.

       @param aName The name of the muscle.

       @param aMaxIsometricForce 
        The force generated by the muscle when it at its optimal resting length,
        has a contraction velocity of zero, and is fully activated 
        (Newtons).
       
       @param aOptimalFiberLength
        The optimal length of the muscle fiber (meters).
                                
       @param aTendonSlackLength
        The resting length of the tendon (meters).

       @param aPennationAngle
        The angle of the fiber relative to the tendon when the fiber is at its
        optimal resting length (radians).
    */
    Millard2012EquilibriumMuscle(const std::string &aName,
                                double aMaxIsometricForce,
                                double aOptimalFiberLength,
                                double aTendonSlackLength,
                                double aPennationAngle);

    


//==============================================================================
// Get Properties
//==============================================================================   
  

   /**
   @param s the state of the system
   @return the normalized force term associated with tendon element,
            \f$\mathbf{f}_{SE}(\hat{l}_{T})\f$, in the equilibrium equation 
   */
   double getTendonForceMultiplier(SimTK::State& s) const;

   /**
   @returns the MuscleFirstOrderActivationDynamicModel 
            that this muscle model uses
   */
    const MuscleFirstOrderActivationDynamicModel& getActivationModel() const;

    /**
   @returns the MuscleFixedWidthPennationModel 
            that this muscle model uses
   */
    const MuscleFixedWidthPennationModel& getPennationModel() const;

    /**
    @returns the ActiveForceLengthCurve that this muscle model uses
    */
    const ActiveForceLengthCurve& getActiveForceLengthCurve() const;

    /**
    @returns the ForceVelocityInverseCurve that this muscle model uses
    */
    const ForceVelocityInverseCurve& getForceVelocityInverseCurve() const;

    /**
    @returns the FiberForceLengthCurve that this muscle model uses
    */
    const FiberForceLengthCurve& getFiberForceLengthCurve() const;

    /**
    @returns the TendonForceLengthCurve that this muscle model uses
    */
    const TendonForceLengthCurve& getTendonForceLengthCurve() const;

    

    /**
    @returns the stiffness of the muscle fibers along the tendon (N/m)
    */
    double getFiberStiffnessAlongTendon(const SimTK::State& s) const;
    
    /**
     @returns the minimum fiber length, which is the maximum of two values:
        the smallest fiber length allowed by the pennation model, and the 
        minimum fiber length in the active force length curve. When the fiber
        length reaches this value, it is constrained to this value until the 
        fiber velocity goes positive.
    */
    double getMinimumFiberLength() const;

    /**
     @returns the minimum fiber length along the tendon, which is the maximum of 
        two values:
        the smallest fiber length along the teond allowed by the pennation 
        model, and the minimum fiber length along the tendon in the active force
        length curve. When the fiber length reaches this value, it is 
        constrained to this length along the tendon until the fiber velocity 
        goes positive.
    */
    double getMinimumFiberLengthAlongTendon() const;

    /**
    @returns the minimum activation level allowed by the muscle model. Note that
             this equilibrium model, like all equilibrium models has a 
             singularity when activation goes to 0.0, which means that a 
             non-zero lower bound is required.
    */
    double getMinimumActivation() const;

    /**
    @returns the maximum pennation angle allowed by this muscle model. Note that
            this equilibrium model, like all equilibrium models has a 
            singularity when pennation hits Pi/2.0. This requires that the 
            maximum pennation angle must be less than Pi/2.0.
    */
    double getMaximumPennationAngle() const;


    bool getUseReducedFiberDynamics() const;

//==============================================================================
// Set Properties
//==============================================================================

    /**
    @param aActivationMdl the MuscleFirstOrderActivationDynamicModel that this
                          muscle model uses to simulate activation dynamics
    */
    void setActivationModel(
            MuscleFirstOrderActivationDynamicModel& aActivationMdl);

    /**
    @param minActivation will set the minimum activation property in the 
            Activation model. This function is provided to ensure that the 
            desired minimum activation will not cause a numerical singularity 
            in this model.
    @returns true if the value was acceptable and the property was set
    */
    bool setMinimumActivation(double minActivation);


    /**
    @param aActiveForceLengthCurve the ActiveForceLengthCurve that this muscle
                            model uses to scale active fiber force as a function
                            of length
    */
    void setActiveForceLengthCurve(
            ActiveForceLengthCurve& aActiveForceLengthCurve);

    /**
    @param aForceVelocityInverseCurve the ForceVelocityInverseCurve that this
                            muscle model uses to calculate the derivative of
                            fiber length.
    */
    void setForceVelocityInverseCurve(
            ForceVelocityInverseCurve& aForceVelocityInverseCurve);

    /**
    @param aFiberForceLengthCurve the FiberForceLengthCurve that this muscle 
                            model uses to calculate the passive force the muscle
                            fiber generates as the length of the fiber changes
    */
    void setFiberForceLengthCurve(
            FiberForceLengthCurve& aFiberForceLengthCurve);
    
    /**
    @param aTendonForceLengthCurve the TendonForceLengthCurve that this muscle
                            model uses to define the tendon force length curve
    */
    void setTendonForceLengthCurve(
            TendonForceLengthCurve& aTendonForceLengthCurve);

    /**
    @param maxPennationAngle is the maximum pennation (radians). This method
           will set the maximum pennation angle property of the pennation
           model, and is provided to ensure that the desired maximum pennation 
           angle will not cause a numerical singularity in this model.
    @returns true if the value was acceptable and the property was set
    */
    bool setMaximumPennationAngle(double maxPennationAngle);

    /**
    @param use the value to set the reduced fiber dynamics flag to. Note that the
           ignore_tendon_compliance must be set to false for this option
           to be used. This function cannot be called after the call to 
           initSystem has been made.
    */
    bool setUseReducedFiberDynamics(bool use);



//==============================================================================
// State Variable Related Functions
//==============================================================================
    /**
    @returns the default activation level that is used as an initial condition
             if none is provided by the user.
    */
    double getDefaultActivation() const;

    /**
    @returns the default fiber length that is used as an initial condition
             if none is provided by the user.
    */
    double getDefaultFiberLength() const;

    /**
    @param s The state of the system
    @returns the time derivative of activation
    */
    double getActivationRate(const SimTK::State& s) const;

    /**
    @param s The state of the system
    @returns the velocity of the fiber (m/s)
    */
    double getFiberVelocity(const SimTK::State& s) const;

    /**
    @param activation the default activation level that is used to initialize
           the muscle
    */
    void setDefaultActivation(double activation);

    /**
    @param fiberLength the default fiber length that is used to initialize
           the muscle
    */
    void setDefaultFiberLength(double fiberLength);

    /**
    @param s the state of the system
    @param activation the desired activation level
    */
    void setActivation(SimTK::State& s, double activation) const;

    /**
    @param s the state of the system
    @param fiberLength the desired fiber length (m)
    */
    void setFiberLength(SimTK::State& s, double fiberLength) const;

    /**
    @returns A string arraw of the state variable names
    */
    Array<std::string> getStateVariableNames() const FINAL_11;

    /**
    @param stateVariableName the name of the state varaible in question
    @returns The system index of the state variable in question
    */
    SimTK::SystemYIndex getStateVariableSystemIndex(
        const std::string &stateVariableName) const FINAL_11;

//==============================================================================
// Public Computations Muscle.h
//==============================================================================

    /**
    @param s the state of the system
    @returns the tensile force the muscle is generating in N
    */
    double computeActuation(const SimTK::State& s) const FINAL_11;


    /** This function computes the fiber length such that muscle fiber and 
        tendon are developing the same force, and so that the velocity of
        the entire muscle-tendon is spread between the fiber and the tendon
        according to their relative compliances.
        
        @param s the state of the system
    */
    void computeInitialFiberEquilibrium(SimTK::State& s) const FINAL_11;
    

    ///@cond TO BE DEPRECATED. 
    /*  Once the ignore_tendon_compliance flag is implemented correctly get rid 
        of this method as it duplicates code in calcMuscleLengthInfo,
        calcFiberVelocityInfo, and calcMuscleDynamicsInfo
    */
    /*
    @param activation of the muscle [0-1]
    @param fiberLength in (m)
    @param fiberVelocity in (m/s)
    @returns the force component generated by the fiber that is associated only
             with activation (the parallel element is not included)   
    */
    double calcActiveFiberForceAlongTendon( double activation, 
                                            double fiberLength, 
                                            double fiberVelocity) const; 

    virtual double calcInextensibleTendonActiveFiberForce(SimTK::State& s, 
                                             double aActivation) const FINAL_11;
    ///@endcond
protected:
    /** Courtesy of Ajay*/
    void postScale(const SimTK::State& s, const ScaleSet& aScaleSet);

        /** Calculate activation rate */
    double calcActivationRate(const SimTK::State& s) const; 


//==============================================================================
//Muscle Inferface requirements
//==============================================================================

    /**calculate muscle's position related values such fiber and tendon lengths,
    normalized lengths, pennation angle, etc... */
    void calcMuscleLengthInfo(const SimTK::State& s, 
                              MuscleLengthInfo& mli) const OVERRIDE_11;  


    /** calculate muscle's velocity related values such fiber and tendon 
        velocities,normalized velocities, pennation angular velocity, etc... */
    void  calcFiberVelocityInfo(const SimTK::State& s, 
                                FiberVelocityInfo& fvi) const OVERRIDE_11;

    /** calculate muscle's active and passive force-length, force-velocity, 
        tendon force, relationships and their related values */
    void  calcMuscleDynamicsInfo(const SimTK::State& s, 
                                 MuscleDynamicsInfo& mdi) const OVERRIDE_11;

//==============================================================================
//ModelComponent Interface requirements
//==============================================================================
    /**Sets up the ModelComponent from the model, if necessary*/
    void connectToModel(Model& model) FINAL_11;

    /**Creates the ModelComponent so that it can be used in simulation*/
	void addToSystem(SimTK::MultibodySystem& system) const FINAL_11;

    /**Initializes the state of the ModelComponent*/
	void initStateFromProperties(SimTK::State& s) const FINAL_11;
    
    /**Sets the default state for ModelComponent*/
    void setPropertiesFromState(const SimTK::State& s) FINAL_11;
	
    /**computes state variable derivatives*/
    SimTK::Vector computeStateVariableDerivatives(
        const SimTK::State& s) const FINAL_11;

//==============================================================================
//State derivative helper methods
//==============================================================================
	
    /**
    Set the derivative of an actuator state, specified by name
    @param s the state    
    @param aStateName The name of the state to set.
    @param aValue The value to set the state to.
    */
    void setStateVariableDeriv( const SimTK::State& s, 
                                const std::string &aStateName, 
                                double aValue) const;
   
    /**
     Get the derivative of an actuator state, by index.
     @param s the state 
     @param aStateName the name of the state to get.
     @return The value of the state derivative
    */
	double getStateVariableDeriv(   const SimTK::State& s, 
                                    const std::string &aStateName) const;

private:
    //The name used to access the activation state
    static const std::string STATE_ACTIVATION_NAME;
    //The name used to access the fiber length state
	static const std::string STATE_FIBER_LENGTH_NAME;

    static const std::string MODELING_OPTION_USE_REDUCED_MODEL_NAME;
    
    //static const std::string PREVIOUS_FIBER_LENGTH_NAME;
    //static const std::string PREVIOUS_FIBER_VELOCITY_NAME;

    void setNull();
    void constructProperties();
    
    /*Builds all of the components that are necessary to use this 
    muscle model in simulation*/
    void buildMuscle();

    /*Checks to make sure that none of the muscle model's properties have
    changed. If they have changed, then the muscle is rebuilt.*/
    void ensureMuscleUpToDate() const;

    //Approximate muscle dynamics are the same as the regular 
    //equilibrium muscle dynamics


    /*
    Calculates the force-velocity multiplier
    @param a activation
    @param fal the fiber active force length multiplier
    @param fpe the fiber force length multiplier
    @param fse the tendon force length multiplier
    @param cosphi the cosine of the pennation angle
    @param caller the name of the function calling this function. This name
           is provided to generate meaningful exception messages

    @return the force velocity multiplier
    */
    double calcFv(  double a, 
                    double fal, 
                    double fpe, 
                    double fk,                     
                    double fcphi, 
                    double fse, 
                    double cosphi,
                    std::string& caller) const;

    /*
    
    @param fiso the maximum isometric force the fiber can generate  
    @param a activation
    @param fal the fiber active force length multiplier
    @param fv the fiber force velocity multiplier
    @param fpe the fiber force length multiplier  
    @param cosphi the cosine of the pennation angle    

    @return the force generated by the fiber, in the direction of the fiber
    */
    double calcFiberForce(  double fiso, 
                            double a, 
                            double fal,
                            double fv,                             
                            double fpe) const;    


    /*
    
    @param fiso the maximum isometric force the fiber can generate  
    @param a activation
    @param fal the fiber active force length multiplier
    @param fv the fiber force velocity multiplier   

    @return the active force generated by the fiber
    */
    double calcActiveFiberForce(  double fiso, 
                            double a, 
                            double fal,
                            double fv) const;  

    /*
    @param fiso the maximum isometric force the fiber can generate  
    @param a activation
    @param fal the fiber active force length multiplier
    @param fv the fiber force velocity multiplier
    @param fpe the fiber force length multiplier 
    @param cosphi the cosine of the pennation angle    

    @return the force generated by the fiber, in the direction of the tendon
    */
    double calcFiberForceAlongTendon(  double fiso, 
                                        double a, 
                                        double fal,
                                        double fv,                             
                                        double fpe,                                       
                                        double cosPhi) const;  


    /*
    @param fiso the maximum isometric force the fiber can generate  
    @param a activation
    @param fal the fiber active force length multiplier
    @param fv the fiber force velocity multiplier
    @param fpe the fiber force length multiplier
    @param sinphi the sine of the pennation angle
    @param cosphi the cosine of the pennation angle    
    @param lce the fiber length
    @param lceN the normalized fiber length
    @param optFibLen the optimal fiber length

    @return the stiffness (d_Fm/d_lce) of the fiber in the direction of the fiber
    */
    double calcFiberStiffness(  double fiso, 
                                double a, 
                                double fal,
                                double fv,                             
                                double fpe,                                
                                double sinPhi,
                                double cosPhi,
                                double lce,
                                double lceN,
                                double optFibLen) const;

    /*
    @param dFm_d_lce the stiffness of the fiber in the direction of the fiber
    @param sinphi the sine of the pennation angle
    @param cosphi the cosine of the pennation angle    
    @param lce the fiber length

    @return the stiffness (d_FmAT/d_lceAT) of the fiber in the direction of 
            the tendon.
    */
    double calc_DFiberForceAT_DFiberLengthAT(  double dFm_d_lce,                                               
                                               double sinPhi,
                                               double cosPhi,
                                               double lce) const;
    /*
    @param fiso the maximum isometric force the fiber can generate  
    @param a activation
    @param fal the fiber active force length multiplier
    @param fv the fiber force velocity multiplier
    @param fpe the fiber  force length multiplier
    @param sinphi the sine of the pennation angle
    @param cosphi the cosine of the pennation angle    
    @param lce the fiber length
    @param lceN the normalized fiber length
    @param optFibLen the optimal fiber length

    @return the partial derivative of fiber force along the tendon with respect
            to small changes in fiber length (in the direction of the fiber)
    */
    double calc_DFiberForceAT_DFiberLength( double fiso, 
                                            double a, 
                                            double fal,
                                            double fv,                             
                                            double fpe,                                            
                                            double sinPhi,
                                            double cosPhi,
                                            double lce,
                                            double lceN,
                                            double optFibLen) const;

    /*
    @param dFt_d_tl the partial derivative of tendon force with respect to small
                    changes in tendon length (tendon stiffness) in (N/m)
    @param lce the fiber length
    @param sinphi the sine of the pennation angle
    @param cosphi the cosine of the pennation angle        
    @return the partial derivative of tendon force with respect to small changes 
            in fiber length
    */
    double calc_DTendonForce_DFiberLength(  double dFt_d_tl, 
                                            double lce,
                                            double sinphi,
                                            double cosphi,
                                            std::string& caller) const;

    //=====================================================================
    // Private Utility Class Members
    //      -Computes activation dynamics and fiber kinematics
    //=====================================================================

    // This object defines the pennation model used for this muscle model.
    // Using a ClonePtr saves us from having to write a destructor, copy
    // constructor, or copy assignment. This will be zeroed on construction,
    // cleaned up on destruction, and cloned when copying.
    //SimTK::ClonePtr<MuscleFixedWidthPennationModel> penMdl;
    MuscleFixedWidthPennationModel penMdl;

    //Used in muscle initialization, and also in the calcActiveFiberForce
    //function. This curve is too expensive to make more than once, so 
    //we're storing it here as a private member variable
    ForceVelocityCurve fvCurve;

    //Here I'm using the 'm_' to prevent me from trashing this variable with
    //a poorly chosen local variable.
    double m_minimumFiberLength;
    double m_minimumFiberLengthAlongTendon;

    //Stores the fiber length and velocity of the last time step
    //used by the reduced method
    mutable SimTK::Vec5 reducedFiberStateHint;

    /*
    Solves fiber length and velocity to satisfy the equilibrium equations. 
    Fiber velocity is shared between the tendon and the fiber based on their
    relative mechanical stiffnesses. 

    @param aActivation the initial activation of the muscle
    @param pathLength length of the whole muscle
    @param pathLengtheningSpeed lengthening speed of the muscle path
    @param aSolTolerance the desired relative tolerance of the equilibrium 
           solution
    @param aMaxIterations the maximum number of Newton steps allowed before
           attempts to initialize the model are given up, and an exception is 
           thrown.
    */
    SimTK::Vector estimateElasticTendonFiberState(double aActivation, 
                            double pathLength, double pathLengtheningSpeed,
                            double aSolTolerance, int aMaxIterations) const;
    
    /*
    A method that solves for the fiber length and fiber velocity by solving 
    the equilibrium equation, and the derivative of the equilibrium equation
    by assuming that the acceleration of fiber length is 0.

    @param previousFiberLength : A hint
    @param previousFiberVelocity : A hint
    @param deltaTime : the amount of time that has passed between the hint and
                       the current call.
    @param aActivation the initial activation of the muscle
    @param dactivation_dt the time derivative of activation
    @param pathLength length of the whole muscle
    @param pathLengtheningSpeed lengthening speed of the muscle path
    @param aSolTolerance the desired relative tolerance of the equilibrium 
           solution
    @param aMaxIterations the maximum number of Newton steps allowed before
           attempts to initialize the model are given up, and an exception is 
           thrown.

    @returns A vector
       Index: Name
           0: flag_status    : 0 converged, 1 fiber at min length, 2 diverged
           1: solnErr        : 2 norm of the error of the equilibrium equation
                               and its first derivative
           2: iterations     : Number of Newton iterations required
           3: fiberLength    : length of the fiber in meters
           4: fiberVelocity  : velocity of the fiber in meters/s
           5: tendonForce    : force tendon is exerting in N

    */
    SimTK::Vector estimateElasticTendonFiberState2(
                    SimTK::Vec5 hint,
                    double aActivation, double dactivation_dt, 
                    double pathLength, double pathLengtheningSpeed,
                    double aSolTolerance, int aMaxIterations) const;

    //Returns true if the fiber length is currently shorter than the minimum
    //value allowed by the pennation model and the active force length curve
    bool isFiberStateClamped(double lce, 
                            double dlceN) const;

    //Returns the maximum of the minimum fiber length, and the current fiber 
    //length
    double clampFiberLength(double lce) const;

    //Returns true if fiber length is a state
    bool isFiberLengthAState() const;
    //Returns true if the tendon is elastic
    bool isTendonElastic() const;
    //Returns true if activation is a state
    bool isActivationAState() const;
    //Returns the simulation method called prior to the call to initSystem
    int getInitialSimulationMethod() const;
    //Calculates a flag that indicates the exact configuration of the simulation
    int calcSimulationMethod(bool ignoreTendonCompliance, 
                             bool ignoreActivationDynamics,
                             bool useReducedFiberDynamics) const;
    
};    

} // end of namespace OpenSim

#endif // OPENSIM_Millard2012EquilibriumMuscle_h__
