/******************************************************************
*                                                                 *
*    Copyright (C) Microsoft Corporation. All rights reserved.    *
*                                                                 *
******************************************************************/
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include "ipmi.h"
#include "app.h"

const NetFnTable gNetFnTbl[] = {
	{APP_NETFN, gAppCmdTbl, &gAppCmdTblSize},
	/* This is the END - Do not list any NetFn beyond this point */
	{0xFF, NULL, 0}
};

const uint8_t gNetFnTblSize = sizeof(gNetFnTbl)/sizeof(NetFnTable);	

uint8_t CalcChecksum (uint8_t * data, uint8_t size)
{
	uint8_t i = 0;
	uint8_t chksum = 0;
	if ((0 == size) || (NULL == data))
	{
		printf ("Checksum calculation failed\n");
	}
	else
	{
		for (i = 0; i < size; i++)
		{
			chksum += data[i];
		}
		chksum = ~chksum + 1;
	}
	return chksum;
}

bool IPMIRMCPSessionlessValidation (uint8_t * data)
{
	RMCPHeader * rhdr;
	IPMISessionHeader * ihdr;
	IPMIMessageReq * ireq;
	uint8_t chksum = 0;

	/* Check if data is valid */
	if (NULL == data)
		return false;

	/* Validate RMCP Header fields */
	rhdr = (RMCPHeader*)data;
	if ((ASF_2_0_VERSION != rhdr->version) || 
	    (0xFF != rhdr->seqnum) ||
	    (IPMI_CLASS != rhdr->class))
		return false;

	/* Validate IPMI Session Header fields */
	ihdr = (IPMISessionHeader*) (data + sizeof(RMCPHeader));
	if((0 != ihdr->authtype) ||
	   (0 != ntohl(ihdr->seqnum)) ||
	   (0 != ntohl(ihdr->sessionID)))
		return false;

	/* Validate checksum */
	ireq = (IPMIMessageReq*)(data + sizeof(RMCPHeader) + sizeof(IPMISessionHeader));
	chksum = CalcChecksum((uint8_t*)ireq, 2);
	if (chksum != ireq->checksum)
		return false;

	/* Check slave address */
	if (BMC_SLAVE_ADDR != ireq->rsAddr)
		return false;

	/* Passed all tests for an out-of-session IPMI Request */
	return true;
}
