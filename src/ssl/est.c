/*
    est.c - Embedded Secure Transport

    Individual sockets are not thread-safe. Must only be used by a single thread.

    Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "mpr.h"

#if BIT_PACK_EST

#include    "est.h"

/************************************* Defines ********************************/
/*
    Per-route SSL configuration
 */
typedef struct EstConfig {
    rsa_context     rsa;                /* RSA context */
    x509_cert       cert;               /* Certificate (own) */
    x509_cert       cabundle;           /* Certificate bundle to veryify peery */
    int             *ciphers;           /* Set of acceptable ciphers */
    char            *dhKey;             /* DH keys */
} EstConfig;

/*
    Per socket state
 */
typedef struct EstSocket {
    MprSocket       *sock;              /* MPR socket object */
    EstConfig       *cfg;               /* Configuration */
    havege_state    hs;                 /* Random HAVEGE state */
    ssl_context     ctx;                /* SSL state */
    ssl_session     session;            /* SSL sessions */
} EstSocket;

static MprSocketProvider *estProvider;  /* EST socket provider */
static EstConfig *defaultEstConfig;     /* Default configuration */

/*
    Regenerate using: dh_genprime
    Generated on 1/1/2013
 */
static char *dhG = "4";
static char *dhKey =
    "E4004C1F94182000103D883A448B3F80"
    "2CE4B44A83301270002C20D0321CFD00"
    "11CCEF784C26A400F43DFB901BCA7538"
    "F2C6B176001CF5A0FD16D2C48B1D0C1C"
    "F6AC8E1DA6BCC3B4E1F96B0564965300"
    "FFA1D0B601EB2800F489AA512C4B248C"
    "01F76949A60BB7F00A40B1EAB64BDD48" 
    "E8A700D60B7F1200FA8E77B0A979DABF";

/*
    Thread-safe session list
 */
static MprList *sessions;

/***************************** Forward Declarations ***************************/

static void     closeEst(MprSocket *sp, bool gracefully);
static void     disconnectEst(MprSocket *sp);
static void     estTrace(void *fp, int level, char *str);
static int      estHandshake(MprSocket *sp);
static char     *getEstState(MprSocket *sp);
static int      listenEst(MprSocket *sp, cchar *host, int port, int flags);
static void     manageEstConfig(EstConfig *cfg, int flags);
static void     manageEstProvider(MprSocketProvider *provider, int flags);
static void     manageEstSocket(EstSocket *ssp, int flags);
static ssize    readEst(MprSocket *sp, void *buf, ssize len);
static int      upgradeEst(MprSocket *sp, MprSsl *sslConfig, cchar *peerName);
static ssize    writeEst(MprSocket *sp, cvoid *buf, ssize len);

static int      setSession(ssl_context *ssl);
static int      getSession(ssl_context *ssl);

/************************************* Code ***********************************/
/*
    Create the Openssl module. This is called only once
    MOB - should this be Create or Open
 */
PUBLIC int mprCreateEstModule()
{
    if ((estProvider = mprAllocObj(MprSocketProvider, manageEstProvider)) == NULL) {
        return MPR_ERR_MEMORY;
    }
    estProvider->upgradeSocket = upgradeEst;
    estProvider->closeSocket = closeEst;
    estProvider->disconnectSocket = disconnectEst;
    estProvider->listenSocket = listenEst;
    estProvider->readSocket = readEst;
    estProvider->writeSocket = writeEst;
    estProvider->socketState = getEstState;
    mprAddSocketProvider("est", estProvider);
    sessions = mprCreateList(-1, -1);

    if ((defaultEstConfig = mprAllocObj(EstConfig, manageEstConfig)) == 0) {
        return MPR_ERR_MEMORY;
    }
    defaultEstConfig->dhKey = dhKey;
    return 0;
}


static void manageEstProvider(MprSocketProvider *provider, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(defaultEstConfig);
        mprMark(sessions);
    }
}


