/*
    nanossl.c - Mocana NanoSSL

    Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#define ASYNC 0

#include    "mpr.h"

#if BIT_PACK_NANOSSL
 #include "common/moptions.h"
 #include "common/mdefs.h"
 #include "common/mtypes.h"
 #include "common/merrors.h"
 #include "common/mrtos.h"
 #include "common/mtcp.h"
 #include "common/mocana.h"
 #include "common/random.h"
 #include "common/vlong.h"
 #include "crypto/hw_accel.h"
 #include "crypto/crypto.h"
 #include "crypto/pubcrypto.h"
 #include "crypto/ca_mgmt.h"
 #include "ssl/ssl.h"
 #include "asn1/oiddefs.h"

/************************************* Defines ********************************/
/*
    Per-route SSL configuration
 */
typedef struct NanoConfig {
    certDescriptor  cert;
    certDescriptor  ca;
} NanoConfig;

/*
    Per socket state
 */
typedef struct NanoSocket {
    MprSocket       *sock;
    NanoConfig       *cfg;
    sbyte4          handle;
    int             connected;
} NanoSocket;

static MprSocketProvider *nanoProvider;
static NanoConfig *defaultNanoConfig;

#if BIT_DEBUG
    #define SSL_HELLO_TIMEOUT   15000
    #define SSL_RECV_TIMEOUT    300000
#else
    #define SSL_HELLO_TIMEOUT   15000000
    #define SSL_RECV_TIMEOUT    30000000
#endif

#define KEY_SIZE                1024
#define MAX_CIPHERS             16

/***************************** Forward Declarations ***************************/

static void     nanoClose(MprSocket *sp, bool gracefully);
static void     nanoDisconnect(MprSocket *sp);
static char     *getNanoState(MprSocket *sp);
static Socket   nanoListen(MprSocket *sp, cchar *host, int port, int flags);
static void     nanoLog(sbyte4 module, sbyte4 severity, sbyte *msg);
static void     manageNanoConfig(NanoConfig *cfg, int flags);
static void     manageNanoProvider(MprSocketProvider *provider, int flags);
static void     manageNanoSocket(NanoSocket *ssp, int flags);
static ssize    nanoRead(MprSocket *sp, void *buf, ssize len);
static int      setNanoCiphers(MprSocket *sp, cchar *cipherSuite);
static int      nanoUpgrade(MprSocket *sp, MprSsl *sslConfig, cchar *peerName);
static ssize    nanoWrite(MprSocket *sp, cvoid *buf, ssize len);

static void     DEBUG_PRINT(void *where, void *msg);
static void     DEBUG_PRINTNL(void *where, void *msg);

/************************************* Code ***********************************/
/*
    Create the Openssl module. This is called only once
 */
PUBLIC int mprCreateNanoSslModule()
{
    sslSettings     *settings;

    if ((nanoProvider = mprAllocObj(MprSocketProvider, manageNanoProvider)) == NULL) {
        return MPR_ERR_MEMORY;
    }
    nanoProvider->upgradeSocket = nanoUpgrade;
    nanoProvider->closeSocket = nanoClose;
    nanoProvider->disconnectSocket = nanoDisconnect;
    nanoProvider->listenSocket = nanoListen;
    nanoProvider->readSocket = nanoRead;
    nanoProvider->writeSocket = nanoWrite;
    nanoProvider->socketState = getNanoState;
    mprAddSocketProvider("nanossl", nanoProvider);

    if ((defaultNanoConfig = mprAllocObj(NanoConfig, manageNanoConfig)) == 0) {
        return MPR_ERR_MEMORY;
    }
    if (MOCANA_initMocana() < 0) {
        mprError("NanoSSL initialization failed");
        return MPR_ERR_CANT_INITIALIZE;
    }
    MOCANA_initLog(nanoLog);

#if ASYNC
    #define MAX_SSL_CONNECTIONS_ALLOWED 10
    if (SSL_ASYNC_init(MAX_SSL_CONNECTIONS_ALLOWED, MAX_SSL_CONNECTIONS_ALLOWED) < 0) {
        mprError("SSL_ASYNC_init failed");
        return MPR_ERR_CANT_INITIALIZE;
    }
#else
    if (SSL_init(SOMAXCONN, 0) < 0) {
        mprError("SSL_init failed");
        return MPR_ERR_CANT_INITIALIZE;
    }
#endif
    settings = SSL_sslSettings();
    settings->sslTimeOutHello = SSL_HELLO_TIMEOUT;
    settings->sslTimeOutReceive = SSL_RECV_TIMEOUT;
    return 0;
}


