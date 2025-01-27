/* copyright (c) 2022 - 2025 grunfink et al. / MIT license */

#ifndef _XS_TIME_H

#define _XS_TIME_H

#include <time.h>

xs_str *xs_str_time(time_t t, const char *fmt, int local);
#define xs_str_localtime(t, fmt) xs_str_time(t, fmt, 1)
#define xs_str_utctime(t, fmt)   xs_str_time(t, fmt, 0)
#define xs_str_iso_date(t) xs_str_time(t, "%Y-%m-%dT%H:%M:%SZ", 0)
time_t xs_parse_iso_date(const char *iso_date, int local);
time_t xs_parse_time(const char *str, const char *fmt, int local);
#define xs_parse_localtime(str, fmt) xs_parse_time(str, fmt, 1)
#define xs_parse_utctime(str, fmt) xs_parse_time(str, fmt, 0)
xs_str *xs_str_time_diff(time_t time_diff);

#ifdef XS_IMPLEMENTATION

xs_str *xs_str_time(time_t t, const char *fmt, int local)
/* returns a string with a formated time */
{
    struct tm tm;
    char tmp[64];

    if (t == 0)
        t = time(NULL);

    if (local)
        localtime_r(&t, &tm);
    else
        gmtime_r(&t, &tm);

    strftime(tmp, sizeof(tmp), fmt, &tm);

    return xs_str_new(tmp);
}


xs_str *xs_str_time_diff(time_t time_diff)
/* returns time_diff in seconds to 'human' units (d:hh:mm:ss) */
{
    int secs  = time_diff % 60;
    int mins  = (time_diff /= 60) % 60;
    int hours = (time_diff /= 60) % 24;
    int days  = (time_diff /= 24);

    return xs_fmt("%d:%02d:%02d:%02d", days, hours, mins, secs);
}


char *strptime(const char *s, const char *format, struct tm *tm);

time_t xs_parse_time(const char *str, const char *fmt, int local)
{
    time_t t = 0;

#ifndef WITHOUT_STRPTIME

    struct tm tm = {0};

    strptime(str, fmt, &tm);

    /* try to guess the Daylight Saving Time */
    if (local)
        tm.tm_isdst = -1;

    t = local ? mktime(&tm) : timegm(&tm);

#endif /* WITHOUT_STRPTIME */

    return t;
}


time_t xs_parse_iso_date(const char *iso_date, int local)
/* parses a YYYY-MM-DDTHH:MM:SS date string */
{
    time_t t = 0;

#ifndef WITHOUT_STRPTIME

    t = xs_parse_time(iso_date, "%Y-%m-%dT%H:%M:%S", local);

#else /* WITHOUT_STRPTIME */

    struct tm tm = {0};

    if (sscanf(iso_date, "%d-%d-%dT%d:%d:%d",
        &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
        &tm.tm_hour, &tm.tm_min, &tm.tm_sec) == 6) {

        tm.tm_year -= 1900;
        tm.tm_mon -= 1;

        if (local)
            tm.tm_isdst = -1;

        t = local ? mktime(&tm) : timegm(&tm);
    }

#endif /* WITHOUT_STRPTIME */

    return t;
}


#endif /* XS_IMPLEMENTATION */

#endif /* _XS_TIME_H */
