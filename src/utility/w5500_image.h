#pragma once

void w5500Init();

bool w5500GotToken(const char* url, char **data, uint32_t* len);

void w5500PostImage(const char* url, const char* tok, const uint8_t *data, uint32_t len, uint32_t voltage);

void w5500ImageLoop();
