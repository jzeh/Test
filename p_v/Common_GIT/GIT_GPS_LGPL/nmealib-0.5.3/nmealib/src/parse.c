
#include <string.h>
#include <stdio.h>
#include "nmea/tok.h"
#include "nmea/parse.h"
#include "nmea/context.h"
#include "nmea/gmath.h"
#include "nmea/units.h"

#include "HalLedDriver.h"

#include "AutolinkConfiguration.h"

#define MAX_GPS_LED_DELAY_CNT 1

extern	int g_nGPSTransmitLedDelayCount;
extern double g_dSavedGpsLon;
extern double g_dSavedGpsLat;


extern void SetGPSTransmitLedOnOff();
extern int Get_GPS_Vailication();
//extern double Get_GPS_Lat();
//extern double Get_GPS_Lon();
//extern int Get_GPS_SatellitesNum();

/*
 *
 * NMEA library
 * URL: http://nmea.sourceforge.net
 * Author: Tim (xtimor@gmail.com)
 * Licence: http://www.gnu.org/licenses/lgpl.html
 * $Id: parse.c 17 2008-03-11 11:56:11Z xtimor $
 *
 */

/**
 * \file parse.h
 * \brief Functions of a low level for analysis of
 * packages of NMEA stream.
 *
 * \code
 * ...
 * ptype = nmea_pack_type(
 *     (const char *)parser->buffer + nparsed + 1,
 *     parser->buff_use - nparsed - 1);
 *
 * if(0 == (node = malloc(sizeof(nmeaParserNODE))))
 *     goto mem_fail;
 *
 * node->pack = 0;
 *
 * switch(ptype)
 * {
 * case GPGGA:
 *     if(0 == (node->pack = malloc(sizeof(nmeaGPGGA))))
 *         goto mem_fail;
 *     node->packType = GPGGA;
 *     if(!nmea_parse_GPGGA(
 *         (const char *)parser->buffer + nparsed,
 *         sen_sz, (nmeaGPGGA *)node->pack))
 *     {
 *         free(node);
 *         node = 0;
 *     }
 *     break;
 * case GPGSA:
 *     if(0 == (node->pack = malloc(sizeof(nmeaGPGSA))))
 *         goto mem_fail;
 *     node->packType = GPGSA;
 *     if(!nmea_parse_GPGSA(
 *         (const char *)parser->buffer + nparsed,
 *         sen_sz, (nmeaGPGSA *)node->pack))
 *     {
 *         free(node);
 *         node = 0;
 *     }
 *     break;
 * ...
 * \endcode
 */

int _nmea_parse_time(const char *buff, int buff_sz, nmeaTIME *res)
{
    int success = 0;

    switch(buff_sz)
    {
    case sizeof("hhmmss") - 1:
        success = (3 == nmea_scanf(buff, buff_sz,
            "%2d%2d%2d", &(res->hour), &(res->min), &(res->sec)
            ));
        break;
    case sizeof("hhmmss.s") - 1:
    case sizeof("hhmmss.ss") - 1:
    case sizeof("hhmmss.sss") - 1:
        success = (4 == nmea_scanf(buff, buff_sz,
            "%2d%2d%2d.%d", &(res->hour), &(res->min), &(res->sec), &(res->hsec)
            ));
        break;
    default:
//        nmea_error("Parse of time error (format error)!\r\n");
        success = 0;
        break;
    }

    return (success?0:-1);
}

/**
 * \brief Define packet type by header (nmeaPACKTYPE).
 * @param buff a constant character pointer of packet buffer.
 * @param buff_sz buffer size.
 * @return The defined packet type
 * @see nmeaPACKTYPE
 */