static void manageNanoProvider(MprSocketProvider *provider, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(defaultNanoConfig);

    } else if (flags & MPR_MANAGE_FREE) {
        defaultNanoConfig = 0;
        SSL_releaseTables();
        MOCANA_freeMocana();
    }
}


static void manageNanoConfig(NanoConfig *cfg, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        ;
    } else if (flags & MPR_MANAGE_FREE) {
        CA_MGMT_freeCertificate(&cfg->cert);
    }
}


static void manageNanoSocket(NanoSocket *np, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(np->cfg);
        mprMark(np->sock);

    } else if (flags & MPR_MANAGE_FREE) {
        if (np->handle) {
#if ASYNC
            SSL_ASYNC_closeConnection(np->handle);
#else
            SSL_closeConnection(np->handle);
#endif
            np->handle = 0;
        }
    }
}


static void nanoClose(MprSocket *sp, bool gracefully)
{
    NanoSocket      *np;

    np = sp->sslSocket;
    if (np->handle) {
#if ASYNC
        SSL_ASYNC_closeConnection(np->handle);
#else
        SSL_closeConnection(np->handle);
#endif
        np->handle = 0;
    }
    sp->service->standardProvider->closeSocket(sp, gracefully);
}


/*
    Initialize a new server-side connection
 */
static Socket nanoListen(MprSocket *sp, cchar *host, int port, int flags)
{
    assert(sp);
    assert(port);
    return sp->service->standardProvider->listenSocket(sp, host, port, flags);
}


/*
    Upgrade a standard socket to use TLS
 */
