#ifndef SNTP_TIME_SYNC_H
#define SNTP_TIME_SYNC_H

/*
 * Start the NTP server sync task
 */
void sntp_time_sync_task_start(void);

/*
 * returns the local time if set.
 * @return local time buffer
 */
char *sntp_time_sync_get_time(void);

#endif // DEBUGn
