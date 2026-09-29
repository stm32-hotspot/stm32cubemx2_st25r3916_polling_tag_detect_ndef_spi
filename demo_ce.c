/**
  ******************************************************************************
  * @file    demo_ce.c
  * @brief   Body function to manage card emul mode
  ******************************************************************************
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

/* Includes ------------------------------------------------------------------*/
#include "demo_ce.h"
#include "utils.h"
#include "rfal_rf.h"
#include "rfal_nfca.h"
#include "rfal_nfcf.h"
#include "nfc_conf.h"

/* Private typedef -----------------------------------------------------------*/
/** @defgroup CE_CardEmul_Private_Typedef
  * @{
  */
enum States
{
  STATE_IDLE         = 0, /*!< Emulated Tag state idle.                 */
  STATE_APP_SELECTED = 1, /*!< Emulated Tag state application selected. */
  STATE_CC_SELECTED  = 2, /*!< Emulated Tag state CCFile selected.      */
  STATE_FID_SELECTED = 3  /*!< Emulated Tag state FileID selected.      */
};
/**
  * @}
  */

/* Private define ------------------------------------------------------------*/
/** @defgroup CE_CardEmul_Private_Define
  * @{
  */

#define NDEF_SIZE      256   /*!< Max NDEF size emulated. Range: 0005h - 7FFFh.   */
#define T4T_CLA_00     0x00  /*!< CLA value for type 4 command.                   */
#define T4T_INS_SELECT 0xA4  /*!< INS value for select command.                   */
#define T4T_INS_READ   0xB0  /*!< INS value for read-binary command.              */
#define T4T_INS_UPDATE 0xD6  /*!< INS value for update command.                   */
#define FID_CC         0xE103 /*!< File ID number for CCFile.                    */
#define FID_NDEF       0x0001 /*!< File ID number for NDEF file.                 */
#define T3T_BLOCK_SIZE 0x10   /*!< Block size in Type 3 Tag.                     */
/**
  * @}
  */

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/** @defgroup CE_CardEmul_Private_Variables
  * @{
  */
static uint8_t gNfcfNfcid[RFAL_NFCF_NFCID2_LEN];
static uint8_t ndefFile[NDEF_SIZE];  /*!< Buffer to store NDEF File.              */
static int8_t  nState       = STATE_IDLE;  /*!< Type 4 tag emulation status.        */
static int32_t nSelectedIdx = -1;         /*!< Current file selected.              */
static int32_t nFiles       = 2;          /*!< Number of files emulated.           */

/**
  * CCLEN : Indicates the size of this CC File. <BR>
  * T4T_VNo : Indicates the Mapping Version. <BR>
  * MLe : Max R-APDU size. <BR>
  * MLc : Max C-APDU size. <BR>
  * NDEF FCI T : Indicates the NDEF-File_Ctrl_TLV. <BR>
  * NDEF FCI L : The length of the V-field. <BR>
  * NDEF FCI V1 : NDEF File Identifier. <BR>
  * NDEF FCI V2 : NDEF File size. <BR>
  * NDEF FCI V3 : NDEF Read AC. <BR>
  * NDEF FCI V4 : NDEF Write AC. <BR>
  */
static uint8_t ccfile[] =
{
  0x00, 0x0F,                                       /* CCLEN      */
  0x20,                                             /* T4T_VNo    */
  0x00, 0x7F,                                       /* MLe        */
  0x00, 0x7F,                                       /* MLc        */
  0x04,                                             /* T          */
  0x06,                                             /* L          */
  (FID_NDEF & 0xFF00) >> 8, (FID_NDEF & 0x00FF),    /* V1         */
                      (NDEF_SIZE & 0xFF00) >> 8, (NDEF_SIZE & 0x00FF),  /* V2         */
                      0x00,                                             /* V3         */
                      0x00                                              /* V4         */
};

