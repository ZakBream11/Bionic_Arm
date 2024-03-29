void emgSetup()
{

  pinMode(ERROR_LED, OUTPUT);
  pinMode(LED_BUILTIN,OUTPUT);
  pinMode(A0,INPUT);
  

  // setup for time cost measure
  // using micros()
  timeBudget = 1e6 / sampleRate;
  // micros will overflow and auto return to zero every 70 minutes

  initialiseSensors();
}

void emgLoop()
{
  /* add main program code here */
  /*------------start here-------------------*/
  runTime = micros();


  // temp1 = streamSensorData(0);
  // temp2 = streamSensorData(1);

  temp1 = emg[0].readSensorData();
  temp2 = emg[1].readSensorData();

  Serial.print("Sensor_1:");
  Serial.println(temp1);
  Serial.print("Sensor_2:");
  Serial.println(temp2);
  
  averageVal1 = smoothing(temp1,1); // Obtain sensor 1 data
  averageVal2 = smoothing(temp2,2); // Obtain sensor 2 data
  envelopeVal = envelope(averageVal1,averageVal2);
  

  Serial.print("Avg_1:");
  Serial.println(averageVal1);
  Serial.print("Avg_2:");
  Serial.println(averageVal2);

  updateControlSignal(envelopeVal);

  runTime = micros() - runTime;

  if(TIMING_DEBUG)
    timingDebug();

  /*------------end here---------------------*/

  // In order to make sure the operating frequency of the code
  // matches the sampling rate

  maintainOperatingFrequency();
}



// On change of control signal set the flag
void updateControlSignal(int controlSignal)
{
 // dev Serial.println("Control signal received: ");
 // dev Serial.println(controlSignal);

  if (prevControlSignal != controlSignal)
  {
    switch (controlSignal)
    {
    case 3:
      if (!toggleFist)
        toggleFist = 1;
      break;
    case 6:
      if (!toggleElbow)
        toggleElbow = 1;
      break;
    case 0:
     // dev Serial.println("Received Control Signal 0");
    default:
     // dev Serial.println("Invalid input");
      break;
    }
    prevControlSignal = controlSignal;
  }
}


long smoothing(int temp1, int sensorChannel){
  long movingAverage; // Not needed to be global. Re-stated each call.
  
  
  long total1 = 0; // Reset for each call
  long total2 = 0;

  if(sensorChannel==1){ // If called with sensor channel 1
    
    readings1[readIndex1] = temp1;
    
    for(int i=0;i<averageLength;i++){
      total1 = total1 + readings1[i];
    }

    readIndex1+=1;

    if(readIndex1 >= averageLength-1){
      readIndex1 = 0;
    }

    movingAverage = total1/averageLength;
  }
  
  if(sensorChannel==2){ // If called with sensor channel 2
    
    readings2[readIndex2] = temp1;
    
    for(int i=0;i<averageLength;i++){
      total2 = total2 + readings2[i];
    }

    readIndex2+=1;

    if(readIndex2 >= averageLength-1){
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

  int controlSig=0;


  int printFlagCombination = 0;
  
  int i; // For use as a counter / iterator

 // dev Serial.println("Channel 1 averaged: ");
 // dev Serial.println(temp1);
 // dev Serial.println("Channel 2 averaged: ");
 // dev Serial.println(temp2);

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
      //Serial.println(envelopePeakValue1);
      currentTime1 = millis(); // Recording the current elapsed time
      if((currentTime1 - eventStartTime1)>=durationThreshold1*1.5){ // Event been going on for a significant time
        //Serial.println(1);
        
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
   //Serial.println(eventArea1);
    if(eventDuration1<=durationThreshold1){ // Under the time threshold for a short signal
      if(eventArea1<=intensityThreshold1){ // Here we know channel 1, duration short, intensity low.
        //Serial.println(2);
        

      }
      else if(eventArea1>intensityThreshold1){ // Here we know channel 1, duration is short, intensity high
        controlSig=3;
       // dev Serial.println("env (3): " + String(controlSig));
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
      //Serial.println(eventArea2);
      currentTime2 = millis(); // Recording the current elapsed time
      if((currentTime2 - eventStartTime2)>=durationThreshold2*1.5){ // Event been going on for a significant time
        //Serial.println(4);
        
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

    //Serial.println(eventArea2);
    if(eventDuration2<=durationThreshold2){ // Under the time threshold for a short signal
      if(eventArea2<=intensityThreshold2){ // Here we know channel 2, duration short, intensity low.
        //Serial.println(5);
        

      }
      else if(eventArea2>intensityThreshold2){ // Here we know channel 2, duration is short, intensity high
        //// dev Serial.println(6);
        controlSig=6;
       // dev Serial.println("env (6): " + String(controlSig));
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

  //printFlagCombination = printFlag1+printFlag2;

 // dev Serial.println("env (default): " + String(controlSig));

 
  Serial.print("Env_1:");
  Serial.println(previousEnvelope1);
  Serial.print("Env_2:");
  Serial.println(previousEnvelope2);

  return controlSig;
    

}
  
