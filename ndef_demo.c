/******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/*! \file
 *
 *  \author
 *
 *  \brief Demo application
 *
 *  This demo shows how to poll for several types of NFC cards/devices and how
 *  to exchange data with these devices, using the RFAL library.
 *
 *  This demo does not fully implement the activities according to the
 *  standards, it performs the required to communicate with a card/device and
 *  retrieve its UID. Also blocking methods are used for data exchange which
 *  can lead to long periods of blocking CPU/MCU.
 *  For standard compliant example please refer to the Examples provided
 *  with the RFAL library.
 *
 */

/*
 ******************************************************************************
 * INCLUDES
 ******************************************************************************
 */
#include "demo.h"
#include "utils.h"
#include "rfal_nfc.h"
#include "ndef_poller.h"
#include "ndef_t2t.h"
#include "ndef_t4t.h"
#include "ndef_t5t.h"
#include "ndef_message.h"
#include "ndef_types.h"
#include "ndef_dump.h"
#include "mx_button.h"
/*
******************************************************************************
* GLOBAL DEFINES
******************************************************************************
*/

/* Definition of possible states the demo state machine could have */
#define DEMO_ST_NOTINIT               0   /*!< Demo State:  Not initialized */
#define DEMO_ST_START_DISCOVERY       1   /*!< Demo State:  Start Discovery */
#define DEMO_ST_DISCOVERY             2   /*!< Demo State:  Discovery       */

#define NDEF_DEMO_READ                0U  /*!< NDEF menu read               */
#define NDEF_DEMO_WRITE_MSG1          1U  /*!< NDEF menu write 1 record     */
#define NDEF_DEMO_WRITE_MSG2          2U  /*!< NDEF menu write 2 records    */
#define NDEF_DEMO_FORMAT_TAG          3U  /*!< NDEF menu format tag         */
#if NDEF_FEATURE_FULL_API
#define NDEF_DEMO_MAX_FEATURES        4U  /*!< Number of menu items         */
#else
#define NDEF_DEMO_MAX_FEATURES        1U  /*!< Number of menu items         */
#endif /* NDEF_FEATURE_FULL_API */

#define NDEF_WRITE_FORMAT_TIMEOUT   10000U /*!< Timeout before going back to
                                            *  read mode when write/format is
                                            *  selected */
#define NDEF_LED_BLINK_DURATION       250U /*!< Led blink duration           */

#define DEMO_RAW_MESSAGE_BUF_LEN     8192 /*!< Raw message buffer len       */

#define DEMO_ST_MANUFACTURER_ID        0x02U /*!< ST Manufacturer ID        */

#define TOTAL_DURATION_1000MS 1000U
#define TOTAL_DURATION_60000MS 60000U

/*
 ******************************************************************************
 * LOCAL VARIABLES
 ******************************************************************************
 */

/* P2P communication data */
static uint8_t NFCID3[] =
{
  0x01, 0xFE, 0x03, 0x04, 0x05,
  0x06, 0x07, 0x08, 0x09, 0x0A
};
static uint8_t GB[] =
{
  0x46, 0x66, 0x6d, 0x01, 0x01,
  0x11, 0x02, 0x02, 0x07, 0x80,
  0x03, 0x02, 0x00, 0x03, 0x04,
  0x01, 0x32, 0x07, 0x01, 0x03
};

#if RFAL_FEATURE_NFC_DEP
/* P2P communication data */
static uint8_t ndefLLCPSYMM[] = { 0x00, 0x00 };
static uint8_t ndefInit[] =
{
  0x05, 0x20, 0x06, 0x0F, 0x75, 0x72, 0x6E, 0x3A, 0x6E,
  0x66, 0x63, 0x3A, 0x73, 0x6E, 0x3A, 0x73, 0x6E, 0x65,
  0x70, 0x02, 0x02, 0x07, 0x80, 0x05, 0x01, 0x02
};
static const uint8_t ndefSnepPrefix[] =
{
  0x13, 0x20, 0x00, 0x10,
  0x02, 0x00, 0x00, 0x00
};
static const uint8_t URL[] = "st.com/st25-demo";
static ndefConstBuffer bufURL = { URL, sizeof(URL) - 1U };
static uint8_t ndefUriBuffer[255];
#endif /* RFAL_FEATURE_NFC_DEP */

#if RFAL_SUPPORT_CE && RFAL_FEATURE_LISTEN_MODE
#if RFAL_SUPPORT_MODE_LISTEN_NFCA
/* NFC-A CE config */
/* 4-byte UIDs with first byte 0x08 would need random number for the
 * subsequent 3 bytes. 4-byte UIDs with first byte 0x*F are Fixed number,
 * not unique, use for this demo. 7-byte UIDs need a manufacturer ID and
 * need to assure uniqueness of the rest.
 */
static uint8_t ceNFCA_NFCID[]    = { 0x5F, 'S', 'T', 'M' };
static uint8_t ceNFCA_SENS_RES[] = { 0x02, 0x00 };
static uint8_t ceNFCA_SEL_RES    = 0x20;
#endif /* RFAL_SUPPORT_MODE_LISTEN_NFCA */

#if RFAL_SUPPORT_MODE_LISTEN_NFCF
/* NFC-F CE config */
static uint8_t ceNFCF_nfcid2[]   =
{
  0x02, 0xFE, 0x11, 0x22,
  0x33, 0x44, 0x55, 0x66
};
static uint8_t ceNFCF_SC[]       = { 0x12, 0xFC };
static uint8_t ceNFCF_SENSF_RES[] =
{
  0x01,                                                   /* SENSF_RES        */
  0x02, 0xFE, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66,         /* NFCID2           */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0x7F, 0x00,         /* PAD, MRTI, PAD2  */
  0x00, 0x00                                              /* RD               */
};
#endif /* RFAL_SUPPORT_MODE_LISTEN_NFCF */
#endif /* RFAL_SUPPORT_CE && RFAL_FEATURE_LISTEN_MODE */

static uint8_t *ndefStates[] =
{
  (uint8_t *)"INVALID",
  (uint8_t *)"INITIALIZED",
  (uint8_t *)"READ/WRITE",
  (uint8_t *)"READ-ONLY"
};

static const uint8_t *ndefDemoFeatureDescription[NDEF_DEMO_MAX_FEATURES] =
{
  (uint8_t *)"1. Tap a tag to read its content",
#if NDEF_FEATURE_FULL_API
  (uint8_t *)"2. Present a tag to write a Text record",
  (uint8_t *)"3. Present a tag to write a URI record and an Android "
  "Application record",
  (uint8_t *)"4. Present an ST tag to format",
#endif /* NDEF_FEATURE_FULL_API */
};

#if NDEF_FEATURE_FULL_API
static uint8_t ndefURI[]          = "st.com/st25-demo";
static uint8_t ndefTEXT[]         = "Welcome to ST NDEF demo";
static uint8_t ndefTextLangCode[] = "en";

