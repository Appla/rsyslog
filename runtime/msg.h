/* msg.h
 * Header file for all msg-related functions.
 *
 * File begun on 2007-07-13 by RGerhards (extracted from syslogd.c)
 *
 * Copyright 2007 Rainer Gerhards and Adiscon GmbH.
 *
 * This file is part of the rsyslog runtime library.
 *
 * The rsyslog runtime library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * The rsyslog runtime library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with the rsyslog runtime library.  If not, see <http://www.gnu.org/licenses/>.
 *
 * A copy of the GPL can be found in the file "COPYING" in this distribution.
 * A copy of the LGPL can be found in the file "COPYING.LESSER" in this distribution.
 */
#include "template.h" /* this is a quirk, but these two are too interdependant... */

#ifndef	MSG_H_INCLUDED
#define	MSG_H_INCLUDED 1

/* some configuration constants */
#define CONF_RAWMSG_BUFSIZE 101
#define CONF_TAG_BUFSIZE 33 /* RFC says 32 chars (+ \0), but in practice we see longer ones... */

#include <pthread.h>
#include "obj.h"
#include "syslogd-types.h"
#include "template.h"

/* rgerhards 2004-11-08: The following structure represents a
 * syslog message. 
 *
 * Important Note:
 * The message object is used for multiple purposes (once it
 * has been created). Once created, it actully is a read-only
 * object (though we do not specifically express this). In order
 * to avoid multiple copies of the same object, we use a
 * reference counter. This counter is set to 1 by the constructer
 * and increased by 1 with a call to MsgAddRef(). The destructor
 * checks the reference count. If it is more than 1, only the counter
 * will be decremented. If it is 1, however, the object is actually
 * destroyed. To make this work, it is vital that MsgAddRef() is
 * called each time a "copy" is stored somewhere.
 */
struct msg {
	BEGINobjInstance;	/* Data to implement generic object - MUST be the first data element! */
	flowControl_t flowCtlType; /**< type of flow control we can apply, for enqueueing, needs not to be persisted because
				        once data has entered the queue, this property is no longer needed. */
	pthread_mutexattr_t mutAttr;
	pthread_mutex_t mut;
	bool	bDoLock;	 /* use the mutex? */
	bool	bParseHOSTNAME;	/* should the hostname be parsed from the message? */
	short	iRefCount;	/* reference counter (0 = unused) */
	   /* background: the hostname is not present on "regular" messages
	    * received via UNIX domain sockets from the same machine. However,
	    * it is available when we have a forwarder (e.g. rfc3195d) using local
	    * sockets. All in all, the parser would need parse templates, that would
	    * resolve all these issues... rgerhards, 2005-10-06
	    */
	short	iSeverity;	/* the severity 0..7 */
	short	iFacility;	/* Facility code 0 .. 23*/
	uchar	*pszRawMsg;	/* message as it was received on the wire. This is important in case we
				 * need to preserve cryptographic verifiers.  */
	int	iLenRawMsg;	/* length of raw message */
	short	offAfterPRI;	/* offset, at which raw message WITHOUT PRI part starts in pszRawMsg */
	uchar	*pszMSG;	/* the MSG part itself */
	int	iLenMSG;	/* Length of the MSG part */
	int	iLenTAG;	/* Length of the TAG part */
	uchar	*pszHOSTNAME;	/* HOSTNAME from syslog message */
	int	iLenHOSTNAME;	/* Length of HOSTNAME */
	uchar	*pszRcvFrom;	/* System message was received from */
	int	iLenRcvFrom;	/* Length of pszRcvFrom */
	uchar	*pszRcvFromIP;	/* IP of system message was received from */
	int	iLenRcvFromIP;	/* Length of pszRcvFromIP */
	uchar *pszInputName;	/* name of the input module that submitted this message */
	int	iLenInputName;	/* Length of pszInputName */
	short	iProtocolVersion;/* protocol version of message received 0 - legacy, 1 syslog-protocol) */
	cstr_t *pCSProgName;	/* the (BSD) program name */
	cstr_t *pCSStrucData;   /* STRUCTURED-DATA */
	cstr_t *pCSAPPNAME;	/* APP-NAME */
	cstr_t *pCSPROCID;	/* PROCID */
	cstr_t *pCSMSGID;	/* MSGID */
	time_t ttGenTime;	/* time msg object was generated, same as tRcvdAt, but a Unix timestamp.
				   While this field looks redundant, it is required because a Unix timestamp
				   is used at later processing stages (namely in the output arena). Thanks to
				   the subleties of how time is defined, there is no reliable way to reconstruct
				   the Unix timestamp from the syslogTime fields (in practice, we may be close
				   enough to reliable, but I prefer to leave the subtle things to the OS, where
				   it obviously is solved in way or another...). */
	struct syslogTime tRcvdAt;/* time the message entered this program */
	char *pszRcvdAt3164;	/* time as RFC3164 formatted string (always 15 charcters) */
	char *pszRcvdAt3339;	/* time as RFC3164 formatted string (32 charcters at most) */
	char *pszRcvdAt_SecFrac;/* time just as fractional seconds  (6 charcters) */
	char *pszRcvdAt_MySQL;	/* rcvdAt as MySQL formatted string (always 14 charcters) */
        char *pszRcvdAt_PgSQL;  /* rcvdAt as PgSQL formatted string (always 21 characters) */
	struct syslogTime tTIMESTAMP;/* (parsed) value of the timestamp */
	char *pszTIMESTAMP3164;	/* TIMESTAMP as RFC3164 formatted string (always 15 charcters) */
	char *pszTIMESTAMP3339;	/* TIMESTAMP as RFC3339 formatted string (32 charcters at most) */
	char *pszTIMESTAMP_MySQL;/* TIMESTAMP as MySQL formatted string (always 14 charcters) */
        char *pszTIMESTAMP_PgSQL;/* TIMESTAMP as PgSQL formatted string (always 21 characters) */
        char *pszTIMESTAMP_SecFrac;/* TIMESTAMP fractional seconds (always 6 characters) */
	int msgFlags;		/* flags associated with this message */
	ruleset_t *pRuleset;	/* ruleset to be used for processing this message */
	/* some fixed-size buffers to save malloc()/free() for frequently used fields (from the default templates) */
	uchar szRawMsg[CONF_RAWMSG_BUFSIZE];	/* most messages are small, and these are stored here (without malloc/free!) */
	union {
		uchar	*pszTAG;	/* pointer to tag value */
		uchar	szBuf[CONF_TAG_BUFSIZE];
	} TAG;
	char pszTimestamp3164[16];
	char pszTimestamp3339[33];
};


