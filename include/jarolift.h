#pragma once

void cmd_up(uint8_t channel);
void cmd_down(uint8_t channel);
void cmd_stop(uint8_t channel);
void cmd_setShade(uint8_t channel);
void cmd_shade(uint8_t channel);
void cmd_learn(uint8_t channel);
void cmd_setDevCnt(uint16_t value);
bool getCC1101State();
uint8_t getCC1101Rssi();
uint16_t cmd_getDevCnt();

void jaroliftSetup();
void jaroliftCyclic();