static uint8_t ndefAndroidPackName[] = "com.st.st25nfc";
#endif /* NDEF_FEATURE_FULL_API */

/*
 ******************************************************************************
 * LOCAL VARIABLES
 ******************************************************************************
 */

static rfalNfcDiscoverParam discParam;
static uint8_t              state = DEMO_ST_NOTINIT;

static ndefContext          ndefCtx;
static uint8_t              ndefDemoFeature     = NDEF_DEMO_READ;
static uint8_t              ndefDemoPrevFeature = 0xFF;
static bool                 verbose             = false;

static uint8_t              rawMessageBuf[DEMO_RAW_MESSAGE_BUF_LEN];

static uint32_t             timer;
static uint32_t             timerLed;
static bool                 ledOn;

/*
******************************************************************************
* LOCAL FUNCTION PROTOTYPES
******************************************************************************
*/

static void demoNdef(rfalNfcDevice *nfcDevice);
static void ndefCCDump(ndefContext *ctx);
#if NDEF_FEATURE_T5T
static void ndefDumpSysInfo(ndefContext *ctx);
#endif /* NDEF_FEATURE_T5T */

#if NDEF_FEATURE_FULL_API
static bool ndefIsSTTag(ndefContext *ctx);
static void LedNotificationWriteDone(void);
#endif /* NDEF_FEATURE_FULL_API */

static void demoP2P(void);
ReturnCode  demoTransceiveBlocking(uint8_t *txBuf,
                                   uint16_t txBufSize,
                                   uint8_t **rxBuf,
                                   uint16_t **rcvLen,
                                   uint32_t fwt);

static void ledsOn(void);
static void ledsOff(void);
static void ndefShowDemoUsage(void);

/*!
 *****************************************************************************
 * \brief Check user button
 *
 *  This function checks whether the user button has been pressed
 *****************************************************************************
 */

static void checkUserButton(void)
{
  /* Check if USER button is pressed */
  if (HAL_GPIO_ReadPin(PLATFORM_USER_BUTTON_PORT, PLATFORM_USER_BUTTON_PIN))
  {
    ndefDemoFeature++;
    ndefDemoFeature %= NDEF_DEMO_MAX_FEATURES;

    ledsOff();
    ndefDemoPrevFeature = ndefDemoFeature;
    PLATFORM_LOG("%s\r\n", ndefDemoFeatureDescription[ndefDemoFeature]);

    /* Debounce button */
    while (HAL_GPIO_ReadPin(PLATFORM_USER_BUTTON_PORT,
                            PLATFORM_USER_BUTTON_PIN))
    {
    }

    if (ndefDemoFeature != NDEF_DEMO_READ)
    {
      timer    = PLATFORM_TIMER_CREATE(NDEF_WRITE_FORMAT_TIMEOUT);
      timerLed = PLATFORM_TIMER_CREATE(NDEF_LED_BLINK_DURATION);
    }
  }
}

/*!
 *****************************************************************************
 * \brief Show usage
 *
 *  This function displays usage information
 *****************************************************************************
 */
static void ndefShowDemoUsage(void)
{
#if NDEF_FEATURE_FULL_API
  uint32_t i;

  PLATFORM_LOG("Use the User button to cycle among the different modes:\r\n");
  for (i = 0; i < SIZEOF_ARRAY(ndefDemoFeatureDescription); i++)
  {
    PLATFORM_LOG("%s\r\n", ndefDemoFeatureDescription[i]);
  }
  PLATFORM_LOG("In Write or Format mode (menu 2, 3 or 4), the demo returns "
               "to Read mode (menu 1) if no tag detected after %d seconds\r\n\n",
               NDEF_WRITE_FORMAT_TIMEOUT / 1000U);
#endif /* NDEF_FEATURE_FULL_API */
}

/*!
 *****************************************************************************
 * \brief Demo Ini
 *
 *  This function Initializes the required layers for the demo
 *
 * \return true  : Initialization ok
 * \return false : Initialization failed
 *****************************************************************************
 */
bool demoIni(void)
{
  ReturnCode err;

  /* Enable this block if you want verbose when debugging on certain MCUs */
  /*
  if ((CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) != 0)
  {
      verbose = true;
  }
  */

  ndefShowDemoUsage();

  err = rfalNfcInitialize();
  if (err == ERR_NONE)
  {
    rfalNfcDefaultDiscParams(&discParam);

    discParam.devLimit = 1U;

    ST_MEMCPY(&discParam.nfcid3, NFCID3, sizeof(NFCID3));
    ST_MEMCPY(&discParam.GB, GB, sizeof(GB));
    discParam.GBLen       = sizeof(GB);
    discParam.p2pNfcaPrio = true;

    discParam.notifyCb      = NULL;
    discParam.totalDuration = TOTAL_DURATION_1000MS;
    /* For the demo, enable the NFC Technologies based on RFAL feature
     * switches */
    discParam.techs2Find    = RFAL_NFC_TECH_NONE;

#if RFAL_FEATURE_NFCA
    discParam.techs2Find   |= RFAL_NFC_POLL_TECH_A;
#endif /* RFAL_FEATURE_NFCA */

#if RFAL_FEATURE_NFCB
    discParam.techs2Find   |= RFAL_NFC_POLL_TECH_B;
#endif /* RFAL_FEATURE_NFCB */

#if RFAL_FEATURE_NFCF
    discParam.techs2Find   |= RFAL_NFC_POLL_TECH_F;
#endif /* RFAL_FEATURE_NFCF */

#if RFAL_FEATURE_NFCV
    discParam.techs2Find   |= RFAL_NFC_POLL_TECH_V;
#endif /* RFAL_FEATURE_NFCV */

#if RFAL_FEATURE_ST25TB
    discParam.techs2Find   |= RFAL_NFC_POLL_TECH_ST25TB;
#endif /* RFAL_FEATURE_ST25TB */

#if ST25R95
    /* ST25R95 cannot support 256 bytes of data block */
    discParam.isoDepFS = RFAL_ISODEP_FSXI_128;
#endif /* ST25R95 */

#if RFAL_SUPPORT_MODE_POLL_ACTIVE_P2P && RFAL_FEATURE_NFC_DEP
    discParam.techs2Find |= RFAL_NFC_POLL_TECH_AP2P;
#endif /* RFAL_SUPPORT_MODE_POLL_ACTIVE_P2P && RFAL_FEATURE_NFC_DEP */

#if RFAL_SUPPORT_MODE_LISTEN_ACTIVE_P2P && RFAL_FEATURE_NFC_DEP && RFAL_FEATURE_LISTEN_MODE
    discParam.techs2Find |= RFAL_NFC_LISTEN_TECH_AP2P;
#endif /* RFAL_SUPPORT_MODE_LISTEN_ACTIVE_P2P && RFAL_FEATURE_NFC_DEP && RFAL_FEATURE_LISTEN_MODE */

#if DEMO_CARD_EMULATION_ONLY
    discParam.totalDuration = TOTAL_DURATION_60000MS;             /* 60 seconds */
    discParam.techs2Find    = RFAL_NFC_TECH_NONE; /* Only CE    */
#endif /* DEMO_CARD_EMULATION_ONLY */

#if RFAL_SUPPORT_CE && RFAL_FEATURE_LISTEN_MODE

#if RFAL_SUPPORT_MODE_LISTEN_NFCA
    /* Set configuration for NFC-A CE */
    ST_MEMCPY(discParam.lmConfigPA.SENS_RES,
              ceNFCA_SENS_RES,
              RFAL_LM_SENS_RES_LEN);      /* SENS_RES / ATQA            */
    ST_MEMCPY(discParam.lmConfigPA.nfcid,
              ceNFCA_NFCID,
              RFAL_LM_NFCID_LEN_04);      /* NFCID / UID                */
    discParam.lmConfigPA.nfcidLen = RFAL_LM_NFCID_LEN_04;
    discParam.lmConfigPA.SEL_RES  = ceNFCA_SEL_RES;

    discParam.techs2Find |= RFAL_NFC_LISTEN_TECH_A;
#endif /* RFAL_SUPPORT_MODE_LISTEN_NFCA */

#if RFAL_SUPPORT_MODE_LISTEN_NFCF
    /* Set configuration for NFC-F CE */
    ST_MEMCPY(discParam.lmConfigPF.SC,
              ceNFCF_SC,
              RFAL_LM_SENSF_SC_LEN);          /* System Code           */
    ST_MEMCPY(&ceNFCF_SENSF_RES[RFAL_NFCF_CMD_LEN],
              ceNFCF_nfcid2,
              RFAL_NFCID2_LEN);               /* NFCID2 in SENSF_RES   */
    ST_MEMCPY(discParam.lmConfigPF.SENSF_RES,
              ceNFCF_SENSF_RES,
              RFAL_LM_SENSF_RES_LEN);         /* Poll Response         */

    discParam.techs2Find |= RFAL_NFC_LISTEN_TECH_F;
#endif /* RFAL_SUPPORT_MODE_LISTEN_NFCF */
#endif /* RFAL_SUPPORT_CE && RFAL_FEATURE_LISTEN_MODE */

    /* Check for valid configuration by calling Discover once */
    err = rfalNfcDiscover(&discParam);
    rfalNfcDeactivate(RFAL_NFC_DEACTIVATE_IDLE);

    if (err != ERR_NONE)
    {
      return false;
    }

    state = DEMO_ST_START_DISCOVERY;
    return true;
  }
  return false;
}

