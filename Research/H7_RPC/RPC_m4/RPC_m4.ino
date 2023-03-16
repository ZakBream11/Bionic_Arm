#include "Arduino.h"
#include "RPC.h"

void setup()
{

  pinMode(LED_BUILTIN, OUTPUT);

  // Initialize RPC library; this also boots the M4 core
  RPC.begin();
  Serial.begin(115200);
  while (!Serial) {} // Wait until the Serial connection is ready

  // Both CPUs will execute this instruction, just at different times
  randomSeed(analogRead(A0)); // Initializes the pseudo-random number generator
}

void loop()
{
  // On M4 let's blink an LED. While it's blinking, the callSubstractFromM4() thread is running,
  // so it will execute roughly 3 times (2000 / 700 ms)
  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);

  int a = random(100);
  int b = random(100);
  // PRC.print works like a Serial port, but it needs a receiver (in this case the M7)
  // to actually print the strings to the Serial port
  RPC.println("M4: calling add with " + String(a) + " and " + String(b));
  // Let's invoke addOnM7() and wait for a result.
  // This will be delayed by the forced delay() in addOnM7() function
  // Exercise: if you are not interested in the result of the operation, what operation would you invoke?
  auto result = RPC.call("remoteAdd", a, b).as<int>();
  RPC.println("M4: Result is " + String(a) + " + " + String(b) + " = " + String(result));
}