int nmea_pack_type(const char *buff, int buff_sz)
{
#if defined(USE_UBLOX_GPS)
	const char *pheads[] = {
        "GPGGA",
		"GNGGA",	// GPGGA
        "GPGSA",
        "GNGSA",	// GPGSA
		"GPGSV",
		"GLGSV", 	// GPGSV
        "GPRMC",
		"GNRMC",	// GNRMC
        "GPVTG",
		"GNVTG",	// GPVTG
		"GNGLL",
    };

//    NMEA_ASSERT(buff);

    if(buff_sz < 5)
        return GPNON;
    else if(0 == memcmp(buff, pheads[0], 5))
			return GPGGA;
	else if(0 == memcmp(buff, pheads[1], 5))
      return GPGGA;
    else if(0 == memcmp(buff, pheads[2], 5))
      return GPGSA;
    else if(0 == memcmp(buff, pheads[3], 5))
      return GPGSA;
    else if(0 == memcmp(buff, pheads[4], 5))
      return GPGSV;
    else if(0 == memcmp(buff, pheads[5], 5))
      return GPGSV;
    else if(0 == memcmp(buff, pheads[6], 5))
      return GPRMC;
    else if(0 == memcmp(buff, pheads[7], 5))
      return GPRMC;
    else if(0 == memcmp(buff, pheads[8], 5))
      return GPVTG;
    else if(0 == memcmp(buff, pheads[9], 5))
      return GPVTG;
	else if(0 == memcmp(buff, pheads[10], 5))
      return GNGLL;

    return GPNON;
#else

    const char *pheads[] = {
        "GPGGA",
        "GPGSA",
        "GPGSV",
        "GPRMC",
        "GPVTG",
    };

//    NMEA_ASSERT(buff);

    if(buff_sz < 5)
        return GPNON;
    else if(0 == memcmp(buff, pheads[0], 5))
        return GPGGA;
    else if(0 == memcmp(buff, pheads[1], 5))
        return GPGSA;
    else if(0 == memcmp(buff, pheads[2], 5))
        return GPGSV;
    else if(0 == memcmp(buff, pheads[3], 5))
        return GPRMC;
    else if(0 == memcmp(buff, pheads[4], 5))
        return GPVTG;

    return GPNON;
#endif
}

/**
 * \brief Find tail of packet ("\r\n") in buffer and check control sum (CRC).
 * @param buff a constant character pointer of packets buffer.
 * @param buff_sz buffer size.
 * @param res_crc a integer pointer for return CRC of packet (must be defined).
 * @return Number of bytes to packet tail.
 */
int nmea_find_tail(const char *buff, int buff_sz, int *res_crc)
{
    static const int tail_sz = 3 /* *[CRC] */ + 2 /* \r\n */;

    const char *end_buff = buff + buff_sz;
    int nread = 0;
    int crc = 0;

//    NMEA_ASSERT(buff && res_crc);

    *res_crc = -1;

    for(;buff < end_buff; ++buff, ++nread)
    {
        if(('$' == *buff) && nread)
        {
            buff = 0;
            break;
        }
        else if('*' == *buff)
        {
#if defined(USE_UBLOX_GPS)
			if(buff + tail_sz <= end_buff && '\r' == buff[3] && '\n' == buff[4])
#else
			if(buff + tail_sz <= end_buff && '\r' == buff[3] && '\n' == buff[4])
#endif
            {
                *res_crc = nmea_atoi(buff + 1, 2, 16);
                nread = buff_sz - (int)(end_buff - (buff + tail_sz));
                if(*res_crc != crc)
                {
                    *res_crc = -1;
                    buff = 0;
                }
            }

            break;
        }
        else if(nread)
            crc ^= (int)*buff;
    }

    if(*res_crc < 0 && buff)
        nread = 0;

    return nread;
}

/**
 * \brief Parse GGA packet from buffer.
 * @param buff a constant character pointer of packet buffer.
 * @param buff_sz buffer size.
 * @param pack a pointer of packet which will filled by function.
 * @return 1 (true) - if parsed successfully or 0 (false) - if fail.
 */
