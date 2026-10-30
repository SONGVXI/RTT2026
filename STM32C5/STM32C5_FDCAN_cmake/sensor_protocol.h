#ifndef SENSOR_PROTOCOL_H
#define SENSOR_PROTOCOL_H
#include <stdint.h>

#define SENSOR_CAN_MAGIC 0xA1U
#define SENSOR_STATUS_MAGIC 0xA2U
#define SENSOR_CAN_REQUEST_ID 0x101U
#define SENSOR_CAN_RESPONSE_ID 0x102U

struct sensor_sample { uint16_t temperature_x10, humidity_x10, light_raw; };
struct sensor_limits { uint16_t temperature_x10, humidity_x10, light_raw; };
struct sensor_status { uint8_t led_on, exceeded; };

static inline uint16_t sensor_u16(const uint8_t *p)
{ return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8)); }

/* 8 bytes: magic, sequence, temperature*10 LE16, humidity*10 LE16, raw LE16. */
static inline void sensor_encode(uint8_t p[8], uint8_t sequence, const struct sensor_sample *s)
{
    p[0]=SENSOR_CAN_MAGIC; p[1]=sequence;
    p[2]=(uint8_t)s->temperature_x10; p[3]=(uint8_t)(s->temperature_x10>>8);
    p[4]=(uint8_t)s->humidity_x10; p[5]=(uint8_t)(s->humidity_x10>>8);
    p[6]=(uint8_t)s->light_raw; p[7]=(uint8_t)(s->light_raw>>8);
}

static inline int sensor_decode(const uint8_t p[8], struct sensor_sample *s)
{
    if (p[0] != SENSOR_CAN_MAGIC) return 0;
    s->temperature_x10=sensor_u16(p+2); s->humidity_x10=sensor_u16(p+4);
    s->light_raw=sensor_u16(p+6);
    return s->temperature_x10<=500U && s->humidity_x10<=1000U;
}

static inline unsigned sensor_exceeded(const struct sensor_sample *s, const struct sensor_limits *v)
{
    return (unsigned)(s->temperature_x10>v->temperature_x10) +
           (unsigned)(s->humidity_x10>v->humidity_x10) + (unsigned)(s->light_raw>v->light_raw);
}

/* Bind the status to the full request, not just its wrapping 8-bit sequence. */
static inline uint32_t sensor_request_crc32(const uint8_t request[8])
{
    uint32_t crc=0xFFFFFFFFU;
    unsigned i,bit;
    for(i=0;i<8;i++)
    {
        crc^=request[i];
        for(bit=0;bit<8;bit++) crc=(crc>>1)^((crc&1U)?0xEDB88320U:0U);
    }
    return ~crc;
}

/* Reply: A2, sequence, ST LED(0/1), ST exceeded count(0..3), request CRC32 LE. */
static inline void sensor_status_encode(uint8_t reply[8], const uint8_t request[8], unsigned count)
{
    uint32_t crc=sensor_request_crc32(request);
    reply[0]=SENSOR_STATUS_MAGIC; reply[1]=request[1];
    reply[2]=(uint8_t)(count>=2U); reply[3]=(uint8_t)count;
    reply[4]=(uint8_t)crc; reply[5]=(uint8_t)(crc>>8);
    reply[6]=(uint8_t)(crc>>16); reply[7]=(uint8_t)(crc>>24);
}

static inline int sensor_status_decode(const uint8_t reply[8], const uint8_t request[8], struct sensor_status *status)
{
    uint32_t received=(uint32_t)reply[4]|((uint32_t)reply[5]<<8)|
                      ((uint32_t)reply[6]<<16)|((uint32_t)reply[7]<<24);
    if(reply[0]!=SENSOR_STATUS_MAGIC || reply[1]!=request[1] || reply[2]>1U || reply[3]>3U ||
       reply[2]!=(uint8_t)(reply[3]>=2U) || received!=sensor_request_crc32(request)) return 0;
    status->led_on=reply[2]; status->exceeded=reply[3];
    return 1;
}

/* SET accepts whole degrees C / whole percent RH / raw, with no trailing text.
 * The output limits are updated only after every field passes validation. */
static inline int sensor_parse_set(const char *p, struct sensor_limits *v)
{
    uint32_t values[3]={0,0,0};
    unsigned i;
    if (p[0]!='S' || p[1]!='E' || p[2]!='T' || (p[3]!=' ' && p[3]!='\t')) return 0;
    p+=3;
    for(i=0;i<3;i++)
    {
        while(*p==' ' || *p=='\t') p++;
        if (*p<'0' || *p>'9') return 0;
        while(*p>='0' && *p<='9')
        {
            values[i]=values[i]*10U+(uint32_t)(*p++-'0');
            if(values[i]>65535U) return 0;
        }
        if(i<2 && *p!=' ' && *p!='\t') return 0;
    }
    while(*p==' ' || *p=='\t') p++;
    if(*p || values[0]>50U || values[1]>100U) return 0;
    v->temperature_x10=(uint16_t)(values[0]*10U);
    v->humidity_x10=(uint16_t)(values[1]*10U);
    v->light_raw=(uint16_t)values[2];
    return 1;
}
#endif
