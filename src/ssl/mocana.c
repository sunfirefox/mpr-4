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

#if TEST_CERT || 1
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
    mprAddSocketProvider("est", mocProvider);

    if ((defaultMocConfig = mprAllocObj(MocConfig, manageMocConfig)) == 0) {
        return MPR_ERR_MEMORY;
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
        SSL_releaseTables();
        MOCANA_freeMocana();
    }
}


static void manageMocConfig(MocConfig *cfg, int flags)
{
    if (flags & MPR_MANAGE_FREE) {
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
    assure(sp);
    assure(port);
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

    assure(sp);

    if (ssl == 0) {
        ssl = mprCreateSsl(sp->flags & MPR_SOCKET_SERVER);
    }
    if ((mp = (MocSocket*) mprAllocObj(MocSocket, manageMocSocket)) == 0) {
        return MPR_ERR_MEMORY;
    }
    mp->sock = sp;
    sp->sslSocket = mp;
    sp->ssl = ssl;

    lock(ssl);
    if (ssl->pconfig) {
        mp->cfg = cfg = ssl->pconfig;
    } else {
        /*
            One time setup for the SSL configuration for this MprSsl
         */
        //  LOCKING
        if ((cfg = mprAllocObj(MocConfig, manageMocConfig)) == 0) {
            unlock(ssl);
            return MPR_ERR_MEMORY;
        }
        mp->cfg = ssl->pconfig = cfg;
        if (ssl->certFile) {
            if ((rc = MOCANA_readFile((sbyte*) ssl->certFile, &cfg->cert.pCertificate, &cfg->cert.certLength))) {
                mprError("MOCANA: Unable to read certificate %s", ssl->certFile); 
                CA_MGMT_freeCertificate(&cfg->cert);
                unlock(ssl);
                return MPR_ERR_CANT_READ;
            }
        }
        if (ssl->keyFile) {
            if ((rc = MOCANA_readFile((sbyte*) ssl->keyFile, &cfg->cert.pKeyBlob, &cfg->cert.keyBlobLength)) < 0) {
                mprError("MOCANA: Unable to read key file %s", ssl->keyFile); 
                CA_MGMT_freeCertificate(&cfg->cert);
                unlock(ssl);
            }
        }
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
//  MOB - move to est
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
    sp->flags |= MPR_SOCKET_TRACED;
}
#endif


/*
    Return the number of bytes read. Return -1 on errors and EOF. Distinguish EOF via mprIsSocketEof
 */
static ssize readMoc(MprSocket *sp, void *buf, ssize len)
{
    MocSocket   *mp;
    int         rc, nbytes;

    mp = (MocSocket*) sp->sslSocket;
    assure(mp);
    assure(mp->cfg);

    if ((rc = SSL_negotiateConnection(mp->handle)) < 0) {
        mprLog(4, "MOCANA: readMoc: Cannot handshake: error %d", rc);
        sp->flags |= MPR_SOCKET_EOF;
        return -1;
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
        sp->flags |= MPR_SOCKET_PENDING;
        mprRecallWaitHandlerByFd(sp->fd);
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
        assure(0);
        return -1;
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


#if KEEP
/*
    Called to verify X509 client certificates
    MOB -- what about this?
 */
static int verifyX509Certificate(int ok, X509_STORE_CTX *xContext)
{
    X509        *cert;
    SSL         *handle;
    MocSocket   *mp;
    MprSsl      *ssl;
    char        subject[260], issuer[260], peer[260];
    int         error, depth;
    
    subject[0] = issuer[0] = '\0';

    handle = (SSL*) X509_STORE_CTX_get_app_data(xContext);
    mp = (MocSocket*) SSL_get_app_data(handle);
    ssl = mp->sock->ssl;

    cert = X509_STORE_CTX_get_current_cert(xContext);
    depth = X509_STORE_CTX_get_error_depth(xContext);
    error = X509_STORE_CTX_get_error(xContext);

    ok = 1;
    if (X509_NAME_oneline(X509_get_subject_name(cert), subject, sizeof(subject) - 1) < 0) {
        ok = 0;
    }
    if (X509_NAME_oneline(X509_get_issuer_name(xContext->current_cert), issuer, sizeof(issuer) - 1) < 0) {
        ok = 0;
    }
    if (X509_NAME_get_text_by_NID(X509_get_subject_name(xContext->current_cert), NID_commonName, peer, 
            sizeof(peer) - 1) < 0) {
        ok = 0;
    }
    if (ok && ssl->verifyDepth < depth) {
        if (error == 0) {
            error = X509_V_ERR_CERT_CHAIN_TOO_LONG;
        }
    }
    switch (error) {
    case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
        /* Normal self signed certificate */
    case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
    case X509_V_ERR_CERT_UNTRUSTED:
    case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY:
        if (ssl->verifyIssuer) {
            /* Issuer can't be verified */
            ok = 0;
        }
        break;

    case X509_V_ERR_CERT_CHAIN_TOO_LONG:
    case X509_V_ERR_CERT_HAS_EXPIRED:
    case X509_V_ERR_CERT_NOT_YET_VALID:
    case X509_V_ERR_CERT_REJECTED:
    case X509_V_ERR_CERT_SIGNATURE_FAILURE:
    case X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD:
    case X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD:
    case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
    case X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE:
    case X509_V_ERR_INVALID_CA:
    default:
        ok = 0;
        break;
    }
    if (ok) {
        mprLog(3, "MOCANA: Certificate verified: subject %s", subject);
        mprLog(4, "MOCANA: Issuer: %s", issuer);
        mprLog(4, "MOCANA: Peer: %s", peer);
    } else {
        mprLog(1, "MOCANA: Certification failed: subject %s (more trace at level 4)", subject);
        mprLog(4, "MOCANA: Issuer: %s", issuer);
        mprLog(4, "MOCANA: Peer: %s", peer);
        mprLog(4, "MOCANA: Error: %d: %s", error, X509_verify_cert_error_string(error));
    }
    return ok;
}
#endif


#if UNUSED
static void mocTrace(void *fp, int level, char *str)
{
    level += 3;
    if (level <= MPR->logLevel) {
        str = sclone(str);
        str[slen(str) - 1] = '\0';
        mprLog(level, "%s", str);
    }
}
#endif

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