/*!
 *****************************************************************************
 * \brief Demo Cycle
 *
 *  This function executes the demo state machine.
 *  It must be called periodically
 *****************************************************************************
 */
void demoCycle(void)
{
  static rfalNfcDevice *nfcDevice;

#if RFAL_FEATURE_NFCA
  rfalNfcaSensRes sensRes;
  rfalNfcaSelRes  selRes;
#endif /* RFAL_FEATURE_NFCA */

#if RFAL_FEATURE_NFCB
  rfalNfcbSensbRes sensbRes;
  uint8_t          sensbResLen;
#endif /* RFAL_FEATURE_NFCB */

#if RFAL_FEATURE_NFCF
  uint8_t           devCnt     = 0;
  rfalFeliCaPollRes cardList[1];
  uint8_t           collisions = 0U;
  rfalNfcfSensfRes *sensfRes;
#endif /* RFAL_FEATURE_NFCF */

#if RFAL_FEATURE_NFCV
  rfalNfcvInventoryRes invRes;
  uint16_t             rcvdLen;
#endif /* RFAL_FEATURE_NFCV */

  rfalNfcWorker(); /* Run RFAL worker periodically */

  if ((ndefDemoFeature != NDEF_DEMO_READ) && (PLATFORM_TIMER_IS_EXPIRED(timer)))
  {
    PLATFORM_LOG("Timer expired, back to Read mode...\r\n");
    ndefDemoFeature = NDEF_DEMO_READ;
    ledsOff();
  }

  if (ndefDemoFeature != ndefDemoPrevFeature)
  {
    ndefDemoPrevFeature = ndefDemoFeature;
    PLATFORM_LOG("%s\r\n", ndefDemoFeatureDescription[ndefDemoFeature]);
  }

  if (ndefDemoFeature != NDEF_DEMO_READ)
  {
    if (PLATFORM_TIMER_IS_EXPIRED(timerLed))
    {
      timerLed = PLATFORM_TIMER_CREATE(NDEF_LED_BLINK_DURATION);
      ledOn    = !ledOn;
    }

    if (ledOn)
    {
      ledsOn();
    }
    else
    {
      ledsOff();
    }
  }

  checkUserButton();

  switch (state)
  {
    /*******************************************************************************/
    case DEMO_ST_START_DISCOVERY:
      ledsOff();

      rfalNfcDeactivate(RFAL_NFC_DEACTIVATE_IDLE);
      rfalNfcDiscover(&discParam);

      state = DEMO_ST_DISCOVERY;
      break;

    /*******************************************************************************/
    case DEMO_ST_DISCOVERY:
      if (rfalNfcIsDevActivated(rfalNfcGetState()))
      {
        rfalNfcGetActiveDevice(&nfcDevice);

        ledsOff();
        PLATFORM_DELAY(50);
        ndefDemoPrevFeature = 0xFF; /* Force the display of prompt */
        switch (nfcDevice->type)
        {
            /*******************************************************************************/
#if RFAL_FEATURE_NFCA
          case RFAL_NFC_LISTEN_TYPE_NFCA:

            switch (nfcDevice->dev.nfca.type)
            {
              case RFAL_NFCA_T1T:
                PLATFORM_LOG("ISO14443A/Topaz (NFC-A T1T) TAG "
                             "found. UID: %s\r\n",
                             hex2Str(nfcDevice->nfcid,
                                     nfcDevice->nfcidLen));
                rfalNfcaPollerSleep();
                break;

              case RFAL_NFCA_T4T:
                PLATFORM_LOG("NFCA Passive ISO-DEP device "
                             "found. UID: %s\r\n",
                             hex2Str(nfcDevice->nfcid,
                                     nfcDevice->nfcidLen));
                demoNdef(nfcDevice);
                rfalIsoDepDeselect();
                break;

              case RFAL_NFCA_T4T_NFCDEP:
              case RFAL_NFCA_NFCDEP:
                PLATFORM_LOG("NFCA Passive P2P device found. "
                             "NFCID: %s\r\n",
                             hex2Str(nfcDevice->nfcid,
                                     nfcDevice->nfcidLen));
                demoP2P();
                break;

              default:
                PLATFORM_LOG("ISO14443A/NFC-A card found. "
                             "UID: %s\r\n",
                             hex2Str(nfcDevice->nfcid,
                                     nfcDevice->nfcidLen));
                demoNdef(nfcDevice);
                rfalNfcaPollerSleep();
                break;
            }

            /* Loop until tag is removed from the field */
            PLATFORM_LOG("Operation completed\r\nTag can be "
                         "removed from the field\r\n");
            rfalNfcaPollerInitialize();
            while (rfalNfcaPollerCheckPresence(
                     RFAL_14443A_SHORTFRAME_CMD_WUPA,
                     &sensRes) == ERR_NONE)
            {
              if (((nfcDevice->dev.nfca.type == RFAL_NFCA_T1T) && (!rfalNfcaIsSensResT1T(&sensRes)))
                  || ((nfcDevice->dev.nfca.type != RFAL_NFCA_T1T)
                      && (rfalNfcaPollerSelect(nfcDevice->dev.nfca.nfcId1, nfcDevice->dev.nfca.nfcId1Len, &selRes)
                          != ERR_NONE)))
              {
                break;
              }
              rfalNfcaPollerSleep();
              PLATFORM_DELAY(130);
            }
            break;
#endif /* RFAL_FEATURE_NFCA */

            /*******************************************************************************/
#if RFAL_FEATURE_NFCB
          case RFAL_NFC_LISTEN_TYPE_NFCB:

            PLATFORM_LOG("ISO14443B/NFC-B card found. UID: %s\r\n",
                         hex2Str(nfcDevice->nfcid,
                                 nfcDevice->nfcidLen));

            if (rfalNfcbIsIsoDepSupported(
                  &nfcDevice->dev.nfcb))
            {
              demoNdef(nfcDevice);
              rfalIsoDepDeselect();
            }
            else
            {
              rfalNfcbPollerSleep(
                nfcDevice->dev.nfcb.sensbRes.nfcid0);
            }

            /* Loop until tag is removed from the field */
            PLATFORM_LOG("Operation completed\r\nTag can be "
                         "removed from the field\r\n");
            rfalNfcbPollerInitialize();
            while (rfalNfcbPollerCheckPresence(
                     RFAL_NFCB_SENS_CMD_ALLB_REQ,
                     RFAL_NFCB_SLOT_NUM_1,
                     &sensbRes,
                     &sensbResLen) == ERR_NONE)
            {
              if (ST_BYTECMP(
                    sensbRes.nfcid0,
                    nfcDevice->dev.nfcb.sensbRes.nfcid0,
                    RFAL_NFCB_NFCID0_LEN) != 0)
              {
                break;
              }
              rfalNfcbPollerSleep(
                nfcDevice->dev.nfcb.sensbRes.nfcid0);
              PLATFORM_DELAY(130);
            }
            break;
#endif /* RFAL_FEATURE_NFCB */

            /*******************************************************************************/
#if RFAL_FEATURE_NFCF
          case RFAL_NFC_LISTEN_TYPE_NFCF:

            if (rfalNfcfIsNfcDepSupported(&nfcDevice->dev.nfcf))
            {
              PLATFORM_LOG("NFCF Passive P2P device found. "
                           "NFCID: %s\r\n",
                           hex2Str(nfcDevice->nfcid,
                                   nfcDevice->nfcidLen));
              demoP2P();
            }
            else
            {
              PLATFORM_LOG("Felica/NFC-F card found. UID: %s\r\n",
                           hex2Str(nfcDevice->nfcid,
                                   nfcDevice->nfcidLen));
              demoNdef(nfcDevice);
            }

            /* Loop until tag is removed from the field */
            PLATFORM_LOG("Operation completed\r\nTag can be "
                         "removed from the field\r\n");
            devCnt = 1U;
            rfalNfcfPollerInitialize(RFAL_BR_212);
            while (rfalNfcfPollerPoll(
                     RFAL_FELICA_1_SLOT,
                     RFAL_NFCF_SYSTEMCODE,
                     RFAL_FELICA_POLL_RC_NO_REQUEST,
                     cardList,
                     &devCnt,
                     &collisions) == ERR_NONE)
            {
              /* Skip the length field byte */
              sensfRes =
                (rfalNfcfSensfRes *) & ((uint8_t *)cardList)[1];
              if (ST_BYTECMP(
                    sensfRes->NFCID2,
                    nfcDevice->dev.nfcf.sensfRes.NFCID2,
                    RFAL_NFCF_NFCID2_LEN) != 0)
              {
                break;
              }
              PLATFORM_DELAY(130);
            }
            break;
#endif /* RFAL_FEATURE_NFCF */

            /*******************************************************************************/
#if RFAL_FEATURE_NFCV
          case RFAL_NFC_LISTEN_TYPE_NFCV:
          {
            uint8_t devUID[RFAL_NFCV_UID_LEN];

            /* Copy the UID into local var and reverse for display */
            ST_MEMCPY(devUID,
                      nfcDevice->nfcid,
                      nfcDevice->nfcidLen);
            REVERSE_BYTES(devUID, RFAL_NFCV_UID_LEN);
            PLATFORM_LOG("ISO15693/NFC-V card found. UID: %s\r\n",
                         hex2Str(devUID, RFAL_NFCV_UID_LEN));

            demoNdef(nfcDevice);

            /* Loop until tag is removed from the field */
            PLATFORM_LOG("Operation completed\r\nTag can be "
                         "removed from the field\r\n");
            rfalNfcvPollerInitialize();
            while (rfalNfcvPollerInventory(
                     RFAL_NFCV_NUM_SLOTS_1,
                     RFAL_NFCV_UID_LEN * 8U,
                     nfcDevice->dev.nfcv.InvRes.UID,
                     &invRes,
                     &rcvdLen) == ERR_NONE)
            {
              PLATFORM_DELAY(130);
            }
          }
          break;
#endif /* RFAL_FEATURE_NFCV */

            /*******************************************************************************/
#if RFAL_FEATURE_ST25TB
          case RFAL_NFC_LISTEN_TYPE_ST25TB:

            PLATFORM_LOG("ST25TB card found. UID: %s\r\n",
                         hex2Str(nfcDevice->nfcid,
                                 nfcDevice->nfcidLen));
            break;
#endif /* RFAL_FEATURE_ST25TB */

          /*******************************************************************************/
          case RFAL_NFC_LISTEN_TYPE_AP2P:

            PLATFORM_LOG("NFC Active P2P device found. NFCID3: "
                         "%s\r\n",
                         hex2Str(nfcDevice->nfcid,
                                 nfcDevice->nfcidLen));

            demoP2P();
            break;

          /*******************************************************************************/
          default:
            break;
        }

        rfalNfcDeactivate(RFAL_NFC_DEACTIVATE_IDLE);
#if !defined(DEMO_NO_DELAY_IN_DEMOCYCLE)
        switch (nfcDevice->type)
        {
          case RFAL_NFC_POLL_TYPE_NFCA:
          case RFAL_NFC_POLL_TYPE_NFCF:
            /* When being in card emulation done at delay as some
             * polling devices rely on tags to be re-discoverable
             */
            break;

          default:
            /* Delay before re-starting polling loop to not flood
             * the UART log with re-discovered tags not supporting
             * presence check
             */
            PLATFORM_DELAY(500);
        }
#endif /* DEMO_NO_DELAY_IN_DEMOCYCLE */
        state = DEMO_ST_START_DISCOVERY;
      }
      break;

    /*******************************************************************************/
    case DEMO_ST_NOTINIT:
    default:
      break;
  }
}

