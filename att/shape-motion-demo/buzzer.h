#ifndef buzzer_included
#define buzzer_included

void buzzer_init();
void buzzer_advance_frequency();
void buzzer_set_period(short cycles);
//boolean flags to detect when switch pressed for state machine 
extern unsigned char s1,s2,s3,s4,i;//i is index in song array

#endif // included
