#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <pigpio.h>
#include <one-wire.h>

#define MES_INTERVAL_S 1 // Time interval between every measure and check in seconds
#define OPEN_GNUPLOT_CMD "gnuplot" // "gnuplot -persist" add -persist to persist the plot window
#define HEATER_PIN 17
#define HEAT_ON (1)
#define HEAT_OFF (0)
#define DESIRED_TEMPERATURE 27.0 // desired temperature to be controlled as float in deg C
#define TEMP_HYSTERESIS 0.5 // float in deg C

volatile uint8_t sig_exit = 0;
uint8_t heaterState = HEAT_OFF;

void SignalHandler(int sig);

void gnuplot_init(FILE * lgnuplotPipe)
{
    const char * gnuplotCommands[] = {
        "set xrange[0:500]",
        "set yrange[0:35]",
        "set xlabel 'Time [s]'",
        "set ylabel 'Temperature [deg C]'",
        "set title \"Temperature over time\""};
    
    for(uint8_t i = 0; i < sizeof(gnuplotCommands)/sizeof(gnuplotCommands[0]); i++)
    {
        fprintf(lgnuplotPipe, "%s \n", gnuplotCommands[i]);
    }
    fflush(lgnuplotPipe);
}

void control_heater(void)
{

    float f_temperature = ow_read_temp_f();
    if(OW_INVALID_TEMP != f_temperature)
    {
        if(HEAT_OFF == heaterState && (DESIRED_TEMPERATURE - (TEMP_HYSTERESIS / 2)) > f_temperature)
        { // turn On the Heater if the temp is less than 20 deg C and the heater is OFF
            gpioWrite(HEATER_PIN, 1);
            heaterState = HEAT_ON;
        }
        else if(HEAT_ON == heaterState && (DESIRED_TEMPERATURE + (TEMP_HYSTERESIS / 2)) < f_temperature)
        { // turn OFF the Heater if the temp is more than 21 deg C and the heater is ON
            gpioWrite(HEATER_PIN, 0);
            heaterState = HEAT_OFF;
        }
    }
}

int main(void) {

    uint32_t firstMillis;
    float sec_count = 0.0;
    char c = 0;
    FILE * temp_data_exchange = fopen("data.temp", "w");
    FILE * gnuplotPipe = popen(OPEN_GNUPLOT_CMD, "w");

    gnuplot_init(gnuplotPipe);
    if(gpioInitialise() < 0)
    { // init failed
        printf("pigpio Init Failed\n");
        return(0);
    }
    gpioSetMode(HEATER_PIN, PI_OUTPUT); // Set Heater pin to output
    signal(SIGINT, SignalHandler); // attach handler for SIGINT

    printf("Graph generating.\nCtrl-C for exit.\n");
    firstMillis = gpioTick();
    ow_read_scratchpad();
    sec_count = (gpioTick() - firstMillis) / 1000000.0;
    ow_log_temp(temp_data_exchange, sec_count);
    
    while(0 == sig_exit)
    {
        ow_read_scratchpad();
        sec_count = (gpioTick() - firstMillis) / 1000000.0;
        ow_log_temp(temp_data_exchange, sec_count);
        //printf("gpioTick(): %u; firstMillis: %u; sec_count: %f\n", gpioTick(), firstMillis, sec_count); // debug 
        fprintf(gnuplotPipe, "plot 'data.temp' using 1:2 with lines\n");
        fflush(gnuplotPipe);
        control_heater();
        sleep(MES_INTERVAL_S);
    }

    gpioWrite(HEATER_PIN, 0);
    gpioTerminate();
    fclose(temp_data_exchange);
    fclose(gnuplotPipe);
    printf("Terminated\n");

    return(0);
}

void SignalHandler(int sig)
{
    signal(sig, SIG_IGN); // ignore SIGINT signal
    sig_exit = 1;
}
