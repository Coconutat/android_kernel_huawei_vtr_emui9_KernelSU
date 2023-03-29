#ifndef FI_H_INCLUDED
#define FI_H_INCLUDED


void fi_event_process(int32_t event, uint8_t *pdata, uint16_t len);
void fi_init(void);
void fi_deinit(void);

#endif // FI_H_INCLUDED
