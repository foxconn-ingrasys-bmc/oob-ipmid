/******************************************************************
*                                                                 *
*    Copyright (C) Microsoft Corporation. All rights reserved.    *
*                                                                 *
******************************************************************/

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include "rmcp.h"
#include "ipmi.h"
#include "app.h"

static void usage()
{
	printf ("oob-ipmid [-port <portnum>]\n");
	printf ("\t-port <portnum> is optional. Default port number is 623\n");
	printf ("\tExample: oob-ipmid\n");
	printf ("\tExample: oob-ipmid -port 0x298");
	printf ("\tExample: oob-ipmid -port 664");
	printf ("\n");
}

static int IPMICmdHandler (const uint8_t * req, uint8_t ** res, uint8_t * len)
{
	int retval = 0;
	int i = 0;
	IPMIMessageReq * ireq = (IPMIMessageReq*)req;
	IPMIMessageResp * ires = NULL;
	uint8_t * cmdreq = NULL;
	uint8_t * cmdres = NULL;
	uint8_t cmdlen = 0;
	CommandTable * cmdtbl = NULL;
	uint8_t cmdtblsize = 0;
	int (*cmdhdlr) (const uint8_t *, uint8_t **, uint8_t *) = NULL;
	/* Validate references */
	if ((NULL == req) || (NULL == len))
		return -1;

	cmdlen = *len - sizeof(IPMIMessageReq);
	/* Search NetFn Table */
	for (i = 0; i < gNetFnTblSize; i++)
	{
		/* We haven't found a match */
		if (0xFF == gNetFnTbl[i].netfn)
			return -1;
		/* Found a match - We do support this NetFn */
		if ((ireq->netFn >> 2) == gNetFnTbl[i].netfn)
		{
			cmdtbl = gNetFnTbl[i].cmdtbl;
			cmdtblsize = *gNetFnTbl[i].size;
			break;
		}
	}
	if ((NULL == cmdtbl) || (0 == cmdtblsize))
		return -1;

	for (i = 0; i < cmdtblsize; i++)
	{
		/* We haven't found a match */
		if (0xFF == cmdtbl[i].cmdnum)
			return -1;
		/* Found a match - We do support this Command */
		if (ireq->cmd == cmdtbl[i].cmdnum)
		{
			/* Check if command length is valid */
			if ((0xFF != cmdtbl[i].cmdlen) && (cmdlen != cmdtbl[i].cmdlen))
			{
				return -1;
			}
			cmdhdlr = cmdtbl[i].cmdhdlr;
			break;
		}
	}
	if (NULL == cmdhdlr)
		return -1;

	cmdreq = req + sizeof(IPMIMessageReq);
	retval = cmdhdlr (cmdreq, &cmdres, &cmdlen);
	if (0 == retval)
	{
		if ((NULL == cmdres) || (0 == cmdlen))
			return -1;

		*len = sizeof(IPMIMessageResp) + cmdlen + sizeof(uint8_t);
		*res = (uint8_t *) malloc (*len);
		if (NULL == *res)
			return -1;
		memset (*res, 0, *len);
		
		ires = (IPMIMessageResp *) *res;
		ires->rqAddr = ireq->rqAddr;
		ires->netFn = ireq->netFn + (1<<2);
		ires->checksum = CalcChecksum((uint8_t*)ires, 2);
		ires->rsAddr = ireq->rsAddr;
		ires->rqSeq = ireq->rqSeq;
		ires->cmd = ireq->cmd;

		memcpy (*res + sizeof(IPMIMessageResp), cmdres, cmdlen);
		*(*res + sizeof(IPMIMessageResp) + cmdlen) = CalcChecksum ( *res + 3, sizeof(IPMIMessageResp) - 3 + cmdlen);
	}
	if (cmdres)
		free (cmdres);
	return retval;
}	

static void save_pid (void) {
    pid_t pid = 0;
    FILE *pidfile = NULL;
    pid = getpid();
    if (!(pidfile = fopen("/run/oob-ipmid.pid", "w"))) {
        fprintf(stderr, "failed to open pidfile\n");
        return;
    }
    fprintf(pidfile, "%d\n", pid);
    fclose(pidfile);
}

