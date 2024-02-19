#include "nfc.h"
#include "log.h"
#include "mifare.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#if 0
static const char *libnfc_device = "pn532_uart:/dev/ttyS0";
#endif

nfc_device *open_nfc_initiator(nfc_context **context)
{
#if 0
    if (setenv("LIBNFC_DEFAULT_DEVICE", libnfc_device, 0))
    {
        ERR("failed to set environment variable: %s", strerror(errno));
        return NULL;
    }
#endif

    nfc_init(context);
    if (*context == NULL)
    {
        ERR("unable to init libnfc context");
        return NULL;
    }

    nfc_device *pnd = nfc_open(*context, NULL);
    if (pnd == NULL)
    {
        ERR("Failed to open NFC device");
        goto exit_context;
    }
    INFO("NFC device: %s opened", nfc_device_get_name(pnd));

    if (nfc_initiator_init(pnd) < 0)
    {
        ERR("nfc_initiator_init: %s", nfc_strerror(pnd));
        goto close_device;
    }

    if (nfc_device_set_property_bool(pnd, NP_INFINITE_SELECT, false) < 0)
    {
        ERR("NP_INFINITE_SELECT = false: %s", nfc_strerror(pnd));
        goto close_device;
    }

    return pnd;

close_device:
    nfc_close(pnd);
exit_context:
    nfc_exit(*context);
    return NULL;
}

static int start_raw_mode(nfc_device *pnd)
{
    if (nfc_device_set_property_bool(pnd, NP_HANDLE_CRC, false) < 0)
    {
        ERR("NP_HANDLE_CRC = false: %s", nfc_strerror(pnd));
        return -1;
    }
    if (nfc_device_set_property_bool(pnd, NP_EASY_FRAMING, false) < 0)
    {
        ERR("NP_EASY_FRAMING = false: %s", nfc_strerror(pnd));
        return -1;
    }
    return 0;
}

static int stop_raw_mode(nfc_device *pnd)
{
    if (nfc_device_set_property_bool(pnd, NP_HANDLE_CRC, true) < 0)
    {
        ERR("NP_HANDLE_CRC = true: %s", nfc_strerror(pnd));
        return -1;
    }
    if (nfc_device_set_property_bool(pnd, NP_EASY_FRAMING, true) < 0)
    {
        ERR("NP_EASY_FRAMING = true: %s", nfc_strerror(pnd));
        return -1;
    }
    return 0;
}

static int get_ev1_storage_size(nfc_device *pnd, uint8_t *storage_size)
{
    int ret = 0;

    if (start_raw_mode(pnd) < 0)
        return -1;

    uint8_t tx_buf[] = {0x60, 0x00, 0x00};
    iso14443a_crc_append(tx_buf, 1);

    uint8_t rx_buf[10];
    int szRx = nfc_initiator_transceive_bytes(pnd, tx_buf, sizeof(tx_buf), rx_buf, sizeof(rx_buf), 0);
    if (szRx <= 0)
        ret = -1;
    else
        *storage_size = rx_buf[6];

    if (stop_raw_mode(pnd) < 0)
        ret = -1;
    return ret;
}

int poll_ntag215(nfc_device *pnd, nfc_target *nt)
{
    static const nfc_modulation mifare_modulation = {
        .nmt = NMT_ISO14443A,
        .nbr = NBR_106,
    };

    if (nfc_initiator_select_passive_target(pnd, mifare_modulation, NULL, 0, nt) <= 0)
        return -1;

    // Test if we are dealing with an NTAG21x
    static const uint8_t ntag21x_atqa[] = {0x00, 0x44};
    if (memcmp(nt->nti.nai.abtAtqa, ntag21x_atqa, 2) != 0)
    {
        ERR("tag is not of NTAG21x type");
        return -1;
    }

#if 0
  char uid_str[21] = {0};
  for (size_t i = 0; i < nt->nti.nai.szUidLen; i++)
    sprintf(uid_str + 2 * i, "%02x", nt->nti.nai.abtUid[i]);
  INFO("UID: %s", uid_str);
#endif

    uint8_t storage_size;
    if (get_ev1_storage_size(pnd, &storage_size) < 0)
    {
        ERR("failed to get storage size");
        return -1;
    }

    static const uint8_t ntag215_storage_size = 0x11;
    if (storage_size != ntag215_storage_size)
    {
        ERR("tag is not NTAG215: storage size = 0x%02x", storage_size);
        return -1;
    }

    return 0;
}

int read_user_mem_str(nfc_device *pnd, char *buf, int total_pages, int user_memory_start_page)
{
    int total_bytes_read = 0;

    for (int page = user_memory_start_page; page < total_pages; page += 4)
    {
        // read 4 pages, 4 bytes each => 16 bytes
        mifare_param mp;
        const int bytes_read = (total_pages - page < 4) ? (total_pages - page) * 4 : 16;
        if (!nfc_initiator_mifare_cmd(pnd, MC_READ, page, &mp))
            return 1;
        else
        {
            memcpy(buf + total_bytes_read, mp.mpd.abtData, bytes_read);
            total_bytes_read += bytes_read;

            // check if we have found the terminating zero-byte
            if (memchr(buf, 0x00, total_bytes_read))
                return 0;
        }
    }

    return 1;
}
