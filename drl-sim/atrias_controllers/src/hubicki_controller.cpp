#include <atrias_controllers/controller.h>

#define HUBICKI_ESTIMATED_SPRING_STIFFNESS  0.
#define HUBICKI_ESTIMATED_GEAR_RATIO        20

void hubicki_flight_controller(ControllerInput *, ControllerOutput *, ControllerState *, ControllerData *);
void hubicki_stance_controller(ControllerInput *, ControllerOutput *, ControllerState *, ControllerData *);

extern void initialize_hubicki_controller(ControllerInput *input, ControllerOutput *output, ControllerState *state, 
	ControllerData *data)
{
	HUBICKI_CONTROLLER_STATE(state)->in_flight = true;
	HUBICKI_CONTROLLER_STATE(state)->after_mid_stance = false;

	HUBICKI_CONTROLLER_STATE(state)->peak_ht = 1.0;

	output->motor_torqueA    = 0.;
	output->motor_torqueB    = 0.;
	output->motor_torque_hip = 0.;

	PRINT_MSG("Hubicki Controller Initialized.\n");

	HUBICKI_CONTROLLER_STATE(state)->time_of_last_stance = HUBICKI_CONTROLLER_STATE(state)->time;
}


extern void update_hubicki_controller(ControllerInput *input, ControllerOutput *output, ControllerState *state, 
	ControllerData *data)
{
	//HUBICKI_CONTROLLER_STATE(state)->in_flight = false;

	if ( HUBICKI_CONTROLLER_STATE(state)->in_flight )
	{
		hubicki_flight_controller(input, output, state, data);
	}
	else
	{
		hubicki_stance_controller(input, output, state, data);
	}	
	

	// Regardless of if we are in stance or flight we control the hip the same
	// Do that now.
	float des_hip_ang = 0.99366*input->body_angle + 0.03705;

	// REMOVED BY HUBICKI!  Resulted in oscillation ourside of range
       // if ((des_hip_ang < -0.2007) || (des_hip_ang > 0.148))
       //        des_hip_ang = input->body_angle;

	//  Added Hubicki
	des_hip_ang = CLAMP( des_hip_ang, -0.2007, 0.148 );
	// End Hubicki	


	output->motor_torque_hip = HUBICKI_CONTROLLER_DATA(data)->stance_hip_p_gain * (des_hip_ang - input->hip_angle)
                - HUBICKI_CONTROLLER_DATA(data)->stance_hip_d_gain * input->hip_angle_vel;

	HUBICKI_CONTROLLER_STATE(state)->last_leg_len = cos( ( 2.*PI + input->leg_angleA - input->leg_angleB ) / 2. );

}

extern void takedown_hubicki_controller(ControllerInput *input, ControllerOutput *output, ControllerState *state, 
	ControllerData *data)
{
	output->motor_torqueA 	 = 0.;
	output->motor_torqueB    = 0.;
	output->motor_torque_hip = 0.;
}

