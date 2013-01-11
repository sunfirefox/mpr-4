/*
    matrixssl.c -- Support for secure sockets via MatrixSSL

    Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "bit.h"

#if BIT_PACK_MATRIXSSL
/* 
    Work-around to allow the windows 7.* SDK to be used with VS 2013 
 */
#if _MSC_VER >= 1700
    #define SAL_SUPP_H
    #define SPECSTRING_SUPP_H
#endif
/*
    Matrixssl defines int32, uint32, int64 and uint64, but does not provide HAS_XXX to disable. 
    So must include matrixsslApi.h first and then workaround. 
 */
#if WIN32
 #include   <winsock2.h>
 #include   <windows.h>
#endif
 #include    "matrixsslApi.h"

#define     HAS_INT32 1
#define     HAS_UINT32 1
#define     HAS_INT64 1
#define     HAS_UINT64 1

#include    "mpr.h"

/************************************* Defines ********************************/
/*
    Per SSL configuration structure
 */
typedef struct MatrixConfig {
    sslKeys_t       *keys;
    sslSessionId_t  *session;
} MatrixConfig;

/*
    Per socket extended state
 */
typedef struct MatrixSocket {
    MprSocket       *sock;
    MatrixConfig    *cfg;
    ssl_t           *ctx;               /* MatrixSSL ssl_t structure */
    char            *outbuf;            /* Pending output data */
    char            *peerName;          /* Desired peer name */
    ssize           outlen;             /* Length of outbuf */
    ssize           written;            /* Number of unencoded bytes written */
    int             more;               /* MatrixSSL stack has buffered data */
} MatrixSocket;

#if UNUSED
/*
    Empty CA cert.
 */
