#ifndef NFC_H_
#define NFC_H_

#include <nfc/nfc.h>

static const int ntag215_total_pages = 135;
static const int ntag215_page_size = 4; // bytes
static const int ntag215_total_user_mem = ntag215_total_pages * ntag215_page_size;

nfc_device *open_nfc_initiator(nfc_context **context);
int poll_ntag215(nfc_device *pnd, nfc_target *nt);
int read_user_mem_str(nfc_device *pnd, char *buf, int total_pages, int user_memory_start_page);

#endif