/*!
 *****************************************************************************
 * \brief Demo P2P Exchange
 *
 * Sends a NDEF URI record 'http://www.ST.com' via NFC-DEP (P2P) protocol.
 *
 * This method sends a set of static predefined frames which tries to establish
 * a LLCP connection, followed by the NDEF record, and then keeps sending
 * LLCP SYMM packets to maintain the connection.
 *
 *****************************************************************************
 */
void demoP2P(void)
{
#if RFAL_FEATURE_NFC_DEP

  uint16_t  *rxLen;
  uint8_t   *rxData;
  ReturnCode err;

  ndefBuffer  bufPayload;
  ndefMessage message;
  ndefRecord  record;
  ndefType    uri;

  PLATFORM_LOG(" Initialize device .. ");
  err = demoTransceiveBlocking(ndefInit,
                               (uint16_t)sizeof(ndefInit),
                               &rxData,
                               &rxLen,
                               RFAL_FWT_NONE);
  if (err != ERR_NONE)
  {
    PLATFORM_LOG("failed.");
    return;
  }
  PLATFORM_LOG("succeeded.\r\n");

  err  = ndefRtdUriInit(&uri, NDEF_URI_PREFIX_HTTP_WWW, &bufURL);
  err |= ndefRtdUriToRecord(&uri, &record);

  err |= ndefMessageInit(&message);
  err |= ndefMessageAppend(&message, &record); /* Set MB and ME bits */

  /* Build the SNEP buffer made of the prefix, the length byte and record */
  ST_MEMCPY(ndefUriBuffer, ndefSnepPrefix, sizeof(ndefSnepPrefix));

  /* Skip 1 byte for length byte */
  bufPayload.buffer = ndefUriBuffer + sizeof(ndefSnepPrefix) + 1U;
  bufPayload.length = sizeof(ndefUriBuffer) - sizeof(ndefSnepPrefix);
  err |= ndefMessageEncode(&message, &bufPayload);

  ndefUriBuffer[sizeof(ndefSnepPrefix)] = (uint8_t)bufPayload.length;

  bufPayload.buffer = ndefUriBuffer;
  bufPayload.length =
    (uint32_t)sizeof(ndefSnepPrefix) + 1U + bufPayload.length;

  if (err != ERR_NONE)
  {
    PLATFORM_LOG("NDEF message creation failed (%d)\r\n", err);
    return;
  }

  ndefBufferDump("URL converted to SNEP:\r\n",
                 (ndefConstBuffer *)&bufPayload,
                 true);

  PLATFORM_LOG(" Push NDEF Uri: st.com/st25-demo ...");
  err = demoTransceiveBlocking(bufPayload.buffer,
                               (uint16_t)bufPayload.length,
                               &rxData,
                               &rxLen,
                               RFAL_FWT_NONE);
  if (err != ERR_NONE)
  {
    PLATFORM_LOG("failed.");
    return;
  }
  PLATFORM_LOG("succeeded.\r\n");

  PLATFORM_LOG(" Device present, maintaining connection ");
  while (err == ERR_NONE)
  {
    err = demoTransceiveBlocking(ndefLLCPSYMM,
                                 (uint16_t)sizeof(ndefLLCPSYMM),
                                 &rxData,
                                 &rxLen,
                                 RFAL_FWT_NONE);
    PLATFORM_LOG(".");
    PLATFORM_DELAY(50);
  }
  PLATFORM_LOG("\r\n Device removed.\r\n");
#endif /* RFAL_FEATURE_NFC_DEP */
}

