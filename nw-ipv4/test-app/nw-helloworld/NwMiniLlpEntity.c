/*----------------------------------------------------------------------------*
 *                                                                            *
 *            M I N I M A L I S T I C     U D P     E N T I T Y               *
 *                                                                            *
 *                    Copyright (C) 2010 Amit Chawre.                         *
 *                                                                            *
 *----------------------------------------------------------------------------*/


/**
 * @file NwMiniLlpEntity.c
 * @brief This file contains example of a minimalistic ULP entity.
*/

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include "NwEvt.h"
#include "NwIpv4.h"
#include "NwMiniLogMgrEntity.h"
#include "NwMiniLlpEntity.h"

#ifndef NW_ASSERT
#define NW_ASSERT assert
#endif

#define MAX_UDP_PAYLOAD_LEN             (4096)

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------
 * Private functions
 *--------------------------------------------------------------------------*/

static
void NW_EVT_CALLBACK(nwIpv4DataIndicationCallbackData)
{
  NwIpv4RcT         rc;
  NwU8T         udpBuf[MAX_UDP_PAYLOAD_LEN];
  NwS32T        bytesRead;
  NwU32T        peerLen;
  struct sockaddr_in peer;
  NwMiniLlpEntityT* thiz = (NwMiniLlpEntityT*) arg;

  peerLen = sizeof(peer);

  bytesRead = recvfrom(thiz->hSocket, udpBuf, MAX_UDP_PAYLOAD_LEN , 0, (struct sockaddr *) &peer,(socklen_t*) &peerLen);
  if(bytesRead)
  {
    NW_LOG(NW_LOG_LEVEL_DEBG, "Received UDP message of length %u from %X:%u", bytesRead, ntohl(peer.sin_addr.s_addr), ntohs(peer.sin_port));
    rc = nwIpv4ProcessLlpDataInd(thiz->hIpv4Stack, udpBuf, bytesRead);
  }
  else
  {
    NW_LOG(NW_LOG_LEVEL_ERRO, "%s", strerror(errno));
  }
}


/*---------------------------------------------------------------------------
 * Public functions
 *--------------------------------------------------------------------------*/

NwIpv4RcT nwMiniUdpInit(NwMiniLlpEntityT* thiz, NwIpv4StackHandleT hIpv4Stack, NwU8T* ipAddr)
{
  int sd;
  struct sockaddr_in addr;

#ifndef IPPROTO_GRE
#define IPPROTO_GRE     (47)
#endif
  sd = socket(AF_INET, SOCK_RAW, IPPROTO_GRE);

  if (sd < 0)
  {
    NW_LOG(NW_LOG_LEVEL_ERRO, "%s", strerror(errno));
    NW_ASSERT(0);
  }

  addr.sin_family       = AF_INET;
  addr.sin_addr.s_addr  = inet_addr(ipAddr);
  memset(addr.sin_zero, '\0', sizeof (addr.sin_zero));

  if(bind(sd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
  {
    NW_LOG(NW_LOG_LEVEL_ERRO, "%s", strerror(errno));
    NW_ASSERT(0);
  }

  NW_EVENT_ADD(thiz->ev, sd, nwIpv4DataIndicationCallbackData, thiz, NW_EVT_READ | NW_EVT_PERSIST);

  thiz->hSocket = sd;
  thiz->hIpv4Stack = hIpv4Stack;

  return NW_IPv4_OK;
}

NwIpv4RcT nwMiniUdpDestroy(NwMiniLlpEntityT* thiz)
{
  close(thiz->hSocket);
}

NwIpv4RcT nwMiniUdpDataReq(NwIpv4LlpHandleT llpHandle,
    NwU8T* dataBuf,
    NwU32T dataSize,
    NwU32T peerIpAddr,
    NwU32T peerPort)
{
  struct sockaddr_in peerAddr;
  NwS32T bytesSent;
  NwMiniLlpEntityT* thiz = (NwMiniLlpEntityT*) llpHandle;

  peerAddr.sin_addr.s_addr  = (peerIpAddr);

  bytesSent = sendto (thiz->hSocket, dataBuf, dataSize, 0, (struct sockaddr *) &peerAddr, sizeof(peerAddr));

  if(bytesSent < 0)
  {
    NW_LOG(NW_LOG_LEVEL_ERRO, "%s", strerror(errno));
    NW_ASSERT(0);
  }
  else
  {
    NW_LOG(NW_LOG_LEVEL_DEBG, "Sent %u bytes on handle 0x%x to peer %u.%u.%u.%u", dataSize, llpHandle,
        (peerIpAddr & 0x000000ff),
        (peerIpAddr & 0x0000ff00) >> 8,
        (peerIpAddr & 0x00ff0000) >> 16,
        (peerIpAddr & 0xff000000) >> 24);

  }
  return NW_IPv4_OK;
}



#ifdef __cplusplus
}
#endif