static void manageEstConfig(EstConfig *cfg, int flags)
{
    if (flags & MPR_MANAGE_MARK) {

    } else if (flags & MPR_MANAGE_FREE) {
        rsa_free(&cfg->rsa);
        x509_free(&cfg->cert);
        x509_free(&cfg->cabundle);
        free(cfg->ciphers);
    }
}


/*
    Destructor for an EstSocket object
 */
static void manageEstSocket(EstSocket *est, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(est->cfg);
        mprMark(est->sock);

    } else if (flags & MPR_MANAGE_FREE) {
        ssl_free(&est->ctx);
    }
}


static void closeEst(MprSocket *sp, bool gracefully)
{
    EstSocket       *est;

    est = sp->sslSocket;
    if (!(sp->flags & MPR_SOCKET_EOF)) {
        ssl_close_notify(&est->ctx);
    }
    sp->service->standardProvider->closeSocket(sp, gracefully);
}


/*
    Initialize a new server-side connection
 */
static int listenEst(MprSocket *sp, cchar *host, int port, int flags)
{
    assert(sp);
    assert(port);
    return sp->service->standardProvider->listenSocket(sp, host, port, flags);
}


/*
    Upgrade a standard socket to use TLS
 */
static int upgradeEst(MprSocket *sp, MprSsl *ssl, cchar *peerName)
{
    EstSocket   *est;
    EstConfig   *cfg;
    int         verifyMode;

    assert(sp);

    if (ssl == 0) {
        ssl = mprCreateSsl(sp->flags & MPR_SOCKET_SERVER);
    }
    if ((est = (EstSocket*) mprAllocObj(EstSocket, manageEstSocket)) == 0) {
        return MPR_ERR_MEMORY;
    }
    est->sock = sp;
    sp->sslSocket = est;
    sp->ssl = ssl;

    lock(ssl);
    if (ssl->config) {
        est->cfg = cfg = ssl->config;
    } else {
        /*
            One time setup for the SSL configuration for this MprSsl
         */
        if ((cfg = mprAllocObj(EstConfig, manageEstConfig)) == 0) {
            unlock(ssl);
            return MPR_ERR_MEMORY;
        }
        est->cfg = ssl->config = cfg;
        if (ssl->certFile) {
            //  MOB - openssl uses encrypted and/not 
            if (x509parse_crtfile(&cfg->cert, ssl->certFile) != 0) {
                sp->errorMsg = sfmt("Unable to parse certificate %s", ssl->certFile); 
                unlock(ssl);
                return MPR_ERR_CANT_READ;
            }
        }
        if (ssl->keyFile) {
            //  MOB - last arg is password
            if (x509parse_keyfile(&cfg->rsa, ssl->keyFile, 0) != 0) {
                sp->errorMsg = sfmt("Unable to parse key file %s", ssl->keyFile); 
                unlock(ssl);
                return MPR_ERR_CANT_READ;
            }
        }
        if (ssl->caFile) {
            if (x509parse_crtfile(&cfg->cabundle, ssl->caFile) != 0) {
                sp->errorMsg = sfmt("Unable to parse certificate bundle %s", ssl->caFile); 
                unlock(ssl);
                return MPR_ERR_CANT_READ;
            }
        }
        cfg->dhKey = defaultEstConfig->dhKey;
        cfg->ciphers = ssl_create_ciphers(ssl->ciphers);
    }
    unlock(ssl);
    ssl_free(&est->ctx);

    //  MOB - convert to proper entropy source API
    //  MOB - can't put this in cfg yet as it is not thread safe
    havege_init(&est->hs);
    ssl_init(&est->ctx);
	ssl_set_endpoint(&est->ctx, sp->flags & MPR_SOCKET_SERVER ? SSL_IS_SERVER : SSL_IS_CLIENT);

    /* Optional means to manually verify in estHandshake */
    if (sp->flags & MPR_SOCKET_SERVER) {
        verifyMode = ssl->verifyPeer ? SSL_VERIFY_OPTIONAL : SSL_VERIFY_NO_CHECK;
    } else {
        verifyMode = SSL_VERIFY_OPTIONAL;
    }
    ssl_set_authmode(&est->ctx, verifyMode);
    ssl_set_rng(&est->ctx, havege_rand, &est->hs);
	ssl_set_dbg(&est->ctx, estTrace, NULL);
	ssl_set_bio(&est->ctx, net_recv, &sp->fd, net_send, &sp->fd);

    //  MOB - better if the API took a handle (est)
	ssl_set_scb(&est->ctx, getSession, setSession);
    ssl_set_ciphers(&est->ctx, cfg->ciphers);

	ssl_set_session(&est->ctx, 1, 0, &est->session);
	memset(&est->session, 0, sizeof(ssl_session));

	ssl_set_ca_chain(&est->ctx, &cfg->cabundle, (char*) peerName);
	ssl_set_own_cert(&est->ctx, &cfg->cert, &cfg->rsa);
	ssl_set_dh_param(&est->ctx, dhKey, dhG);

    if (estHandshake(sp) < 0) {
        return -1;
    }
    return 0;
}