/*!
 *****************************************************************************
 * \brief Demo Blocking Transceive
 *
 * Helper function to send data in a blocking manner via the rfalNfc module.
 *
 * \warning A protocol transceive handles long timeouts (several seconds),
 * transmission errors and retransmissions which can lead to a long period of
 * time where the MCU/CPU is blocked in this method.
 *
 *****************************************************************************
 */
ReturnCode demoTransceiveBlocking(uint8_t  *txBuf,
                                  uint16_t  txBufSize,
                                  uint8_t **rxData,
                                  uint16_t **rcvLen,
                                  uint32_t  fwt)
{
  ReturnCode err;

  err = rfalNfcDataExchangeStart(txBuf, txBufSize, rxData, rcvLen, fwt);
  if (err == ERR_NONE)
  {
    do
    {
      rfalNfcWorker();
      err = rfalNfcDataExchangeGetStatus();
    } while (err == ERR_BUSY);
  }
  return err;
}

static void demoNdef(rfalNfcDevice *pNfcDevice)
{
  ReturnCode      err;
  ndefMessage     message;
  uint32_t        rawMessageLen;
  ndefInfo        info;
  ndefBuffer      bufRawMessage;
  ndefConstBuffer bufConstRawMessage;

#if NDEF_FEATURE_FULL_API
  ndefRecord       record1;
  ndefRecord       record2;

  ndefType         text;
  ndefType         uri;
  ndefType         aar;

  ndefConstBuffer8 bufTextLangCode;
  ndefConstBuffer  bufTextLangText;
  ndefConstBuffer  bufUri;
  ndefConstBuffer  bufAndroidPackName;
#endif /* NDEF_FEATURE_FULL_API */

  /* Perform NDEF Context Initialization */
  err = ndefPollerContextInitialization(&ndefCtx, pNfcDevice);
  if (err != ERR_NONE)
  {
    PLATFORM_LOG("NDEF NOT DETECTED (ndefPollerContextInitialization "
                 "returns %d)\r\n",
                 err);
    return;
  }

#if NDEF_FEATURE_T5T
  if (verbose && (pNfcDevice->type == RFAL_NFC_LISTEN_TYPE_NFCV))
  {
    ndefDumpSysInfo(&ndefCtx);
  }
#endif /* NDEF_FEATURE_T5T */

  /* Perform NDEF Detect procedure */
  err = ndefPollerNdefDetect(&ndefCtx, &info);
  if (err != ERR_NONE)
  {
    PLATFORM_LOG("NDEF NOT DETECTED (ndefPollerNdefDetect returns %d)\r\n",
                 err);
    if (ndefDemoFeature != NDEF_DEMO_FORMAT_TAG)
    {
      return;
    }
  }
  else
  {
    PLATFORM_LOG("%s NDEF detected.\r\n", ndefStates[info.state]);
    ndefCCDump(&ndefCtx);

    if (verbose)
    {
      PLATFORM_LOG("NDEF Len: %d, Offset=%d\r\n",
                   ndefCtx.messageLen,
                   ndefCtx.messageOffset);
    }
  }

  switch (ndefDemoFeature)
  {
    /*
     * Demonstrate how to read the NDEF message from the Tag
     */
    case NDEF_DEMO_READ:
      if (info.state == NDEF_STATE_INITIALIZED)
      {
        /* Nothing to read... */
        return;
      }
      err = ndefPollerReadRawMessage(&ndefCtx,
                                     rawMessageBuf,
                                     sizeof(rawMessageBuf),
                                     &rawMessageLen,
                                     true);
      if (err != ERR_NONE)
      {
        PLATFORM_LOG("NDEF message cannot be read "
                     "(ndefPollerReadRawMessage returns %d)\r\n",
                     err);
        return;
      }
      if (verbose)
      {
        bufRawMessage.buffer = rawMessageBuf;
        bufRawMessage.length = rawMessageLen;
        ndefBufferDump(" NDEF Content",
                       (ndefConstBuffer *)&bufRawMessage,
                       verbose);
      }
      bufConstRawMessage.buffer = rawMessageBuf;
      bufConstRawMessage.length = rawMessageLen;
      err = ndefMessageDecode(&bufConstRawMessage, &message);
      if (err != ERR_NONE)
      {
        PLATFORM_LOG("NDEF message cannot be decoded "
                     "(ndefMessageDecode returns %d)\r\n",
                     err);
        return;
      }
      err = ndefMessageDump(&message, verbose);
      if (err != ERR_NONE)
      {
        PLATFORM_LOG("NDEF message cannot be displayed "
                     "(ndefMessageDump returns %d)\r\n",
                     err);
        return;
      }
      break;

#if NDEF_FEATURE_FULL_API
    /*
     * Demonstrate how to encode a text record and write the message
     */
    case NDEF_DEMO_WRITE_MSG1:
      ndefDemoFeature = NDEF_DEMO_READ; /* back to READ after write */
      err  = ndefMessageInit(&message); /* Initialize message       */
      bufTextLangCode.buffer = ndefTextLangCode;
      bufTextLangCode.length =
        (uint32_t)strlen((char *)ndefTextLangCode);

      bufTextLangText.buffer = ndefTEXT;
      bufTextLangText.length =
        (uint32_t)strlen((char *)ndefTEXT);

      err |= ndefRtdTextInit(&text,
                             TEXT_ENCODING_UTF8,
                             &bufTextLangCode,
                             &bufTextLangText);
      err |= ndefRtdTextToRecord(&text, &record1);
      err |= ndefMessageAppend(&message, &record1);
      if (err != ERR_NONE)
      {
        PLATFORM_LOG("Message creation failed\r\n");
        return;
      }
      err = ndefPollerWriteMessage(&ndefCtx, &message);
      if (err != ERR_NONE)
      {
        PLATFORM_LOG("Message cannot be written "
                     "(ndefPollerWriteMessage return %d)\r\n",
                     err);
        return;
      }
      PLATFORM_LOG("Wrote 1 record to the Tag\r\n");
      if (verbose)
      {
        bufRawMessage.buffer = rawMessageBuf;
        bufRawMessage.length = sizeof(rawMessageBuf);
        err = ndefMessageEncode(&message, &bufRawMessage);
        if (err == ERR_NONE)
        {
          ndefBufferDump("Raw message",
                         (ndefConstBuffer *)&bufRawMessage,
                         verbose);
        }
      }
      LedNotificationWriteDone();
      break;

    /*
     * Demonstrate how to encode a URI record and an AAR record,
     * encode the message to a raw buffer and then write the raw buffer.
     */
    case NDEF_DEMO_WRITE_MSG2:
      ndefDemoFeature = NDEF_DEMO_READ;  /* back to READ after write */
      err  = ndefMessageInit(&message);
      bufUri.buffer = ndefURI;
      bufUri.length = (uint32_t)strlen((char *)ndefURI);
      err |= ndefRtdUriInit(&uri,
                            NDEF_URI_PREFIX_HTTP_WWW,
                            &bufUri);
      err |= ndefRtdUriToRecord(&uri, &record1);

      bufAndroidPackName.buffer = ndefAndroidPackName;
      bufAndroidPackName.length =
        sizeof(ndefAndroidPackName) - 1U;
      err |= ndefRtdAarInit(&aar, &bufAndroidPackName);
      err |= ndefRtdAarToRecord(&aar, &record2);

      err |= ndefMessageAppend(&message, &record1);
      err |= ndefMessageAppend(&message, &record2);

      bufRawMessage.buffer = rawMessageBuf;
      bufRawMessage.length = sizeof(rawMessageBuf);
      err |= ndefMessageEncode(&message, &bufRawMessage);
      if (err != ERR_NONE)
      {
        PLATFORM_LOG("Raw message creation failed (%d)\r\n", err);
        return;
      }
      err = ndefPollerWriteRawMessage(&ndefCtx,
                                      bufRawMessage.buffer,
                                      bufRawMessage.length);
      if (err != ERR_NONE)
      {
        PLATFORM_LOG("Message cannot be written "
                     "(ndefPollerWriteRawMessage return %d)\r\n",
                     err);
        return;
      }
      PLATFORM_LOG("Wrote 2 records to the Tag\r\n");
      if (verbose)
      {
        ndefBufferDump("Raw message",
                       (ndefConstBuffer *)&bufRawMessage,
                       verbose);
      }
      LedNotificationWriteDone();
      break;

    /*
     * Demonstrate how to format a Tag
     */
    case NDEF_DEMO_FORMAT_TAG:
      ndefDemoFeature = NDEF_DEMO_READ;
      if (!ndefIsSTTag(&ndefCtx))
      {
        PLATFORM_LOG("Manufacturer ID not found or not an ST tag. "
                     "Format aborted \r\n");
        return;
      }
      PLATFORM_LOG("Formatting Tag...\r\n");
      err = ndefPollerTagFormat(&ndefCtx, NULL, 0U);
      if (err != ERR_NONE)
      {
        PLATFORM_LOG("Tag cannot be formatted "
                     "(ndefPollerTagFormat returns %d)\r\n",
                     err);
        return;
      }
      PLATFORM_LOG("Tag formatted\r\n");
      LedNotificationWriteDone();
      break;
#endif /* NDEF_FEATURE_FULL_API */

    default:
      ndefDemoFeature = NDEF_DEMO_READ;
      break;
  }
}

