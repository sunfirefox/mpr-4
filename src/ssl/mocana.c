/*
    mocana.c - Mocana NanoSSL

    Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "mpr.h"

#if BIT_PACK_MOCANA
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
typedef struct MocConfig {
    certDescriptor  cert;
#if UNUSED
    char            *dhKey;
    rsa_context     rsa;
#endif
} MocConfig;

/*
    Per socket state
 */
typedef struct MocSocket {
    MprSocket       *sock;
    MocConfig       *cfg;
    sbyte4          handle;
#if UNUSED
    int             *ciphers;
    havege_state    hs;
    ssl_context     ssl;
    ssl_session     session;
#endif
} MocSocket;

static MprSocketProvider *mocProvider;
static MocConfig *defaultMocConfig;

//  MOB Cleanup
#define SSL_HELLO_TIMEOUT     15000
#define SSL_RECV_TIMEOUT      300000
#define KEY_SIZE            1024

#if TEST_CERT
nameAttr pNames1[] = {
    {countryName_OID, 0, (ubyte*)"US", 2}                                /* country */
};
nameAttr pNames2[] = {
    {stateOrProvinceName_OID, 0, (ubyte*)"California", 10}               /* state or providence */
};
nameAttr pNames3[] = {
    {localityName_OID, 0, (ubyte*)"Menlo Park", 10}                      /* locality */
};
nameAttr pNames4[] = {
    {organizationName_OID, 0, (ubyte*)"Mocana Corporation", 18}          /* company name */
};
nameAttr pNames5[] = {
    {organizationalUnitName_OID, 0, (ubyte*)"Engineering", 11}           /* organizational unit */
};
nameAttr pNames6[] = {
    {commonName_OID, 0, (ubyte*)"sslexample.mocana.com", 21}             /* common name */
};
nameAttr pNames7[] = {
    {pkcs9_emailAddress_OID, 0, (ubyte*)"info@mocana.com", 15}           /* pkcs-9-at-emailAddress */
};
relativeDN pRDNs[] = {
    {pNames1, 1},
    {pNames2, 1},
    {pNames3, 1},
    {pNames4, 1},
    {pNames5, 1},
    {pNames6, 1},
    {pNames7, 1}
};
certDistinguishedName testCert = {
    pRDNs,
    7,
    (sbyte*) "030526000126Z",    /* certificate start date (time format yymmddhhmmss) */
    (sbyte*) "330524230126Z"     /* certificate end date */
};
#endif
/***************************** Forward Declarations ***************************/

static void     closeMoc(MprSocket *sp, bool gracefully);
static void     disconnectMoc(MprSocket *sp);
static char     *getMocState(MprSocket *sp);
static int      listenMoc(MprSocket *sp, cchar *host, int port, int flags);
static void     manageMocConfig(MocConfig *cfg, int flags);
static void     manageMocProvider(MprSocketProvider *provider, int flags);
static void     manageMocSocket(MocSocket *ssp, int flags);
static ssize    readMoc(MprSocket *sp, void *buf, ssize len);
static int      upgradeMoc(MprSocket *sp, MprSsl *sslConfig, cchar *peerName);
static ssize    writeMoc(MprSocket *sp, cvoid *buf, ssize len);

/************************************* Code ***********************************/
/*
    Create the Openssl module. This is called only once
 */
PUBLIC int mprCreateMocanaModule()
{
    sslSettings     *settings;

    if ((mocProvider = mprAllocObj(MprSocketProvider, manageMocProvider)) == NULL) {
        return MPR_ERR_MEMORY;
    }
    mocProvider->upgradeSocket = upgradeMoc;
    mocProvider->closeSocket = closeMoc;
    mocProvider->disconnectSocket = disconnectMoc;
    mocProvider->listenSocket = listenMoc;
    mocProvider->readSocket = readMoc;
    mocProvider->writeSocket = writeMoc;
    mocProvider->socketState = getMocState;
    mprAddSocketProvider("mocana", mocProvider);

    if ((defaultMocConfig = mprAllocObj(MocConfig, manageMocConfig)) == 0) {
        return MPR_ERR_MEMORY;
    }
#if FUTURE
    MOCANA_initLog
#endif
    if (MOCANA_initMocana() < 0) {
        mprError("MOCANA_initMocana failed");
        return MPR_ERR_CANT_INITIALIZE;
    }
    if (SSL_init(SOMAXCONN, 0) < 0) {
        mprError("SSL_init failed");
        return MPR_ERR_CANT_INITIALIZE;
    }
    settings = SSL_sslSettings();
    settings->sslTimeOutHello = SSL_HELLO_TIMEOUT;
    settings->sslTimeOutReceive = SSL_RECV_TIMEOUT;

#if UNUSED
    //  MOB - should generate
    defaultMocConfig->dhKey = dhKey;
#endif
    return 0;
}


