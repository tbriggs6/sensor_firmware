/*
 * rtc.c
 *
 *  Created on: Jun 23, 2016
 *      Author: user
 */



#include <contiki.h>

#include "rtc.h"

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif


static long offset = 0;


void rtc_setoffset(unsigned long off) {
	offset = off;
}

unsigned long rtc_getoffset( ) {
	return offset;
}

unsigned long rtc_gettime() {
	return clock_seconds() + offset;
}

void rtc_settime(unsigned int seconds) {
	offset = seconds - clock_seconds();
}

static int is_leap(int year) {
	int leap = 0;
	if ((year % 4) == 0) leap = 1;
	if ((year % 100) == 0) leap = 0;
	if ((year % 400) == 0) leap = 1;

	return leap;
}
static int compute_index(int year) {
	switch (year / 100) {
	case 18:
		return 5;
	case 19:
		return 3;
	case 20:
		return 2;
	case 21:
		return 0;
	default:
		return -1;
	}
}

static int doomsday(int year) {
	int index = compute_index(year);
	int y = year % 100;
	int a = y / 12;
	int b = (y % 12);
	int c = b / 4;

	int d = (a + b + c) % 7;

	return (d + index) % 7;
}

static int days_since_near_years(int mon, int day, int leap_year) {
	int sum = 0;

	/**
	 * why -1?  suppose we want feb 2.  mon is 2.  Case statement
	 * would add 28 (or 29) to sum, and then add 2.  by sub-1,
	 * this starts adding at the previous month, so we add 31
	 * for january, then 2 for feb 2nd.
	 */
	switch (mon - 1) {
	case 11:
		sum += 30; // nov
	case 10:
		sum += 31; // oct
	case 9:
		sum += 30; // sept
	case 8:
		sum += 31; // aug
	case 7:
		sum += 31; // jul
	case 6:
		sum += 30; // jun
	case 5:
		sum += 31; // may
	case 4:
		sum += 30; // aprl
	case 3:
		sum += 31; // mar
	case 2:
		if (leap_year)
			sum += 29;
		else
			sum += 28;
	case 1:
		sum += 31; // jan
	}

	return sum + day;
}

static int date_difference(int mon1, int day1, int mon2, int day2, int leap_year) {
	int days1 = days_since_near_years(mon1, day1, leap_year);
	int days2 = days_since_near_years(mon2, day2, leap_year);

	return days1 - days2;
}

static int day_of_week(int year, int month, int day) {
	int dday = doomsday(year);
	int leap_year = is_leap(year);

// dooms day is 3/7
	int days = date_difference(3, 7, month, day, leap_year);

	return (dday - days) % 7;

}

void gmttime(date_t *date, unsigned long seconds) {


	date->second = seconds % 60;
	date->minute = (seconds / 60) % 60;
	date->hour = (seconds / 3600) % 24;

	unsigned long days = seconds / 86400;

	date->year = EPOCH_YEAR;
	while (days > 365) {

		int leap = is_leap(date->year);
		days = days - ((leap == 1) ? 366 : 365);
		date->year++;
	}

	int monthDays[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

	date->yearDay = days;
	date->month = 1;
	int i;
	for (i = 0; i < 12; i++) {
		int mdays = monthDays[i];
		if ((i == 1) && (is_leap(date->year)))
			mdays = 29;

		if (days >= mdays) {
			days -= mdays;
			date->month++;
		} else
			break;
	}

	date->day = days + 1; // theydon't start at 1
	// at this point, year, month, and day should be set


	date->weekDay = day_of_week(date->year, date->month, date->day);
}


unsigned long mktime(date_t *date)
{
	int days = 0;
	int year = date->year;

	while (year > EPOCH_YEAR) {
		if (is_leap(year)) days += 366;
		else days += 365;
		year--;
	}

	days += days_since_near_years(date->month, date->day, is_leap(date->year));

	unsigned int seconds = days * 86400;
	seconds += (date->hour) * 3600;
	seconds += (date->minute) * 60;
	seconds += (date->second);

	return seconds;
}