static uchar CAcertSrvBuf[] = { 
    48, 130, 2, 144, 48, 130, 1, 249, 160, 3, 2, 1, 2,
    2, 1, 31, 48, 13, 6, 9, 42, 134, 72, 134, 247, 13,
    1, 1, 5, 5, 0, 48, 129, 129, 49, 35, 48, 33, 6,
    3, 85, 4, 3, 12, 26, 77, 97, 116, 114, 105, 120, 83,
    83, 76, 32, 83, 97, 109, 112, 108, 101, 32, 83, 101, 114,
    118, 101, 114, 32, 67, 65, 49, 11, 48, 9, 6, 3, 85,
    4, 6, 12, 2, 85, 83, 49, 11, 48, 9, 6, 3, 85,
    4, 8, 12, 2, 87, 65, 49, 16, 48, 14, 6, 3, 85,
    4, 7, 12, 7, 83, 101, 97, 116, 116, 108, 101, 49, 31,
    48, 29, 6, 3, 85, 4, 10, 12, 22, 80, 101, 101, 114, 
    83, 101, 99, 32, 78, 101, 116, 119, 111, 114, 107, 115, 44,
    32, 73, 110, 99, 46, 49, 13, 48, 11, 6, 3, 85, 4,
    11, 12, 4, 84, 101, 115, 116, 48, 30, 23, 13, 49, 48,
    48, 51, 48, 50, 49, 55, 49, 57, 48, 55, 90, 23, 13, 
    49, 51, 48, 51, 48, 49, 49, 55, 49, 57, 48, 55, 90,
    48, 129, 129, 49, 35, 48, 33, 6, 3, 85, 4, 3, 12,
    26, 77, 97, 116, 114, 105, 120, 83, 83, 76, 32, 83, 97,
    109, 112, 108, 101, 32, 83, 101, 114, 118, 101, 114, 32, 67,
    65, 49, 11, 48, 9, 6, 3, 85, 4, 6, 12, 2, 85,
    83, 49, 11, 48, 9, 6, 3, 85, 4, 8, 12, 2, 87,
    65, 49, 16, 48, 14, 6, 3, 85, 4, 7, 12, 7, 83,
    101, 97, 116, 116, 108, 101, 49, 31, 48, 29, 6, 3, 85, 
    4, 10, 12, 22, 80, 101, 101, 114, 83, 101, 99, 32, 78,
    101, 116, 119, 111, 114, 107, 115, 44, 32, 73, 110, 99, 46, 
    49, 13, 48, 11, 6, 3, 85, 4, 11, 12, 4, 84, 101,
    115, 116, 48, 129, 159, 48, 13, 6, 9, 42, 134, 72, 134,
    247, 13, 1, 1, 1, 5, 0, 3, 129, 141, 0, 48, 129,
    137, 2, 129, 129, 0, 161, 37, 100, 65, 3, 153, 4, 51,
    190, 127, 211, 25, 110, 88, 126, 201, 223, 171, 11, 203, 100,
    10, 206, 214, 152, 173, 187, 96, 24, 202, 103, 225, 54, 18,
    236, 171, 140, 125, 209, 68, 184, 212, 195, 191, 210, 98, 123,
    237, 205, 213, 23, 209, 245, 108, 70, 236, 20, 25, 140, 68,
    255, 68, 53, 29, 23, 231, 200, 206, 199, 45, 74, 46, 86, 
    10, 229, 243, 126, 72, 33, 184, 21, 16, 76, 55, 28, 97, 
    3, 79, 192, 236, 241, 47, 12, 220, 59, 158, 29, 243, 133,
    125, 83, 30, 232, 243, 131, 16, 217, 8, 2, 185, 80, 197,
    139, 49, 11, 191, 233, 160, 55, 73, 239, 43, 151, 12, 31,
    151, 232, 145, 2, 3, 1, 0, 1, 163, 22, 48, 20, 48,
    18, 6, 3, 85, 29, 19, 1, 1, 1, 4, 8, 48, 6,
    1, 1, 1, 2, 1, 1, 48, 13, 6, 9, 42, 134, 72,
    134, 247, 13, 1, 1, 5, 5, 0, 3, 129, 129, 0, 11,
    36, 231, 0, 168, 211, 179, 52, 55, 21, 63, 125, 230, 86,
    116, 204, 101, 189, 20, 132, 242, 238, 119, 80, 44, 174, 74,
    125, 81, 98, 111, 223, 242, 96, 20, 192, 210, 7, 248, 91,
    101, 37, 85, 46, 0, 132, 72, 12, 21, 100, 132, 5, 122,
    170, 109, 207, 114, 159, 182, 95, 106, 103, 135, 80, 158, 99, 
    44, 126, 86, 191, 79, 126, 249, 99, 3, 18, 40, 128, 21,
    127, 185, 53, 51, 128, 20, 24, 249, 23, 90, 5, 171, 46,
    164, 3, 39, 94, 98, 103, 213, 169, 59, 10, 10, 74, 205,
    201, 16, 116, 204, 66, 111, 187, 36, 87, 144, 81, 194, 26,
    184, 76, 67, 209, 132, 6, 150, 183, 119, 59
};
#endif

/***************************** Forward Declarations ***************************/

static void     closeMss(MprSocket *sp, bool gracefully);
static void     disconnectMss(MprSocket *sp);
static ssize    flushMss(MprSocket *sp);
static char     *getMssState(MprSocket *sp);
static int      handshakeMss(MprSocket *sp, short cipherSuite);
static ssize    innerRead(MprSocket *sp, char *userBuf, ssize len);
static int      listenMss(MprSocket *sp, cchar *host, int port, int flags);
static void     manageMatrixSocket(MatrixSocket *msp, int flags);
static void     manageMatrixConfig(MatrixConfig *cfg, int flags);
static ssize    processMssData(MprSocket *sp, char *buf, ssize size, ssize nbytes, int *readMore);
static ssize    readMss(MprSocket *sp, void *buf, ssize len);
static int      upgradeMss(MprSocket *sp, MprSsl *ssl, cchar *peerName);
static int      verifyCert(ssl_t *ssl, psX509Cert_t *cert, int32 alert);
static ssize    writeMss(MprSocket *sp, cvoid *buf, ssize len);

/************************************ Code ************************************/

PUBLIC int mprCreateMatrixSslModule()
{
    MprSocketProvider   *provider;

    if ((provider = mprAllocObj(MprSocketProvider, 0)) == NULL) {
        return MPR_ERR_MEMORY;
    }
    provider->closeSocket = closeMss;
    provider->disconnectSocket = disconnectMss;
    provider->flushSocket = flushMss;
    provider->listenSocket = listenMss;
    provider->readSocket = readMss;
    provider->socketState = getMssState;
    provider->writeSocket = writeMss;
    provider->upgradeSocket = upgradeMss;
    mprAddSocketProvider("matrixssl", provider);

    if (matrixSslOpen() < 0) {
        return 0;
    }
    return 0;
}


