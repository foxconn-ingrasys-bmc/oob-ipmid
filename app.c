/******************************************************************
*                                                                 *
*    Copyright (C) Microsoft Corporation. All rights reserved.    *
*                                                                 *
******************************************************************/

#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "app.h"

#define GPIO_SYSFS	"/sys/class/gpio/"

#define PMDU_NODE0_BUF	386
#define PMDU_NODE1_BUF	387
#define PMDU_NODE2_BUF	388
#define PMDU_NODE3_BUF	389
#define PMDU_NODE4_BUF	390
#define PMDU_NODE5_BUF	391

const CommandTable gAppCmdTbl[] = {
	{GET_CH_AUTH_CMD, sizeof(GetChAuthCapReq), Cmd_GetChannelAuthCapabilities},
	/* This is the END - Do not list any commands beyond this point */
	{0xFF, 0xFF, NULL}
};

const uint8_t gAppCmdTblSize = (uint8_t) sizeof(gAppCmdTbl)/sizeof(CommandTable);

static const uint16_t gpionum[] = {
PMDU_NODE0_BUF,
PMDU_NODE1_BUF,
PMDU_NODE2_BUF,
PMDU_NODE3_BUF,
PMDU_NODE4_BUF,
PMDU_NODE5_BUF
};

static int8_t readgpio (uint16_t gpionum, uint8_t * value)
{
	char gpiofile[256]; //Arbitrary file length
	char exportfile[256];
	FILE * gpiofp = NULL;
	FILE * exportfp = NULL;
	int retval = 0;

	memset (gpiofile, 0, sizeof(gpiofile));
	snprintf (gpiofile, sizeof(gpiofile), "%sgpio%d/value", GPIO_SYSFS, gpionum);
	/* Open GPIO file to read value */
	gpiofp = fopen(gpiofile, "r");
	if (NULL == gpiofp)
	{
		/* If GPIO file is not accessible, try exporting it */
		memset (exportfile, 0, sizeof(exportfile));
		snprintf (exportfile, sizeof(exportfile), "%sexport", GPIO_SYSFS);
		exportfp = fopen(exportfile, "w");
		if (NULL == exportfp)
		{
			perror ("Error opening GPIO export file");
			return -1;
		}
		if (0 > fprintf (exportfp, "%d", gpionum))
		{
			perror ("Error writing to GPIO export file");
			return -1;
		}
		fflush(exportfp);
		fclose(exportfp);
		/* Now that it has been exported, retry opening GPIO file */
		gpiofp = fopen(gpiofile, "rb");
		if (NULL == gpiofp)
		{
			perror ("Error opening GPIO file");
			return -1;
		}
	}
	fseek(gpiofp, 0L, SEEK_SET);
	retval = fscanf (gpiofp, "%c", value);
	if ((0 == retval) || (EOF == retval))
	{
		perror ("Error reading GPIO value");
		fclose (gpiofp);
		return -1;		
	}
	/* Convert string to integer */
	*value -= '0';
	/* Close and exit */
	fclose (gpiofp);
	return 0;
}

static uint32_t get_ipv4_by_ifname (const char* ifname)
{
	int sock;
	struct ifreq ifr;
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		return 0;
	}
	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);
	if (ioctl(sock, SIOCGIFADDR, &ifr) == -1) {
		close(sock);
		return 0;
	}
	close(sock);
	return ((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr.s_addr;
}

static uint8_t get_slot_id_by_ipv4 (uint32_t ipv4)
{
	return (((ipv4 >> 24) & 0xFF) + 3) / 4;
}

int Cmd_GetChannelAuthCapabilities (const uint8_t * req, uint8_t ** res, uint8_t * len)
{
	GetChAuthCapReq * request = (GetChAuthCapReq*) req;
	GetChAuthCapResp * response = NULL;
	uint8_t slotid = 0;
	uint8_t gpioval = 2;
	uint8_t i = 0;
	uint8_t retval = 0;

	/* Validate request fields */
	if (((request->chnum & CHNUM_MASK) != LAN_CHANNEL) && ((request->chnum & CHNUM_MASK) != THIS_CHANNEL))
		return -1;

	slotid = get_slot_id_by_ipv4(get_ipv4_by_ifname("eth0"));

	response = malloc (sizeof(GetChAuthCapResp));
	if (NULL == response)
		return -1;

	memset (response, 0, sizeof(GetChAuthCapResp));

	/* Fill response */
	response->compcode   = CC_NORMAL;
	response->chnum      = LAN_CHANNEL;
	response->authtype   = IPMI_V1_5_SUP;
	response->authstatus = PER_MSG_AUTH_DISABLED | USER_LEVEL_AUTH_DISABLED | LOGIN_DISABLED;
	response->IPMIsupport= SYSTEM_ID; //Repurposing IPMI Reserved bits to send system ID
	response->OEMID[0]   = (uint8_t) (MSFT_IANA >> 0);
	response->OEMID[1]   = (uint8_t) (MSFT_IANA >> 8);
	response->OEMID[2]   = (uint8_t) (MSFT_IANA >> 16);
    // TODO only the 6-LSB are occupied to slot ID?
	response->OEMauxdata = REST_SUP | (slotid & SLOT_ID_MASK); //Repurposing OEM Auth field to send protocol and slot info

	*res = (uint8_t *)response;
	*len = sizeof(GetChAuthCapResp);

	return 0;
	
}