static void disconnectEst(MprSocket *sp)
{
    sp->service->standardProvider->disconnectSocket(sp);
}


/*
    Initiate or continue SSL handshaking with the peer. This routine does not block.
    Return -1 on errors, 0 incomplete and awaiting I/O, 1 if successful
 */
static int estHandshake(MprSocket *sp)
{
    EstSocket   *est;
    int         rc, vrc, trusted;

    est = (EstSocket*) sp->sslSocket;
    trusted = 1;
    sp->flags |= MPR_SOCKET_HANDSHAKING;

    while (est->ctx.state != SSL_HANDSHAKE_OVER && (rc = ssl_handshake(&est->ctx)) != 0) {
        if (rc == EST_ERR_NET_TRY_AGAIN) {
            if (!mprGetSocketBlockingMode(sp)) {
                return 0;
            }
            continue;
        }
        break;
    }
    sp->flags &= ~MPR_SOCKET_HANDSHAKING;

    /*
        Analyze the handshake result
     */
    if (rc < 0) {
        sp->errorMsg = sfmt("Cannot handshake: error -0x%x", -rc);
        mprLog(4, "%s", sp->errorMsg);
        sp->flags |= MPR_SOCKET_EOF;
        errno = EPROTO;
        return -1;
       
    } else if ((vrc = ssl_get_verify_result(&est->ctx)) != 0) {
        if (vrc & BADCERT_EXPIRED) {
            sp->errorMsg = sfmt("Certificate expired");

        } else if (vrc & BADCERT_REVOKED) {
            sp->errorMsg = sfmt("Certificate revoked");

        } else if (vrc & BADCERT_CN_MISMATCH) {
            sp->errorMsg = sfmt("Certificate common name mismatch");

        } else if (vrc & BADCERT_NOT_TRUSTED) {
            if (est->ctx.peer_cert->next && est->ctx.peer_cert->next->version == 0) {
                //  MOB - est should have dedicated EST error code for this.
                sp->errorMsg = sfmt("Self-signed certificate");
            } else {
                sp->errorMsg = sfmt("Certificate not trusted");
            }
            trusted = 0;

        } else {
            if (est->ctx.client_auth && !sp->ssl->certFile) {
                sp->errorMsg = sfmt("Server requires a client certificate");
            } else if (rc == EST_ERR_NET_CONN_RESET) {
                sp->errorMsg = sfmt("Peer disconnected");
            } else {
                sp->errorMsg = sfmt("Cannot handshake: error -0x%x", -rc);
            }
        }
        mprLog(4, "%s", sp->errorMsg);
        if (sp->ssl->verifyPeer) {
            /* 
               If not verifying the issuer, permit certs that are only untrusted (no other error).
               This allows self-signed certs.
             */
            if (!sp->ssl->verifyIssuer && !trusted) {
                return 1;
            } else {
                sp->flags |= MPR_SOCKET_EOF;
                errno = EPROTO;
                return -1;
            }
        }
    }
    return 1;
}


/*
    Return the number of bytes read. Return -1 on errors and EOF. Distinguish EOF via mprIsSocketEof.
    If non-blocking, may return zero if no data or still handshaking.
 */