#if NDEF_FEATURE_T2T
static void ndefT2TCCDump(ndefContext *ctx)
{
  ndefConstBuffer bufCcBuf;

  PLATFORM_LOG(" * Magic: %2.2Xh Version: %d.%d Size: %d (%d bytes) \r\n"
               " * readAccess: %2.2xh writeAccess: %2.2xh \r\n",
               ctx->cc.t2t.magicNumber,
               ctx->cc.t2t.majorVersion,
               ctx->cc.t2t.minorVersion,
               ctx->cc.t2t.size,
               ctx->cc.t2t.size * 8U,
               ctx->cc.t2t.readAccess,
               ctx->cc.t2t.writeAccess);
  bufCcBuf.buffer = ctx->ccBuf;
  bufCcBuf.length = 4U;
  ndefBufferDump(" CC Raw Data", &bufCcBuf, verbose);
}
#endif /* NDEF_FEATURE_T2T */

#if NDEF_FEATURE_T3T
static void ndefT3TAIBDump(ndefContext *ctx)
{
  ndefConstBuffer bufCcBuf;

  PLATFORM_LOG(" * Version: %d.%d Size: %d (%d bytes) NbR: %d NbW: %d\r\n"
               " * WriteFlag: %2.2xh RWFlag: %2.2xh \r\n",
               ctx->cc.t3t.majorVersion,
               ctx->cc.t3t.minorVersion,
               ctx->cc.t3t.nMaxB,
               ctx->cc.t3t.nMaxB * 16U,
               ctx->cc.t3t.nbR,
               ctx->cc.t3t.nbW,
               ctx->cc.t3t.writeFlag,
               ctx->cc.t3t.rwFlag);
  bufCcBuf.buffer = ctx->ccBuf;
  bufCcBuf.length = 16U;
  ndefBufferDump(" CC Raw Data", &bufCcBuf, verbose);
}
#endif /* NDEF_FEATURE_T3T */

