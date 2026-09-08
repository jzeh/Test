
#include "nmea/context.h"
/*
 *
 * NMEA library
 * URL: http://nmea.sourceforge.net
 * Author: Tim (xtimor@gmail.com)
 * Licence: http://www.gnu.org/licenses/lgpl.html
 * $Id: context.c 17 2008-03-11 11:56:11Z xtimor $
 *
 */

#include <string.h>
#include <stdarg.h>
#include <stdio.h>


nmeaPROPERTY * nmea_property()
{
    static nmeaPROPERTY prop = {
        0, 0, NMEA_DEF_PARSEBUFF
        };

    return &prop;
}

void nmea_trace_buff(const char *buff, int buff_size)
{
    nmeaTraceFunc func = nmea_property()->trace_func;
    if(func && buff_size)
        (*func)(buff, buff_size);
}
