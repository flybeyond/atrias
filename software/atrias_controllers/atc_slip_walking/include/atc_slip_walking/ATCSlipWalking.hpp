/**
 * @file ATCSlipWalking.hpp
 * @brief A Spring Loaded Inverted Pendulum (SLIP) template model based
 * walking controller.
 * @author Mikhail Jones
 */

#ifndef ATCSlipWalking_HPP
#define ATCSlipWalking_HPP

// Include the ATC class
#include <atrias_control_lib/ATC.hpp>

// Our logging data type
#include "atc_slip_walking/controller_log_data.h"

// The type transmitted from the GUI to the controller
#include "atc_slip_walking/controller_input.h"

// The type transmitted from the controller to the GUI
#include "atc_slip_walking/controller_status.h"

// Our subcontroller types
#include <asc_common_toolkit/ASCCommonToolkit.hpp>
#include <asc_hip_boom_kinematics/ASCHipBoomKinematics.hpp>
#include <asc_interpolation/ASCInterpolation.hpp>
#include <asc_leg_force/ASCLegForce.hpp>
#include <asc_pd/ASCPD.hpp>
#include <asc_rate_limit/ASCRateLimit.hpp>

// Datatypes
#include <deque>
#include <robot_invariant_defs.h>
#include <robot_variant_defs.h>
#include <atrias_msgs/robot_state.h>
#include <atrias_shared/controller_structs.h>
#include <atrias_shared/atrias_parameters.h>

// Functions
#include <numeric>  // std::accumulate

// Namespaces we're using
using namespace std;

// Our namespaces
namespace atrias {
namespace controller {

class ATCSlipWalking : public ATC<
    atc_slip_walking::controller_log_data_,
    atc_slip_walking::controller_input_,
    atc_slip_walking::controller_status_>
{
    public:
        /**
         * @brief The constructor for this controller.
         * @param name The name of this component.
         */
        ATCSlipWalking(string name);

    private:
        /**
         * @brief This is the main function for the top-level controller.
         */
        void controller();

        /**
         * @brief These are functions for the top-level controller.
         */
        void checkSafeties();
        void updateController();
        void hipController();
        void standingController();
        void shutdownController();
        void stanceController(atrias_msgs::robot_state_leg*, atrias_msgs::controller_output_leg*, ASCLegForce*, ASCRateLimit*);
        void singleSupportEvents(atrias_msgs::robot_state_leg*, atrias_msgs::robot_state_leg*, std::deque<double>*);
        void legSwingController(atrias_msgs::robot_state_leg*, atrias_msgs::robot_state_leg*, atrias_msgs::controller_output_leg*, ASCPD*, ASCPD*);
        void doubleSupportEvents(atrias_msgs::robot_state_leg*, atrias_msgs::robot_state_leg*, ASCRateLimit*);
        void resetFlightLegParameters(atrias_msgs::robot_state_leg*, ASCRateLimit*);
        bool detectStance(atrias_msgs::robot_state_leg*, std::deque<double>*);
        void updateToeFilter(uint16_t, std::deque<double>*);
        std::tuple<double, double> legForceControl(LegForce, atrias_msgs::robot_state_leg, atrias_msgs::robot_state_location);

        /**
         * @brief These are sub controllers used by the top level controller.
         */
        ASCCommonToolkit ascCommonToolkit;
        ASCHipBoomKinematics ascHipBoomKinematics;
        ASCInterpolation ascInterpolation;
        ASCLegForce ascLegForceL, ascLegForceR;
        ASCPD ascPDLmA, ascPDLmB, ascPDRmA, ascPDRmB, ascPDLh, ascPDRh;
        ASCRateLimit ascRateLimitLmA, ascRateLimitLmB, ascRateLimitRmA, ascRateLimitRmB, ascRateLimitLh, ascRateLimitRh, ascRateLimitLr0, ascRateLimitRr0;

        /**
         * @brief These are all the variables used by the top level controller.
         */
        // Controller state variables
        int controllerState, walkingState, switchMethod;

        // Walking gait definition values
        double q1, q2, q3, q4;
        double s, ds, sPrev; // Time invariant measure of gait progress
        double r0, fa, dfa; // Spring parameters
        double swingLegRetraction; // The amount the leg retracts during swing
        double stanceLegExtension; // The amount the leg extends during stance to inject energy
        double torsoAngle; // Torso angle offset
        double rExtension; // Leg extension parameter

        // Torso state variables and control
        double qb, dqb;
        double rcom;
        double q, dq;
        double ft, dft;

        // Hip state variables
        double qLh, qRh; // Hip angles
        LeftRight toePosition; // Desired toe positions measures from boom center axis
        double qvpp, rvpp; // VPP parameters

        // Motor and leg variables
        double rSl, drSl, qSl, dqSl; // Stance leg states
        double rFl, drFl, qFl, dqFl; // Flight leg states
        double qmSA, dqmSA, qmSB, dqmSB; // Stance motor states
        double qmFA, dqmFA, qmFB, dqmFB; // Flight motor states
        double qFm, rFm;             // Flight motor states
        double rFdefl;               // Flight leg radial deflection
        LegForce forceSl;
        double k1_11, k1_22, k2_11, k2_22;  // Feedback linearization force control gains

        // Leg parameters at exit state (event trigger)
        double reFm, qeFm; // Flight leg motor states

        // Leg parameters at target states
        double rtFm, r0Sl; // Only length as angle is in q(1:4)

        // Temporary state parameters
        double qm, dqm, rm, drm;

        // State transistion events
        bool isForwardStep, isTrigger; // Logical preventing backstepping issues

        // Toe switch variables
        deque<double> rFilteredToe;
        deque<double> lFilteredToe;

        // Misc margins, ratelimiters and other debug values
        double legRateLimit, hipRateLimit, springRateLimit;
        double currentLimit, velocityLimit, deflectionLimit;
        bool isManualSwingLegTO, isManualSwingLegTD;
};

}
}

#endif // ATCSlipWalking_HPP
