#pragma once

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "Scale.h"

enum class WeighMyBruMessageType : uint8_t {
  SYSTEM = 0x0A,
  WEIGHT = 0x0B
};

class BluetoothScale : public BLEServerCallbacks, public BLECharacteristicCallbacks {
public:
    BluetoothScale();
    ~BluetoothScale();
    
    void begin(Scale* scale);
    void end();
    void update();
    bool isConnected();
    void sendWeight(float weight);
    void handleTareCommand();
    
    // BLE Server callbacks
    void onConnect(BLEServer* pServer) override;
    void onDisconnect(BLEServer* pServer) override;
    
    // BLE Characteristic callbacks
    void onWrite(BLECharacteristic* pCharacteristic) override;

private:
    Scale* scale;
    BLEServer* server;
    BLEService* service;
    BLECharacteristic* weightCharacteristic;
    BLECharacteristic* commandCharacteristic;
    BLEAdvertising* advertising;
    
    bool deviceConnected;
    bool oldDeviceConnected;
    uint32_t lastHeartbeat;
    uint32_t lastWeightSent;
    float lastWeight;
    
    // WeighMyBru protocol constants
    static const uint8_t PRODUCT_NUMBER = 0x03;
    static const size_t PROTOCOL_LENGTH = 20;
    static const uint32_t HEARTBEAT_INTERVAL = 2000; // 2 seconds
    static const uint32_t WEIGHT_SEND_INTERVAL = 50; // 50ms (20 updates/sec) - faster for GaggiMate
    
    // WeighMyBru UUIDs - unique to avoid conflicts with Bookoo scales
    static const char* SERVICE_UUID;
    static const char* WEIGHT_CHARACTERISTIC_UUID;
    static const char* COMMAND_CHARACTERISTIC_UUID;
    
    void initializeBLE();
    void startAdvertising();
    void stopAdvertising();
    void sendMessage(WeighMyBruMessageType msgType, const uint8_t* payload, size_t length);
    void sendHeartbeat();
    void sendNotificationRequest();
    void processIncomingMessage(uint8_t* data, size_t length);
    uint8_t calculateChecksum(const uint8_t* data, size_t length);
    void sendWeightNotification(float weight);
};
