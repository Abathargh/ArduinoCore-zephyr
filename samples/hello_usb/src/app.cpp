/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <Arduino.h>

void setup() {
	SerialUSB.begin(115200);
}

void loop() {
	static unsigned long n = 0;
	SerialUSB.print("Hello from the Ventuno Q MCU - snap version");
	SerialUSB.println(n++);
	delay(1000);
}
