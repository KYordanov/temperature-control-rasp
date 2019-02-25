#include <stdio.h>
#include <pigpio.h>
#include <one-wire.h>

#define DIG_OUT_INVALID (0xFFFFu)
#define ONE_WIRE 4
#define BUS_IDLE gpioSetMode(ONE_WIRE, PI_INPUT)
#define BUS_PULL_LOW gpioSetMode(ONE_WIRE, PI_OUTPUT); gpioWrite(ONE_WIRE, 0)
#define NOK (1)
#define OK (0)

uint16_t digital_output = DIG_OUT_INVALID;

float ow_read_temp_f(void){
    float f_temperature;
    if(digital_output != DIG_OUT_INVALID)
    {
        f_temperature = (digital_output & 0x07FF) * 0.0625;
        if(0xF000 == (digital_output & 0xF000))
        {
            f_temperature *= -1.0;
        }
    }
    else
    {
        f_temperature = OW_INVALID_TEMP;
    }
    return f_temperature;
}


uint8_t ow_init_seq(void)
{
    // MASTER TX RESET PULS
    BUS_PULL_LOW;
    gpioDelay(500);
    BUS_IDLE;

    // DS18B20 waits 15-60 us
    gpioDelay(65);

    // DS18B20 TX PRESENCE PULSE
    if(1 == gpioRead(ONE_WIRE))
    {
        //printf("No Presence Pulse received.\n");
        return NOK;
    }

    gpioDelay(500 - 65); // MASTER RX 480us minimum
    return OK;
}

static inline void ow_write0(void)
{
    BUS_PULL_LOW;
    gpioDelay(60); // 60us < TX "0" < 120us
    BUS_IDLE;
    gpioDelay(5); // 1us < TREC < inf
}

static inline void ow_write1(void)
{
    BUS_PULL_LOW;
    gpioDelay(5); // > 1us
    BUS_IDLE;
    gpioDelay(60);
}

static inline uint8_t ow_read(void)
{
    uint8_t read_val = 0;
    BUS_PULL_LOW;
    gpioDelay(3); // > 1us TINIT
    BUS_IDLE;
    gpioDelay(10); // > 10us TRC
    read_val = gpioRead(ONE_WIRE);

    gpioDelay(65 - 11); // to 65us
    return read_val;
}

void ow_tr_byte(uint8_t tr_byte)
{
    for(uint8_t i = 0; i < 8; i++)
    {
        if(0x01 == (tr_byte & 0x01))
        {
            ow_write1();
        }
        else
        {
            ow_write0();
        }
        tr_byte = tr_byte >> 1;
    }
}

uint8_t ow_rx_byte(void)
{
    uint8_t rx_byte = 0;
    for(uint8_t i = 0; i < 8; i++)
    {
        rx_byte = rx_byte >> 1;
        if(1 == ow_read())
        {
            rx_byte |= 0x80;
        }
    }
    return rx_byte;
}

void ow_rom_command_skip(void)
{
    ow_tr_byte(0xCC);
}

void ow_function_command_convert_temperature(void)
{
    ow_tr_byte(0x44);

    while(0 == ow_read())gpioDelay(3); // wait to finish the conversation
}

void ow_function_command_read_scratchpad(uint8_t* scratch_buff)
{
    ow_tr_byte(0xBE);

    for(uint8_t i = 0; i < 9; i++)
    {
        scratch_buff[i] = ow_rx_byte();
    }
}

void ow_log_temp(FILE * stream, float time_s)
{
    if(digital_output != DIG_OUT_INVALID)
    {
        float f_temperature = 0.0;
        f_temperature = (digital_output & 0x07FF) * 0.0625;
        if(0xF000 == (digital_output & 0xF000))
        {
            f_temperature *= -1.0;
        }
        fprintf(stream, "%lf %lf\n", time_s, f_temperature);
        fflush(stream);
    }
}

void ow_show_temp(void)
{
    if(digital_output != DIG_OUT_INVALID)
    {
        float f_temperature = 0.0;
        f_temperature = (digital_output & 0x07FF) * 0.0625;
        if(0xF000 == (digital_output & 0xF000))
        {
            f_temperature *= -1.0;
        }
        printf("T = %f deg C\n", f_temperature);
    }
}

uint8_t ow_get_crc(uint8_t *data, uint8_t count);
void ow_read_scratchpad(void)
{
    uint8_t scratch_buff[10] = {0};

    if(NOK == ow_init_seq())
    {
        return;
    }

    // MASTER TX ROM COMMAND
    ow_rom_command_skip();

    // MASTER TX FUNCTION COMMAND
    ow_function_command_convert_temperature();

    if(NOK == ow_init_seq())
    {
        return;
    }
    // MASTER TX ROM COMMAND
    ow_rom_command_skip();

    // MASTER TX FUNCTION COMMAND
    ow_function_command_read_scratchpad(scratch_buff);

    scratch_buff[9] = ow_get_crc(scratch_buff, 8);
    if(scratch_buff[8] == scratch_buff[9]) // check if calculated crc match recieved one
    {
        digital_output = scratch_buff[0] | ((uint16_t)scratch_buff[1] << 8);
    }
    else
    {
        digital_output = DIG_OUT_INVALID;
    }
}

const uint8_t crc88540_table[256] = {
    0, 94,188,226, 97, 63,221,131,194,156,126, 32,163,253, 31, 65,
  157,195, 33,127,252,162, 64, 30, 95,  1,227,189, 62, 96,130,220,
   35,125,159,193, 66, 28,254,160,225,191, 93,  3,128,222, 60, 98,
  190,224,  2, 92,223,129, 99, 61,124, 34,192,158, 29, 67,161,255,
   70, 24,250,164, 39,121,155,197,132,218, 56,102,229,187, 89,  7,
  219,133,103, 57,186,228,  6, 88, 25, 71,165,251,120, 38,196,154,
  101, 59,217,135,  4, 90,184,230,167,249, 27, 69,198,152,122, 36,
  248,166, 68, 26,153,199, 37,123, 58,100,134,216, 91,  5,231,185,
  140,210, 48,110,237,179, 81, 15, 78, 16,242,172, 47,113,147,205,
   17, 79,173,243,112, 46,204,146,211,141,111, 49,178,236, 14, 80,
  175,241, 19, 77,206,144,114, 44,109, 51,209,143, 12, 82,176,238,
   50,108,142,208, 83, 13,239,177,240,174, 76, 18,145,207, 45,115,
  202,148,118, 40,171,245, 23, 73,  8, 86,180,234,105, 55,213,139,
   87,  9,235,181, 54,104,138,212,149,203, 41,119,244,170, 72, 22,
  233,183, 85, 11,136,214, 52,106, 43,117,151,201, 74, 20,246,168,
  116, 42,200,150, 21, 75,169,247,182,232, 10, 84,215,137,107, 53
};

uint8_t ow_get_crc(uint8_t *data, uint8_t count)
{
  uint8_t result=0;

  while(count--) {
    result = crc88540_table[result ^ *data++];
  }

  return result;
}
