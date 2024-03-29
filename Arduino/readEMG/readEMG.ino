#include "EMG_Sensor.h"
#include "EMGFilters.h"

#define SENSOR1_PIN A1
#define SENSOR2_PIN A2

#define ERROR_LED 13

// Modify value according to number of sensors used
#define SENSOR_COUNT 2
// Set 1 for Serial Plotting and 0 for Putty CSV Export
int enableSerialPlot = 1;

// Set 0 if Timing o/p need not be printed
#define TIMING_DEBUG 0

unsigned long runTime;
unsigned long timeBudget;

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// START OF TUNING PARAMETERS

const long intensityThreshold1 = 10000; // Threshold for hard-coded determination of low or high intensity
const long intensityThreshold2 = 10000;

const long durationThreshold1 = 900; // Threshold (ms) for the distinction between short and long signal
const long durationThreshold2 = 900;

const int averageLength = 1000; // Constant (need tuning?)
const int envelopeReach = 100; // May need tuning!!

// END OF TUNING PARAMETERS
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


int envelopeIndex1 = 0;
int valueArray1;
int temp1 = 0; 
int temp2 = 0; 
long averageVal1 = 0;
long envelopeVal = 0;
int signalReadings1[envelopeReach];
int eventFlag1 = 0; // Toggle between 0 and 1 if the envelope rises above 0 or falls back to 0 (start and end flag)
long eventArea1 = 0; // Variable to track the intensity (cumulative) as an event occurs
long eventStartTime1 = 0; // Variable to record the start time for any one event
long eventStopTime1 = 0; // Variable to record the stop time for any one event
long previousEnvelope1 = 0;
int gestureID = 0; // Initial state for the gesture identifier

long averageVal2 = 0;
int envelopeIndex2 = 0;
int signalReadings2[envelopeReach];
int eventFlag2 = 0; // Toggle between 0 and 1 if the envelope rises above 0 or falls back to 0 (start and end flag)
long eventArea2 = 0; // Variable to track the intensity (cumulative) as an event occurs
long eventStartTime2 = 0; // Variable to record the start time for any one event
long eventStopTime2 = 0; // Variable to record the stop time for any one event
long previousEnvelope2 = 0;

int channelID = 1; // Default value for channel selection


// discrete filters must works with fixed sample frequence
// our emg filter only support "SAMPLE_FREQ_500HZ" or "SAMPLE_FREQ_1000HZ"
// other sampleRate inputs will bypass all the EMG_FILTER
SAMPLE_FREQUENCY sampleRate = SAMPLE_FREQ_500HZ;

EMG_Sensor emg[SENSOR_COUNT] = {EMG_Sensor(SENSOR1_PIN, sampleRate, 10), EMG_Sensor(SENSOR2_PIN, sampleRate, 10)};

void setup()
{
  // open serial
  Serial.begin(500000);
  //Serial.println("<Arduino is ready>");

  pinMode(ERROR_LED, OUTPUT);
  pinMode(LED_BUILTIN,OUTPUT);
  pinMode(2,INPUT);
  pinMode(3,INPUT);
  pinMode(4,INPUT);
  pinMode(5,INPUT);
  pinMode(A0,INPUT);
  

  // setup for time cost measure
  // using micros()
  timeBudget = 1e6 / sampleRate;
  // micros will overflow and auto return to zero every 70 minutes

  initialiseSensors();
}

void loop()
{
  /* add main program code here */
  /*------------start here-------------------*/
  runTime = micros();


  // temp1 = streamSensorData(0);
  // temp2 = streamSensorData(1);

  temp1 = emg[0].readSensorData();
  temp2 = emg[1].readSensorData();
  
  averageVal1 = smoothing(temp1,1); // Obtain sensor 1 data
  averageVal2 = smoothing(temp2,2); // Obtain sensor 2 data
  envelopeVal = envelope(averageVal1,averageVal2);
  
  //Serial.println(averageVal1);
  

  // Switch toggling for channel labelling
  if(analogRead(A0)==0){
    channelID = 1; // Switch 'untoggled' state
  }
  
  if(analogRead(A0)==1023){
    channelID = 2; // Switch 'toggled' state
  }
  
  // Button press for gesture labelling
  if(digitalRead(2)==HIGH){
    gestureID = 1;
  }
  if(digitalRead(3)==HIGH){
    gestureID = 2;
  }
  if(digitalRead(4)==HIGH){
    gestureID = 3;
  }
  if(digitalRead(5)==HIGH){
    gestureID = 4;
  }


  

  if(envelopeVal == 1){ // If 1 then an event in channel 1 occured (Labelling)
    //Serial.print(1); // Printing the channel that detected an event
    //Serial.print(",");
    //Serial.print(channelID); // The ID of the channel that is the 'intended' signal mover
    //Serial.print(",");
    //Serial.println(gestureID); // The ID/type of the gesture or signal intended
  }

  if(envelopeVal == 2){ // If 2 then an event in channel 2 occured (Labelling)
    //Serial.print(2); // Printing the channel that detected an event
    //Serial.print(",");
    //Serial.print(channelID); // The ID of the channel that is the 'intended' signal mover
    //Serial.print(",");
    //Serial.println(gestureID); // The ID/type of the gesture or signal intended
  }

  if(envelopeVal == 3){ // If 3 then an event in both channels occured (Labelling)
    //Serial.print(3); // Printing the channel that detected an event
    //Serial.print(",");
    //Serial.print(channelID); // The ID of the channel that is the 'intended' signal mover
    //Serial.print(",");
    //Serial.println(gestureID); // The ID/type of the gesture or signal intended
  }

  

  runTime = micros() - runTime;

  if(TIMING_DEBUG)
    timingDebug();

  /*------------end here---------------------*/

  // In order to make sure the operating frequency of the code
  // matches the sampling rate

  maintainOperatingFrequency();
}