static uint32_t pdwFileSize[] =
{
  sizeof(ccfile),
  NDEF_SIZE
}; /*!< Current emulated files size. */

/**
  * NDEF length. <BR>
  * NDEF Header: MB, ME, SR, Well known Type. <BR>
  * NDEF type length. <BR>
  * NDEF payload length. <BR>
  * NDEF Type : URI. <BR>
  * NDEF URI abbreviation field : http://www. <BR>
  * NDEF URI string : st.com/st25-demo. <BR>
  */
static const uint8_t ndef_uri[] =
{
  0x00, 0x15,                      /* NDEF length                 */
  0xD1,                            /* NDEF Header                 */
  0x01,                            /* NDEF type length            */
  0x11,                            /* NDEF payload length         */
  0x55,                            /* NDEF Type                   */
  0x01,                            /* NDEF URI abbreviation field */
  0x73, 0x74, 0x2E, 0x63, 0x6F,    /* NDEF URI string             */
  0x6D, 0x2F, 0x73, 0x74, 0x32,
  0x35, 0x2D, 0x64, 0x65, 0x6D, 0x6F
};

__WEAK const uint8_t *demoNdefFile    = ndef_uri;
__WEAK uint32_t       demoNdefFileLen = sizeof(ndef_uri);

/**
  * Ver : Indicates the NDEF mapping version. <BR>
  * Nbr : Indicates the number of blocks that can be read. <BR>
  * Nbw : Indicates the number of blocks that can be written. <BR>
  * NmaxB : Indicates the maximum number of blocks available for NDEF data. <BR>
  * WriteFlag : Indicates whether a previous NDEF write procedure has finished
  *             or not. <BR>
  * RWFlag : Indicates data can be updated or not. <BR>
  * Ln : Size of the actual stored NDEF data in bytes. <BR>
  * Checksum : allows the Reader/Writer to check whether the Attribute Data are
  *            correct. <BR>
  */
static uint8_t InformationBlock[] =
{
  0x10,                                             /* Ver        */
  0x08,                                             /* Nbr        */
  0x08,                                             /* Nbw        */
  0x00, 0x0F,                                       /* NmaxB      */
  0x00, 0x00, 0x00, 0x00,                           /* RFU        */
  0x00,                                             /* WriteFlag  */
  0x01,                                             /* RWFlag     */
  0x00, 0x00, 0x15,                                 /* Ln         */
  0x00, 0x45                                        /* Checksum   */
};
/**
  * @}
  */

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/
/** @defgroup CE_CardEmul_Private_Functions
  * @{
  */

/**
  *****************************************************************************
  * @brief  Compare two commands supplied in parameters.
  *
  * @param[in] cmd   Pointer to the received command.
  * @param[in] find  Pointer to the available command.
  * @param[in] len   Length of the available command.
  *
  * @retval true  Same command.
  * @retval false Different command.
  *****************************************************************************
  */
static bool cmdCompare(uint8_t *cmd, uint8_t *find, uint16_t len)
{
  for (int32_t i = 0; i < 20; i++)
  {
    if (memcmp(&cmd[i], find, len) == 0)
    {
      return true;
    }
  }
  return false;
}

/**
  *****************************************************************************
  * @brief  Manage the T4T Select answer to the reader.
  *
  * @param[in]  cmdData Pointer to the received command.
  * @param[out] rspData Pointer to the answer to send.
  *
  * @return Answer size.
  *****************************************************************************
  */
