#ifndef SMARTCARE_H_INCLUDED
#define SMARTCARE_H_INCLUDED


void smartcare_event_process(int32_t event, uint8_t *pdata, uint16_t len);
void smartcare_init(void);
void smartcare_deinit(void);

#endif // SMARTCARE_H_INCLUDED
