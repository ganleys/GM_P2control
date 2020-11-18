#ifndef _GPS_H
#define _GPS_H

#include <TinyGPS.h>
#include <ArduinoJson.h>

void gps_setup(void);
void gps_loop(void);
void smartdelay(unsigned long ms);
int8_t gps_get_location(int8_t cell, JsonObject *jstr);
void print_float(float val, float invalid, int len, int prec);
void print_int(unsigned long val, unsigned long invalid, int len);
void print_date(TinyGPS &gps);
void print_str(const char *str, int len);

#endif /*_GPS_H*/