static uint16_t demoCeT4TSelect(uint8_t *cmdData, uint8_t *rspData)
{
  bool success = false;
  /*
   * Cmd: CLA(1) | INS(1) | P1(1) | P2(1) | Lc(1) | Data(n) | [Le(1)]
   * Rsp: [FCI(n)] | SW12
   *
   * Select App by Name NDEF:       00 A4 04 00 07 D2 76 00 00 85 01 01 00
   * Select App by Name NDEF 4 ST:  00 A4 04 00 07 A0 00 00 00 03 00 00 00
   * Select CC FID:                 00 A4 00 0C 02 xx xx
   * Select NDEF FID:               00 A4 00 0C 02 xx xx
   */

  uint8_t aid[]         = { 0xD2, 0x76, 0x00, 0x00, 0x85, 0x01, 0x01 };
  uint8_t fidCC[]       = { (uint8_t)(FID_CC >> 8), (uint8_t)(FID_CC & 0xFF) };
  uint8_t fidNDEF[]     = { (uint8_t)(FID_NDEF >> 8), (uint8_t)(FID_NDEF & 0xFF) };
  uint8_t selectFileId[] = { 0xA4, 0x00, 0x0C, 0x02, 0x00, 0x01 };

  if (cmdCompare(cmdData, aid, sizeof(aid)))
  {
    /* Select application. */
    nState  = STATE_APP_SELECTED;
    success = true;
  }
  else if ((nState >= STATE_APP_SELECTED) && cmdCompare(cmdData, fidCC, sizeof(fidCC)))
  {
    /* Select CC. */
    nState       = STATE_CC_SELECTED;
    nSelectedIdx = 0;
    success      = true;
  }
  else if ((nState >= STATE_APP_SELECTED)
           && (cmdCompare(cmdData, fidNDEF, sizeof(fidNDEF))
               || cmdCompare(cmdData, selectFileId, sizeof(selectFileId))))
  {
    /* Select NDEF. */
    nState       = STATE_FID_SELECTED;
    nSelectedIdx = 1;
    success      = true;
  }
  else
  {
    nState = STATE_IDLE;
  }

  rspData[0] = success ? 0x90U : 0x6AU;
  rspData[1] = success ? 0x00U : 0x82U;

  return 2U;
}

/**
  *****************************************************************************
  * @brief  Manage the T4T Read answer to the reader.
  *
  * @param[in]  cmdData    Pointer to the received command.
  * @param[out] rspData    Pointer to the answer to send.
  * @param[in]  rspDataLen Size of the answer buffer.
  *
  * @return Answer size.
  *****************************************************************************
  */
static uint16_t demoCeT4TRead(uint8_t *cmdData,
                              uint8_t *rspData,
                              uint16_t rspDataLen)
{
  /*
   * Cmd: CLA(1) | INS(1) | P1(1) | P2(1) | Le(1)
   * Rsp: BytesRead | SW12
   */
  uint16_t offset = (uint16_t)((cmdData[2] << 8) | cmdData[3]);
  uint16_t toRead = cmdData[4];
  uint8_t *ppbMemory;

  if (rspDataLen < 2U)
  {
    /* Must ensure appropriate buffer. */
    ST25R3916_DRV_PLATFORM_ERROR_HANDLE();
  }

  /* Any file selected. */
  if ((nSelectedIdx < 0) || (nSelectedIdx >= nFiles))
  {
    rspData[0] = 0x6AU;
    rspData[1] = 0x82U;
    return 2U;
  }

  /* offset + length exceed file size. */
  if ((uint32_t)(offset + toRead) > pdwFileSize[nSelectedIdx])
  {
    toRead = (uint16_t)(pdwFileSize[nSelectedIdx] - offset);
  }

  if (rspDataLen < (uint16_t)(toRead + 2U))
  {
    rspData[0] = 0x6FU;
    rspData[1] = 0x00U;
    return 2U;
  }

  ppbMemory = (nSelectedIdx == 0) ? ccfile : ndefFile;

  memcpy(rspData, &ppbMemory[offset], toRead);

  rspData[toRead]     = 0x90U;
  rspData[toRead + 1] = 0x00U;
  return (uint16_t)(toRead + 2U);
}

/**
  *****************************************************************************
  * @brief  Manage the T4T Update answer to the reader.
  *
  * @param[in] cmdData Pointer to the received command.
  * @param[in] rspData Pointer to the answer to send.
  *
  * @return Answer size.
  *****************************************************************************
  */