/* message flags (msgFlags), not an enum for historical reasons
 */
#define NOFLAG		0x000	/* no flag is set (to be used when a flag must be specified and none is required) */
#define INTERNAL_MSG	0x001	/* msg generated by logmsgInternal() --> special handling */
/* 0x002 not used because it was previously a known value - rgerhards, 2008-10-09 */
#define IGNDATE		0x004	/* ignore, if given, date in message and use date of reception as msg date */
#define MARK		0x008	/* this message is a mark */
#define NEEDS_PARSING	0x010	/* raw message, must be parsed before processing can be done */
#define PARSE_HOSTNAME	0x020	/* parse the hostname during message parsing */


/* function prototypes
 */
PROTOTYPEObjClassInit(msg);
rsRetVal msgConstruct(msg_t **ppThis);
rsRetVal msgConstructWithTime(msg_t **ppThis, struct syslogTime *stTime, time_t ttGenTime);
rsRetVal msgDestruct(msg_t **ppM);
msg_t* MsgDup(msg_t* pOld);
msg_t *MsgAddRef(msg_t *pM);
void setProtocolVersion(msg_t *pM, int iNewVersion);
void MsgSetInputName(msg_t *pMsg, uchar*, size_t);
rsRetVal MsgSetAPPNAME(msg_t *pMsg, char* pszAPPNAME);
rsRetVal MsgSetPROCID(msg_t *pMsg, char* pszPROCID);
rsRetVal MsgSetMSGID(msg_t *pMsg, char* pszMSGID);
void MsgAssignTAG(msg_t *pMsg, uchar *pBuf);
void MsgSetTAG(msg_t *pMsg, uchar* pszBuf, size_t lenBuf);
void MsgSetRuleset(msg_t *pMsg, ruleset_t*);
rsRetVal MsgSetFlowControlType(msg_t *pMsg, flowControl_t eFlowCtl);
rsRetVal MsgSetStructuredData(msg_t *pMsg, char* pszStrucData);
void MsgSetRcvFrom(msg_t *pMsg, uchar* pszRcvFrom);
rsRetVal MsgSetRcvFromIP(msg_t *pMsg, uchar* pszRcvFromIP);
void MsgAssignHOSTNAME(msg_t *pMsg, char *pBuf);
void MsgSetHOSTNAME(msg_t *pMsg, uchar* pszHOSTNAME);
rsRetVal MsgSetAfterPRIOffs(msg_t *pMsg, short offs);
void MsgSetMSG(msg_t *pMsg, char* pszMSG);
void MsgSetRawMsgWOSize(msg_t *pMsg, char* pszRawMsg);
void MsgSetRawMsg(msg_t *pMsg, char* pszRawMsg, size_t lenMsg);
void moveHOSTNAMEtoTAG(msg_t *pM);
char *MsgGetProp(msg_t *pMsg, struct templateEntry *pTpe,
                 cstr_t *pCSPropName, unsigned short *pbMustBeFreed);
char *textpri(char *pRes, size_t pResLen, int pri);
rsRetVal msgGetMsgVar(msg_t *pThis, cstr_t *pstrPropName, var_t **ppVar);
rsRetVal MsgEnableThreadSafety(void);

/* TODO: remove these five (so far used in action.c) */
char *getMSG(msg_t *pM);
char *getHOSTNAME(msg_t *pM);
char *getPROCID(msg_t *pM);
char *getAPPNAME(msg_t *pM);
int getMSGLen(msg_t *pM);

char *getHOSTNAME(msg_t *pM);
int getHOSTNAMELen(msg_t *pM);
char *getProgramName(msg_t *pM);
int getProgramNameLen(msg_t *pM);
uchar *getRcvFrom(msg_t *pM);

/* The MsgPrepareEnqueue() function is a macro for performance reasons.
 * It needs one global variable to work. This is acceptable, as it gains
 * us quite some performance and is fully abstracted using this header file.
 * The important thing is that no other module is permitted to actually
 * access that global variable! -- rgerhards, 2008-01-05
 */
extern void (*funcMsgPrepareEnqueue)(msg_t *pMsg);
#define MsgPrepareEnqueue(pMsg) funcMsgPrepareEnqueue(pMsg)

#endif /* #ifndef MSG_H_INCLUDED */
/* vim:set ai:
 */
