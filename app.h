/******************************************************************
*                                                                 *
*    Copyright (C) Microsoft Corporation. All rights reserved.    *
*                                                                 *
******************************************************************/

#ifndef _OOB_APP_H
#define _OOB_APP_H

#include <stdint.h>
#include <stdbool.h>
#include "rmcp.h"
#include "ipmi.h"

#define GET_CH_AUTH_CMD	0x38

#define LAN_CHANNEL	0x01
#define THIS_CHANNEL	0x0E

#define CHNUM_MASK	0x0F
#define PRIV_MASK	0x0F

#define IPMI_V1_5	0x00
#define IPMI_CAP_BIT	7
#define IPMI_V1_5_SUP	(IPMI_V1_5 << IPMI_CAP_BIT)
#define AUTH_NONE_BIT	0
#define AUTH_NONE_SUP	(1 << AUTH_NONE_BIT)

#define IPMI_PROTOCOL	0x0
#define REST_PROTOCOL	0x1
#define BLADE_PROTO_BIT 6
#define BLADE_PROT_MASK 0xC0
#define REST_SUP	((REST_PROTOCOL << BLADE_PROTO_BIT) & BLADE_PROT_MASK)

#define SLOT_ID_MASK	0x3F

#define PER_MSG_AUTH_BIT		4
#define PER_MSG_AUTH_DISABLED 		(1 << PER_MSG_AUTH_BIT)
#define USER_LEVEL_AUTH_BIT		3
#define USER_LEVEL_AUTH_DISABLED 	(1 << USER_LEVEL_AUTH_BIT)
#define LOGIN_STATUS_MASK		0x7
#define LOGIN_STATUS_BIT		0
#define LOGIN_DISABLED  		((0 & LOGIN_STATUS_MASK) << LOGIN_STATUS_BIT)

#define PROJECT_SYSTEM_ID		0x03
#define SYSTEM_ID_MASK			0x3F
#define SYSTEM_ID_BIT			2
#define SYSTEM_ID			((PROJECT_SYSTEM_ID & SYSTEM_ID_MASK) << SYSTEM_ID_BIT)

typedef struct _GET_CH_AUTH_CAP_REQ {
	uint8_t chnum;
	uint8_t maxprivilege;
} GetChAuthCapReq;

typedef struct _GET_CH_AUTH_CAP_RESP {
	uint8_t compcode;
	uint8_t chnum;
	uint8_t authtype;
	uint8_t authstatus;
	uint8_t IPMIsupport;
	uint8_t OEMID[3];
	uint8_t OEMauxdata;
} GetChAuthCapResp;

typedef struct _RMCP_GET_AUTH_CAP_REQ {
	RMCPHeader rmcpheader;
	IPMISessionHeader ipmiheader;
	IPMIMessageReq ipmimessage;
	GetChAuthCapReq getauthreq;
	uint8_t	checksum;
} RMCPGetAuthReq;

typedef struct _RMCP_GET_AUT_CAP_RESP {
        RMCPHeader rmcpheader;
        IPMISessionHeader ipmiheader;
        IPMIMessageReq ipmimessage;
	GetChAuthCapResp getauthresp;
        uint8_t checksum;
} RMCPGetAuthResp;

extern const CommandTable gAppCmdTbl[];
extern const uint8_t gAppCmdTblSize;

extern int Cmd_GetChannelAuthCapabilities (const uint8_t * req, uint8_t ** res, uint8_t * len);

#endif //_OOB_APP_H