static uint16_t demoCeT4TUpdate(uint8_t *cmdData, uint8_t *rspData)
{
  uint32_t offset = (uint32_t)((cmdData[2] << 8) | cmdData[3]);
  uint32_t length = cmdData[4];

  if (nSelectedIdx != 1)
  {
    rspData[0] = 0x6AU;
    rspData[1] = 0x82U;
    return 2U;
  }

  if ((offset + length) > pdwFileSize[nSelectedIdx])
  {
    rspData[0] = 0x62U;
    rspData[1] = 0x82U;
    return 2U;
  }

  memcpy(&ndefFile[offset], &cmdData[5], length);

  rspData[0] = 0x90U;
  rspData[1] = 0x00U;
  return 2U;
}

/**
  *****************************************************************************
  * @brief  Manage the T3T Check (read) answer to the reader.
  *
  * @param[in]  cmdData    Pointer to the received command.
  * @param[out] rspData    Pointer to the answer to send.
  * @param[in]  rspDataLen Size of the answer buffer.
  *
  * @return Answer size.
  *****************************************************************************
  */
static uint16_t demoCeT3TCheck(uint8_t *cmdData,
                               uint8_t *rspData,
                               uint16_t rspDataLen)
{
  /*
   * Cmd: cmd | NFCID2 | NoS | Service code list | NoB | Block list
   * Rsp: rsp | NFCID2 | Status Flag 1 | Status Flag 2 | NoB | Block Data
   */
  uint8_t *block;
  uint16_t blocknb[256];
  uint32_t idx   = 0U;
  uint32_t cnt   = 0U;
  uint32_t nbmax = 0U;

  rspData[idx++] = (uint8_t)(RFAL_NFCF_CMD_READ_WITHOUT_ENCRYPTION + 1U);

  if (ST_BYTECMP(gNfcfNfcid,
                 &cmdData[RFAL_NFCF_LENGTH_LEN + RFAL_NFCF_CMD_LEN],
                 RFAL_NFCF_NFCID2_LEN) == 0)
  {
    ST_MEMCPY(&rspData[idx], gNfcfNfcid, RFAL_NFCF_NFCID2_LEN);
    idx += RFAL_NFCF_NFCID2_LEN;
  }
  else
  {
    return 0U;
  }

  if ((cmdData[10] != 1U) || ((cmdData[11] != 0x09U) && (cmdData[11] != 0x0BU)) || (cmdData[13] == 0U)
      || (cmdData[13] > InformationBlock[1]))
  {
    rspData[idx++] = 0xFFU;
    rspData[idx++] = 0xFFU;
    return (uint16_t)idx;
  }
  else
  {
    rspData[idx++] = 0x00U;
    rspData[idx++] = 0x00U;
  }

  if (rspDataLen < (uint16_t)(11U + (cmdData[13] * T3T_BLOCK_SIZE)))
  {
    ST25R3916_DRV_PLATFORM_ERROR_HANDLE();
  }

  rspData[idx++] = cmdData[13]; /* Nob. */

  block = &cmdData[14];
  for (cnt = 0U; cnt < cmdData[13]; cnt++)
  {
    if ((*block & 0x0FU) >= cmdData[10])
    {
      rspData[idx - 3U] = 0xFFU;
      rspData[idx - 2U] = 0x80U; /* Proprietary value for specific error. */
      return (uint16_t)(idx - 1U);
    }

    if (*block & 0x80U)
    {
      blocknb[cnt] = *(block + 1);
      block += 2;
    }
    else
    {
      blocknb[cnt] = *(block + 2);
      blocknb[cnt] <<= 8;
      blocknb[cnt] |= *(block + 1);
      block += 3;
    }

    nbmax  = InformationBlock[3];
    nbmax  = (nbmax << 8) | InformationBlock[4];
    if (blocknb[cnt] > nbmax)
    {
      rspData[idx - 3U] = 0xFFU;
      rspData[idx - 2U] = 0x70U;
      return (uint16_t)(idx - 1U);
    }
  }

  for (cnt = 0U; cnt < cmdData[13]; cnt++)
  {
    if (blocknb[cnt] == 0x0000U)
    {
      ST_MEMCPY(&rspData[idx], InformationBlock, sizeof(InformationBlock));
      idx += sizeof(InformationBlock);
    }
    else
    {
      ST_MEMCPY(&rspData[idx],
                &ndefFile[2U + ((blocknb[cnt] - 1U) * T3T_BLOCK_SIZE)],
                T3T_BLOCK_SIZE);
      idx += T3T_BLOCK_SIZE;
    }
  }

  return (uint16_t)idx;
}