static int nanoUpgrade(MprSocket *sp, MprSsl *ssl, cchar *peerName)
{
    NanoSocket   *np;
    NanoConfig   *cfg;
    int         rc;

    assert(sp);

    if (ssl == 0) {
        ssl = mprCreateSsl(sp->flags & MPR_SOCKET_SERVER);
    }
    if ((np = (NanoSocket*) mprAllocObj(NanoSocket, manageNanoSocket)) == 0) {
        return MPR_ERR_MEMORY;
    }
    np->sock = sp;
    sp->sslSocket = np;
    sp->ssl = ssl;

    lock(ssl);
    if (ssl->config && !ssl->changed) {
        np->cfg = cfg = ssl->config;
    } else {
        ssl->changed = 0;
        /*
            One time setup for the SSL configuration for this MprSsl
         */
        if ((cfg = mprAllocObj(NanoConfig, manageNanoConfig)) == 0) {
            unlock(ssl);
            return MPR_ERR_MEMORY;
        }
        np->cfg = ssl->config = cfg;
        if (ssl->certFile) {
            certDescriptor tmp;
            if ((rc = MOCANA_readFile((sbyte*) ssl->certFile, &tmp.pCertificate, &tmp.certLength)) < 0) {
                mprError("NanoSSL: Unable to read certificate %s", ssl->certFile); 
                CA_MGMT_freeCertificate(&tmp);
                unlock(ssl);
                return MPR_ERR_CANT_READ;
            }
            assert(__ENABLE_MOCANA_PEM_CONVERSION__);
            if ((rc = CA_MGMT_decodeCertificate(tmp.pCertificate, tmp.certLength, &cfg->cert.pCertificate, 
                    &cfg->cert.certLength)) < 0) {
                mprError("NanoSSL: Unable to decode PEM certificate %s", ssl->certFile); 
                CA_MGMT_freeCertificate(&tmp);
                unlock(ssl);
                return MPR_ERR_CANT_READ;
            }
            MOCANA_freeReadFile(&tmp.pCertificate);
        }
        if (ssl->keyFile) {
            certDescriptor tmp;
            if ((rc = MOCANA_readFile((sbyte*) ssl->keyFile, &tmp.pKeyBlob, &tmp.keyBlobLength)) < 0) {
                mprError("NanoSSL: Unable to read key file %s", ssl->keyFile); 
                CA_MGMT_freeCertificate(&cfg->cert);
                unlock(ssl);
            }
            if ((rc = CA_MGMT_convertKeyPEM(tmp.pKeyBlob, tmp.keyBlobLength, 
                    &cfg->cert.pKeyBlob, &cfg->cert.keyBlobLength)) < 0) {
                mprError("NanoSSL: Unable to decode PEM key file %s", ssl->keyFile); 
                CA_MGMT_freeCertificate(&tmp);
                unlock(ssl);
                return MPR_ERR_CANT_READ;
            }
            MOCANA_freeReadFile(&tmp.pKeyBlob);    
        }
        if (ssl->caFile) {
            certDescriptor tmp;
            if ((rc = MOCANA_readFile((sbyte*) ssl->caFile, &tmp.pCertificate, &tmp.certLength)) < 0) {
                mprError("NanoSSL: Unable to read CA certificate file %s", ssl->caFile); 
                CA_MGMT_freeCertificate(&tmp);
                unlock(ssl);
                return MPR_ERR_CANT_READ;
            }
            if ((rc = CA_MGMT_decodeCertificate(tmp.pCertificate, tmp.certLength, &cfg->ca.pCertificate, 
                    &cfg->ca.certLength)) < 0) {
                mprError("NanoSSL: Unable to decode PEM certificate %s", ssl->caFile); 
                CA_MGMT_freeCertificate(&tmp);
                unlock(ssl);
                return MPR_ERR_CANT_READ;
            }
            MOCANA_freeReadFile(&tmp.pCertificate);
        }

#if __ENABLE_MOCANA_SSL_CLIENT__ && 0
        if (peerName) {
            /* 0 + 2-byte length prefix + UTF8-encoded string (not NULL terminated) */
            char list[128];
            memset(list, 0, sizeof(list));
            list[2] = slen(peerName);
            scopy(&list[3], sizeof(list) - 4, peerName);
            SSL_setServerNameList(np->handle, list, slen(peerName) + 3);
        }
#endif
#if ASYNC && UNUSED
        if (SSL_ASYNC_initServerCert(&cfg->cert, FALSE, 0)) {
            mprError("SSL_initServerCert failed");
            unlock(ssl);
            return MPR_ERR_CANT_INITIALIZE;
        }
#else
        if (SSL_initServerCert(&cfg->cert, FALSE, 0)) {
            mprError("SSL_initServerCert failed");
            unlock(ssl);
            return MPR_ERR_CANT_INITIALIZE;
        }
#endif
    }
    unlock(ssl);

    if (sp->flags & MPR_SOCKET_SERVER) {
#if ASYNC
        //  SSL_ASYNC_acceptConnection - mob does this start handshaking?
        //  MOB - this must be serialized - read all doc for other such warnings
        if ((np->handle = SSL_ASYNC_acceptConnection(sp->fd)) < 0) {
            return -1;
        }
#else
        if ((np->handle = SSL_acceptConnection(sp->fd)) < 0) {
            return -1;
        }
#endif
    } else {
        //  MOB - need client side
        //  MOB (client only) - SSL_ASYNC_start(np->handle);
    }
    return 0;
}


static void nanoDisconnect(MprSocket *sp)
{
    sp->service->standardProvider->disconnectSocket(sp);
}