long smoothing(int temp1, int sensorChannel){
  long movingAverage; // Not needed to be global. Re-stated each call.
  int readings1[averageLength]; // Length of the averaging filter to be applied.
  int readings2[averageLength];
  int readIndex1 = 0;
  int readIndex2 = 0;
  long total1 = 0; // Reset for each call
  long total2 = 0;

  if(sensorChannel==1){ // If called with sensor channel 1
    total1 = total1 - readings1[readIndex1];
    readings1[readIndex1] = temp1;
    total1 = total1 + readings1[readIndex1];
    readIndex1 = readIndex1 + 1;

    if(readIndex1 >= averageLength){
      readIndex1 = 0;
    }

    movingAverage = total1/averageLength;
  }
  
  if(sensorChannel==2){ // If called with sensor channel 2
    total2 = total2 - readings2[readIndex2];
    readings2[readIndex2] = temp2;
    total2 = total2 + readings2[readIndex2];
    readIndex2 = readIndex2 + 1;

    if(readIndex2 >= averageLength){
      readIndex2 = 0;
    }

    movingAverage = total2/averageLength;
  }

  return movingAverage; // Returning new average value regardless of sensor channel
  
}

int envelope(long temp1, long temp2){

  long envelopePeakValue1=0; // Declaring a variable in which to store the maximum recorded value
  long tempStorage1 = 0;
  long eventDuration1 = 0;
  int printFlag1 = 0;
  int currentReading1=0;
  long currentTime1 = 0;

  long envelopePeakValue2=0; // Declaring a variable in which to store the maximum recorded value
  long tempStorage2 = 0;
  long eventDuration2 = 0;
  int printFlag2 = 0;
  int currentReading2=0;
  long currentTime2 = 0;


  int printFlagCombination = 0;
  
  int i; // For use as a counter / iterator

  // Channel 1 start
  signalReadings1[envelopeIndex1] = temp1; // Storing the most recent reading in the array
  envelopeIndex1 = envelopeIndex1+1;

  if(envelopeIndex1 >= envelopeReach){ // Wrap back around to the start of the array
    envelopeIndex1 = 0;
  }

  for(i=0;i<envelopeReach;i++){ // Finding the largest element in the current envelope reach
    currentReading1 = signalReadings1[i]; 
    if(currentReading1 > envelopePeakValue1){ // Comparing to the current largest value in the array
      envelopePeakValue1 = currentReading1; // If larger, assign as the new largest.
    }
  }

  //Serial.print(envelopePeakValue1);
  //Serial.print(",");

    // NOW CHECKING FOR EVENTS


  // CHANNEL 1: DURING AN EVENT
  if((eventFlag1 == 1)&&(envelopePeakValue1 > 0)){ // If the envelope is above zero (and during an event)
      eventArea1 = eventArea1 + (envelopePeakValue1*envelopePeakValue1); // Adding on the squared envelope value

      currentTime1 = millis(); // Recording the current elapsed time
      if((currentTime1 - eventStartTime1)>=durationThreshold1*1.5){ // Event been going on for a significant time
        Serial.print(1);
        Serial.println(",");
      }

  }

  // CHANNEL 1: START FLAG DETECTED
  if((previousEnvelope1 == 0)&&(envelopePeakValue1 > 0)){ // If the envelope has just picked up above 0 (CAN ONLY BE TRUE ONCE)
    eventFlag1 = 1; // Signal the start of an event
    eventArea1 = eventArea1 + (envelopePeakValue1*envelopePeakValue1); // Adding on the squared envelope value
    eventStartTime1 = millis(); // Or some function to record the current time at the event start point
  }

  // CHANNEL 1: STOP FLAG DETECTED
  if((previousEnvelope1 > 0)&&(envelopePeakValue1 == 0)&&(eventFlag1 == 1)){ // If the envelope has just fallen back to 0 (CAN ONLY BE TRUE ONCE)
    eventStopTime1 = millis();
    eventDuration1 = eventStopTime1 - eventStartTime1;
    eventFlag1 = 0;
    // Now need to save these features / export them
    //Serial.print("Intensity:");
    //Serial.print(eventArea1);
    //Serial.print(",");
    //Serial.print("Duration:");
    //Serial.print(eventDuration1);
    //Serial.print(",");

    if(eventDuration1<=durationThreshold1){ // Under the time threshold for a short signal
      if(eventArea1<=intensityThreshold1){ // Here we know channel 1, duration short, intensity low.
        Serial.print(2);
        Serial.println(",");

      }
      else if(eventArea1>intensityThreshold1){ // Here we know channel 1, duration is short, intensity high
        Serial.print(3);
        Serial.println(",");
      }
    }

    // No need for another case, because the long durations will currently be dealt with
    // in the 'during event' location. Decision making based upon stop flag of an already long event is
    // not necessary and would lead to uneccesary delays.

    eventDuration1=0; // Resetting trackers
    eventStartTime1=0;
    eventStopTime1=0;
    eventArea1=0;
    printFlag1 = 1;
  }

  tempStorage1 = envelopePeakValue1; // Store the current largest envelope value
  previousEnvelope1 = envelopePeakValue1;
  envelopePeakValue1 = 0; // Set the current best back to zero ready for the next function call

  

  // Channel 2 start
  signalReadings2[envelopeIndex2] = temp2; // Storing the most recent reading in the array
  envelopeIndex2 = envelopeIndex2+1;

  if(envelopeIndex2 >= envelopeReach){
    envelopeIndex2 = 0;
  }

  for(i=0;i<envelopeReach;i++){ // Finding the largest element in the current envelope reach
    currentReading2 = signalReadings2[i];
    if(currentReading2 > envelopePeakValue2){ // Comparing to the current largest value in the array
      envelopePeakValue2 = currentReading2; // If larger, assign as the new largest.
    }
  }

  //Serial.println(envelopePeakValue2);

  // NOW CHECKING FOR EVENTS

  // CHANNEL 2: DURING AN EVENT
  if((eventFlag2 == 1)&&(envelopePeakValue2 > 0)){ // If the envelope is above zero (and during an event)
      eventArea2 = eventArea2 + (envelopePeakValue2*envelopePeakValue2); // Adding on the squared envelope value

      currentTime2 = millis(); // Recording the current elapsed time
      if((currentTime2 - eventStartTime2)>=durationThreshold2*1.5){ // Event been going on for a significant time
        Serial.print(4);
        Serial.println(",");
      }

  }


  // CHANNEL 2: START FLAG DETECTED
  if((previousEnvelope2 == 0)&&(envelopePeakValue2 > 0)){ // If the envelope has just picked up above 0 (CAN ONLY BE TRUE ONCE)
    eventFlag2 = 1; // Signal the start of an event
    eventArea2 = eventArea2 + (envelopePeakValue2*envelopePeakValue2); // Adding on the squared envelope value
    eventStartTime2 = millis(); // Or some function to record the current time at the event start point
  }

  // CHANNEL 2: STOP FLAG DETECTED
  if((previousEnvelope2 > 0)&&(envelopePeakValue2 == 0)&&(eventFlag2 == 1)){ // If the envelope has just fallen back to 0 (CAN ONLY BE TRUE ONCE)
    eventStopTime2 = millis();
    eventDuration2 = eventStopTime2 - eventStartTime2;
    eventFlag2 = 0;
    // Now need to save these features / export them
    //Serial.print("Intensity:");
    //Serial.print(eventArea2);
    //Serial.print(",");
    //Serial.print("Duration:");
    //Serial.print(eventDuration2);
    //Serial.print(",");

    if(eventDuration2<=durationThreshold2){ // Under the time threshold for a short signal
      if(eventArea2<=intensityThreshold2){ // Here we know channel 2, duration short, intensity low.
        Serial.print(5);
        Serial.println(",");

      }
      else if(eventArea2>intensityThreshold2){ // Here we know channel 2, duration is short, intensity high
        Serial.print(6);
        Serial.println(",");
      }
    }

    eventDuration2=0; // Resetting trackers
    eventStartTime2=0;
    eventStopTime2=0;
    eventArea2=0;
    printFlag2 = 2;
  }

  tempStorage2 = envelopePeakValue2; // Store the current largest envelope value
  previousEnvelope2 = envelopePeakValue2;
  envelopePeakValue2 = 0; // Set the current best back to zero ready for the next function call

  printFlagCombination = printFlag1+printFlag2;

  return printFlagCombination;
    

}
  
