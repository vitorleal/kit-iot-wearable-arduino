/*
 * MMA7660.cpp - Interface a MMA thermometer to an AVR via i2c
 * Version 0.1 - http://www.timzaman.com/
 * Copyright (c) 2011 Tim Zaman
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
 
#include "MMA7660.h"
#include "Arduino.h"
#include <Wire.h>

#define MMA7660_ADDRESS 0x4C //This is the I2C address for our chip.

int MMA_XYar[64] = {0,3,5,8,11,14,16,19,22,25,28,31,34,38,41,45,49,53,58,63,70,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-80,-70,-63,-58,-53,-49,-45,-41,-38,-34,-31,-28,-25,-22,-19,-16,-14,-11,-8,-5,-3}; 
int MMA_Zar[64] = {90,87,84,82,79,76,74,70,68,65,62,59,56,52,49,45,41,37,32,27,20,10,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-10,-20,-27,-32,-37,-41,-45,-49,-52,-56,-59,-52,-65,-68,-71,-74,-76,-79,-82,-85,-87};
 
/* PUBLIC METHODS */

MMA::MMA()
{
}

void MMA::init()
{
	Wire.begin();       //Initiate the Wire library and join the I2C bus as a master	
	MMA7660_I2C_SEND(0x07,0x00); // Setting up MODE to Stand by to set SR
    MMA7660_I2C_SEND(0x06,0x10);
    MMA7660_I2C_SEND(0x08,0x00);  // Setting up SR register to 120 samples active and auto sleep mode
    MMA7660_I2C_SEND(0x07,0x01); //Setting up MODE Active to START measures
}

// This can be called at 100ms intervals to get new data
void MMA::getValues(int *x,int *y,int *z)
{
  Wire.beginTransmission(MMA7660_ADDRESS);
  Wire.write(0x00);  // register to read
  Wire.endTransmission();
  Wire.requestFrom(MMA7660_ADDRESS, 3); // read a byte

   if(Wire.available()){
     *x=Wire.read();
     *y=Wire.read();
     *z=Wire.read();
   }
   *x=MMA_XYar[*x];
   *y=MMA_XYar[*y];
   *z=MMA_Zar[*z];
}


void MMA::MMA7660_I2C_SEND(unsigned char REG_ADDRESS, unsigned  char DATA){  //SEND data to MMA7660
  Wire.beginTransmission(MMA7660_ADDRESS);
  Wire.write(REG_ADDRESS);
  Wire.write(DATA);
  Wire.endTransmission();
}


// Set the default object
MMA MMA7660 = MMA();










