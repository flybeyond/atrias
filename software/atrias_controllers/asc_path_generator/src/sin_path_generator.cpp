/*! \file controller_component.cpp
 *  \author Andrew Peekema
 *  \brief Orocos Component code for the sin path generator subcontroller.
 */

#include <asc_path_generator/sin_path_generator.h>

namespace atrias {
namespace controller {

ASCSinPathGenerator::ASCSinPathGenerator(std::string name):
    RTT::TaskContext(name),
    logPort(name + "_log")
{
    this->provides("sinGen")
        ->addOperation("runController", &ASCSinPathGenerator::runController, this, OwnThread)
        .doc("Run the controller.");
    this->provides("sinGen")
        ->addOperation("reset", &ASCSinPathGenerator::reset, this, OwnThread);

    // For logging
    // Create a port
    addPort(logPort); 
    // Connect with buffer size 100000 so we get all data.
    ConnPolicy policy = RTT::ConnPolicy::buffer(100000);
    // Transport type = ROS
    policy.transport = 3;
    // ROS topic name
    policy.name_id = "/" + name + "_log";
    // Construct the stream between the port and ROS topic
    logPort.createStream(policy);

    // Setup the controller
    accumulator = 0.0;

    log(Info) << "[ASCSG] Constructed!" << endlog();
}

void ASCSinPathGenerator::reset(void)
{
    // Reset the accumulator
    accumulator = 0.0;
}

MotorState ASCSinPathGenerator::runController(double frequency, double amplitude)
{
    // The sin input
    accumulator += 0.001*2.0*M_PI*frequency;
    if (accumulator >= 2.0*M_PI)
        accumulator -= 2.0*M_PI;

    // Return the function value and its derivative
    sinOut.ang = amplitude*sin(accumulator);
    sinOut.vel = amplitude*cos(accumulator)*2.0*M_PI*frequency;

    // Stuff the msg and push to ROS for logging
    logData.ang = sinOut.ang;
    logData.vel = sinOut.vel;
    logPort.write(logData);

    return sinOut;
}

bool ASCSinPathGenerator::configureHook() {
    log(Info) << "[ASCSG] configured!" << endlog();
    return true;
}

bool ASCSinPathGenerator::startHook() {
    log(Info) << "[ASCSG] started!" << endlog();
    return true;
}

void ASCSinPathGenerator::updateHook() {
}

void ASCSinPathGenerator::stopHook() {
    log(Info) << "[ASCSG] stopped!" << endlog();
}

void ASCSinPathGenerator::cleanupHook() {
    log(Info) << "[ASCSG] cleaned up!" << endlog();
}

ORO_CREATE_COMPONENT(ASCSinPathGenerator)

}
}