#if NDEF_FEATURE_T4T
static void ndefT4TCCDump(ndefContext *ctx)
{
  ndefConstBuffer bufCcBuf;

  PLATFORM_LOG(" * CCLEN: %d T4T_VNo: %xh MLe: %d MLc: %d FileId: %2.2x%2.2xh "
               "FileSize: %d\r\n * readAccess: %2.2xh writeAccess: %2.2xh\r\n",
               ctx->cc.t4t.ccLen,
               ctx->cc.t4t.vNo,
               ctx->cc.t4t.mLe,
               ctx->cc.t4t.mLc,
               ctx->cc.t4t.fileId[0],
               ctx->cc.t4t.fileId[1],
               ctx->cc.t4t.fileSize,
               ctx->cc.t4t.readAccess,
               ctx->cc.t4t.writeAccess);
  bufCcBuf.buffer = ctx->ccBuf;
  bufCcBuf.length = ctx->cc.t4t.ccLen;
  ndefBufferDump(" CC File Raw Data", &bufCcBuf, verbose);
}
#endif /* NDEF_FEATURE_T4T */

#if NDEF_FEATURE_T5T
static void ndefT5TCCDump(ndefContext *ctx)
{
  ndefConstBuffer bufCcBuf;

  PLATFORM_LOG(" * Block Length: %d\r\n", ctx->subCtx.t5t.blockLen);
  PLATFORM_LOG(" * %d bytes CC\r\n"
               " * Magic: %2.2Xh Version: %d.%d MLEN: %d (%d bytes) \r\n"
               " * readAccess: %2.2xh writeAccess: %2.2xh \r\n",
               ctx->cc.t5t.ccLen,
               ctx->cc.t5t.magicNumber,
               ctx->cc.t5t.majorVersion,
               ctx->cc.t5t.minorVersion,
               ctx->cc.t5t.memoryLen,
               ctx->cc.t5t.memoryLen * 8U,
               ctx->cc.t5t.readAccess,
               ctx->cc.t5t.writeAccess);
  PLATFORM_LOG(" * [%c] Special Frame\r\n",
               ctx->cc.t5t.specialFrame ? 'X' : ' ');
  PLATFORM_LOG(" * [%c] Multiple block Read\r\n",
               ctx->cc.t5t.multipleBlockRead ? 'X' : ' ');
  PLATFORM_LOG(" * [%c] Lock Block\r\n",
               ctx->cc.t5t.lockBlock ? 'X' : ' ');
  bufCcBuf.buffer = ctx->ccBuf;
  bufCcBuf.length = ctx->cc.t5t.ccLen;
  ndefBufferDump(" CC Raw Data", &bufCcBuf, verbose);
}
#endif /* NDEF_FEATURE_T5T */

static void ndefCCDump(ndefContext *ctx)
{
  if ((ctx == NULL) || !verbose)
  {
    return;
  }
  PLATFORM_LOG("%s",
               (ctx->type == NDEF_DEV_T3T)
               ? "NDEF Attribute Information Block\r\n"
               : "NDEF Capability Container\r\n");
  switch (ctx->type)
  {
#if NDEF_FEATURE_T2T
    case NDEF_DEV_T2T:
      ndefT2TCCDump(ctx);
      break;
#endif /* NDEF_FEATURE_T2T */
#if NDEF_FEATURE_T3T
    case NDEF_DEV_T3T:
      ndefT3TAIBDump(ctx);
      break;
#endif /* NDEF_FEATURE_T3T */
#if NDEF_FEATURE_T4T
    case NDEF_DEV_T4T:
      ndefT4TCCDump(ctx);
      break;
#endif /* NDEF_FEATURE_T4T */
#if NDEF_FEATURE_T5T
    case NDEF_DEV_T5T:
      ndefT5TCCDump(ctx);
      break;
#endif /* NDEF_FEATURE_T5T */
    default:
      break;
  }
}

