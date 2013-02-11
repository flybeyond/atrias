#ifndef __ATC_FORCE_CONTROL_DEMO_H__
#define __ATC_FORCE_CONTROL_DEMO_H__

/*! \file controller_component.h
 *  \author Mikhail Jones
 *  \brief Orocos Component header for atc_force_control_demo controller.
 */

// Orocos 
#include <rtt/os/main.h>
#include <rtt/RTT.hpp>
#include <rtt/Logger.hpp>
#include <rtt/TaskContext.hpp>
#include <rtt/OperationCaller.hpp>
#include <rtt/Component.hpp>

// C
#include <stdlib.h>
#include <atrias_shared/GuiPublishTimer.h>
#include <atrias_shared/globals.h>
#include <robot_invariant_defs.h>

// Datatypes
#include <atc_force_control_demo/controller_input.h>
#include <atc_force_control_demo/controller_status.h>
#include <atc_force_control_demo/controller_log_data.h>
#include <atrias_msgs/robot_state.h>
#include <atrias_msgs/controller_output.h>
#include <atrias_shared/controller_structs.h>

using namespace RTT;
using namespace Orocos;
using namespace atc_force_control_demo;

namespace atrias {
using namespace shared;
namespace controller {

class ATCForceControlDemo : public TaskContext {
private:
    // This Operation is called by the RT Operations Manager.
    atrias_msgs::controller_output runController(atrias_msgs::robot_state rs);

    atrias_msgs::controller_output co;

    // Logging
    controller_log_data              logData;
    OutputPort<controller_log_data>  logPort;

    // For the GUI
    shared::GuiPublishTimer                         *pubTimer;
    controller_input                                guiIn;
    controller_status                               guiOut;
    OutputPort<controller_status>                   guiDataOut;
    InputPort<controller_input>                     guiDataIn;
    
    // ASCLegToMotorTransforms
    OperationCaller<MotorAngle(double, double)> legToMotorPos;

    // ASCLegForce
    OperationCaller<AB(LegForce legForce, double kp, double kd, atrias_msgs::robot_state_leg leg, atrias_msgs::robot_state_location position)> legForceToMotorCurrent;

	// ASCHipInverseKinematics
	OperationCaller<LeftRight(LeftRight, atrias_msgs::robot_state_leg, atrias_msgs::robot_state_leg, atrias_msgs::robot_state_location position)> toePositionToHipAngle;
	
	// Leg position control variables
	MotorAngle lMotorAngle;
	MotorAngle rMotorAngle;
	
	// Leg force control variables
	LegForce legForce;
	AB motorCurrent;
	
	// Hip control variables
	LeftRight toePosition;
	LeftRight hipAngle;
	

public:
    // Constructor
    ATCForceControlDemo(std::string name);

    // Standard Orocos hooks
    bool configureHook();
    bool startHook();
    void updateHook();
    void stopHook();
    void cleanupHook();
};

}
}

#endif