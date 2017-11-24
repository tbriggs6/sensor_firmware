/*
 * rtc.h
 *
 *  Created on: Jun 24, 2016
 *      Author: user
 */

#ifndef RTC_H_
#define RTC_H_

#define EPOCH_YEAR 1970

typedef struct {
	int year;
	int month;
	int day;
	int yearDay;
	int weekDay;
	int hour;
	int minute;
	int second;
} date_t;

void gmttime(date_t *date, unsigned long seconds) ;
unsigned long mktime(date_t *date);

void rtc_setoffset(unsigned long off);

unsigned long rtc_getoffset( );

unsigned long rtc_gettime();

void rtc_settime(unsigned int seconds);

#endif /* RTC_H_ */
