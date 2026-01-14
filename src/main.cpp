#include <Arduino.h>
#include "config.h"
#include "setup.h"
#include "loop.h"

void setup() {
  setupInit();
}

void loop() {
  loopMain();
}