static void manageMatrixConfig(MatrixConfig *cfg, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        ;
    } else if (flags & MPR_MANAGE_FREE) {
        if (cfg->keys) {
            matrixSslDeleteKeys(cfg->keys);
            cfg->keys = 0;
        }
        matrixSslClose();
    }
}


static void manageMatrixSocket(MatrixSocket *msp, int flags)
{
    MprSocketService    *ss;

    if (flags & MPR_MANAGE_MARK) {
        mprMark(msp->sock);
        mprMark(msp->peerName);

    } else if (flags & MPR_MANAGE_FREE) {
        //MOB - goahead does matrixSslEncodeClosureAlert, matrixSslGetOutdata here
        if (msp->ctx) {
            matrixSslDeleteSession(msp->ctx);
        }
        ss = MPR->socketService;
        mprRemoveItem(ss->secureSockets, msp->sock);
    }
}


/*
    Close the socket
 */
static void closeMss(MprSocket *sp, bool gracefully)
{
    MatrixSocket    *msp;
    uchar           *obuf;
    int             nbytes;

    assert(sp);

    lock(sp);
    msp = sp->sslSocket;
    assert(msp);

    if (!(sp->flags & MPR_SOCKET_EOF) && msp->ctx) {
        /*
            Flush data. Append a closure alert to any buffered output data, and try to send it.
            Don't bother retrying or blocking, we're just closing anyway.
         */
        matrixSslEncodeClosureAlert(msp->ctx);
        if ((nbytes = matrixSslGetOutdata(msp->ctx, &obuf)) > 0) {
            /* Ignore return */
            sp->service->standardProvider->writeSocket(sp, obuf, nbytes);
        }
    }
    sp->service->standardProvider->closeSocket(sp, gracefully);
    unlock(sp);
}


static int listenMss(MprSocket *sp, cchar *host, int port, int flags)
{
    return sp->service->standardProvider->listenSocket(sp, host, port, flags);
}


static int upgradeMss(MprSocket *sp, MprSsl *ssl, cchar *peerName)
{
    MprSocketService    *ss;
    MatrixSocket        *msp;
    MatrixConfig        *cfg;
    char                *password;
    uint32              cipherSuite;

    ss = sp->service;
    assert(ss);
    assert(sp);

    if ((msp = (MatrixSocket*) mprAllocObj(MatrixSocket, manageMatrixSocket)) == 0) {
        return MPR_ERR_MEMORY;
    }
    //  MOB - why locking?
    lock(sp);
    msp->sock = sp;
    sp->sslSocket = msp;
    sp->ssl = ssl;
    password = 0;

    mprAddItem(ss->secureSockets, sp);

    if (ssl->config) {
        msp->cfg = cfg = ssl->config;

    } else {
        if ((ssl->config = mprAllocObj(MatrixConfig, manageMatrixConfig)) == 0) {
            unlock(sp);
            return MPR_ERR_MEMORY;
        }
        msp->cfg = cfg = ssl->config;

        //  OPT - does this need to be done for each MprSsl or just once?
        if (matrixSslNewKeys(&cfg->keys) < 0) {
            mprError("MatrixSSL: Cannot create new MatrixSSL keys");
            unlock(sp);
            return MPR_ERR_CANT_INITIALIZE;
        }
        /*
            Read the certificate and the key file for this server. FUTURE - If using encrypted private keys, 
            we could prompt through a dialog box or on the console, for the user to enter the password
            rather than using NULL as the password here.
         */
        password = NULL;
        if (matrixSslLoadRsaKeys(cfg->keys, ssl->certFile, ssl->keyFile, password, NULL) < 0) {
            mprError("MatrixSSL: Could not read or decode certificate or key file."); 
            unlock(sp);
            return MPR_ERR_CANT_READ;
        }
    }
    unlock(sp);

    /* 
        Associate a new ssl session with this socket. The session represents the state of the ssl protocol 
        over this socket. Session caching is handled automatically by this api.
     */
    if (sp->flags & MPR_SOCKET_SERVER) {
        if (matrixSslNewServerSession(&msp->ctx, cfg->keys, verifyCert) < 0) {
            unlock(sp);
            return MPR_ERR_CANT_CREATE;
        }
    } else {
        msp->peerName = sclone(peerName);
        if (matrixSslLoadRsaKeys(cfg->keys, NULL, NULL, password, ssl->caFile) < 0) {
            mprError("MatrixSSL: Could not read or decode certificate or key file."); 
            unlock(sp);
            return MPR_ERR_CANT_INITIALIZE;
        }
        cipherSuite = 0;
        if (matrixSslNewClientSession(&msp->ctx, cfg->keys, NULL, cipherSuite, verifyCert, NULL, NULL) < 0) {
            unlock(sp);
            return MPR_ERR_CANT_CONNECT;
        }
        if (handshakeMss(sp, 0) < 0) {
            unlock(sp);
            return MPR_ERR_CANT_CONNECT;
        }
    }
    unlock(sp);
    return 0;
}


