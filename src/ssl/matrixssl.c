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
    Matrixssl defines int*, uint*, but does not provide HAS_XXX or any other means to disable. Ugh!
    So must include matrixsslApi.h first and then workaround. 
 */
#if WIN32
 #include   <winsock2.h>
 #include   <windows.h>
#endif
 #include    "matrixsslApi.h"

#define     HAS_INT16 1
#define     HAS_UINT16 1
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
    MprSocket       *sock;              /* Underlying socket */
    MatrixConfig    *cfg;               /* SSL config */
    ssl_t           *ctx;               /* MatrixSSL ssl_t structure */
    char            *outbuf;            /* Pending output data */
    char            *peerName;          /* Desired peer name */
    ssize           outlen;             /* Length of outbuf */
    ssize           written;            /* Number of unencoded bytes written */
    int             more;               /* MatrixSSL stack has buffered data */
    MprBuf          *peerCert;          /* Parsed peer certificate */
} MatrixSocket;

/***************************** Forward Declarations ***************************/

static void     closeMss(MprSocket *sp, bool gracefully);
static void     disconnectMss(MprSocket *sp);
static ssize    flushMss(MprSocket *sp);
static char     *getMssState(MprSocket *sp);
static int      handshakeMss(MprSocket *sp, short cipherSuite);
static ssize    innerRead(MprSocket *sp, char *userBuf, ssize len);
static Socket   listenMss(MprSocket *sp, cchar *host, int port, int flags);
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
    if (flags & MPR_MANAGE_MARK) {
        mprMark(msp->sock);
        mprMark(msp->peerName);
        mprMark(msp->peerCert);

    } else if (flags & MPR_MANAGE_FREE) {
        if (msp->ctx) {
            matrixSslDeleteSession(msp->ctx);
        }
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
            sp->service->standardProvider->writeSocket(sp, obuf, nbytes);
        }
    }
    sp->service->standardProvider->closeSocket(sp, gracefully);
    mprRemoveItem(sp->service->secureSockets, msp->sock);
    unlock(sp);
}


