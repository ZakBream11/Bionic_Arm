

void motorSetup(){
  srvArrHand[0].attach(THUMB_PIN);
  srvArrHand[1].attach(THUMB_BASE_PIN);
  srvArrHand[2].attach(INDEX_PIN);
  srvArrHand[3].attach(MIDDLE_PIN);
  srvArrHand[4].attach(RING_PINKY_PIN);

  srvArrElbow[0].attach(ELBOW_L_PIN);
  srvArrElbow[1].attach(ELBOW_R_PIN);

  nsTimeBudget = 2000;
  gesturesPtr = new RealTimeGestures(srvArrHand, srvArrElbow, 2);

  // safe start position
  srvArrHand[0].write(180);
  //srvArrHand[0].write(0); // WITHOUT MOVEABLE THUMB
  srvArrHand[1].write(0);
  srvArrHand[2].write(180);
  srvArrHand[3].write(180);
  srvArrHand[4].write(180);

  srvArrElbow[0].write(100); // LEFT ELBOW MOTOR
  srvArrElbow[1].write(80); // RIGHT ELBOW MOTOR

  delay(1000);
  srvArrElbow[0].write(180); // LEFT ELBOW MOTOR
  srvArrElbow[1].write(0); // RIGHT ELBOW MOTOR
  delay(1000);
}

void motorLoop() {
  loopStartTime = micros();
  (*gesturesPtr).periodicUpdate();

  if (toggleFist) {
    (*gesturesPtr).toggleFist();
    toggleFist = 0;
  } else if (toggleElbow) {
    (*gesturesPtr).toggleElbow();
    toggleElbow = 0;
  }
    
  nsTimeLapsed = micros() - loopStartTime;
  if (nsTimeLapsed <  nsTimeBudget) {
    delayMicroseconds(nsTimeBudget - nsTimeLapsed);
  }
}

