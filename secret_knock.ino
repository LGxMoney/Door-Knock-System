#include <RH_ASK.h>
#include <SPI.h>        // Not actually used but needed to compile

/* 
   Secret Knock Sensor Transmitter

   Listens for specified knock pattern to send a message to a reciever
   that opens a door from the inside. 
   
   Using a piezo element to detect knocks on a pad, the software listens 
   for the intervals between each detected knock and stores it in a buffer
   to analyze when the attempt is complete. If the attempt is correct, it 
   validates it and sends a signal to the reciever letting it know to open
   the door.

   Hardware used: 
    -Arduino Nano (any I/O microcontroller could work for this project)
    -Piezo Element (with 1M Ohm)
    -433 MHz RF transmitter
    -LED's (with 220 Ohm resistors)
    -Jumper Wires
   
   Created by Garrett Money
   12/18/2018
*/

/* -------------- Hardware and pinout Variables Setup ----------------- */
const int ledDisplay[7] = { 5,6,7,8,9,10,11 }; //pinOut array for the LED display
const int knockSensor = A4;                    //the piezo is connected to analog pin 0
int sensorReading = 0;                         //variable to store the value read from the knockSensor pin
const int threshold = 20;                      //threshold value to decide when the detected sound is a knock or not

RH_ASK driver;                                 //Create the Radio Head driver to control RF Transmitter


/* ------------------ DataStructures and Variables Setup ---------------*/
double leniencyInterval = .23;            //Allows some cushion for differences in tempos
double listener[7];                       //The buffer used to store knocks and compare against the knock pattern matrix
double timer = 0;                         //custom timer used to measure the intervals
int knockIndex = 0;                       //Keeps track of which knock we are on according to the listener buffer (columns in knockPattern)
int patternIndex = 0;                     //Keeps track of which knock pattern we are going to accept (this currently does not change)


//Stores recorded intervals between knocks. This is what we want our attempts to sound like to send the signal.
double knockPattern[3][7] = { {0.01, 0.26, 0.12, 0.02, 0.29, 0.65, 0.28}, 
                              {0.01, 0.26, 0.12, 0.02, 0.29, 0.65, 0.28},
                              {0.01, 0.26, 0.12, 0.02, 0.29, 0.65, 0.28} }; 


/* ------------- Statistics and Recording Variables --------------------*/
int attempts = 0;                         //Keeps track of how many attempts since boot
double avg[7];                            //Used for averaging the desired pattern over however many attemps
bool active = false;                      //Used to determine when to start the timer


/*------------------------------Functions and Code ---------------------------------------*/

/**
 * Setup initializes all of our pinouts
 * starts a serial port for debugging
 * and ensures our Radio Head Transmitter is functioning
 */
void setup() 
{
  //initialize all LED pins to output
  for(int i = 0; i < 7; i++)
  {
    pinMode(ledDisplay[i], OUTPUT);
  }

  //Used for debugging on the Serial Monitor
  Serial.begin(9600);       

  //If the initialization of the Radio fails, print error to monitor
  if (!driver.init())
  {
    Serial.println("init failed");
  }
}

/**
 * Loop runs all of our main logic and calls other helper functions
 * when needed. Runs directly after setup.
 */