int nmea_parse_GPGGA(const char *buff, int buff_sz, nmeaGPGGA *pack)
{
    char time_buff[NMEA_TIMEPARSE_BUF];

//    NMEA_ASSERT(buff && pack);

    memset(pack, 0, sizeof(nmeaGPGGA));

    nmea_trace_buff(buff, buff_sz);

    if(14 != nmea_scanf(buff, buff_sz,
        "$GPGGA,%s,%f,%C,%f,%C,%d,%d,%f,%f,%C,%f,%C,%f,%d*",
        &(time_buff[0]),
        &(pack->lat), &(pack->ns), &(pack->lon), &(pack->ew),
        &(pack->sig), &(pack->satinuse), &(pack->HDOP), &(pack->elv), &(pack->elv_units),
        &(pack->diff), &(pack->diff_units), &(pack->dgps_age), &(pack->dgps_sid)))
    {
        nmea_error("GPGGA parse error!\r\n");
        return 0;
    }

    if(0 != _nmea_parse_time(&time_buff[0], (int)strlen(&time_buff[0]), &(pack->utc)))
    {
//        nmea_error("GPGGA time parse error!\r\n");
        return 0;
    }

    return 1;
}

/**
 * \brief Parse GSA packet from buffer.
 * @param buff a constant character pointer of packet buffer.
 * @param buff_sz buffer size.
 * @param pack a pointer of packet which will filled by function.
 * @return 1 (true) - if parsed successfully or 0 (false) - if fail.
 */
int nmea_parse_GPGSA(const char *buff, int buff_sz, nmeaGPGSA *pack)
{
//    NMEA_ASSERT(buff && pack);

    memset(pack, 0, sizeof(nmeaGPGSA));

    nmea_trace_buff(buff, buff_sz);

    if(17 != nmea_scanf(buff, buff_sz,
        "$GPGSA,%C,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%f,%f,%f*",
        &(pack->fix_mode), &(pack->fix_type),
        &(pack->sat_prn[0]), &(pack->sat_prn[1]), &(pack->sat_prn[2]), &(pack->sat_prn[3]), &(pack->sat_prn[4]), &(pack->sat_prn[5]),
        &(pack->sat_prn[6]), &(pack->sat_prn[7]), &(pack->sat_prn[8]), &(pack->sat_prn[9]), &(pack->sat_prn[10]), &(pack->sat_prn[11]),
        &(pack->PDOP), &(pack->HDOP), &(pack->VDOP)))
    {
        nmea_error("GPGSA parse error!\r\n");
        return 0;
    }

    return 1;
}

/**
 * \brief Parse GSV packet from buffer.
 * @param buff a constant character pointer of packet buffer.
 * @param buff_sz buffer size.
 * @param pack a pointer of packet which will filled by function.
 * @return 1 (true) - if parsed successfully or 0 (false) - if fail.
 */
int nmea_parse_GPGSV(const char *buff, int buff_sz, nmeaGPGSV *pack)
{
    int nsen, nsat;

//    NMEA_ASSERT(buff && pack);

    memset(pack, 0, sizeof(nmeaGPGSV));

    nmea_trace_buff(buff, buff_sz);

    nsen = nmea_scanf(buff, buff_sz,
        "$GPGSV,%d,%d,%d,"
        "%d,%d,%d,%d,"
        "%d,%d,%d,%d,"
        "%d,%d,%d,%d,"
        "%d,%d,%d,%d*",
        &(pack->pack_count), &(pack->pack_index), &(pack->sat_count),
        &(pack->sat_data[0].id), &(pack->sat_data[0].elv), &(pack->sat_data[0].azimuth), &(pack->sat_data[0].sig),
        &(pack->sat_data[1].id), &(pack->sat_data[1].elv), &(pack->sat_data[1].azimuth), &(pack->sat_data[1].sig),
        &(pack->sat_data[2].id), &(pack->sat_data[2].elv), &(pack->sat_data[2].azimuth), &(pack->sat_data[2].sig),
        &(pack->sat_data[3].id), &(pack->sat_data[3].elv), &(pack->sat_data[3].azimuth), &(pack->sat_data[3].sig));

    nsat = (pack->pack_index - 1) * NMEA_SATINPACK;
    nsat = (nsat + NMEA_SATINPACK > pack->sat_count)?pack->sat_count - nsat:NMEA_SATINPACK;
    nsat = nsat * 4 + 3 /* first three sentence`s */;

    if(nsen < nsat || nsen > (NMEA_SATINPACK * 4 + 3))
    {
        nmea_error("GPGSV parse error!\r\n");
        return 0;
    }

    return 1;
}

