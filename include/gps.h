#include <TinyGPS.h>

void gps_setup(void);
void gps_loop(void);
void smartdelay(unsigned long ms);
void print_float(float val, float invalid, int len, int prec);
void print_int(unsigned long val, unsigned long invalid, int len);
void print_date(TinyGPS &gps);
void print_str(const char *str, int len);