static void checkCert(MprSocket *sp)
{
    NanoSocket  *np;
    MprCipher   *cp;
    MprSsl      *ssl;
    ubyte2      cipher;
    ubyte4      ecurve;
   
    ssl = sp->ssl;
    np = (NanoSocket*) sp->sslSocket;

    if (ssl->caFile) {
        mprLog(4, "NanoSSL: Using certificates from %s", ssl->caFile);
    } else if (ssl->caPath) {
        mprLog(4, "NanoSSL: Using certificates from directory %s", ssl->caPath);
    }
#if MOB
    if (ssl->peer_cert) {
        mprLog(4, "NanoSSL: client supplied no certificate");
    } else {
        mprRawLog(4, "%s", x509parse_cert_inf("", ssl->peer_cert));
    }
#endif
    /*
        Trace cipher being used
     */
    if (SSL_getCipherInfo(np->handle, &cipher, &ecurve) < 0) {
        mprLog(0, "Cannot get cipher info");
        return;
    }
    for (cp = mprCiphers; cp->name; cp++) {
        if (cp->code == cipher) {
            break;
        }
    }
    if (cp) {
        mprLog(0, "NanoSSL connected using cipher: %s, %x", cp->name, (int) cipher);
    } else {
        mprLog(0, "NanoSSL connected using cipher: %x", (int) cipher);
    }
}


#if FUTURE
static sbyte4 findCertificateInStore(sbyte4 connectionInstance, certDistinguishedName *pLookupCertDN, certDescriptor* pReturnCert)
{
    certDistinguishedName   issuer;
    sbyte4                  status;

    status = CA_MGMT_extractCertDistinguishedName(pCertificate, certificateLength, 0, &issuer);

    if (0 > status) {
        return status;
    }
    /* for this example implementation, we only recognize one certificate authority */

    pReturnCert->pCertificate = fedoraRootCert;                                                        
    pReturnCert->certLength = sizeof(fedoraRootCert);                                                  
    pReturnCert->cookie         = 0;                                                                       
    return 0;                                                                                              
} 

static sbyte4 releaseStoreCertificate(sbyte4 connectionInstance, certDescriptor* pFreeCert)
{                                                                                                          
    /* just need to release the certificate, not the key blob */                                           
    if (0 != pFreeCert->pCertificate) {
        if (0 != pFreeCert->cookie) {
            free(pFreeCert->pCertificate);
        }
        pFreeCert->pCertificate = 0;
    }
    return 0;
}


static sbyte4 verifyCertificateInStore(sbyte4 connectionInstance, ubyte *pCertificate, ubyte4 certificateLength, sbyte4 isSelfSigned)
{
    return 0;
}
#endif /* FUTURE */