/**
 * \brief Parse RMC packet from buffer.
 * @param buff a constant character pointer of packet buffer.
 * @param buff_sz buffer size.
 * @param pack a pointer of packet which will filled by function.
 * @return 1 (true) - if parsed successfully or 0 (false) - if fail.
 */
//$GPRMC,114455.532,A,3735.0079,N,12701.6446,E,0.000000,121.61,110706,,*0A

int nmea_parse_GPRMC(const char *buff, int buff_sz, nmeaGPRMC *pack)
{
    int nsen;
    char time_buff[NMEA_TIMEPARSE_BUF]={0,};

//    NMEA_ASSERT(buff && pack);

    memset(pack, 0, sizeof(nmeaGPRMC));

    nmea_trace_buff(buff, buff_sz);

    nsen = nmea_scanf(buff, buff_sz,
        "$GPRMC,%s,%C,%f,%C,%f,%C,%f,%f,%2d%2d%2d,%f,%C,%C*",
        &(time_buff[0]),
        &(pack->status), &(pack->lat), &(pack->ns), &(pack->lon), &(pack->ew),
        &(pack->speed), &(pack->direction),
        &(pack->utc.day), &(pack->utc.mon), &(pack->utc.year),
        &(pack->declination), &(pack->declin_ew), &(pack->mode));

#if defined(USE_UBLOX_GPS)
	if(nsen != 12 && nsen != 14)
#else
    if(nsen != 13 && nsen != 14)
#endif
    {
        //nmea_error("()");
        return 0;
    }

    if(0 != _nmea_parse_time(&time_buff[0], (int)strlen(&time_buff[0]), &(pack->utc)))
    {
//        nmea_error("GPRMC time parse error!\r\n");
        return 0;
    }

/*MONI 
    printf("Y:%d,M:%d,D:%d,h:%d,m:%d,s:%d\n",pack->utc.year,pack->utc.mon,pack->utc.day,
                                             pack->utc.hour,pack->utc.min,pack->utc.sec);

    if(pack->utc.year < 90)
        pack->utc.year += 100;
    pack->utc.mon -= 1;
*/

#ifndef ENABLE_UBLOX_GPS_DATA
	if ( g_nGPSTransmitLedDelayCount == 0 ) {
		g_nGPSTransmitLedDelayCount = MAX_GPS_LED_DELAY_CNT;
	}

	if(Get_GPS_Vailication() == 1)	SetGPSTransmitLedOnOff();
	else SetLedOnOffCtl(LED_OFF, eLED_GPS);
#endif

	return 1;
}

/**
 * \brief Parse VTG packet from buffer.
 * @param buff a constant character pointer of packet buffer.
 * @param buff_sz buffer size.
 * @param pack a pointer of packet which will filled by function.
 * @return 1 (true) - if parsed successfully or 0 (false) - if fail.
 */
int nmea_parse_GPVTG(const char *buff, int buff_sz, nmeaGPVTG *pack)
{
//    NMEA_ASSERT(buff && pack);

    memset(pack, 0, sizeof(nmeaGPVTG));

    nmea_trace_buff(buff, buff_sz);
#if defined(USE_UBLOX_GPS)
    if(9 != nmea_scanf(buff, buff_sz,
        "$GPVTG,%f,%C,%f,%C,%f,%C,%f,%C,%C*",
        &(pack->dir), &(pack->dir_t),
        &(pack->dec), &(pack->dec_m),
        &(pack->spn), &(pack->spn_n),
        &(pack->spk), &(pack->spk_k),
        &(pack->posMode)))
#else
    if(8 != nmea_scanf(buff, buff_sz,
        "$GPVTG,%f,%C,%f,%C,%f,%C,%f,%C*",
        &(pack->dir), &(pack->dir_t),
        &(pack->dec), &(pack->dec_m),
        &(pack->spn), &(pack->spn_n),
        &(pack->spk), &(pack->spk_k)))
#endif
    {
        nmea_error("GPVTG parse error!\r\n");
        return 0;
    }

    if( pack->dir_t != 'T' ||
        pack->dec_m != 'M' ||
        pack->spn_n != 'N' ||
        pack->spk_k != 'K')
    {
//        nmea_error("GPVTG parse error (format error)!\r\n");
        return 0;
    }

    return 1;
}

