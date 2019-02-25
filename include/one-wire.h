/*
example include file
*/

#define OW_INVALID_TEMP -300.0

float ow_read_temp_f(void);
void ow_log_temp(FILE * stream, float time_s);
void ow_show_temp(void);
void ow_read_scratchpad(void);
