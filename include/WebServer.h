#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "Scale.h"
#include "FlowRate.h"
#include "BluetoothScale.h"

extern float calibrationFactor;

void setupWebServer(Scale &scale, FlowRate &flowRate, BluetoothScale &bluetoothScale);

#endif