static void manageMocProvider(MprSocketProvider *provider, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(defaultMocConfig);

    } else if (flags & MPR_MANAGE_FREE) {
        defaultMocConfig = 0;
        SSL_releaseTables();
        MOCANA_freeMocana();
    }
}


static void manageMocConfig(MocConfig *cfg, int flags)
{
    if (flags & MPR_MANAGE_FREE) {
        //  MOB - to where read was done
        MOCANA_freeReadFile(&cfg->cert.pCertificate);
        MOCANA_freeReadFile(&cfg->cert.pKeyBlob);    
    }
}


/*
    Create an SSL configuration for a route. An application can have multiple different SSL configurations for 
    different routes. There is default SSL configuration that is used when a route does not define a configuration 
    and also for clients.
 */
/*
    Destructor for an MocSocket object
 */
static void manageMocSocket(MocSocket *mp, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(mp->cfg);
        mprMark(mp->sock);
#if UNUSED
        mprMark(mp->ciphers);
#endif

    } else if (flags & MPR_MANAGE_FREE) {
        if (mp->handle) {
            SSL_closeConnection(mp->handle);
        }
    }
}


static void closeMoc(MprSocket *sp, bool gracefully)
{
    MocSocket       *mp;

    mp = sp->sslSocket;
    if (!(sp->flags & MPR_SOCKET_EOF)) {
        if (mp->handle) {
            SSL_closeConnection(mp->handle);
        }
        mp->handle = 0;
    }
    sp->service->standardProvider->closeSocket(sp, gracefully);
}


/*
    Initialize a new server-side connection
 */
static int listenMoc(MprSocket *sp, cchar *host, int port, int flags)
{
    assert(sp);
    assert(port);
    return sp->service->standardProvider->listenSocket(sp, host, port, flags);
}


/*
    Upgrade a standard socket to use TLS
 */