#if UNUSED
/*
    Store the name in printable form into buf; no more than (end - buf) characters will be written
 */
static void parseCert(MprBuf *buf, char *prefix, x509_name * dn)
{
    x509_name   *name;
    int         i;
    uchar       c;

    memset(s, 0, sizeof(s));
    name = dn;

    while (name != NULL) {
        mprPutToBuf(p, end - p, "%s", prefix);
        if (memcmp(name->oid.p, OID_X520, 2) == 0) {
            switch (name->oid.p[2]) {
            case X520_COMMON_NAME:
                mprPutToBuf(buf, "CN=");
                break;

            case X520_COUNTRY:
                mprPutToBuf(buf, "C=");
                break;

            case X520_LOCALITY:
                mprPutToBuf(buf, "L=");
                break;

            case X520_STATE:
                mprPutToBuf(buf, "ST=");
                break;

            case X520_ORGANIZATION:
                mprPutToBuf(buf, "O=");
                break;

            case X520_ORG_UNIT:
                mprPutToBuf(buf, "OU=");
                break;

            default:
                mprPutToBuf(buf, "0x%02X=", name->oid.p[2]);
                break;
            }
        } else if (memcmp(name->oid.p, OID_PKCS9, 8) == 0) {
            switch (name->oid.p[8]) {
            case PKCS9_EMAIL:
                mprPutToBuf(buf, "EMAIL=");
                break;

            default:
                mprPutToBuf(buf, "0x%02X=", name->oid.p[8]);
                break;
            }
        } else {
            mprPutToBuf(buf, "\?\?=");
        }
        for (i = 0; i < name->val.len; i++) {
            if (i >= (int)sizeof(s) - 1) {
                break;
            }
            c = name->val.p[i];
            if (c < 32 || c == 127 || (c > 128 && c < 160))
                s[i] = '?';
            else
                s[i] = c;
        }
        s[i] = '\0';
        mprPutToBuf(buf, "%s", s);
        name = name->next;
        mprPutToBuf(buf, ", ");
    }
}
#endif


static char *getMssState(MprSocket *sp)
{
    MatrixSocket    *msp;
    ssl_t           *ctx;
    MprBuf          *buf;

    msp = sp->sslSocket;
    ctx = msp->ctx;
    buf = mprCreateBuf(0, 0);
#if UNUSED
    char            *own, *peer;
    certState(buf, sp->acceptIp ? "CLIENT_" : "SERVER_", ctx->keys->cdert);
    certState(buf, sp->acceptIp ? "SERVER_" : "CLIENT_", ctx->cert);
#endif
    return mprGetBufStart(buf);
}


/*
    Validate certificates
 */
