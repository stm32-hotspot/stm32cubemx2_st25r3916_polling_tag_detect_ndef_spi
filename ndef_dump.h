/**
  *******************************************************************************
  * @file    ndef_dump.h
  * @author  MMY Application Team
  * @brief   Header for ndef_dump.c module
  *******************************************************************************
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

/*
 *      PROJECT:   NDEF firmware
 *      Revision:
 *      LANGUAGE:  ISO C99
 */

/*! \file
 *
 *  \author
 *
 *  \brief NDEF message dump utils header file
 *
 */

#ifndef NDEF_DUMP_H
#define NDEF_DUMP_H

/*
 ******************************************************************************
 * INCLUDES
 ******************************************************************************
 */

#include "rfal_platform.h"
#include "ndef_record.h"
#include "ndef_message.h"
#include "ndef_types.h"

/*
 ******************************************************************************
 * GLOBAL DEFINES
 ******************************************************************************
 */

/*
 ******************************************************************************
 * GLOBAL TYPES
 ******************************************************************************
 */

/*
 ******************************************************************************
 * GLOBAL FUNCTION PROTOTYPES
 ******************************************************************************
 */

/*!
 *****************************************************************************
 * Dump an NDEF record
 *****************************************************************************
 */
ReturnCode ndefRecordDump(const ndefRecord *record, bool verbose);

/*!
 *****************************************************************************
 * Dump an NDEF message
 *****************************************************************************
 */
ReturnCode ndefMessageDump(const ndefMessage *message, bool verbose);

#if NDEF_TYPE_FLAT_SUPPORT
/*!
 *****************************************************************************
 * Dump a Flat type
 *****************************************************************************
 */
ReturnCode ndefFlatPayloadTypeDump(const ndefType *flat);
#endif /* NDEF_TYPE_FLAT_SUPPORT */

#if NDEF_TYPE_EMPTY_SUPPORT
/*!
 *****************************************************************************
 * Dump an Empty type
 *****************************************************************************
 */
ReturnCode ndefEmptyTypeDump(const ndefType *empty);
#endif /* NDEF_TYPE_EMPTY_SUPPORT */

#if NDEF_TYPE_RTD_DEVICE_INFO_SUPPORT
/*!
 *****************************************************************************
 * Dump a Device Information RTD well-known type
 *****************************************************************************
 */
ReturnCode ndefRtdDeviceInfoDump(const ndefType *devInfo);
#endif /* NDEF_TYPE_RTD_DEVICE_INFO_SUPPORT */

#if NDEF_TYPE_RTD_TEXT_SUPPORT
/*!
 *****************************************************************************
 * Dump a Text RTD well-known type
 *****************************************************************************
 */
ReturnCode ndefRtdTextDump(const ndefType *text);
#endif /* NDEF_TYPE_RTD_TEXT_SUPPORT */

#if NDEF_TYPE_RTD_URI_SUPPORT
/*!
 *****************************************************************************
 * Dump a URI RTD well-known type
 *****************************************************************************
 */
ReturnCode ndefRtdUriDump(const ndefType *uri);
#endif /* NDEF_TYPE_RTD_URI_SUPPORT */

#if NDEF_TYPE_RTD_AAR_SUPPORT
/*!
 *****************************************************************************
 * Dump an External RTD type
 *****************************************************************************
 */
ReturnCode ndefRtdAarDump(const ndefType *ext);
#endif /* NDEF_TYPE_RTD_AAR_SUPPORT */

#if NDEF_TYPE_RTD_WLC_SUPPORT
/*!
 *****************************************************************************
 * Dump a RTD WLC Capability type
 *****************************************************************************
 */
ReturnCode ndefRtdWlcCapabilityDump(const ndefType *type);

/*!
 *****************************************************************************
 * Dump a RTD WLC Status and Information type
 *****************************************************************************
 */
ReturnCode ndefRtdWlcStatusInfoDump(const ndefType *type);

/*!
 *****************************************************************************
 * Dump a RTD WLC Poll Info type
 *****************************************************************************
 */
ReturnCode ndefRtdWlcInfoDump(const ndefType *type);

/*!
 *****************************************************************************
 * Dump a RTD WLC Listen Control type
 *****************************************************************************
 */
ReturnCode ndefRtdWlcControlDump(const ndefType *type);
#endif /* NDEF_TYPE_RTD_WLC_SUPPORT */

#if NDEF_TYPE_MEDIA_SUPPORT
/*!
 *****************************************************************************
 * Dump a Media type
 *****************************************************************************
 */
ReturnCode ndefMediaTypeDump(const ndefType *media);
#endif /* NDEF_TYPE_MEDIA_SUPPORT */

#if NDEF_TYPE_BLUETOOTH_SUPPORT
/*!
 *****************************************************************************
 * Dump a Bluetooth EIR type
 *****************************************************************************
 */
ReturnCode ndefBluetoothEirDump(const char *string,
                                const uint8_t *eir,
                                bool verbose);

/*!
 *****************************************************************************
 * Dump a Bluetooth type
 *****************************************************************************
 */
ReturnCode ndefBluetoothDump(const ndefType *type);
#endif /* NDEF_TYPE_BLUETOOTH_SUPPORT */

#if NDEF_TYPE_VCARD_SUPPORT
/*!
 *****************************************************************************
 * Dump a vCard Media type
 *****************************************************************************
 */
ReturnCode ndefMediaVCardDump(const ndefType *vCard);
#endif /* NDEF_TYPE_VCARD_SUPPORT */

#if NDEF_TYPE_WIFI_SUPPORT
/*!
 *****************************************************************************
 * Dump a Wifi Media type
 *****************************************************************************
 */
ReturnCode ndefMediaWifiDump(const ndefType *wifi);
#endif /* NDEF_TYPE_WIFI_SUPPORT */

/*!
 *****************************************************************************
 * Dump a supported type
 *****************************************************************************
 */
ReturnCode ndefTypeDump(const ndefType *type);

/*!
 *****************************************************************************
 * Dump a well-known type
 *****************************************************************************
 */
ReturnCode ndefRecordDumpType(const ndefRecord *record);

/*!
 *****************************************************************************
 * Dump a raw buffer stored in a ndefBuffer
 *****************************************************************************
 */
ReturnCode ndefBufferDump(const char *string,
                          const ndefConstBuffer *bufPayload,
                          bool verbose);

/*!
 *****************************************************************************
 * Dump a raw buffer stored in a ndefBuffer
 *****************************************************************************
 */
ReturnCode ndefBufferPrint(const char *prefix,
                           const ndefConstBuffer *bufPayload,
                           const char *suffix);

/*!
 *****************************************************************************
 * Dump a raw buffer stored in a ndefBuffer8
 *****************************************************************************
 */
ReturnCode ndefBuffer8Print(const char *prefix,
                            const ndefConstBuffer8 *bufPayload,
                            const char *suffix);

#endif /* NDEF_DUMP_H */