#if defined(USE_UBLOX_GPS)
int nmea_parse_GNGLL(const char *buff, int buff_sz, nmeaGNGLL *pack)
{
//    NMEA_ASSERT(buff && pack);

    memset(pack, 0, sizeof(nmeaGNGLL));

    nmea_trace_buff(buff, buff_sz);

    if(7 != nmea_scanf(buff, buff_sz,
        "$GNGLL,%f,%C,%f,%C,%f,%C,%C*",
        &(pack->lat), &(pack->ns),
        &(pack->lon), &(pack->ew),
        &(pack->time), &(pack->status),
        &(pack->posMode)))
    {
        nmea_error("GNGLL parse error!\r\n");
        return 0;
    }

    return 1;
}

void nmea_GNGLL2info(nmeaGNGLL *pack, nmeaINFO *info)
{
//    NMEA_ASSERT(pack && info);

    //info->utc.year = pack->time.year;
    //info->utc.mon = pack->time.mon;
    //info->utc.day = pack->time.day;
    info->utc.hour = pack->time.hour;
    info->utc.min = pack->time.min;
    info->utc.sec = pack->time.sec;
    info->utc.hsec = pack->time.hsec;

    if ( pack->status == 'A' )
    {
        info->lat = ((pack->ns == 'N')?pack->lat:-(pack->lat));
        info->lon = ((pack->ew == 'E')?pack->lon:-(pack->lon));
    }
    info->smask |= GNGLL;
}

#endif

/**
 * \brief Fill nmeaINFO structure by GGA packet data.
 * @param pack a pointer of packet structure.
 * @param info a pointer of summary information structure.
 */
void nmea_GPGGA2info(nmeaGPGGA *pack, nmeaINFO *info)
{
//    NMEA_ASSERT(pack && info);

    //info->utc.year = pack->utc.year;
    //info->utc.mon = pack->utc.mon;
    //info->utc.day = pack->utc.day;
    info->utc.hour = pack->utc.hour;
    info->utc.min = pack->utc.min;
    info->utc.sec = pack->utc.sec;
    info->utc.hsec = pack->utc.hsec;
    info->sig = pack->sig;
    info->satinfo.inuse = pack->satinuse;
    info->HDOP = pack->HDOP;
    info->elv = pack->elv;
	
    if(info->sig >= NMEA_SIG_FIXED_2D_3D)
    {
    	if(pack->lat != 0.0) info->lat = ((pack->ns == 'N')?pack->lat:-(pack->lat));
		if(pack->lon != 0.0) info->lon = ((pack->ew == 'E')?pack->lon:-(pack->lon));
	}

    info->smask |= GPGGA;

    // 20170720 James Jean : GPS fixï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ ï¿½Ö¾ï¿½ï¿?ï¿½ï¿½.
	// fix ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿?ï¿½Ê¾ï¿½ ï¿½ß°ï¿½ï¿½ï¿½.
    if(NMEA_SIG_NO_FIX == info->sig) info->fix = NMEA_FIX_NO_FIX;
}

/**
 * \brief Fill nmeaINFO structure by GSA packet data.
 * @param pack a pointer of packet structure.
 * @param info a pointer of summary information structure.
 */
void nmea_GPGSA2info(nmeaGPGSA *pack, nmeaINFO *info)
{
    int i, j, nuse = 0;

//    NMEA_ASSERT(pack && info);

    info->fix = pack->fix_type;
    if(pack->fix_type == NMEA_FIX_NO_FIX)
    {
        info->sig = NMEA_SIG_NO_FIX;
    }
    else 
    {
        info->sig = NMEA_SIG_FIXED_2D_3D;
    }
    info->PDOP = pack->PDOP;
    info->HDOP = pack->HDOP;
    info->VDOP = pack->VDOP;

	if(info->satinfo.inview > NMEA_MAXSAT)  info->satinfo.inview = NMEA_MAXSAT;		//170416 LWH ï¿½ï¿½ï¿½ï¿½Úµï¿?ï¿½ß°ï¿½
    for(i = 0; i < NMEA_MAXSAT; ++i)
    {
        for(j = 0; j < info->satinfo.inview; ++j)
        {
            if(pack->sat_prn[i] && pack->sat_prn[i] == info->satinfo.sat[j].id)
            {
                info->satinfo.sat[j].in_use = 1;
                nuse++;
            }
        }
    }

    info->satinfo.inuse = nuse;
    info->smask |= GPGSA;
}