static int verifyCert(ssl_t *ssl, psX509Cert_t *cert, int32 alert)
{
    MprSocketService    *ss;
    MprSocket           *sp;
    struct tm           t;
    char                *c;
    int                 next, y, m, d;

    ss = MPR->socketService;

    /*
        Find our handle. This is really ugly because the matrix api does not provide a handle
     */
    lock(ss);
    sp = 0;
    for (ITERATE_ITEMS(ss->secureSockets, sp, next)) {
        if (sp->ssl && ((MatrixSocket*) sp->sslSocket)->ctx == ssl) {
            break;
        }
    }
    unlock(ss);

    if (!sp) {
        /* Should not get here */
        assert(sp);
        return SSL_ALLOW_ANON_CONNECTION;
    }

    if (alert > 0) {
        if (alert == SSL_ALERT_UNKNOWN_CA) {
            if (sp->ssl->verifyIssuer) {
                return alert;
            }
        }
        if (!sp->ssl->verifyPeer) {
            return SSL_ALLOW_ANON_CONNECTION;
        }
        return alert;
    }
#if FUTURE
    msp = sp->sslSocket;
    if (msp->peerName && !smatch(msp->peerName, cert->subject.commonName)) {
        mprError("SSL certificate Common name mismatch");
        return PS_FAILURE;
    }
#endif

	/* 
        Validate the 'not before' date 
     */
    mprDecodeLocalTime(&t, mprGetTime());
	if ((c = cert->notBefore) != NULL) {
		if (strlen(c) < 8) {
			return PS_FAILURE;
		}
		/* 
            UTCTIME, defined in 1982, has just a 2 digit year 
         */
		if (cert->timeType == ASN_UTCTIME) {
			y =  2000 + 10 * (c[0] - '0') + (c[1] - '0'); 
            c += 2;
		} else {
			y = 1000 * (c[0] - '0') + 100 * (c[1] - '0') + 10 * (c[2] - '0') + (c[3] - '0'); 
            c += 4;
		}
		m = 10 * (c[0] - '0') + (c[1] - '0'); 
        c += 2;
		d = 10 * (c[0] - '0') + (c[1] - '0'); 
        y -= 1900;
        m -= 1;
		if (t.tm_year < y) {
            return PS_FAILURE; 
        }
		if (t.tm_year == y) {
			if (t.tm_mon < m || (t.tm_mon == m && t.tm_mday < d)) {
                mprError("Certificate not yet valid");
                return PS_FAILURE;
            }
		}
	}

	/* 
        Validate the 'not after' date 
     */
	if ((c = cert->notAfter) != NULL) {
		if (strlen(c) < 8) {
			return PS_FAILURE;
		}
		if (cert->timeType == ASN_UTCTIME) {
			y =  2000 + 10 * (c[0] - '0') + (c[1] - '0'); 
            c += 2;
		} else {
			y = 1000 * (c[0] - '0') + 100 * (c[1] - '0') + 10 * (c[2] - '0') + (c[3] - '0'); 
            c += 4;
		}
		m = 10 * (c[0] - '0') + (c[1] - '0'); 
        c += 2;
		d = 10 * (c[0] - '0') + (c[1] - '0'); 
        y -= 1900;
        m -= 1;
		if (t.tm_year > y) {
            return PS_FAILURE; 
        } else if (t.tm_year == y) {
			if (t.tm_mon > m || (t.tm_mon == m && t.tm_mday > d)) {
                mprError("Certificate has expired");
                return PS_FAILURE;
            }
		}
	}
	return PS_SUCCESS;
}


static void disconnectMss(MprSocket *sp)
{
    sp->service->standardProvider->disconnectSocket(sp);
}


static ssize blockingWrite(MprSocket *sp, cvoid *buf, ssize len)
{
    MprSocketProvider   *standard;
    ssize               written, bytes;
    int                 prior;

    standard = sp->service->standardProvider;
    prior = mprSetSocketBlockingMode(sp, 1);
    for (written = 0; len > 0; ) {
        if ((bytes = standard->writeSocket(sp, buf, len)) < 0) {
            mprSetSocketBlockingMode(sp, prior);
            return bytes;
        }
        buf = (char*) buf + bytes;
        len -= bytes;
        written += bytes;
    }
    mprSetSocketBlockingMode(sp, prior);
    return written;
}


/*
    Construct the initial HELLO message to send to the server and initiate
    the SSL handshake. Can be used in the re-handshake scenario as well.
    This is a blocking routine.
 */