static Socket listenMss(MprSocket *sp, cchar *host, int port, int flags)
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
    int                 flags;

    ss = sp->service;
    assert(ss);
    assert(sp);

    if ((msp = (MatrixSocket*) mprAllocObj(MatrixSocket, manageMatrixSocket)) == 0) {
        return MPR_ERR_MEMORY;
    }
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

        if (matrixSslNewKeys(&cfg->keys) < 0) {
            mprError("MatrixSSL: Cannot create new MatrixSSL keys");
            unlock(sp);
            return MPR_ERR_CANT_INITIALIZE;
        }
        /*
            Read the certificate and the key file for this server.
         */
        password = NULL;
        if (matrixSslLoadRsaKeys(cfg->keys, ssl->certFile, ssl->keyFile, password, ssl->caFile) < 0) {
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
        flags = (ssl->verifyPeer) ? SSL_FLAGS_CLIENT_AUTH : 0;
        if (matrixSslNewServerSession(&msp->ctx, cfg->keys, NULL, flags) < 0) {
            mprError("MatrixSSL: Cannot create new server session");
            unlock(sp);
            return MPR_ERR_CANT_CREATE;
        }
        matrixSslSetCertValidator(msp->ctx, (sslCertCb_t)verifyCert);

    } else {
        msp->peerName = sclone(peerName);
        cipherSuite = 0;
        if (matrixSslNewClientSession(&msp->ctx, cfg->keys, NULL, cipherSuite, verifyCert, NULL, NULL, 0) < 0) {
            mprError("MatrixSSL: Cannot create new client session");
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


/*
    Store the name in printable form into buf; no more than (end - buf) characters will be written
 */
static void parseCert(MprBuf *buf, char *prefix, psX509Cert_t *cert)
{
    mprPutToBuf(buf, "%s_S_CN=%s,", prefix, cert->subject.commonName);
    mprPutToBuf(buf, "%s_S_C=%s,", prefix, cert->subject.country);
    mprPutToBuf(buf, "%s_S_L=%s,", prefix, cert->subject.locality);
    mprPutToBuf(buf, "%s_S_ST=%s,", prefix, cert->subject.state);
    mprPutToBuf(buf, "%s_S_O=%s,", prefix, cert->subject.organization);
    mprPutToBuf(buf, "%s_S_OU=%s,", prefix, cert->subject.orgUnit);

    mprPutToBuf(buf, "%s_I_CN=%s,", prefix, cert->issuer.commonName);
    mprPutToBuf(buf, "%s_I_C=%s,", prefix, cert->issuer.country);
    mprPutToBuf(buf, "%s_I_L=%s,", prefix, cert->issuer.locality);
    mprPutToBuf(buf, "%s_I_ST=%s,", prefix, cert->issuer.state);
    mprPutToBuf(buf, "%s_I_O=%s,", prefix, cert->issuer.organization);
    mprPutToBuf(buf, "%s_I_OU=%s,", prefix, cert->issuer.orgUnit);
}


/*
    Get the SSL state with key=values for both peer and own certs
 */
static char *getMssState(MprSocket *sp)
{
    MatrixSocket    *msp;
    MprBuf          *buf;
    ssl_t           *ctx;
    cchar           *cipherName;

    msp = sp->sslSocket;
    ctx = msp->ctx;
    cipherName = mprGetSslCipherName(msp->ctx->cipher->ident);
    buf = mprCreateBuf(0, 0);
    mprPutToBuf(buf, "PROVIDER=matrixssl,CIPHER=%s,", cipherName ? cipherName: "unknown");
    mprPutBlockToBuf(buf, mprGetBufStart(msp->peerCert), mprGetBufLength(msp->peerCert));
    parseCert(buf, sp->acceptIp ? "CLIENT" : "SERVER", ctx->keys->cert);
    mprLog(5, "MatrixSSL certs: %s", mprGetBufStart(buf));
    return mprGetBufStart(buf);
}


/*
    Validate certificates
 */
static int verifyCert(ssl_t *ssl, psX509Cert_t *cert, int32 alert)
{
    MprSocketService    *ss;
    MprSocket           *sp;
    MatrixSocket        *msp;
    struct tm           t;
    cchar               *cipherName;
    char                *c;
    int                 next, y, m, d, cipherCode;

    ss = MPR->socketService;

    /*
        Find our handle. This is really ugly because the matrix api does not provide a handle.
     */
    lock(ss);
    sp = 0;
    for (ITERATE_ITEMS(ss->secureSockets, sp, next)) {
        if (sp->ssl && ((MatrixSocket*) sp->sslSocket)->ctx == ssl) {
            break;
        }
    }
    unlock(ss);
    msp = sp->sslSocket;
    cipherCode = msp->ctx->cipher->ident;
    cipherName = mprGetSslCipherName(cipherCode);
    if (cipherName) {
        mprLog(3, "MatrixSSL connected using cipher: %s, %x", cipherName, cipherCode);
    } else {
        mprLog(3, "MatrixSSL connected using cipher: %x", cipherCode);
    }
    if (!sp) {
        /* Should not get here */
        assert(sp);
        return SSL_ALLOW_ANON_CONNECTION;
    }
    if (!sp->ssl->verifyPeer) {
        return SSL_ALLOW_ANON_CONNECTION;
    }
    if (alert > 0) {
        if (alert == SSL_ALERT_CERTIFICATE_REVOKED) {
            sp->errorMsg = sclone("Certificate revoked");
        } else if (alert == SSL_ALERT_UNKNOWN_CA) {
            if (memcmp(cert->issuer.hash, cert->subject.hash, SHA1_HASH_SIZE) == 0) {
                sp->errorMsg = sclone("Self-signed certificate");
            } else {
                sp->errorMsg = sclone("Certificate not trusted");
            }
            if (!sp->ssl->verifyIssuer) {
                //  ssl->sec.anon = 1;
                return SSL_ALLOW_ANON_CONNECTION;
            }
        } else {
            sp->errorMsg = sfmt("Cannot handshake: error %d", alert);
        }
        return alert;
    }
    if (msp->peerName && !smatch(msp->peerName, cert->subject.commonName)) {
        sp->errorMsg = sclone("Certificate Common name mismatch");
        return PS_FAILURE;
    }
	/* 
        Validate the 'not before' date 
     */
    mprDecodeUniversalTime(&t, mprGetTime());
	if ((c = cert->notBefore) != NULL) {
		if (strlen(c) < 8) {
            sp->errorMsg = sclone("Corrupt certificate");
			return PS_FAILURE;
		}
		/* 
            UTCTIME, defined in 1982, has just a 2 digit year 
         */
		if (cert->notBeforeTimeType == ASN_UTCTIME) {
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
            sp->errorMsg = sclone("Corrupt certificate");
            return PS_FAILURE; 
        }
		if (t.tm_year == y) {
			if (t.tm_mon < m || (t.tm_mon == m && t.tm_mday < d)) {
                sp->errorMsg = sclone("Certificate not yet active");
                return PS_FAILURE;
            }
		}
	}

	/* 
        Validate the 'not after' date 
     */
	if ((c = cert->notAfter) != NULL) {
		if (strlen(c) < 8) {
            sp->errorMsg = sclone("Corrupt certificate");
			return PS_FAILURE;
		}
		if (cert->notAfterTimeType == ASN_UTCTIME) {
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
            sp->errorMsg = sclone("Corrupt certificate");
            return PS_FAILURE; 
        } else if (t.tm_year == y) {
			if (t.tm_mon > m || (t.tm_mon == m && t.tm_mday > d)) {
                sp->errorMsg = sclone("Certificate expired");
                return PS_FAILURE;
            }
		}
	}
    /*
        Must parse here as MatrixSSL frees this if you have both client and server enabled in the library
     */
    msp->peerCert = mprCreateBuf(0, 0);
    parseCert(msp->peerCert, sp->acceptIp ? "SERVER" : "CLIENT", cert);
    mprLog(3, "MatrixSSL: Certificate verified");
    mprLog(4, "MatrixSSL: %s", mprGetBufStart(msp->peerCert));
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


static int32 handshakeIsComplete(ssl_t *ssl)
{	
	return (ssl->hsState == SSL_HS_DONE) ? PS_TRUE : PS_FALSE;
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
            if (handshakeIsComplete(msp->ctx)) {
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
            if (msp->ctx->err != SSL_ALERT_NONE && msp->ctx->err != SSL_ALLOW_ANON_CONNECTION) {
                return -1;
            }
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
        if ((nbytes = standard->readSocket(sp, mbuf, msize)) < 0) {
            return nbytes;
        } else if (nbytes > 0) {
            nbytes = processMssData(sp, buf, size, nbytes, &readMore);
            if (nbytes < 0 || nbytes > 0) {
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
        mprHiddenSocketData(sp, msp->more, MPR_READABLE);
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
