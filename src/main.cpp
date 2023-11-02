/**************************************************************************************************
Nom du fichier : main.cpp
Auteur : Maxime Boucher
Date de création : 2023-10-17

Description : Librairie permettant la mise en application du capteur de ligne

Notes :

Modifications :

***************************************************************************************************/

// *************************************************************************************************
//  INCLUDES
// *************************************************************************************************

#include "capteurLigne.h"
#include "PIDLigne.h"
#include "Move.h"
#include <LibRobus.h>
#include <float.h>
#include <mathX.h>
#include <movement.h>
#include <couleur.h>
#include <sifflet.h>

using namespace Movement;

// *************************************************************************************************
//  CONSTANTES
// *************************************************************************************************

#define CLAW_SERVO SERVO_1
#define ARM_SERVO SERVO_2
const uint8_t LINE_FOLLOWER_PINS[] = {38, 41, 43, 42, 49, 48, 47, 46};

int movementIndex = -1;

// *************************************************************************************************
//  FONCTIONS LOCALES
// *************************************************************************************************

// *************************************************************************************************
//  STRUCTURES ET UNIONS
// *************************************************************************************************

enum ClawState
{
    OPENED,
    CLOSED,
};

enum ArmState
{
    EXTENDED_RIGHT,
    NOT_EXTENDED,
    EXTENDED_LEFT,
};

// *************************************************************************************************
// VARIABLES GLOBALES
// *************************************************************************************************

ArmState armState = NOT_EXTENDED;
ClawState clawState = OPENED;

void setupPID()
{
    setPIDVelocity(0.15, 0.001, 0);
    setPIDAngular(0.1, 0.001, 0);
}

void setupServo()
{
    SERVO_Enable(ARM_SERVO);
    SERVO_SetAngle(ARM_SERVO, 90);
    SERVO_Enable(CLAW_SERVO);
}

void setClaw(ClawState state)
{
    clawState = state;
}

void setup()
{
    setupPID();
    setupServo();
    BoardInit();
    // Good value : 8, 0.001, 0.15
    PIDLigne::initPID(2.7387791339, 2.625137795, 8, 0, 0.1, LINE_FOLLOWER_PINS, 45);
    Serial.begin(9600);

    // CapteurLigne::initLine(LINE_FOLLOWER_PINS, 45);
    ENCODER_Reset(RIGHT);
    ENCODER_Reset(LEFT);
    Sifflet::init();

    setClaw(CLOSED);

    for (int i; i < 100; i++)
        CapteurLigne::isVariation(1);
}

bool activateServoForDistance(float id, float distance, float targetAngle, float resetAngle)
{
    static float initialDistance = INACTIVE;

    if (Movement::isInactive(initialDistance))
    {
        SERVO_SetAngle(id, targetAngle);
    }

    bool distanceReached = Movement::distanceFlag(distance, &initialDistance);

    SERVO_SetAngle(id, distanceReached ? resetAngle : targetAngle);

    return distanceReached;
}

void setArm(ArmState state)
{
    armState = state;
}

void loopLineFollower()
{
    PIDLigne::WheelVelocities wheelVelocities = PIDLigne::computeWheelSpeed(WHEEL_BASE_DIAMETER, 10.0);
    setWheelSpeed(wheelVelocities.rightWheelSpeed, wheelVelocities.leftWheelSpeed);
}

void followWall(float id, float velocity, float radius = 13.3, float distance = 425.0)
{
    float baseAngularVelocity = velocity / radius;

    float angularVelocity = sigmoid(ROBUS_ReadIR(id), distance, 1, 25.0, -2) * (id == RIGHT ? baseAngularVelocity : -baseAngularVelocity);

    Movement::move(velocity, angularVelocity);
}

void updateServos()
{
    switch (armState)
    {
    case EXTENDED_RIGHT:
        if (activateServoForDistance(ARM_SERVO, 85, 180, 40))
        {
            armState = NOT_EXTENDED;
        }
        break;

    case EXTENDED_LEFT:
        if (activateServoForDistance(ARM_SERVO, 85, 0, 40))
        {
            armState = NOT_EXTENDED;
        }
        break;
    }

    SERVO_SetAngle(CLAW_SERVO, clawState == OPENED ? 75 : 110);
}