static int handshakeMss(MprSocket *sp, short cipherSuite)
{
    MatrixSocket    *msp;
    ssize           rc, written, toWrite;
    char            *obuf, buf[BIT_MAX_BUFFER];
    int             mode;

    msp = sp->sslSocket;

    toWrite = matrixSslGetOutdata(msp->ctx, (uchar**) &obuf);
    if ((written = blockingWrite(sp, obuf, toWrite)) < 0) {
        mprError("MatrixSSL: Error in socketWrite");
        return MPR_ERR_CANT_INITIALIZE;
    }
    matrixSslSentData(msp->ctx, (int) written);
    mode = mprSetSocketBlockingMode(sp, 1);

    while (1) {
        /*
            Reading handshake records should always return 0 bytes, we aren't expecting any data yet.
         */
        if ((rc = innerRead(sp, buf, sizeof(buf))) == 0) {
            if (mprIsSocketEof(sp)) {
                mprSetSocketBlockingMode(sp, mode);
                return MPR_ERR_CANT_INITIALIZE;
            }
            if (matrixSslHandshakeIsComplete(msp->ctx)) {
                break;
            }
        } else {
            mprError("MatrixSSL: sslRead error in sslDoHandhake, rc %d", rc);
            mprSetSocketBlockingMode(sp, mode);
            return MPR_ERR_CANT_INITIALIZE;
        }
    }
    mprSetSocketBlockingMode(sp, mode);
    return 0;
}


/*
    Process incoming data. Some is app data, some is SSL control data.
 */
static ssize processMssData(MprSocket *sp, char *buf, ssize size, ssize nbytes, int *readMore)
{
    MatrixSocket    *msp;
    uchar           *data, *obuf;
    ssize           toWrite, written, copied, sofar;
    uint32          dlen;
    int                 rc;

    msp = (MatrixSocket*) sp->sslSocket;
    *readMore = 0;
    sofar = 0;

    /*
        Process the received data. If there is application data, it is returned in data/dlen
     */
    rc = matrixSslReceivedData(msp->ctx, (int) nbytes, &data, &dlen);

    while (1) {
        switch (rc) {
        case PS_SUCCESS:
            return sofar;

        case MATRIXSSL_REQUEST_SEND:
            toWrite = matrixSslGetOutdata(msp->ctx, &obuf);
            if ((written = blockingWrite(sp, obuf, toWrite)) < 0) {
                mprError("MatrixSSL: Error in process");
                return MPR_ERR_CANT_INITIALIZE;
            }
            matrixSslSentData(msp->ctx, (int) written);
            *readMore = 1;
            return 0;

        case MATRIXSSL_REQUEST_RECV:
            /* Partial read. More read data required */
            *readMore = 1;
            msp->more = 1;
            return 0;

        case MATRIXSSL_HANDSHAKE_COMPLETE:
            *readMore = 0;
            return 0;

        case MATRIXSSL_RECEIVED_ALERT:
            assert(dlen == 2);
            if (data[0] == SSL_ALERT_LEVEL_FATAL) {
                return MPR_ERR;
            } else if (data[1] == SSL_ALERT_CLOSE_NOTIFY) {
                //  ignore - graceful close
                return 0;
            } else {
                //  ignore
            }
            rc = matrixSslProcessedData(msp->ctx, &data, &dlen);
            break;

        case MATRIXSSL_APP_DATA:
            copied = min((ssize) dlen, size);
            memcpy(buf, data, copied);
            buf += copied;
            size -= copied;
            data += copied;
            dlen = dlen - (int) copied;
            sofar += copied;
            msp->more = ((ssize) dlen > size) ? 1 : 0;
            if (!msp->more) {
                /* The MatrixSSL buffer has been consumed, see if we can get more data */
                rc = matrixSslProcessedData(msp->ctx, &data, &dlen);
                break;
            }
            return sofar;

        default:
            return MPR_ERR;
        }
    }
}


/*
    Return number of bytes read. Return -1 on errors and EOF
 */
static ssize innerRead(MprSocket *sp, char *buf, ssize size)
{
    MprSocketProvider   *standard;
    MatrixSocket        *msp;
    uchar               *mbuf;
    ssize               nbytes;
    int                 msize, readMore;

    msp = (MatrixSocket*) sp->sslSocket;
    standard = sp->service->standardProvider;
    do {
        /*
            Get the MatrixSSL read buffer to read data into
         */
        if ((msize = matrixSslGetReadbuf(msp->ctx, &mbuf)) < 0) {
            return MPR_ERR_BAD_STATE;
        }
        readMore = 0;
        if ((nbytes = standard->readSocket(sp, mbuf, msize)) > 0) {
            if ((nbytes = processMssData(sp, buf, size, nbytes, &readMore)) > 0) {
                return nbytes;
            }
        }
    } while (readMore);
    return 0;
}