/*
    Initiate or continue SSL handshaking with the peer. This routine does not block.
    Return -1 on errors, 0 incomplete and awaiting I/O, 1 if successful
*/
static int nanoHandshake(MprSocket *sp)
{
    NanoSocket  *np;
    ubyte4      flags;
    int         rc;

    np = (NanoSocket*) sp->sslSocket;
    sp->flags |= MPR_SOCKET_HANDSHAKING;
    if (setNanoCiphers(sp, sp->ssl->ciphers) < 0) {
        return 0;
    }
#if NEXT
    SSL_sslSettings()->funcPtrCertificateStoreVerify  = verifyCertificateInStore;
    SSL_sslSettings()->funcPtrCertificateStoreLookup  = findCertificateInStore;
    SSL_sslSettings()->funcPtrCertificateStoreRelease = releaseStoreCertificate;
#endif
    SSL_getSessionFlags(np->handle, &flags);
    if (sp->ssl->verifyPeer) {
        flags |= SSL_FLAG_REQUIRE_MUTUAL_AUTH;
    } else {
        flags |= SSL_FLAG_NO_MUTUAL_AUTH_REQUEST;
    }
    SSL_setSessionFlags(np->handle, flags);
    rc = 0;

    while (!np->connected) {
        //  MOB - is this sync or async
        //  MOB - doc says only do this in sync
        if ((rc = SSL_negotiateConnection(np->handle)) < 0) {
            break;
#if FUTURE
            if (!mprGetSocketBlockingMode(sp)) {
                DISPLAY_ERROR(0, rc); 
                mprLog(4, "NanoSSL: Cannot handshake: error %d", rc);
                errno = EPROTO;
                return 0;
            }
            continue;
#endif
        }
        np->connected = 1;
        break;
    }
    sp->flags &= ~MPR_SOCKET_HANDSHAKING;

    /*
        Analyze the handshake result
    */
    if (rc < 0) {
        if (rc == ERR_SSL_UNKNOWN_CERTIFICATE_AUTHORITY) {
            sp->errorMsg = sclone("Unknown certificate authority");

        /* Common name mismatch, cert revoked */
        } else if (rc == ERR_SSL_PROTOCOL_PROCESS_CERTIFICATE) {
            sp->errorMsg = sclone("Bad certificate");
        } else if (rc == ERR_SSL_NO_SELF_SIGNED_CERTIFICATES) {
            sp->errorMsg = sclone("Self-signed certificate");
        } else if (rc == ERR_SSL_CERT_VALIDATION_FAILED) {
            sp->errorMsg = sclone("Certificate does not validate");
        }

        /*
            MOB - conditions to review
            - certificate expired
            - certificate revoked
            - self-signed cert
            - cert not trusted
            - common name mismatch
            - peer disconnected
            - verify peer / issuer
         */
        DISPLAY_ERROR(0, rc); 
        mprLog(4, "NanoSSL: Cannot handshake: error %d", rc);
        errno = EPROTO;
        return -1;
    }
    checkCert(sp);
    return 1;
}


/*
    Return the number of bytes read. Return -1 on errors and EOF. Distinguish EOF via mprIsSocketEof
 */
static ssize nanoRead(MprSocket *sp, void *buf, ssize len)
{
    NanoSocket  *np;
    sbyte4      nbytes, count;
    int         rc;

    np = (NanoSocket*) sp->sslSocket;
    assert(mp);
    assert(np->cfg);

    if (!np->connected && (rc = nanoHandshake(sp)) <= 0) {
        return rc;
    }
    while (1) {
#if ASYNC
        ubyte *remainingBuf;
        ubyte4 remainingCount;
        rc = SSL_ASYNC_recvMessage(np->handle, buf, nbytes);
        mprLog(5, "NanoSSL: ssl_read %d", rc);
        if (rc < 0) {
            /*
                MOB - close notify, conn reset
             */
            //  MOB - DISPLAY_ERROR for a text message
            return -1;
        } 
#else
        rc = SSL_recv(np->handle, buf, (sbyte4) len, &nbytes, 0);
        mprLog(5, "NanoSSL: ssl_read %d", rc);
        if (rc < 0) {
            return -1;
        }
        break;
#endif
    }
#if !ASYNC
    SSL_recvPending(np->handle, &count);
    mprHiddenSocketData(sp, count, MPR_READABLE);
#endif
    return nbytes;
}


/*
    Write data. Return the number of bytes written or -1 on errors.
 */
