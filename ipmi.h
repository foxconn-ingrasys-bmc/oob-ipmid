/******************************************************************
*                                                                 *
*    Copyright (C) Microsoft Corporation. All rights reserved.    *
*                                                                 *
******************************************************************/

#ifndef _OOB_IPMI_H
#define _OOB_IPMI_H

#include <stdint.h>
#include <stdbool.h>

#define BMC_SLAVE_ADDR  0x20

#define CHASSIS_NETFN   0x00	
#define BRIDGE_NETFN    0x02
#define SENSOR_NETFN    0x04
#define APP_NETFN       0x06
#define FIRMWARE_NETFN  0x08
#define STORAGE_NETFN   0x0A
#define TRANSPORT_NETFN 0x0C

#define CC_NORMAL       0x00

/* This struct is not 4 byte aligned */
#pragma pack (push, 1)
typedef struct _IPMI_SESSION_HEADER {
	uint8_t  authtype;
	uint32_t seqnum;
	uint32_t sessionID;
	uint8_t  payloadlen;
} IPMISessionHeader;
#pragma pack (pop)

typedef struct _IPMI_MESSAGE_REQ {
	uint8_t  rsAddr;
	uint8_t  netFn;
	uint8_t  checksum;
	uint8_t  rqAddr;
	uint8_t  rqSeq;
	uint8_t  cmd;
}  IPMIMessageReq;

typedef struct _IPMI_MESSAGE_RESP {
	uint8_t  rqAddr;
	uint8_t  netFn;
	uint8_t  checksum;
	uint8_t  rsAddr;
	uint8_t  rqSeq;
	uint8_t  cmd;
}  IPMIMessageResp;

typedef struct _COMMAND_TABLE {
	uint8_t cmdnum;
	uint8_t	cmdlen;
	int (*cmdhdlr) (const uint8_t *, uint8_t **, uint8_t *);
} CommandTable;

typedef struct _NETFN_TABLE {
	uint8_t netfn;
	const CommandTable * cmdtbl;
	const uint8_t * size;
} NetFnTable;

extern const NetFnTable gNetFnTbl[];
extern const uint8_t gNetFnTblSize;

extern uint8_t CalcChecksum (uint8_t * data, uint8_t size);
extern bool IPMIRMCPSessionlessValidation (uint8_t * data);

#endif //_OOB_IPMI_H
