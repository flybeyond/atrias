#ifndef CONNMANAGER_H
#define CONNMANAGER_H

/** @file
  * Contains a class which runs does the main EtherCAT communications in the 
  * ECat Connector.
  */

class ConnManager;

// Orocos
#include <rtt/os/Timer.hpp>
#include <rtt/os/Mutex.hpp>
#include <rtt/os/MutexLock.hpp>
#include <rtt/Logger.hpp>
#include <rtt/os/TimeService.hpp>

// SOEM
extern "C" {
#include <ethercattype.h>
#include <ethercatmain.h>
#include <ethercatconfig.h>
#include <ethercatdc.h>
}

#include <time.h>

#include "atrias_ecat_conn/ECatConn.h"
#include <atrias_msgs/controller_output.h>
#include <robot_invariant_defs.h>

#define EC_TIMEOUT_US      500
#define TIMING_FILTER_GAIN 100

namespace atrias {

namespace ecatConn {

class ConnManager : public RTT::os::Timer {
	/** @brief Lets us access what we need to in ECatConn.
	  */
	ECatConn*      eCatConn;
	
	/** @brief This is where SOEM stores its data.
	  */
	char           IOmap[4096];
	
	/** @brief Used by \a breakLoop() to stop the main loop.
	  */
	bool           done;
	
	/** @brief Protects access to SOEM's data.
	  */
	RTT::os::Mutex eCatLock;
	
	/** @brief Prevents a race condition on loop shutdown.
	  */
	RTT::os::Mutex timerLock;
	
	/** @brief Sends and receives an EtherCAT frame.
	  * Does not grab eCatLock -- must already have it.
	  */
	void           cycleECat();
	
	
	// These store times for the cyclic loop.
	RTT::os::TimeService::nsecs targetTime;
	RTT::os::TimeService::nsecs filtered_overshoot;
	
	public:
		/** @brief The constructor.
		  * @param ecat_conn A pointer to the ECatConn instance.
		  */
		ConnManager(ECatConn* ecat_conn);
		
		/** @brief Shuts down EtherCAT.
		  */
		~ConnManager();
		
		/** @brief Does most of the basic EtherCAT configuration.
		  * @return Success
		  */
		bool configure();
		
		/** @brief Start the main loop.
		  * @return Success.
		  */
		bool initialize();
		
		/** @brief The main ECat receive loop. Called cyclicly.
		  */
		void timeout(TimerId timer_id);
		
		/** @brief Sends new outputs over ECat.
		  * @param controller_output The new outputs.
		  */
		void sendControllerOutput(atrias_msgs::controller_output& controller_output);
		
		/** @brief Stops the main loop.
		  * @return Success
		  */
		void stop();
};

}

}

#endif // CONNMANAGER_H