void loop() 
{
   //Read the sensor and store it in the variable sensorReading:
   sensorReading = analogRead(knockSensor);

  //If the first knock was big enough to pass our threshold (20), listen for more
  if (sensorReading >= threshold) 
  {
    active = true;                             //begin listening with timer
    digitalWrite(ledDisplay[0], HIGH);         //update first pin

    //since someone is attempting a knock pattern, listen for more until reset or success
    while(active)
    {
      sensorReading = analogRead(knockSensor); //reinitialize knock sensor to listen for next knock
      timer += .01;                            //update timer (measures in 10ms)
      delay(10);                               //allow time to pass to sync with timer variable
      
      if(timer >= 3.00)                        //timout on the attempt (reset all)
      {
        sensorReading = 0;
        verifyPattern();
        reset(0);
      }

      //Read subsequent knock
      if (sensorReading >= threshold) 
      {
         delay(150);
         sensorReading = 0;
         listener[knockIndex] = timer;
         Serial.print("knock: ");
         Serial.println(knockIndex);
         
         digitalWrite(ledDisplay[knockIndex], HIGH);
         timer = 0;
         knockIndex = (knockIndex + 1);

         if (knockIndex >= 7)
         {
            if(verifyPattern())
            {
              reset(1);
            }
            active = false;
            knockIndex = 0;
            testAvg();
            initializeListener();
         }
       }
    }
  }
}

/**
 * Verifies the attemptedPattern
 */
bool verifyPattern()
{  
  bool validAttempt = true;
  
  for(int i = 0; i < 7; i++)
  {
    double high = knockPattern[patternIndex][i] + leniencyInterval;
    double low = knockPattern[patternIndex][i] - leniencyInterval;
    printResults(high, low, i);
    if(listener[i] <= low || listener[i] >= high)
    {
      validAttempt = false;
    }
  }
  if(validAttempt)
  {
    Serial.println("CORRECT KNOCK PATTERN!!!");
    openDoor();
  }
  else
  {
     Serial.println("INCORRECT KNOCK PATTERN!!!");
     reset(0);
  }
  return validAttempt;
}

void printResults(double high, double low, int i)
{
    Serial.print("[");
    Serial.print(high);
    Serial.print(", ");
    Serial.print(low);
    Serial.print("]"); 
    Serial.println();
    Serial.print("Actual: ");
    Serial.print(listener[i]);
    Serial.println();
    Serial.println();
}

void reset(int status)
{
  if (status == 0)
  {
        Serial.println("Failure - RESETTING!!!");
        attempts += 1;
        sensorReading = 0;
        timer = 0;
        active = false;
        
        for(int i = knockIndex; i >= 0; i--)
        {
          digitalWrite(ledDisplay[i], LOW);
          delay(40);
        }
        knockIndex = 0;
  }
  if (status == 1)
  {
        Serial.println("Success - RESETTING!!!");
        attempts += 1;
        sensorReading = 0;
        timer = 0;
        active = false;
        knockIndex = 0;
        for(int j = 0; j < 5; j++)
        {
            for(int j = 0; j < 7; j++)
            {
              digitalWrite(ledDisplay[j], LOW);
            }
            delay(300);
            for(int j = 0; j < 7; j++)
            {
              digitalWrite(ledDisplay[j], HIGH);
            }
            delay(300);
            for(int j = 0; j < 7; j++)
            {
              digitalWrite(ledDisplay[j], LOW);
            }
        }
  }
}

void initializeListener()
{   
    for(unsigned int i = 0; i < sizeof(listener)/4; i++)
    {
       listener[i] = 0;
    }
}

void openDoor()
{
  const char *msg = "Let's open this bitch";
    driver.send((uint8_t *)msg, strlen(msg));
    driver.waitPacketSent();
  Serial.println("opening door");
}

void testAvg()
{
  Serial.println("Full pattern: ");
    for(unsigned int i = 0; i < sizeof(avg)/4; i++)
    {
       avg[i] = avg[i] + listener[i];
       Serial.print(listener[i]);
       Serial.print(" ");
    }
    attempts++;
    Serial.println();
    Serial.println("Number of attempts: ");
    Serial.print(attempts);
    Serial.println();

    if(attempts%30 == 10)
    {
      for(unsigned int i = 0; i < sizeof(avg)/4; i++)
      {
       avg[i] = avg[i] / attempts;
      }
      Serial.println();
      for(unsigned int i = 0; i < sizeof(avg)/4; i++)
      {
         Serial.print("Averages = ");
         Serial.print(avg[i]);
         Serial.print(" ");
      }
      
    }
}