static ssize readEst(MprSocket *sp, void *buf, ssize len)
{
    EstSocket   *est;
    int         rc;

    est = (EstSocket*) sp->sslSocket;
    assert(est);
    assert(est->cfg);

    if (est->ctx.state != SSL_HANDSHAKE_OVER) {
        if ((rc = estHandshake(sp)) <= 0) {
            return rc;
        }
    }
    while (1) {
        rc = ssl_read(&est->ctx, buf, (int) len);
        mprLog(5, "EST: ssl_read %d", rc);
        if (rc < 0) {
            if (rc == EST_ERR_NET_TRY_AGAIN)  {
                break;
            } else if (rc == EST_ERR_SSL_PEER_CLOSE_NOTIFY) {
                mprLog(5, "EST: connection was closed gracefully\n");
                sp->flags |= MPR_SOCKET_EOF;
                return -1;
            } else if (rc == EST_ERR_NET_CONN_RESET) {
                mprLog(5, "EST: connection reset");
                sp->flags |= MPR_SOCKET_EOF;
                return -1;
            } else {
                mprLog(4, "EST: read error -0x%", -rc);
                sp->flags |= MPR_SOCKET_EOF;
                return -1;
            }
        }
        break;
    }
    mprHiddenSocketData(sp, est->ctx.out_left, MPR_READABLE);
    return rc;
}


/*
    Write data. Return the number of bytes written or -1 on errors or socket closure.
    If non-blocking, may return zero if no data or still handshaking.
 */
static ssize writeEst(MprSocket *sp, cvoid *buf, ssize len)
{
    EstSocket   *est;
    ssize       totalWritten;
    int         rc;

    est = (EstSocket*) sp->sslSocket;
    if (len <= 0) {
        assert(0);
        return -1;
    }
    if (est->ctx.state != SSL_HANDSHAKE_OVER) {
        if ((rc = estHandshake(sp)) <= 0) {
            return rc;
        }
    }
    totalWritten = 0;
    do {
        rc = ssl_write(&est->ctx, (uchar*) buf, (int) len);
        mprLog(7, "EST: written %d, requested len %d", rc, len);
        if (rc <= 0) {
            if (rc == EST_ERR_NET_TRY_AGAIN) {                                                          
                /* MOB - but what about buffered pending data? */
                break;
            }
            if (rc == EST_ERR_NET_CONN_RESET) {                                                         
                mprLog(0, "ssl_write peer closed");
                return -1;
            } else {
                mprLog(0, "ssl_write failed rc -0x%x", -rc);
                return -1;
            }
        } else {
            totalWritten += rc;
            buf = (void*) ((char*) buf + rc);
            len -= rc;
            mprLog(7, "EST: write: len %d, written %d, total %d", len, rc, totalWritten);
        }
    } while (len > 0);

    mprHiddenSocketData(sp, est->ctx.out_left, MPR_WRITABLE);
    return totalWritten;
}


static char *getEstState(MprSocket *sp)
{
    EstSocket       *est;
    ssl_context     *ctx;
    MprBuf          *buf;
    char            *own, *peer;
    char            cbuf[5120];

    est = sp->sslSocket;
    ctx = &est->ctx;
    buf = mprCreateBuf(0, 0);
    mprPutToBuf(buf, "CIPHER=%s, ", ssl_get_cipher(ctx));

    own =  sp->acceptIp ? "SERVER_" : "CLIENT_";
    peer = sp->acceptIp ? "CLIENT_" : "SERVER_";
    x509parse_cert_info(peer, cbuf, sizeof(cbuf), ctx->peer_cert);
    mprPutStringToBuf(buf, cbuf);
    x509parse_cert_info(own, cbuf, sizeof(cbuf), ctx->own_cert);
    mprPutStringToBuf(buf, cbuf);
    mprTrace(4, "EST state: %s", mprGetBufStart(buf));
    return mprGetBufStart(buf);
}


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
        mprLog(level, "EST: %s", str);
    }
}

#endif /* BIT_PACK_EST */

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