void hubicki_flight_controller(ControllerInput *input, ControllerOutput *output, ControllerState *state, 
	ControllerData *data)
{
   
    	// Spring deflections for force control.  These can be problematic on the real robot, since they require good sensor calibration.
	float spring_defA = input->leg_angleA - input->motor_angleA;
	float spring_defB = input->leg_angleB - input->motor_angleB;
        
	// Negated 02/09
	float des_leg_ang = PI/2. - HUBICKI_CONTROLLER_DATA(data)->leg_ang_gain * input->xVelocity 
		+ HUBICKI_CONTROLLER_DATA(data)->hor_vel_gain * HUBICKI_CONTROLLER_DATA(data)->des_hor_vel;

	// Generate the motor torques.
	float des_mtr_angA = des_leg_ang - PI + acos(HUBICKI_CONTROLLER_DATA(data)->preferred_leg_len);
	float des_mtr_angB = des_leg_ang + PI - acos(HUBICKI_CONTROLLER_DATA(data)->preferred_leg_len); 

	float leg_angle = ( input->leg_angleA + input->leg_angleB ) / 2.;
	float leg_length = - 0.5 * sin( input->leg_angleA ) - 0.5 * sin( input->leg_angleB );

	// XXX: This is a hack to keep the robot in one place.
	// GCF = gain control factor
	float gcf = CLAMP(MAX(ABS(des_mtr_angA - input->motor_angleA), 
			      ABS(des_mtr_angB - input->motor_angleB)) / 0.05, 0, 1);

	output->motor_torqueA = gcf * HUBICKI_CONTROLLER_DATA(data)->flight_p_gain * (des_mtr_angA - input->motor_angleA) 
		- HUBICKI_CONTROLLER_DATA(data)->flight_d_gain * input->motor_velocityA;
	output->motor_torqueB = gcf * HUBICKI_CONTROLLER_DATA(data)->flight_p_gain * (des_mtr_angB - input->motor_angleB) 
		- HUBICKI_CONTROLLER_DATA(data)->flight_d_gain * input->motor_velocityB;

	//=========================================================================//

	// Figure out the next state.
        //PRINT_MSG("00<%f> <%f>", ABS(spring_defA), ABS(spring_defB));
	//if ( ( input->zPosition - leg_length * sin( leg_angle ) < 0.02 ) && ( (ABS(input->motor_angleA - input->leg_angleA) > HUBICKI_CONTROLLER_DATA(data)->stance_spring_threshold) 
	//	|| (ABS(input->motor_angleB - input->leg_angleB) > HUBICKI_CONTROLLER_DATA(data)->stance_spring_threshold) ) )
	if ( input->toe_switch == 1)
	{
		// Check to see if ground contact has occured.
		PRINT_MSG("TD!\n");

		HUBICKI_CONTROLLER_STATE(state)->in_flight = false;
	}

	// Check to see if we have reached a new peak height.
	HUBICKI_CONTROLLER_STATE(state)->peak_ht = MAX( input->zPosition, HUBICKI_CONTROLLER_STATE(state)->peak_ht );
}