void updateEverything()
{
    switch (armState)
    {
    case EXTENDED_RIGHT:
        if (activateServoForDistance(ARM_SERVO, 85, 180, 40))
        {
            armState = NOT_EXTENDED;
        }
        break;

    case EXTENDED_LEFT:
        if (activateServoForDistance(ARM_SERVO, 85, 0, 40))
        {
            armState = NOT_EXTENDED;
        }
        break;
    }

    // SERVO_SetAngle(CLAW_SERVO, clawState == OPENED ? 75 : 110);
    updateServos();
    updatePIDs();
    CapteurLigne::isVariation(1);
}

byte state = 0; // Remettre à zéro pour le sifflet

void loop()
{
    // Serial.println(state);
    delay(5);
    switch (state)
    {
    case 0: // Attente du sifflet, détection de la couleur de départ

        if (!Sifflet::active)
        {
            if (Sifflet::update(1.0))
            {
                Sifflet::trigger();
            }
        }
        if (Sifflet::active)
        {
            state = 2;
        }
        break;

    case 1: // Suivi du vert, détection du cup
        static byte state2 = 0;
        switch (state2)
        {
        case 0:
            moveUnited(10, 0.5, 0);
            if (CapteurLigne::isVariation(100))

                state2++;
            break;
        case 1:
            if (forward(10, 2.7387791339))
                state2++;
            break;
        case 2:
            if (rotate(15, 18, (PI / 2.0)))
                state2++;
            break;
        case 3:
            if (forward(15, 24))
                state2++;
            break;
        case 4:
            if (rotate(15, 18, (PI / 2.0) - 0.1))
                state2++;
            break;
        case 5:
            if (forward(15, 80))
                state2++;
            if (ROBUS_ReadIR(LEFT) > 500)
            {
                setArm(EXTENDED_LEFT);
            }
            break;
        case 6:
            forward(10, INFINITY);
            if (ROBUS_ReadIR(LEFT) > 500)
            {
                setArm(EXTENDED_LEFT);
            }
            if (CapteurLigne::isVariation(150))
            {
                forward(10, INFINITY, true);
                setArm(EXTENDED_RIGHT);
                state2++;
            }
            break;
        case 7:
            if (forward(5, 10))
            {
                state = 3;
            }
            break;
        }
        break;

    case 2: // Suivi du jaune, détection du cup
        static byte state3 = 0;
        switch (state3)
        {
        case 0:
            moveUnited(10, 0.5, 0);
            if (CapteurLigne::isVariation(110))
                state3++;
            break;
        case 1:
            if (forward(10, 2.7387791339))
                state3++;
            break;

        case 2:
            if (rotate(15, 31, PI / 2.0))
                state3++;
            break;

        case 3:
            if (forward(15, 24))
                state3++;
            break;
        case 4:
            if (rotate(15, 32, (PI / 2.0) + 0.1))
                state3++;
            break;
        case 5:
            if (forward(15, 80))
                state3++;
            if (ROBUS_ReadIR(RIGHT) > 500)
            {
                setArm(EXTENDED_RIGHT);
            }
            break;
        case 6:
            forward(15, INFINITY);
            if (ROBUS_ReadIR(RIGHT) > 500)
            {
                setArm(EXTENDED_RIGHT);
            }
            if (CapteurLigne::isVariation(50))
            {
                forward(15, INFINITY, true);
                setArm(EXTENDED_LEFT);
                state3++;
            }
            break;
        case 7:
            if (rotate(15, 6, (2 * PI) / 3))
            {
                state3++;
            }
            break;
        case 8:
            if (rotate(15, -3, PI / 2))
            {
                state = 3;
            }

            break;
        }
        break;

    case 3: // Suivi de la ligne, détection de retour à la couleur

        loopLineFollower();
        break;

    case 4: // On fait un tour et puis le shortcut
    case 5:

        followWall(RIGHT, 15);

        if (CapteurLigne::isVariation(110))
        {
            state++;
        }
        break;

    case 6: // Feni

        move(0, 0);

        if (ROBUS_IsBumper(3))
        {
            setClaw(CLOSED);
            state = 0; // On restart le parcours
        }
        break;
    case 87:
        Sifflet::update(1.0);
        break;
    case 88:
        move(2, 0);
        break;
    case 89:
        CapteurLigne::isVariation(100);
        break;
    case 90:
        Serial.println(Couleur::Get());
        break;
    }

    updateEverything();
}