int main (int argc, char**argv)
{
	int       fd = 0;
	struct    sockaddr_in serv_addr, cli_addr;
	socklen_t socklen = sizeof(serv_addr);
	uint8_t   buffer [512];	// Buffer size is arbitrary value
	uint16_t  readlen = 0;
	int16_t   writelen = 0;
	long      port = PRIMARY_RMCP_PORT;
	RMCPHeader     * header;
	ASFMessageReq  * RMCPPingReq;
	ASFMessageResp * RMCPPongResp;
	uint8_t * msgreq;
	uint8_t * msgres;
	uint8_t msglen;

	if (3 == argc)
	{
		if (0 == strncasecmp (argv[1], "-port", sizeof("-port")))
		{
			port = strtol(argv[2], NULL, 0); 
			if ((LONG_MIN == port) || (LONG_MAX == port))
			{
				printf ("Port %s is invalid\n", argv[2]);
				usage();
				return EXIT_FAILURE;
			}
		}
		else
		{
			printf ("Unrecognized argument %s\n", argv[1]);
			usage();
			return EXIT_FAILURE;
		}
	}
	else if (1 != argc)
	{
		usage();
		return EXIT_SUCCESS;
	}

    save_pid();

	/* Create a UDP socket and bind to RMCP port */
	fd = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	memset (&serv_addr, 0, sizeof(serv_addr));
	memset (&cli_addr, 0, sizeof(cli_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons((u_short)port);
	if (0 == bind (fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)))
	{
		/* Run forever */
		while (1)
		{
			readlen = recvfrom(fd, buffer, sizeof(buffer), 0, (struct sockaddr*)&cli_addr, &socklen);
			if (readlen > 0)
			{
				header = (RMCPHeader*)buffer;
				/* Do some sanity checks on received packet */
				if (ASF_2_0_VERSION != header->version)
				{
					printf ("Version %d not supported\n", header->version);
					continue;
				}
				if (readlen < (sizeof(RMCPHeader) + sizeof(ASFMessageReq)))
				{
					printf ("Received packet length is less than Ping request size\n");
					continue;
				}
				if (ASF_CLASS == header->class)
				{
					/* TODO: RMCP Ping request is processed here. Shall move to appropriate file*/
					RMCPPingReq = (ASFMessageReq*)&buffer[sizeof(RMCPHeader)];
					if ((ASF_IANA == ntohl (RMCPPingReq->IANAEntNum)) && (PING_REQ_CMD == RMCPPingReq->msgtype))
					{
						/* RMCP Ping Received */
						RMCPPongResp = (ASFMessageResp*) &buffer[sizeof(RMCPHeader)];
						RMCPPongResp->msgtype = PONG_RESP_CMD;
						RMCPPongResp->datalen = 16;
						/* For portability, make sure host and network byte orders are handled properly */
						RMCPPongResp->OEMIANA = htonl(MSFT_IANA);
						writelen = sendto (fd, buffer, sizeof(RMCPHeader) + sizeof(ASFMessageResp), 0, (struct sockaddr*)&cli_addr, socklen);
						if (writelen <= 0)
						{
							perror ("Error writing to socket: ");
						}
					}
					
				}
				else if (IPMI_CLASS == header->class)
				{
					/* IPMI Request received */
					/* If validation fails, just drop the packet and wait for next. 
					Don't have to respond back to the request with error completion code */
					if (false == IPMIRMCPSessionlessValidation(buffer))
						continue;
					if (readlen != sizeof (RMCPGetAuthReq))
					{
						/* Received packet length is not same as expected Get Channel Authentication Capabilities size */
						continue;
					}
					msgreq = buffer + sizeof(RMCPHeader) + sizeof(IPMISessionHeader);
					msgres = NULL;
					msglen = readlen - sizeof(RMCPHeader) - sizeof(IPMISessionHeader) - sizeof(uint8_t); //Checksum2
					/* Respond only to requests that we can service, simply ignore the rest */
					if(0 == IPMICmdHandler ((const uint8_t *)msgreq, &msgres, &msglen))
					{
						/* Drop if response is invalid */
						if ((NULL == msgres) || (0 == msglen))
							continue;

						/* We are reusing buffer for both request and response */
						memcpy (buffer + sizeof(RMCPHeader) + sizeof(IPMISessionHeader), msgres, msglen);
						((IPMISessionHeader*)(buffer + sizeof(RMCPHeader)))->payloadlen = msglen;
						/* Add RMCP and IPMI header to length */
						writelen = msglen + sizeof(RMCPHeader) + sizeof(IPMISessionHeader);
						writelen = sendto (fd, buffer, writelen, 0, (struct sockaddr*)&cli_addr, socklen);
						/* TODO: If bytes written to socket is less than packet size, this exception should be handled */
						if (writelen <= 0)
						{
							perror ("Error writing to socket: ");
						}
					}
					/* Free any memory that might have been allocated in IPMICmdHandler */
					if (msgres)
						free(msgres);
				}
			}
			else
			{
				perror ("RMCP Header not received:");
			}
		}
	}
	else
	{
		perror ("Binding to IPMI socket failed: ");
	}
	/* Ideally, we shouldn't be reaching here. If we ever reach here, do a graceful exit*/
	shutdown(fd, SHUT_RDWR);
	close(fd);
	printf ("OOB IPMI Daemon is exiting\n");
	return 0;
}