static ssize nanoWrite(MprSocket *sp, cvoid *buf, ssize len)
{
    NanoSocket  *np;
    ssize       totalWritten;
    int         rc, count, sent;

    np = (NanoSocket*) sp->sslSocket;
    if (len <= 0) {
        assert(0);
        return -1;
    }
    if (!np->connected && (rc = nanoHandshake(sp)) <= 0) {
        return rc;
    }
    totalWritten = 0;
    do {
#if ASYNC
        rc = SSL_ASYNC_sendMessage(np->handle, (sbyte*) buf, (int) len, &sent);
        mprLog(7, "NanoSSL: written %d, requested len %d", sent, len);
        if (rc <= 0) {
            //  MOB - should this set EOF. What about other providers?
            mprLog(0, "NanoSSL: SSL_send failed sent %d", rc);
            return -1;
        }
        totalWritten += sent;
        buf = (void*) ((char*) buf + sent);
        len -= sent;
        mprLog(7, "NanoSSL: write: len %d, written %d, total %d", len, sent, totalWritten);
#else
        rc = sent = SSL_send(np->handle, (sbyte*) buf, (int) len);
        mprLog(7, "NanoSSL: written %d, requested len %d", sent, len);
        if (rc <= 0) {
            //  MOB - should this set EOF. What about other providers?
            mprLog(0, "NanoSSL: SSL_send failed sent %d", rc);
            return -1;
        }
        totalWritten += sent;
        buf = (void*) ((char*) buf + sent);
        len -= sent;
        mprLog(7, "NanoSSL: write: len %d, written %d, total %d", len, sent, totalWritten);
#endif
    } while (len > 0);

#if !ASYNC
    SSL_sendPending(np->handle, &count);
    mprHiddenSocketData(sp, count, MPR_WRITABLE);
#endif
    return totalWritten;
}


static char *getNanoState(MprSocket *sp)
{
#if MOB
    sslGetCipherInfo


    NanoSocket       *moc;
    ssl_context     *ctx;
    MprBuf          *buf;
    char            *own, *peer;
    char            cbuf[5120];         //  MOB - must not be a static buffer

    if ((moc = sp->sslSocket) == 0) {
        return 0;
    }
    ctx = &est->ctx;
    buf = mprCreateBuf(0, 0);
    mprPutToBuf(buf, "PROVIDER=est,CIPHER=%s,", ssl_get_cipher(ctx));

    own =  sp->acceptIp ? "SERVER_" : "CLIENT_";
    peer = sp->acceptIp ? "CLIENT_" : "SERVER_";
    if (ctx->peer_cert) {
        x509parse_cert_info(peer, cbuf, sizeof(cbuf), ctx->peer_cert);
        mprPutStringToBuf(buf, cbuf);
    }
    if (ctx->own_cert) {
        x509parse_cert_info(own, cbuf, sizeof(cbuf), ctx->own_cert);
        mprPutStringToBuf(buf, cbuf);
    }
    mprTrace(5, "EST state: %s", mprGetBufStart(buf));
    return mprGetBufStart(buf);
#endif
    return "";
}


static int setNanoCiphers(MprSocket *sp, cchar *cipherSuite)
{
    NanoSocket  *np;
    char        *suite, *cipher, *next;
    ubyte2      *ciphers;
    int         count, cipherCode;


    np = sp->sslSocket;
    ciphers = malloc((MAX_CIPHERS + 1) * sizeof(ubyte2));

    if (!cipherSuite || cipherSuite[0] == '\0') {
        return 0;
    }
    suite = strdup(cipherSuite);
    count = 0;
    while ((cipher = stok(suite, ":, \t", &next)) != 0 && count < MAX_CIPHERS) {
        if ((cipherCode = mprGetSslCipherCode(cipher)) < 0) {
            mprError("Requested cipher %s is not supported by this provider", cipher);
        } else {
            ciphers[count++] = cipherCode;
        }
        suite = 0;
    }
    if (SSL_enableCiphers(np->handle, ciphers, count) < 0) {
        mprError("Requested cipher suite %s is not supported by this provider", cipherSuite);
        return MPR_ERR_BAD_STATE;
    }
    return 0;
}


static void DEBUG_PRINT(void *where, void *msg)
{
    mprRawLog(0, "%s", msg);
}

static void DEBUG_PRINTNL(void *where, void *msg)
{
    mprLog(0, "%s", msg);
}

static void nanoLog(sbyte4 module, sbyte4 severity, sbyte *msg)
{
    mprLog(0, "%s", (cchar*) msg);
}

#endif /* BIT_PACK_NANOSSL */

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
