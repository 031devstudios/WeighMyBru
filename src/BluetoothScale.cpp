#include "BluetoothScale.h"
#include <Arduino.h>

// UUIDs for WeighMyBru protocol - unique to avoid conflicts with Bookoo scales
const char* BluetoothScale::SERVICE_UUID = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
const char* BluetoothScale::WEIGHT_CHARACTERISTIC_UUID = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E";
const char* BluetoothScale::COMMAND_CHARACTERISTIC_UUID = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E";

BluetoothScale::BluetoothScale() 
    : scale(nullptr), server(nullptr), service(nullptr), 
      weightCharacteristic(nullptr), commandCharacteristic(nullptr),
      advertising(nullptr), deviceConnected(false), oldDeviceConnected(false),
      lastHeartbeat(0), lastWeightSent(0), lastWeight(0.0f) {
}

BluetoothScale::~BluetoothScale() {
    end();
}

void BluetoothScale::begin(Scale* scaleInstance) {
    scale = scaleInstance;
    initializeBLE();
    startAdvertising();
    Serial.println("BluetoothScale: Started advertising as WeighMyBru");
}

void BluetoothScale::end() {
    if (server) {
        stopAdvertising();
        BLEDevice::deinit();
        server = nullptr;
        service = nullptr;
        weightCharacteristic = nullptr;
        commandCharacteristic = nullptr;
        advertising = nullptr;
    }
}

void BluetoothScale::initializeBLE() {
    // Initialize BLE Device with WeighMyBru name
    BLEDevice::init("WeighMyBru");
    
    // Create BLE Server
    server = BLEDevice::createServer();
    server->setCallbacks(this);
    
    // Create BLE Service
    service = server->createService(SERVICE_UUID);
    
    // Create Weight Characteristic (for notifications)
    weightCharacteristic = service->createCharacteristic(
        WEIGHT_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_NOTIFY |
        BLECharacteristic::PROPERTY_INDICATE
    );
    
    // Add Client Characteristic Configuration Descriptor for notifications
    BLE2902* weightDescriptor = new BLE2902();
    weightDescriptor->setNotifications(true);
    weightCharacteristic->addDescriptor(weightDescriptor);
    
    // Create Command Characteristic (for receiving commands)
    commandCharacteristic = service->createCharacteristic(
        COMMAND_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_WRITE_NR |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    commandCharacteristic->setCallbacks(this);
    
    // Start the service
    service->start();
    
    // Get advertising object
    advertising = BLEDevice::getAdvertising();
    advertising->addServiceUUID(SERVICE_UUID);
    advertising->setScanResponse(false);
    advertising->setMinPreferred(0x0);
}

void BluetoothScale::startAdvertising() {
    if (advertising) {
        advertising->start();
    }
}

void BluetoothScale::stopAdvertising() {
    if (advertising) {
        advertising->stop();
    }
}

void BluetoothScale::update() {
    uint32_t now = millis();
    
    // Handle connection state changes
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // Give the bluetooth stack time to get ready
        server->startAdvertising();
        Serial.println("BluetoothScale: Start advertising after disconnect");
        oldDeviceConnected = deviceConnected;
    }
    
    if (deviceConnected && !oldDeviceConnected) {
        Serial.println("BluetoothScale: Client connected");
        oldDeviceConnected = deviceConnected;
        lastHeartbeat = now;
        
        // Send initialization response for WeighMyBru client
        delay(100); // Give time for connection to stabilize
        sendNotificationRequest();
    }
    
    if (deviceConnected) {
        // Send weight updates - faster for GaggiMate brewing applications
        if (scale && (now - lastWeightSent >= WEIGHT_SEND_INTERVAL)) {
            float currentWeight = scale->getCurrentWeight();
            // Send all weight updates for real-time brewing feedback
            sendWeightNotification(currentWeight);
            lastWeight = currentWeight;
            lastWeightSent = now;
        }
        
        // Send heartbeat
        if (now - lastHeartbeat >= HEARTBEAT_INTERVAL) {
            sendHeartbeat();
            lastHeartbeat = now;
        }
    }
}

bool BluetoothScale::isConnected() {
    return deviceConnected;
}