void hubicki_stance_controller(ControllerInput *input, ControllerOutput *output, ControllerState *state, 
	ControllerData *data)
{
	// Spring deflections for force control.  These can be problematic on the real robot, since they require good sensor calibration.
	float spring_defA = input->leg_angleA - input->motor_angleA;
	float spring_defB = input->leg_angleB - input->motor_angleB;
	
	//float spring_def_velA = input->leg_velocityA - input->motor_velocityA;
	//float spring_def_velB = input->leg_velocityB - input->motor_velocityB;

	// Limit the desired leg length to help prevent smacking hardstops.
	float leg_len			= cos( ( 2.*PI + input->leg_angleA - input->leg_angleB ) / 2. );
	float zf_leg_len	= cos( ( 2.*PI + input->motor_angleA - input->motor_angleB ) / 2. ); // zero force leg length
	float zf_leg_len_vel = -sin( ( 2.*PI + input->motor_angleA - input->motor_angleB ) / 4. 
		* ( input->motor_velocityA - input->motor_velocityB ) );

	// Check to see if the robot has reached midstance.  If it has, set the after mid stance flag.
	if ( ( !HUBICKI_CONTROLLER_STATE(state)->after_mid_stance ) && ( leg_len > HUBICKI_CONTROLLER_STATE(state)->last_leg_len ) )
	{
		HUBICKI_CONTROLLER_STATE(state)->after_mid_stance = true;
	}

//HUBICKI DEBUG ADD
//HUBICKI_CONTROLLER_STATE(state)->after_mid_stance = false;
// END DEBUG!


	// Find the leg extension during stance to add energy back into the system.
	float leg_ext = 0.;

	// If the robot has reach midstance, extend the leg.
	if ( HUBICKI_CONTROLLER_STATE(state)->after_mid_stance )
	{
		leg_ext = HUBICKI_CONTROLLER_DATA(data)->hop_ht_gain * ( HUBICKI_CONTROLLER_DATA(data)->des_hop_ht - HUBICKI_CONTROLLER_STATE(state)->peak_ht );
	}

	// Limit the desired leg length.
	float des_leg_len = CLAMP( HUBICKI_CONTROLLER_DATA(data)->preferred_leg_len + leg_ext, 0.51, 0.97 );
	float torque = HUBICKI_CONTROLLER_DATA(data)->stance_p_gain * (des_leg_len - zf_leg_len ) 
		- HUBICKI_CONTROLLER_DATA(data)->stance_d_gain * zf_leg_len_vel 
		+ HUBICKI_ESTIMATED_SPRING_STIFFNESS * (zf_leg_len - leg_len) / HUBICKI_ESTIMATED_GEAR_RATIO;

	float leg_angle = ( input->leg_angleA + input->leg_angleB ) / 2.;
	float leg_length = - 0.5 * sin( input->leg_angleA ) - 0.5 * sin( input->leg_angleB );

	output->motor_torqueA =	 -torque;
	output->motor_torqueB =	 torque;
	
	//float des_leg_ang = (input->leg_angleA + input->leg_angleB) / 2.;
	//float des_leg_ang_vel = (input->leg_velocityA + input->leg_velocityB) / 2.;		

	// Deadband for force control.
	/*if ( ABS( des_leg_ang - (input->motor_angleA + input->motor_angleB) / 2. ) < 0.015 )
	{
		des_leg_ang = (input->motor_angleA + input->motor_angleB) / 2.;
	}

	if ( ABS(des_leg_ang_vel ) < 0.1 )
	{
		des_leg_ang_vel = 0.;
	}*/

	//float des_mtr_angA = des_leg_ang - PI + acos(des_leg_len);
	//float des_mtr_angB = des_leg_ang + PI - acos(des_leg_len); 

	// Compute the leg torque for zero hip moment and maintaining hopping height.
	//output->motor_torqueA = HUBICKI_CONTROLLER_DATA(data)->stance_p_gain * (des_mtr_angA - input->motor_angleA) 
	//	+ HUBICKI_CONTROLLER_DATA(data)->stance_d_gain * (des_leg_ang_vel - input->motor_velocityA) + HUBICKI_ESTIMATED_SPRING_STIFFNESS * spring_defA / HUBICKI_ESTIMATED_GEAR_RATIO;
	//output->motor_torqueB = HUBICKI_CONTROLLER_DATA(data)->stance_p_gain * (des_mtr_angB - input->motor_angleB) 
	//	+ HUBICKI_CONTROLLER_DATA(data)->stance_d_gain * (des_leg_ang_vel - input->motor_velocityB) + HUBICKI_ESTIMATED_SPRING_STIFFNESS * spring_defB / HUBICKI_ESTIMATED_GEAR_RATIO;

	// Clamp the torques for now, for added safety.
	//output->motor_torqueA = CLAMP( output->motor_torqueA, -3., 3. );
	//output->motor_torqueB = CLAMP( output->motor_torqueB, -3., 3. );

        //PRINT_MSG("!!<%f> <%f>", ABS(spring_defA), ABS(spring_defB));
       
	//if ( ( input->zPosition - leg_length * sin( leg_angle ) > -0.02 ) && ( ( ABS(spring_defA) < HUBICKI_CONTROLLER_DATA(data)->flight_spring_threshold )
	//	&& ( ABS(spring_defB) < HUBICKI_CONTROLLER_DATA(data)->flight_spring_threshold ) ) )
	if ( input->toe_switch == 1 )
	{
		HUBICKI_CONTROLLER_STATE(state)->time_of_last_stance = HUBICKI_CONTROLLER_STATE(state)->time;
	}

	//if ( input->toe_switch == 0 && 1000 < (HUBICKI_CONTROLLER_STATE(state)->time - HUBICKI_CONTROLLER_STATE(state)->time_of_last_stance))
	if ( input->toe_switch == 0 )
	{
		// Check to see if lift off has occured.

		PRINT_MSG("LO!\n");

		HUBICKI_CONTROLLER_STATE(state)->in_flight = true;

		// Reset peak height.
		HUBICKI_CONTROLLER_STATE(state)->peak_ht = 0.;

		HUBICKI_CONTROLLER_STATE(state)->after_mid_stance = false;
	}
}