/**
  *****************************************************************************
  * @brief  Manage the T3T Update answer to the reader.
  *
  * @param[in] cmdData Pointer to the received command.
  * @param[in] rspData Pointer to the answer to send.
  *
  * @return Answer size.
  *****************************************************************************
  */
static uint16_t demoCeT3TUpdate(uint8_t *cmdData, uint8_t *rspData)
{
  /*
   * Cmd: cmd | NFCID2 | NoS | Service code list | NoB | Block list | Block Data
   * Rsp: rsp | NFCID2 | Status Flag 1 | Status Flag 2
   */
  uint8_t *block;
  uint16_t blocknb[256];
  uint32_t idx   = 0U;
  uint32_t cnt   = 0U;
  uint32_t nbmax = 0U;

  rspData[idx++] = (uint8_t)(RFAL_NFCF_CMD_WRITE_WITHOUT_ENCRYPTION + 1U);

  if (ST_BYTECMP(gNfcfNfcid, &cmdData[RFAL_NFCF_LENGTH_LEN + RFAL_NFCF_CMD_LEN], RFAL_NFCF_NFCID2_LEN) == 0)
  {
    ST_MEMCPY(&rspData[idx], gNfcfNfcid, RFAL_NFCF_NFCID2_LEN);
    idx += RFAL_NFCF_NFCID2_LEN;
  }
  else
  {
    return 0U;
  }

  if ((cmdData[10] != 1U) || (cmdData[11] != 0x09U) || (cmdData[13] == 0U) || (cmdData[13] > InformationBlock[2]))
  {
    rspData[idx++] = 0xFFU;
    rspData[idx++] = 0xFFU;
    return (uint16_t)idx;
  }
  else
  {
    rspData[idx++] = 0x00U;
    rspData[idx++] = 0x00U;
  }

  block = &cmdData[14];
  for (cnt = 0U; cnt < cmdData[13]; cnt++)
  {
    if (*block & 0x80U)
    {
      blocknb[cnt] = *(block + 1);
      block += 2;
    }
    else
    {
      blocknb[cnt] = *(block + 2);
      blocknb[cnt] <<= 8;
      blocknb[cnt] |= *(block + 1);
      block += 3;
    }

    nbmax  = InformationBlock[3];
    nbmax  = (nbmax << 8) | InformationBlock[4];
    if (blocknb[cnt] > nbmax)
    {
      rspData[idx - 2U] = 0xFFU;
      rspData[idx - 1U] = 0x70U;
      return (uint16_t)idx;
    }
  }

  for (cnt = 0U; cnt < cmdData[13]; cnt++)
  {
    if (blocknb[cnt] == 0x0000U)
    {
      ST_MEMCPY(InformationBlock, block, T3T_BLOCK_SIZE);
      block += T3T_BLOCK_SIZE;
    }
    else
    {
      ST_MEMCPY(&ndefFile[2U + ((blocknb[cnt] - 1U) * T3T_BLOCK_SIZE)],
                block,
                T3T_BLOCK_SIZE);
      block += T3T_BLOCK_SIZE;
    }
  }

  rspData[idx++] = 0x00U;
  rspData[idx++] = 0x00U;

  return (uint16_t)idx;
}