void BluetoothScale::sendWeightNotification(float weight) {
    if (!deviceConnected || !weightCharacteristic) return;
    
    // Convert weight to integer (grams * 100 for 0.01g precision)
    int32_t weightInt = (int32_t)(weight * 100);
    
    // Create weight message following WeighMyBru protocol
    uint8_t payload[PROTOCOL_LENGTH] = {0};
    
    // Header
    payload[0] = PRODUCT_NUMBER; // Product number
    payload[1] = static_cast<uint8_t>(WeighMyBruMessageType::WEIGHT); // Message type
    payload[2] = 0x00; // Reserved
    payload[3] = 0x00; // Reserved
    payload[4] = 0x00; // Reserved
    payload[5] = 0x00; // Reserved
    
    // Sign (positive = 43, negative = 45)
    payload[6] = (weightInt >= 0) ? 43 : 45;
    
    // Weight data (3 bytes, big endian)
    uint32_t absWeight = abs(weightInt);
    payload[7] = (absWeight >> 16) & 0xFF;
    payload[8] = (absWeight >> 8) & 0xFF;
    payload[9] = absWeight & 0xFF;
    
    // Fill remaining bytes with zeros
    for (int i = 10; i < PROTOCOL_LENGTH - 1; i++) {
        payload[i] = 0x00;
    }
    
    // Calculate and set checksum (last byte)
    payload[PROTOCOL_LENGTH - 1] = calculateChecksum(payload, PROTOCOL_LENGTH - 1);
    
    // Send notification
    weightCharacteristic->setValue(payload, PROTOCOL_LENGTH);
    weightCharacteristic->notify();
    
    Serial.printf("BluetoothScale: Sent weight %.2fg\n", weight);
}

void BluetoothScale::sendHeartbeat() {
    if (!deviceConnected || !commandCharacteristic) return;
    
    // Send system heartbeat message
    uint8_t payload[] = {0x02, 0x00};
    sendMessage(WeighMyBruMessageType::SYSTEM, payload, sizeof(payload));
    
    Serial.println("BluetoothScale: Heartbeat sent");
}

void BluetoothScale::sendNotificationRequest() {
    if (!deviceConnected) return;
    
    // Send notification request for WeighMyBru initialization
    uint8_t payload[] = {0x06, 0x00, 0x00, 0x00, 0x00, 0x00};
    sendMessage(WeighMyBruMessageType::SYSTEM, payload, sizeof(payload));
    
    Serial.println("BluetoothScale: Notification request sent");
}

void BluetoothScale::sendMessage(WeighMyBruMessageType msgType, const uint8_t* payload, size_t length) {
    if (!deviceConnected || !commandCharacteristic) return;
    
    // Create message buffer
    uint8_t message[length + 1];
    
    // Copy payload
    memcpy(message, payload, length);
    
    // Calculate and append checksum
    message[length] = calculateChecksum(message, length);
    
    // Send via command characteristic
    commandCharacteristic->setValue(message, length + 1);
}

uint8_t BluetoothScale::calculateChecksum(const uint8_t* data, size_t length) {
    uint8_t checksum = data[0];
    for (size_t i = 1; i < length; i++) {
        checksum ^= data[i];
    }
    return checksum;
}

void BluetoothScale::handleTareCommand() {
    if (scale) {
        Serial.println("BluetoothScale: Executing tare command");
        scale->tare(10);
        
        // Send tare confirmation
        uint8_t payload[] = {0x03, 0x0a, 0x01, 0x00, 0x00};
        sendMessage(WeighMyBruMessageType::SYSTEM, payload, sizeof(payload));
    }
}

void BluetoothScale::processIncomingMessage(uint8_t* data, size_t length) {
    if (length < 2) return;
    
    uint8_t productNumber = data[0];
    WeighMyBruMessageType messageType = static_cast<WeighMyBruMessageType>(data[1]);
    
    Serial.printf("BluetoothScale: Received message - Product: 0x%02X, Type: 0x%02X\n", 
                  productNumber, static_cast<uint8_t>(messageType));
    
    if (messageType == WeighMyBruMessageType::SYSTEM) {
        // Check for tare command
        if (length >= 6 && data[2] == 0x0a && data[3] == 0x01) {
            handleTareCommand();
        }
    }
}

// BLE Server Callbacks
void BluetoothScale::onConnect(BLEServer* pServer) {
    deviceConnected = true;
    BLEDevice::stopAdvertising();
    Serial.println("BluetoothScale: Device connected");
}

void BluetoothScale::onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("BluetoothScale: Device disconnected");
}

// BLE Characteristic Callbacks
void BluetoothScale::onWrite(BLECharacteristic* pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    
    if (value.length() > 0) {
        uint8_t* data = (uint8_t*)value.data();
        size_t length = value.length();
        
        Serial.printf("BluetoothScale: Received %d bytes\n", length);
        processIncomingMessage(data, length);
    }
}
