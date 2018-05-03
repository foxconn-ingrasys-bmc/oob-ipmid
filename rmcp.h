/******************************************************************
*                                                                 *
*    Copyright (C) Microsoft Corporation. All rights reserved.    *
*                                                                 *
******************************************************************/

#ifndef _OOB_RMCP_H
#define _OOB_RMCP_H

#include <stdint.h>
#include <stdbool.h>

#define PRIMARY_RMCP_PORT	623

#define ASF_2_0_VERSION 	0x06

#define ASF_CLASS		0x06
#define IPMI_CLASS		0x07

#define ASF_IANA		4542
#define MSFT_IANA		311

#define PING_REQ_CMD		0x80
#define PONG_RESP_CMD		0x40

typedef struct _RMCP_HEADER {
        uint8_t version;
        uint8_t reserved;
        uint8_t seqnum;
        uint8_t class;
} RMCPHeader;

typedef struct _ASF_MESSAGE_REQ {
        uint32_t IANAEntNum;
        uint8_t  msgtype;
        uint8_t  msgtag;
        uint8_t  reserved;
        uint8_t  datalen;
} ASFMessageReq;

typedef struct _ASF_MESSAGE_RESP {
        uint32_t IANAEntNum;
        uint8_t  msgtype;
        uint8_t  msgtag;
        uint8_t  reserved;
        uint8_t  datalen;
        uint32_t OEMIANA;
        uint32_t OEMdefined;
        uint8_t  entities;
        uint8_t  interaction;
        uint8_t  ASFReserved[6];
} ASFMessageResp;


#endif //_OOB_RMCP_H