static int upgradeMoc(MprSocket *sp, MprSsl *ssl, cchar *peerName)
{
    MocSocket   *mp;
    MocConfig   *cfg;
    int         rc, handle;

    assert(sp);

    if (ssl == 0) {
        ssl = mprCreateSsl(sp->flags & MPR_SOCKET_SERVER);
    }
    if ((mp = (MocSocket*) mprAllocObj(MocSocket, manageMocSocket)) == 0) {
        return MPR_ERR_MEMORY;
    }
    mp->sock = sp;
    sp->sslSocket = mp;
    sp->ssl = ssl;
    // verifyMode = (sp->flags & MPR_SOCKET_SERVER && !ssl->verifyPeer) ? SSL_VERIFY_NO_CHECK : SSL_VERIFY_OPTIONAL;

    lock(ssl);
    if (ssl->config && !ssl->changed) {
        mp->cfg = cfg = ssl->config;
    } else {
        ssl->changed = 0;
        /*
            One time setup for the SSL configuration for this MprSsl
         */
        if ((cfg = mprAllocObj(MocConfig, manageMocConfig)) == 0) {
            unlock(ssl);
            return MPR_ERR_MEMORY;
        }
        mp->cfg = ssl->config = cfg;
        if (ssl->certFile) {
            certDescriptor tmp;
            if ((rc = MOCANA_readFile((sbyte*) ssl->certFile, &tmp.pCertificate, &tmp.certLength)) < 0) {
                mprError("MOCANA: Unable to read certificate %s", ssl->certFile); 
                CA_MGMT_freeCertificate(&tmp);
                unlock(ssl);
                return MPR_ERR_CANT_READ;
            }
            assert(__ENABLE_MOCANA_PEM_CONVERSION__);
            if ((rc = CA_MGMT_decodeCertificate(tmp.pCertificate, tmp.certLength, 
                    &cfg->cert.pCertificate, &cfg->cert.certLength)) < 0) {
                mprError("MOCANA: Unable to decode PEM certificate %s", ssl->certFile); 
                CA_MGMT_freeCertificate(&tmp);
                unlock(ssl);
                return MPR_ERR_CANT_READ;
            }
        }
        if (ssl->keyFile) {
            certDescriptor tmp;
            if ((rc = MOCANA_readFile((sbyte*) ssl->keyFile, &tmp.pKeyBlob, &tmp.keyBlobLength)) < 0) {
                mprError("MOCANA: Unable to read key file %s", ssl->keyFile); 
                CA_MGMT_freeCertificate(&cfg->cert);
                unlock(ssl);
            }
            if ((rc = CA_MGMT_convertKeyPEM(tmp.pKeyBlob, tmp.keyBlobLength, 
                    &cfg->cert.pKeyBlob, &cfg->cert.keyBlobLength)) < 0) {
                mprError("MOCANA: Unable to decode PEM key file %s", ssl->keyFile); 
                CA_MGMT_freeCertificate(&tmp);
                unlock(ssl);
                return MPR_ERR_CANT_READ;
            }
        }
        /// if (verifyMode != ...)
        if (SSL_initServerCert(&cfg->cert, FALSE, 0)) {
            mprError("SSL_initServerCert failed");
            unlock(ssl);
            return MPR_ERR_CANT_INITIALIZE;
        }
    }
    unlock(ssl);

    //  MOB - must verify peerName
    if (sp->flags & MPR_SOCKET_SERVER) {
        if ((handle = SSL_acceptConnection(sp->fd)) < 0) {
            return -1;
        }
    } else {
        //  MOB - need client side
    }
    return 0;
}


static void disconnectMoc(MprSocket *sp)
{
    //  MOB - anything to do here?
    sp->service->standardProvider->disconnectSocket(sp);
}


#if UNUSED
static void traceCert(MprSocket *sp)
{
    MprSsl      *ssl;
    Ciphers     *cp;
   
    ssl = sp->ssl;
    if (ssl->session) {
        for (cp = cipherList; cp->name; cp++) {
            if (cp->iana == ssl->session->cipher) {
                break;
            }
        }
        if (cp) {
            mprLog(4, "MOCANA connected using cipher: %s, %x", cp->name, ssl->session->cipher);
        }
    }
    if (ssl->caFile) {
        mprLog(4, "MOCANA: Using certificates from %s", ssl->caFile);
    } else if (ssl->caPath) {
        mprLog(4, "MOCANA: Using certificates from directory %s", ssl->caPath);
    }
    if (ssl->peer_cert) {
        mprLog(4, "MOCANA: client supplied no certificate");
    } else {
        mprRawLog(4, "%s", x509parse_cert_inf("", ssl->peer_cert));
    }
    sp->flags |= MPR_SOCKET_CHECKED;
}
#endif


static int handshakeMoc(MprSocket *sp)
{
    MocSocket   *mp;
    int         rc;

    mp = (MocSocket*) sp->sslSocket;
    sp->flags |= MPR_SOCKET_HANDSHAKING;

    while (1 /* mp->ctx.state != SSL_HANDSHAKE_OVER */) {
        if ((rc = SSL_negotiateConnection(mp->handle)) < 0) {
            if (rc == -1 /* EST_ERR_NET_TRY_AGAIN */) {
                if (!mprGetSocketBlockingMode(sp)) {
                    return 0;
                }
                continue;
            }
            break;
        }
    }
    sp->flags &= ~MPR_SOCKET_HANDSHAKING;

    /*
        Analyze the handshake result
    */
    if (rc < 0) {
        mprLog(4, "MOCANA: readMoc: Cannot handshake: error %d", rc);
        sp->flags |= MPR_SOCKET_EOF;
        errno = EPROTO;
        return -1;
    }
    return 1;
}