/**
 * \brief Fill nmeaINFO structure by GSV packet data.
 * @param pack a pointer of packet structure.
 * @param info a pointer of summary information structure.
 */
void nmea_GPGSV2info(nmeaGPGSV *pack, nmeaINFO *info)
{
  int isat, isi, nsat;

//  NMEA_ASSERT(pack && info);

  if(pack->pack_index > pack->pack_count || pack->pack_index * NMEA_SATINPACK > NMEA_MAXSAT)
      return;

  if(pack->pack_index < 1)
      pack->pack_index = 1;

  info->satinfo.inview = pack->sat_count;

  nsat = (pack->pack_index - 1) * NMEA_SATINPACK;
  nsat = (nsat + NMEA_SATINPACK > pack->sat_count)?pack->sat_count - nsat:NMEA_SATINPACK;

  for(isat = 0; isat < nsat; ++isat) {
    isi = (pack->pack_index - 1) * NMEA_SATINPACK + isat;
    info->satinfo.sat[isi].id = pack->sat_data[isat].id;
    info->satinfo.sat[isi].elv = pack->sat_data[isat].elv;
    info->satinfo.sat[isi].azimuth = pack->sat_data[isat].azimuth;
    info->satinfo.sat[isi].sig = pack->sat_data[isat].sig;
  }

  info->smask |= GPGSV;
}

/**
 * \brief Fill nmeaINFO structure by RMC packet data.
 * @param pack a pointer of packet structure.
 * @param info a pointer of summary information structure.
 */
void nmea_GPRMC2info(nmeaGPRMC *pack, nmeaINFO *info)
{
//    NMEA_ASSERT(pack && info);

    if('A' == pack->status)
    {
        if(NMEA_SIG_NO_FIX == info->sig)
            info->sig = NMEA_SIG_FIXED_2D_3D;
        if(NMEA_FIX_NO_FIX == info->fix)
            info->fix = NMEA_FIX_2D;
    }
    else if('V' == pack->status)
    {
        info->sig = NMEA_SIG_NO_FIX;
        info->fix = NMEA_FIX_NO_FIX;
    }

	if(info->sig >= NMEA_SIG_FIXED_2D_3D)
	{
		info->utc = pack->utc;
		SetGpsUtcTime(info->utc);
		
		if(pack->lat != 0.0) info->lat = ((pack->ns == 'N')?pack->lat:-(pack->lat));
		if(pack->lon != 0.0) info->lon = ((pack->ew == 'E')?pack->lon:-(pack->lon));
	}

	info->speed = pack->speed * NMEA_TUD_KNOTS;
	if( pack->direction != 0.0 )	info->direction = pack->direction;
    info->smask |= GPRMC;

	//printf("%lf,%lf,Lat:%lf,Lon:%lf,val:%d,sat:%d\r\n",Get_GPS_Lat(),Get_GPS_Lon(),Get_GPS_Lat_Origin(),Get_GPS_Lon_Origin(),Get_GPS_Vailication(),Get_GPS_SatellitesNum());
}

/**
 * \brief Fill nmeaINFO structure by VTG packet data.
 * @param pack a pointer of packet structure.
 * @param info a pointer of summary information structure.
 */
void nmea_GPVTG2info(nmeaGPVTG *pack, nmeaINFO *info)
{
//    NMEA_ASSERT(pack && info);

    if( pack->dir != 0.0 )    info->direction = pack->dir;
    info->declination = pack->dec;
    info->speed = pack->spk;
    info->smask |= GPVTG;
}