/**
  *****************************************************************************
  * @brief  Demo CE initialization.
  *
  * Initializes the demo CE.
  *
  * @param[in] nfcfNfcid The NFCID to be used in T3T CE.
  *
  * @return None.
  *****************************************************************************
  */
void demoCeInit(uint8_t *nfcfNfcid)
{
  if (nfcfNfcid != NULL)
  {
    ST_MEMCPY(gNfcfNfcid, nfcfNfcid, RFAL_NFCF_NFCID2_LEN);
  }

  ST_MEMCPY(ndefFile, (uint8_t *)demoNdefFile, demoNdefFileLen);

  InformationBlock[12] = demoNdefFile[0];
  InformationBlock[13] = demoNdefFile[1];
  uint16_t checksum    = 0U;
  for (int32_t i = 0; i < 14; i++)
  {
    checksum += InformationBlock[i];
  }
  InformationBlock[14] = (uint8_t)(checksum >> 8);
  InformationBlock[15] = (uint8_t)(checksum & 0xFF);
}

/**
  *****************************************************************************
  * @brief  Demo CE T4T.
  *
  * Parses the received command and computes the response to be sent back to
  * the reader.
  *
  * @param[in]  rxData    Pointer to the received command.
  * @param[in]  rxDataLen Length of the received command.
  * @param[out] txBuf     Pointer to where the response will be placed.
  * @param[in]  txBufLen  Size of txBuf.
  *
  * @return Response size.
  *****************************************************************************
  */
uint16_t demoCeT4T(uint8_t *rxData,
                   uint16_t rxDataLen,
                   uint8_t *txBuf,
                   uint16_t txBufLen)
{
  if ((txBuf == NULL) || (txBufLen < 2U))
  {
    ST25R3916_DRV_PLATFORM_ERROR_HANDLE();
  }

  if ((rxData != NULL) && (rxDataLen >= 4U))
  {
    if (rxData[0] == T4T_CLA_00)
    {
      switch (rxData[1])
      {
        case T4T_INS_SELECT:
          return demoCeT4TSelect(rxData, txBuf);

        case T4T_INS_READ:
          return demoCeT4TRead(rxData, txBuf, txBufLen);

        case T4T_INS_UPDATE:
          return demoCeT4TUpdate(rxData, txBuf);

        default:
          break;
      }
    }
  }

  txBuf[0] = 0x68U;
  txBuf[1] = 0x00U;
  return 2U;
}

/**
  *****************************************************************************
  * @brief  Demo CE T3T.
  *
  * Parses the received command and computes the response to be sent back to
  * the reader.
  *
  * @param[in]  rxData    Pointer to the received command.
  * @param[in]  rxDataLen Length of the received command.
  * @param[out] txBuf     Pointer to where the response will be placed.
  * @param[in]  txBufLen  Size of txBuf.
  *
  * @return Response size.
  *****************************************************************************
  */
uint16_t demoCeT3T(uint8_t *rxData,
                   uint16_t rxDataLen,
                   uint8_t *txBuf,
                   uint16_t txBufLen)
{
  if ((txBuf == NULL) || (txBufLen < 11U))
  {
    ST25R3916_DRV_PLATFORM_ERROR_HANDLE();
  }

  if ((rxData != NULL) && (rxDataLen >= 4U))
  {
    switch (rxData[1])
    {
      case RFAL_NFCF_CMD_READ_WITHOUT_ENCRYPTION:
        return demoCeT3TCheck(rxData, txBuf, txBufLen);

      case RFAL_NFCF_CMD_WRITE_WITHOUT_ENCRYPTION:
        return demoCeT3TUpdate(rxData, txBuf);

      default:
        break;
    }
  }

  txBuf[0] = 0xFFU;
  txBuf[1] = 0xFFU;
  return 2U;
}