/*
    Return number of bytes read. Return -1 on errors and EOF.
 */
static ssize readMss(MprSocket *sp, void *buf, ssize len)
{
    MatrixSocket    *msp;
    ssize           bytes;

    if (len <= 0) {
        return -1;
    }
    lock(sp);
    bytes = innerRead(sp, buf, len);
    msp = (MatrixSocket*) sp->sslSocket;
    if (msp->more) {
        sp->flags |= MPR_SOCKET_BUFFERED_READ;
        mprRecallWaitHandlerByFd(sp->fd);
    }
    unlock(sp);
    return bytes;
}


/*
    Non-blocking write data. Return the number of bytes written or -1 on errors.
    Returns zero if part of the data was written.

    Encode caller's data buffer into an SSL record and write to socket. The encoded data will always be 
    bigger than the incoming data because of the record header (5 bytes) and MAC (16 bytes MD5 / 20 bytes SHA1)
    This would be fine if we were using blocking sockets, but non-blocking presents an interesting problem.  Example:

        A 100 byte input record is encoded to a 125 byte SSL record
        We can send 124 bytes without blocking, leaving one buffered byte
        We can't return 124 to the caller because it's more than they requested
        We can't return 100 to the caller because they would assume all data
        has been written, and we wouldn't get re-called to send the last byte

    We handle the above case by returning 0 to the caller if the entire encoded record could not be sent. Returning 
    0 will prompt us to select this socket for write events, and we'll be called again when the socket is writable.  
    We'll use this mechanism to flush the remaining encoded data, ignoring the bytes sent in, as they have already 
    been encoded.  When it is completely flushed, we return the originally requested length, and resume normal 
    processing.
 */
static ssize writeMss(MprSocket *sp, cvoid *buf, ssize len)
{
    MatrixSocket    *msp;
    uchar           *obuf;
    ssize           encoded, nbytes, written;

    msp = (MatrixSocket*) sp->sslSocket;

    while (len > 0 || msp->outlen > 0) {
        if ((encoded = matrixSslGetOutdata(msp->ctx, &obuf)) <= 0) {
            if (msp->outlen <= 0) {
                msp->outbuf = (char*) buf;
                msp->outlen = len;
                msp->written = 0;
                len = 0;
            }
            nbytes = min(msp->outlen, SSL_MAX_PLAINTEXT_LEN);
            if ((encoded = matrixSslEncodeToOutdata(msp->ctx, (uchar*) buf, (int) nbytes)) < 0) {
                return encoded;
            }
            msp->outbuf += nbytes;
            msp->outlen -= nbytes;
            msp->written += nbytes;
        }
        if ((written = sp->service->standardProvider->writeSocket(sp, obuf, encoded)) < 0) {
            return written;
        } else if (written == 0) {
            break;
        }
        matrixSslSentData(msp->ctx, (int) written);
    }
    /*
        Only signify all the data has been written if MatrixSSL has absorbed all the data
     */
    return msp->outlen == 0 ? msp->written : 0;
}


/*
    Blocking flush
 */
static ssize flushMss(MprSocket *sp)
{
    return blockingWrite(sp, 0, 0);
}


/*
    Cleanup for all-in-one distributions
 */
#undef SSL_RSA_WITH_3DES_EDE_CBC_SHA
#undef TLS_RSA_WITH_AES_128_CBC_SHA
#undef TLS_RSA_WITH_AES_256_CBC_SHA

#endif /* BIT_PACK_MATRIXSSL */

/*
    @copy   default

    Copyright (c) Embedthis Software LLC, 2003-2013. All Rights Reserved.

    This software is distributed under commercial and open source licenses.
    You may use the Embedthis Open Source license or you may acquire a 
    commercial license from Embedthis Software. You agree to be fully bound
    by the terms of either license. Consult the LICENSE.md distributed with
    this software for full details and other copyrights.

    Local variables:
    tab-width: 4
    c-basic-offset: 4
    End:
    vim: sw=4 ts=4 expandtab

    @end
 */
