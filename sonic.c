/*-----------------------------------------------------

Jannik Beibl 2020 (jan.beibl.19@lehre.mosbach.dhbw.de)

A library for accessing HC-SR04 Ultrasonic sensors utilizing a Octosonar v2.1 board.

Compile with -lwiringPi -lpigpio

gcc -o sonic sonic.c -lwiringPi -lpipgio
sudo ./sonic (must run as sudo because of pigpiod)

Interrupt pin is BCM25 (GPIO_25)

prerelease unstable
-----------------------------------------------------*/

#include <stdio.h>
#include <stdint.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <pigpio.h>

#define ADDRESS 0x38

int handle;
int state = 0;
long long ticks;
long diff;

// Writes the desired byte to the PCF8574 expander.
int setBytes(uint8_t bytes) 
{
   if(handle != 0) // Check if I2C handle is available.
   {
      wiringPiI2CWrite(handle, bytes);
   }
}

// The interrupt service routine to catch rising and falling edges on the interrupt pin.
void ISR(int pin, int level, uint32_t tick)
{
   if(level == 1) // Rising edge
   {
      ticks = tick; 
   }

   if(level == 0) // Falling edge
   {
      diff = tick - ticks;
   }
}

// Sends a trigger signal (logic 0) to the specific sensor.
// The tri-state buffer is set into a low impedance mode.
double measure(uint8_t port)
{
      setBytes(0xff); // Set all outputs high
      delay(10);

      setBytes(~(1 << port) & 0xff); // Pull specific sensor trigger down.
      diff = 0;
      
      for(int i = 0; i<200; i++) // Wait for max 200ms (timeout > 400cm)
      {
         if(diff != 0) 
         {
            break;
         }  

         delay(1);
      }

      if(diff != 0)
      {
         return diff * (340.29 / 20000.0); // Calculate distance by time of flight.
      }
      else 
      {
         return 0;
      }
   
}

int main(int argc, char *argv[])
{
   gpioInitialise();
   gpioSetMode(25, PI_INPUT); // Set pull down on board.
   gpioSetAlertFunc(25, ISR); // Init ISR.

   handle = wiringPiI2CSetup(ADDRESS);

   while(1)
   {
      for(int i = 0; i < 4; i++)
      {
         //delay(100);
         double dist = measure(i);

         if(dist == 0) // If timeout trigger same sensor again.
         {
            i--;
         }
         else 
         {
            printf("%d: %f  ", i+1, dist);
         }
      }

      printf("\n");
   }

   return 0;
}
