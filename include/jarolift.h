#pragma once

void jaroCmdUp(uint8_t channel);
void jaroCmdDown(uint8_t channel);
void jaroCmdStop(uint8_t channel);
void jaroCmdSetShade(uint8_t channel);
void jaroCmdShade(uint8_t channel);
void jaroCmdLearn(uint8_t channel);
void jaroCmdSetDevCnt(uint16_t value);
void jaroCmdReInit();
bool getCC1101State();
uint8_t getCC1101Rssi();
uint16_t jaroGetDevCnt();

void jaroliftSetup();
void jaroliftCyclic();
