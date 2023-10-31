/****************************************************************************************
Nom du fichier : movement.h
Auteur :                   
Date de création : 
  
****************************************************************************************/
#ifndef PROTOTYPE_H
#define PROTOTYPE_H

#include <LibRobus.h>
#include <Arduino.h>
#include "mathX.h"
#include "pid.h"
#include "float.h"

#define WHEEL_BASE_DIAMETER 7.480315
#define WHEEL_DIAMETER 2.992126
#define INACTIVE NAN

namespace Movement
{

    extern float pulseToDist;

    struct WheelVelocities {
        float rightVelocity;
        float leftVelocity;
    };
	
    float computeOrientation();
    float computeDistance();

    bool distanceFlag(float distance, float *initialDistance);
    bool orientationFlag(float angle, float *initialOrientation);
    bool distanceFlag(float distance);
    bool orientationFlag(float angle);
    bool isInactive(float var);

    void rotate(float velocity, float radius);

    void move(float velocity, float angularVelocity);

    void moveUnited(float velocity, float radius, float orientation);

    bool rotate(float velocity, float radius, float angle);

    bool forward(float velocity, float distance);

    float computeRightMotorSpeed()
    {
    
    static float past = 0.0;
    static float speedMotor = 0.0;
    static float oldPulse = 0.0;
    float present = micros();
    float pulse = ENCODER_Read(LEFT);
    speedMotor = 1000000.0 * pulseToDist * float(pulse-oldPulse) / float(present - past);
  
    past = present;
    oldPulse = pulse;
    
    return speedMotor;
  }

    float computeLeftMotorSpeed();
    float computeRightMotorSpeed();
    void updatePIDs();


	namespace {
		// *************************************************************************************************
		// VARIABLES LOCALES
		// *************************************************************************************************
		/* VIDE */

        extern PID::valeursPID rightPID;
        extern PID::valeursPID leftPID;
	}
}

#endif // PROTOTYPE_H