/*
    Return the number of bytes read. Return -1 on errors and EOF. Distinguish EOF via mprIsSocketEof
 */
static ssize readMoc(MprSocket *sp, void *buf, ssize len)
{
    MocSocket   *mp;
    int         rc, nbytes;

    mp = (MocSocket*) sp->sslSocket;
    assert(mp);
    assert(mp->cfg);

    if ( 1 /* MOB  est->ctx.state != SSL_HANDSHAKE_OVER */) {
        if ((rc = handshakeMoc(sp)) <= 0) {
            return rc;
        }
    }
    while (1) {
        rc = SSL_recv(mp->handle, buf, (int) len, &nbytes, SSL_RECV_TIMEOUT);
        mprLog(5, "MOCANA: ssl_read %d", rc);
        if (rc < 0) {
            sp->flags |= MPR_SOCKET_EOF;
            return -1;
        }
        break;
    }
#if UNUSED
    if (mp->ssl.in_left > 0) {
        mprHiddenSocketData(sp, mp->ssl.in_left, MPR_READABLE);
    }
#endif
    return rc;
}


/*
    Write data. Return the number of bytes written or -1 on errors.
 */
static ssize writeMoc(MprSocket *sp, cvoid *buf, ssize len)
{
    MocSocket   *mp;
    ssize       totalWritten;
    int         rc;

    mp = (MocSocket*) sp->sslSocket;
    if (len <= 0) {
        assert(0);
        return -1;
    }
    if ( 1 /* MOB  est->ctx.state != SSL_HANDSHAKE_OVER */) {
        if ((rc = handshakeMoc(sp)) <= 0) {
            return rc;
        }
    }
    totalWritten = 0;
    do {
        rc = SSL_send(mp->handle, (sbyte*) buf, (int) len);
        mprLog(7, "MOCANA: written %d, requested len %d", rc, len);
        if (rc <= 0) {
            //  MOB - should this set EOF. What about other providers?
            mprLog(0, "MOCANA: SSL_send failed rc %d", rc);
            return -1;

        } else {
            totalWritten += rc;
            buf = (void*) ((char*) buf + rc);
            len -= rc;
            mprLog(7, "MOCANA: write: len %d, written %d, total %d", len, rc, totalWritten);
        }
    } while (len > 0);
    return totalWritten;
}


static char *getMocState(MprSocket *sp)
{
#if UNUSED
    MocSocket       *moc;
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


#if UNUSED
/*
    Thread-safe session management
 */
static int getSession(ssl_context *ssl)
{
    ssl_session     *sp;
	time_t          t;
    int             next;

	t = time(NULL);
	if (!ssl->resume) {
		return 1;
    }
    for (ITERATE_ITEMS(sessions, sp, next)) {
        if (ssl->timeout && (t - sp->start) > ssl->timeout) {
            continue;
        }
		if (ssl->session->cipher != sp->cipher || ssl->session->length != sp->length) {
			continue;
        }
		if (memcmp(ssl->session->id, sp->id, sp->length) != 0) {
			continue;
        }
		memcpy(ssl->session->master, sp->master, sizeof(ssl->session->master));
        return 0;
    }
	return 1;
}


static int setSession(ssl_context *ssl)
{
	time_t          t;
    ssl_session     *sp;
    int             next;

	t = time(NULL);
    for (ITERATE_ITEMS(sessions, sp, next)) {
		if (ssl->timeout != 0 && (t - sp->start) > ssl->timeout) {
            /* expired, reuse this slot */
			break;	
        }
		if (memcmp(ssl->session->id, sp->id, sp->length) == 0) {
            /* client reconnected */
			break;	
        }
	}
	if (sp == NULL) {
		if ((sp = mprAlloc(sizeof(ssl_session))) == 0) {
			return 1;
        }
        mprAddItem(sessions, sp);
	}
	memcpy(sp, ssl->session, sizeof(ssl_session));
	return 0;
}


static void estTrace(void *fp, int level, char *str)
{
    level += 3;
    if (level <= MPR->logLevel) {
        mprRawLog(level, "%s: %d: EST: %s", MPR->name, level, str);
    }
}

#endif /* UNUSED */

#endif /* BIT_PACK_MOCANA */

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