#if NDEF_FEATURE_T5T
static void ndefDumpSysInfo(ndefContext *ctx)
{
  ndefSystemInformation *sysInfo;

  if ((ctx == NULL) || !verbose)
  {
    return;
  }

  if (!ctx->subCtx.t5t.sysInfoSupported)
  {
    return;
  }

  sysInfo = &ctx->subCtx.t5t.sysInfo;
  PLATFORM_LOG("System Information\r\n");
  PLATFORM_LOG(" * %d byte(s) memory addressing\r\n",
               ndefT5TSysInfoMOIValue(sysInfo->infoFlags) + 1);
  if (ndefT5TSysInfoDFSIDPresent(sysInfo->infoFlags))
  {
    PLATFORM_LOG(" * DFSID=%2.2Xh\r\n", sysInfo->DFSID);
  }
  if (ndefT5TSysInfoAFIPresent(sysInfo->infoFlags))
  {
    PLATFORM_LOG(" * AFI=%2.2Xh\r\n", sysInfo->AFI);
  }
  if (ndefT5TSysInfoMemSizePresent(sysInfo->infoFlags))
  {
    PLATFORM_LOG(" * %d blocks, %d bytes per block\r\n",
                 sysInfo->numberOfBlock,
                 sysInfo->blockSize);
  }
  if (ndefT5TSysInfoICRefPresent(sysInfo->infoFlags))
  {
    PLATFORM_LOG(" * ICRef=%2.2Xh\r\n", sysInfo->ICRef);
  }
  if (ndefT5TSysInfoCmdListPresent(sysInfo->infoFlags))
  {
    PLATFORM_LOG(" * [%c] ReadSingleBlock                \r\n",
                 ndefT5TSysInfoReadSingleBlockSupported(
                   sysInfo->supportedCmd) ? 'X' : ' ');
    PLATFORM_LOG(" * [%c] WriteSingleBlock               \r\n",
                 ndefT5TSysInfoWriteSingleBlockSupported(
                   sysInfo->supportedCmd) ? 'X' : ' ');
    PLATFORM_LOG(" * [%c] LockSingleBlock                \r\n",
                 ndefT5TSysInfoLockSingleBlockSupported(
                   sysInfo->supportedCmd) ? 'X' : ' ');
    PLATFORM_LOG(" * [%c] ReadMultipleBlocks             \r\n",
                 ndefT5TSysInfoReadMultipleBlocksSupported(
                   sysInfo->supportedCmd) ? 'X' : ' ');
    PLATFORM_LOG(" * [%c] WriteMultipleBlocks            \r\n",
                 ndefT5TSysInfoWriteMultipleBlocksSupported(
                   sysInfo->supportedCmd) ? 'X' : ' ');
    PLATFORM_LOG(" * [%c] Select                         \r\n",
                 ndefT5TSysInfoSelectSupported(
                   sysInfo->supportedCmd) ? 'X' : ' ');
    PLATFORM_LOG(" * [%c] ResetToReady                   \r\n",
                 ndefT5TSysInfoResetToReadySupported(
                   sysInfo->supportedCmd) ? 'X' : ' ');
    PLATFORM_LOG(" * [%c] GetMultipleBlockSecStatus      \r\n",
                 ndefT5TSysInfoGetMultipleBlockSecStatusSupported(
                   sysInfo->supportedCmd) ? 'X' : ' ');
    PLATFORM_LOG(" * [%c] WriteAFI                       \r\n",
                 ndefT5TSysInfoWriteAFISupported(
                   sysInfo->supportedCmd) ? 'X' : ' ');
    PLATFORM_LOG(" * [%c] LockAFI                        \r\n",
                 ndefT5TSysInfoLockAFISupported(
                   sysInfo->supportedCmd) ? 'X' : ' ');
    PLATFORM_LOG(" * [%c] WriteDSFID                     \r\n",
                 ndefT5TSysInfoWriteDSFIDSupported(
                   sysInfo->supportedCmd) ? 'X' : ' ');
    PLATFORM_LOG(" * [%c] LockDSFID                      \r\n",
                 ndefT5TSysInfoLockDSFIDSupported(
                   sysInfo->supportedCmd) ? 'X' : ' ');
    PLATFORM_LOG(" * [%c] GetSystemInformation           \r\n",
                 ndefT5TSysInfoGetSystemInformationSupported(
                   sysInfo->supportedCmd) ? 'X' : ' ');
    PLATFORM_LOG(" * [%c] CustomCmds                     \r\n",
                 ndefT5TSysInfoCustomCmdsSupported(
                   sysInfo->supportedCmd) ? 'X' : ' ');
    PLATFORM_LOG(" * [%c] FastReadMultipleBlocks         \r\n",
                 ndefT5TSysInfoFastReadMultipleBlocksSupported(
                   sysInfo->supportedCmd) ? 'X' : ' ');
    PLATFORM_LOG(" * [%c] ExtReadSingleBlock             \r\n",
                 ndefT5TSysInfoExtReadSingleBlockSupported(
                   sysInfo->supportedCmd) ? 'X' : ' ');
    PLATFORM_LOG(" * [%c] ExtWriteSingleBlock            \r\n",
                 ndefT5TSysInfoExtWriteSingleBlockSupported(
                   sysInfo->supportedCmd) ? 'X' : ' ');
    PLATFORM_LOG(" * [%c] ExtLockSingleBlock             \r\n",
                 ndefT5TSysInfoExtLockSingleBlockSupported(
                   sysInfo->supportedCmd) ? 'X' : ' ');
    PLATFORM_LOG(" * [%c] ExtReadMultipleBlocks          \r\n",
                 ndefT5TSysInfoExtReadMultipleBlocksSupported(
                   sysInfo->supportedCmd) ? 'X' : ' ');
    PLATFORM_LOG(" * [%c] ExtWriteMultipleBlocks         \r\n",
                 ndefT5TSysInfoExtWriteMultipleBlocksSupported(
                   sysInfo->supportedCmd) ? 'X' : ' ');
    PLATFORM_LOG(" * [%c] ExtGetMultipleBlockSecStatus   \r\n",
                 ndefT5TSysInfoExtGetMultipleBlockSecStatusSupported(
                   sysInfo->supportedCmd) ? 'X' : ' ');
    PLATFORM_LOG(" * [%c] FastExtendedReadMultipleBlocks \r\n",
                 ndefT5TSysInfoFastExtendedReadMultipleBlocksSupported(
                   sysInfo->supportedCmd) ? 'X' : ' ');
  }
}
#endif /* NDEF_FEATURE_T5T */

#if NDEF_FEATURE_FULL_API
static bool ndefIsSTTag(ndefContext *ctx)
{
  bool ret = false;

#if defined(STM32L476xx) /* Enable to format any manufacturer tag while debugging */
  if ((CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) != 0U)
  {
    ret = true;
  }
#endif /* STM32L476xx */
  if (ctx == NULL)
  {
    return ret;
  }
  switch (ctx->device.type)
  {
    case RFAL_NFC_LISTEN_TYPE_NFCA:
      if ((ctx->device.dev.nfca.nfcId1Len != 4U) && (ctx->device.dev.nfca.nfcId1[0] == DEMO_ST_MANUFACTURER_ID))
      {
        ret = true;
      }
      break;
    case RFAL_NFC_LISTEN_TYPE_NFCF:
      break;
    case RFAL_NFC_LISTEN_TYPE_NFCB:
      break;
    case RFAL_NFC_LISTEN_TYPE_NFCV:
      if (ctx->device.dev.nfcv.InvRes.UID[6] == DEMO_ST_MANUFACTURER_ID)
      {
        ret = true;
      }
      break;
    default:
      break;
  }
  return ret;
}
#endif /* NDEF_FEATURE_FULL_API */

#if NDEF_FEATURE_FULL_API
static void LedNotificationWriteDone(void)
{
  uint32_t i;

  for (i = 0U; i < 3U; i++)
  {
    ledsOn();
    PLATFORM_DELAY(100);

    ledsOff();
    PLATFORM_DELAY(100);
  }
}
#endif /* NDEF_FEATURE_FULL_API */

static void ledsOn(void)
{
#ifdef ENABLE_LED_LOG
  PRINTF("ALL LED ON\n");
#endif /* ENABLE_LED_LOG */
}

static void ledsOff(void)
{
#ifdef ENABLE_LED_LOG
  PRINTF("ALL LED OFF\n");
#endif /* ENABLE_LED_LOG */
}
