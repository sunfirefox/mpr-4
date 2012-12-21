/*
    mpEst.c - Embedded Secure Transport

    Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "mpr.h"

#if BIT_PACK_EST

#include    "est.h"

/************************************* Defines ********************************/

#ifndef BIT_SERVER_CERT_FILE
    #define BIT_SERVER_CERT_FILE    "server.crt"
#endif
#ifndef BIT_SERVER_KEY_FILE
    #define BIT_SERVER_KEY_FILE     "server.key.pem"
#endif
#ifndef BIT_CLIENT_CERT_FILE
    #define BIT_CLIENT_CERT_FILE    "client.crt"
#endif
#ifndef BIT_CLIENT_CERT_PATH
    #define BIT_CLIENT_CERT_PATH    "certs"
#endif

/*
    MOB - is this per route or per connection
 */
typedef struct EstConfig {
    rsa_context     rsa;
    x509_cert       cert;
    char            *dhKey;
} EstConfig;

/*
    Per socket state
 */
typedef struct EstSocket {
    MprSocket       *sock;
    EstConfig       *cfg;
    int             *ciphers;
    havege_state    hs;
    ssl_context     ssl;
    ssl_session     session;
    int             introduced;
} EstSocket;


static MprSocketProvider *est;
static EstConfig *defaultEstConfig;

//  MOB - GENERATE
static char *dhKey =
    "E4004C1F94182000103D883A448B3F80"
    "2CE4B44A83301270002C20D0321CFD00"
    "11CCEF784C26A400F43DFB901BCA7538"
    "F2C6B176001CF5A0FD16D2C48B1D0C1C"
    "F6AC8E1DA6BCC3B4E1F96B0564965300"
    "FFA1D0B601EB2800F489AA512C4B248C"
    "01F76949A60BB7F00A40B1EAB64BDD48" 
    "E8A700D60B7F1200FA8E77B0A979DABF";

static char *dhg = "4";

#if UNUSED
//  MOB - bit configure somehow
//  MOB - need API
static int ciphers[] = {
	SSL_EDH_RSA_AES_256_SHA,
	SSL_EDH_RSA_CAMELLIA_256_SHA,
	SSL_EDH_RSA_DES_168_SHA,
	SSL_RSA_AES_256_SHA,
	SSL_RSA_CAMELLIA_256_SHA,
	SSL_RSA_AES_128_SHA,
	SSL_RSA_CAMELLIA_128_SHA,
	SSL_RSA_DES_168_SHA,
	SSL_RSA_RC4_128_SHA,
	SSL_RSA_RC4_128_MD5,
	0
};
#endif

//  MOB - push into ets

//MOB http://www.iana.org/assignments/tls-parameters/tls-parameters.xml

#define KEY_RSA         (0x1)
#define KEY_DHr         (0x2)
#define KEY_DHd         (0x4)
#define KEY_EDH         (0x8)
#define KEY_SDP         (0x10)
#define KEY_MASK        (0x1F)

#define AUTH_NULL       (0x1 << 8)
#define AUTH_RSA        (0x2 << 8)
#define AUTH_DSS        (0x4 << 8)
#define AUTH_DH         (0x8 << 8)
#define AUTH_MASK       (0xF << 8)

#define CIPHER_NULL     (0x1 << 16)
#define CIPHER_eNULL    (CIPHER_NULL
#define CIPHER_AES      (0x2 << 16)
#define CIPHER_DES      (0x4 << 16)
#define CIPHER_3DES     (0x8 << 16)
#define CIPHER_RC4      (0x10 << 16)
#define CIPHER_RC2      (0x20 << 16)
#define CIPHER_IDEA     (0x40 << 16)
#define CIPHER_CAMELLIA (0x80 << 16)
#define CIPHER_MASK     (0xFF << 16)

#define MAC_MD5         (0x1 << 24)
#define MAC_SHA1        (0x2 << 24)
#define MAC_SHA         (0x4 << 24)
#define MAC_MASK        (0x7 << 24)

#define CMED             (0x1 << 32)
#define CHIGH            (0x2 << 32)

typedef struct Ciphers {
    int     iana;
    char    *name,
    long    mask;
} Ciphers;

#if UNUSED
#define SSL_RSA_RC4_128_MD5             0x4     /* TLS_RSA_WITH_RC4_128_MD5 */
#define SSL_RSA_RC4_128_SHA             0x5     /* TLS_RSA_WITH_RC4_128_SHA */
#define SSL_RSA_DES_168_SHA             0xA     /* TLS_RSA_WITH_3DES_EDE_CBC_SHA */
#define SSL_EDH_RSA_DES_168_SHA         0x16    /* TLS_DHE_RSA_WITH_3DES_EDE_CBC_SHA */
#define SSL_RSA_AES_128_SHA             0x2F    /* TLS_RSA_WITH_AES_128_CBC_SHA */
#define SSL_RSA_AES_256_SHA             0x35    /* TLS_RSA_WITH_AES_256_CBC_SHA */
#define SSL_EDH_RSA_AES_256_SHA         0x39    /* TLS_DHE_RSA_WITH_AES_256_CBC_SHA */
#define SSL_RSA_CAMELLIA_128_SHA        0x41    /* TLS_RSA_WITH_CAMELLIA_128_CBC_SHA */
#define SSL_RSA_CAMELLIA_256_SHA        0x84    /* TLS_RSA_WITH_CAMELLIA_256_CBC_SHA */
#define SSL_EDH_RSA_CAMELLIA_256_SHA    0x88    /* TLS_DHE_RSA_WITH_CAMELLIA_256_CBC_SHA */

Low     9, 12, 1A, 15
MEDIUM  04 05 07 18 80 8A 96 99 9A 9B
HIGH    0A 13 16 1B 2F 32 33 34 35 38 39 3A 3C 3D 40 41 44 45 46 67 6A 6B 6C 6D 84 87 88 89 8B 8C 8D 9C 9D 9E 9F A2 A3 A6 A7
#endif


static Ciphers cipherList[] = {
{ 0x04, "TLS_RSA_WITH_RC4_128_MD5",              SSL_RSA_RC4_128_MD5,          KEY_RSA | CIPHER_AES      | MAC_MD5 | CHIGH },
{ 0x05, "TLS_RSA_WITH_RC4_128_SHA",              SSL_RSA_RC4_128_SHA,          KEY_RSA | CIPHER_AES      | MAC_SHA | CHIGH },
{ 0x0A, "TLS_RSA_WITH_3DES_EDE_CBC_SHA",         SSL_RSA_DES_168_SHA,          KEY_RSA | CIPHER_AES      | MAC_SHA | CHIGH },
{ 0x16, "TLS_DHE_RSA_WITH_3DES_EDE_CBC_SHA",     SSL_EDH_RSA_DES_168_SHA,      KEY_EDH | CIPHER_AES      | MAC_SHA | CHIGH },
{ 0x2F, "TLS_RSA_WITH_AES_128_CBC_SHA",          SSL_RSA_AES_128_SHA,          KEY_RSA | CIPHER_AES      | MAC_SHA | CHIGH },
{ 0x35, "TLS_RSA_WITH_AES_256_CBC_SHA",          SSL_RSA_AES_256_SHA,          KEY_RSA | CIPHER_AES      | MAC_SHA | CMED },
{ 0x39, "TLS_DHE_RSA_WITH_AES_256_CBC_SHA",      SSL_EDH_RSA_AES_256_SHA,      KEY_EDH | CIPHER_AES      | MAC_SHA | CMED },
{ 0x41, "TLS_RSA_WITH_CAMELLIA_128_CBC_SHA",     SSL_RSA_CAMELLIA_128_SHA,     KEY_RSA | CIPHER_CAMELLIA | MAC_SHA | CHIGH },
{ 0x88, "TLS_RSA_WITH_CAMELLIA_256_CBC_SHA",     SSL_EDH_RSA_CAMELLIA_256_SHA, KEY_EDH | CIPHER_CAMELLIA | MAC_SHA | MED },
{ 0x84, "TLS_DHE_RSA_WITH_CAMELLIA_256_CBC_SHA", SSL_RSA_CAMELLIA_256_SHA,     KEY_RSA | CIPHER_CAMELLIA | MAC_SHA | CHIGH },
{ 0x00, 0, 0 },
};

static MprList *sessions;

/***************************** Forward Declarations ***************************/

static void     closeEst(MprSocket *sp, bool gracefully);
static void     disconnectEst(MprSocket *sp);
static int      listenEst(MprSocket *sp, cchar *host, int port, int flags);
static void     manageEstConfig(EstConfig *cfg, int flags);
static void     manageEstProvider(MprSocketProvider *provider, int flags);
static void     manageEstSocket(EstSocket *ssp, int flags);
static ssize    readEst(MprSocket *sp, void *buf, ssize len);
static void     estTrace(void *fp, int level, char *str);
static int      upgradeEst(MprSocket *sp, MprSsl *sslConfig, int server);
static ssize    writeEst(MprSocket *sp, cvoid *buf, ssize len);

static int      setSession(ssl_context *ssl);
static int      getSession(ssl_context *ssl);

/************************************* Code ***********************************/
/*
    Create the Openssl module. This is called only once
 */
PUBLIC int mprCreateEstModule()
{
    if ((est = mprAllocObj(MprSocketProvider, manageEstProvider)) == NULL) {
        return MPR_ERR_MEMORY;
    }
    est->upgradeSocket = upgradeEst;
    est->closeSocket = closeEst;
    est->disconnectSocket = disconnectEst;
    est->listenSocket = listenEst;
    est->readSocket = readEst;
    est->writeSocket = writeEst;
    mprAddSocketProvider("est", est);
    sessions = mprCreateList(-1, -1);

    if ((defaultEstConfig = mprAllocObj(EstConfig, manageEstConfig)) == 0) {
        return MPR_ERR_MEMORY;
    }
    //  MOB - should generate
    defaultEstConfig->dhKey = dhKey;
    return 0;
}


static void manageEstProvider(MprSocketProvider *est, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(defaultEstConfig);
        mprMark(sessions);
    } else if (flags & MPR_MANAGE_FREE) {
    }
}


static void manageEstConfig(EstConfig *cfg, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        ;
    } else if (flags & MPR_MANAGE_FREE) {
        rsa_free(&cfg->rsa);
        x509_free(&cfg->cert);
    }
}


/*
    Create an SSL configuration for a route. An application can have multiple different SSL configurations for 
    different routes. There is default SSL configuration that is used when a route does not define a configuration 
    and also for clients.
 */
/*
    Destructor for an EstSocket object
 */
static void manageEstSocket(EstSocket *esp, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(esp->ciphers);
        mprMark(esp->sock);

    } else if (flags & MPR_MANAGE_FREE) {
        ssl_free(&esp->ssl);
    }
}


static void closeEst(MprSocket *sp, bool gracefully)
{
    EstSocket       *esp;

    esp = sp->sslSocket;
    if (!(sp->flags & MPR_SOCKET_EOF)) {
        ssl_close_notify(&esp->ssl);
    }
    sp->service->standardProvider->closeSocket(sp, gracefully);
}


/*
    Initialize a new server-side connection
 */
static int listenEst(MprSocket *sp, cchar *host, int port, int flags)
{
    assure(sp);
    assure(port);
    return sp->service->standardProvider->listenSocket(sp, host, port, flags);
}


    //  MOB - should be modifyable and be using ssl->ciphers and parse.
/*
    Parse an Apache style cipher suite. For example:
    '+' to add, '-' to remove, '!' to filter always.

        ALL:!ADH:!EXPORT56:RC4+RSA:+HIGH:+MEDIUM:+LOW:+SSLv2:+EXP:+eNULL
        AES128-SHA:AES256-SHA:RC4-SHA:DES-CBC3-SHA:RC4-MD5

        SSLCipherSuite ECDHE-RSA-AES128-SHA256:AES128-GCM-SHA256:RC4:HIGH:!MD5:!aNULL:!EDH
 */
static int *createCiphers(cchar *cipherSpec)
{
    Ciphers     *cp;
    char        *key, *auth, *encoding, *mac, *token, *next;
    int         *ciphers, i, cipher, add, sub, filter, mask, value, acceptable, type;

    key = auth = encoding = mac = 0;
    cipher = 0;
    mask = 0;
    acceptable = 0;
   
    //  MOB - should this be a hash?
    while ((token = stok(sclone(cipherSpec), ":", &next)) != 0) {
        add = sub = filter = 0;
        type = 0;
        if (*token == '+') {
            add = 1;
            token++;
        } else if (*token == '-') {
            sub = 1;
            token++;
        } else if (*token == '!') {
            filter = 1;
            token++;
        }
        //  MOB - do we need the k, a prefixes?

        if (scaselessmatch(token, "kRSA")) {
            value = KEY_RSA;
        } else if (scaselessmatch(token, "kDHr")) {
            value = KEY_DHr;
        } else if (scaselessmatch(token, "kDHd")) {
            value = KEY_DHd;
        } else if (scaselessmatch(token, "kEDH")) {
            value = KEY_EDH;
        } else if (scaselessmatch(token, "kSRDP")) {
            value = KEY_SDP;
        } else if (scaselessmatch(token, "aNULL")) {
            value = AUTH_NULL;
        } else if (scaselessmatch(token, "aRSA")) {
            value = AUTH_RSA;
        } else if (scaselessmatch(token, "aDSS")) {
            value = AUTH_DSS;
        } else if (scaselessmatch(token, "aDH")) {
            value = AUTH_DH;
        } else if (scaselessmatch(token, "eNULL")) {
            value = CIPHER_NULL;
        } else if (scaselessmatch(token, "NULL")) {
            value = CIPHER_NULL;
        } else if (scaselessmatch(token, "AES")) {
            value = CIPHER_AES;
        } else if (scaselessmatch(token, "DES")) {
            value = CIPHER_DES;
        } else if (scaselessmatch(token, "3DES")) {
            value = CIPHER_3DES;
        } else if (scaselessmatch(token, "RC4")) {
            value = CIPHER_RC4;
        } else if (scaselessmatch(token, "RC2")) {
            value = CIPHER_RC2;
        } else if (scaselessmatch(token, "IDEA")) {
            value = CIPHER_IDEA;
        } else if (scaselessmatch(token, "MD5")) {
            value = MAC_MD5;
        } else if (scaselessmatch(token, "SHA1")) {
            value = MAC_SHA1;
        } else if (scaselessmatch(token, "SHA")) {
            value = MAC_SHA;

#if UNUSED
        /* Aliases */
        } else if (scaselessmatch(token, "SSLv2")) {
        } else if (scaselessmatch(token, "SSLv3")) {
        } else if (scaselessmatch(token, "TLSv1")) {
        } else if (scaselessmatch(token, "EXP")) {
        } else if (scaselessmatch(token, "EXPORT40")) {
        } else if (scaselessmatch(token, "EXPORT56")) {
#endif
        } else if (scaselessmatch(token, "LOW")) {
            for (cp = cipherList, i = 0; cp->mask; cp++) {
                if (cp->mask & LOW) {
                    value |= cp->mask;
                }
            }
        } else if (scaselessmatch(token, "MEDIUM")) {
            for (cp = cipherList, i = 0; cp->mask; cp++) {
                if (cp->mask & MEDIUM) {
                    value |= cp->mask;
                }
            }
        } else if (scaselessmatch(token, "HIGH")) {
            for (cp = cipherList, i = 0; cp->mask; cp++) {
                if (cp->mask & HIGH) {
                    value |= cp->mask;
                }
            }
        } else if (scaselessmatch(token, "RSA")) {
            value = CIPHER_RSA;
#if UNUSED
        } else if (scaselessmatch(token, "DH")) {
        } else if (scaselessmatch(token, "ADH")) {
#endif
        } else if (scaselessmatch(token, "EDH")) {
            value = KEY_EDH;
        } else if (scaselessmatch(token, "DSS")) {
            value = AUTH_DSS;
        } else if (scaselessmatch(token, "NULL")) {
            value = CIPHER_NULL;
        } else {
            for (cp = cipherList, i = 0; cp->mask; cp++) {
                if (scaselessmatch(cp->name, token)) {
                    value |= cp->mask;
                }
            }
        }
        if (add) {
            acceptable |= value;
        } else if (sub) {
            acceptable &= ~value;
        } else if (filter) {
            filter |= value;
        }
    }
    acceptable &= ~filter;
    if (!(acceptable & KEY_MASK)) acceptable |= KEY_MASK;
    if (!(acceptable & AUTH_MASK)) acceptable |= AUTH_MASK;
    if (!(acceptable & CIPHER_MASK)) acceptable |= CIPHER_MASK;
    if (!(acceptable & MAC_MASK)) acceptable |= MAC_MASK;

    ciphers = mprAlloc(sizeof(cipherList) / sizeof(Ciphers) + sizeof(int));
    for (cp = cipherList, i = 0; cp->mask; cp++) {
        if (cp->mask & type) break;
        if (!(cp->mask & acceptable & KEY_MASK)) continue;
        if (!(cp->mask & acceptable & AUTH_MASK)) continue;
        if (!(cp->mask & acceptable & CIPHER_MASK)) continue;
        if (!(cp->mask & acceptable & MAC_MASK)) continue;
        ciphers[i++] = cp->iana;
        mprLog(0, "EST: Select cipher 0x%02x: %s", cp->iana, cp->name);
    }
    return ciphers;
}


/*
    Upgrade a standard socket to use TLS
 */
static int upgradeEst(MprSocket *sp, MprSsl *ssl, int server)
{
    EstSocket   *esp;
    EstConfig   *cfg;

    assure(sp);

    if (ssl == 0) {
        ssl = mprCreateSsl(server);
    }
    if ((esp = (EstSocket*) mprAllocObj(EstSocket, manageEstSocket)) == 0) {
        return MPR_ERR_MEMORY;
    }
    esp->sock = sp;
    sp->sslSocket = esp;
    sp->ssl = ssl;

    lock(ssl);
    if (!ssl->pconfig) {
        /*
            One time setup for the SSL configuration for this MprSsl
         */
        //  LOCKING
        if ((cfg = mprAllocObj(EstConfig, manageEstConfig)) == 0) {
            unlock(ssl);
            return MPR_ERR_MEMORY;
        }
        esp->cfg = ssl->pconfig = cfg;
#if UNUSED || 1
        if (ssl->certFile) {
            //  MOB - openssl uses encrypted and/not 
            if (x509parse_crtfile(&cfg->cert, ssl->certFile) != 0) {
                mprError("EST: Unable to read certificate %s", ssl->certFile); 
                unlock(ssl);
                return MPR_ERR_CANT_READ;
            }
        }
        if (ssl->keyFile) {
            if (x509parse_keyfile(&cfg->rsa, ssl->keyFile, 0) != 0) {
                mprError("EST: Unable to read key file %s", ssl->keyFile); 
                unlock(ssl);
                return MPR_ERR_CANT_READ;
            }
        }
        //MOB
#else
	int ret = x509parse_crt(&cfg->cert, (uchar*) test_srv_crt, (int) strlen(test_srv_crt));
	if (ret != 0) {
		printf(" failed\n  !  x509parse_crt returned %d\n\n", ret);
        return MPR_ERR_CANT_READ;
	}

	ret = x509parse_crt(&cfg->cert, (uchar*) test_ca_crt, (int) strlen(test_ca_crt));
	if (ret != 0) {
		printf(" failed\n  !  x509parse_crt returned %d\n\n", ret);
        return MPR_ERR_CANT_READ;
	}

	ret = x509parse_key(&cfg->rsa, (uchar*) test_srv_key, (int) strlen(test_srv_key), NULL, 0);
	if (ret != 0) {
		printf(" failed\n  !  x509parse_key returned %d\n\n", ret);
        return MPR_ERR_CANT_READ;
	}
#endif
        cfg->dhKey = defaultEstConfig->dhKey;
        //  MOB - see openssl for client certificate config
    }
    unlock(ssl);

    ssl_free(&esp->ssl);

    //  MOB - convert to proper entropy source API
    //  MOB - can't put this in cfg yet as it is not thread safe
    havege_init(&esp->hs);
    ssl_init(&esp->ssl);
	ssl_set_endpoint(&esp->ssl, server);
	ssl_set_authmode(&esp->ssl, (ssl->verifyPeer) ? SSL_VERIFY_REQUIRED : SSL_VERIFY_NONE);
    ssl_set_rng(&esp->ssl, havege_rand, &esp->hs);
	ssl_set_dbg(&esp->ssl, estTrace, NULL);
	ssl_set_bio(&esp->ssl, net_recv, &sp->fd, net_send, &sp->fd);

    //  MOB - better if the API took a handle (esp)
	ssl_set_scb(&esp->ssl, getSession, setSession);

    esp->ciphers = createCiphers(ssl->ciphers);
    ssl_set_ciphers(&esp->ssl, esp->ciphers);

    //  MOB
	ssl_set_session(&esp->ssl, 1, 0, &esp->session);
	memset(&esp->session, 0, sizeof(ssl_session));

	ssl_set_ca_chain(&esp->ssl, cfg->cert.next, NULL);
	ssl_set_own_cert(&esp->ssl, &cfg->cert, &cfg->rsa);
	ssl_set_dh_param(&esp->ssl, dhKey, dhg);

    return 0;
}


static void disconnectEst(MprSocket *sp)
{
    //  MOB - anything to do here?
    sp->service->standardProvider->disconnectSocket(sp);
}


#if TRACE
static void traceCert(MprSocket *sp)
{
    MprSsl      *ssl;
   
    ssl = sp->ssl;
    mprLog(4, "EST connected using cipher: \"%s\" from set %s", SSL_get_cipher(esp->handle), ssl->ciphers);
    if (ssl->caFile) {
        mprLog(4, "EST: Using certificates from %s", ssl->caFile);
    } else if (ssl->caPath) {
        mprLog(4, "EST: Using certificates from directory %s", ssl->caPath);
    }
#if FUTURE
    X509_NAME   *xSubject;
    X509        *cert;
    char        subject[260], issuer[260], peer[260], ebuf[BIT_MAX_BUFFER];
    ulong       serror;
    cert = SSL_get_peer_certificate(esp->handle);
    if (cert == 0) {
        mprLog(4, "EST: client supplied no certificate");
    } else {
        xSubject = X509_get_subject_name(cert);
        X509_NAME_oneline(xSubject, subject, sizeof(subject) -1);
        X509_NAME_oneline(X509_get_issuer_name(cert), issuer, sizeof(issuer) -1);
        X509_NAME_get_text_by_NID(xSubject, NID_commonName, peer, sizeof(peer) - 1);
        mprLog(4, "EST Subject %s", subject);
        mprLog(4, "EST Issuer: %s", issuer);
        mprLog(4, "EST Peer: %s", peer);
        X509_free(cert);
    }
    sp->flags |= MPR_SOCKET_TRACED;
#endif
}
#endif


/*
    Return the number of bytes read. Return -1 on errors and EOF. Distinguish EOF via mprIsSocketEof
 */
static ssize readEst(MprSocket *sp, void *buf, ssize len)
{
    EstSocket   *esp;
    int         rc;

    //  MOB - locking

    esp = (EstSocket*) sp->sslSocket;
    assure(esp);
    assure(esp->cfg);

    while (esp->ssl.state != SSL_HANDSHAKE_OVER && (rc = ssl_handshake(&esp->ssl)) != 0) {
        if (rc != TROPICSSL_ERR_NET_TRY_AGAIN) {
            mprLog(2, "EST: Can't handshake: %d", rc);
            return -1;
        }
    }
    while (1) {
        rc = ssl_read(&esp->ssl, buf, (int) len);
        mprLog(5, "EST: ssl_read %d", rc);
        if (rc < 0) {
            if (rc == TROPICSSL_ERR_NET_TRY_AGAIN)  {
                continue;
            } else if (rc == TROPICSSL_ERR_SSL_PEER_CLOSE_NOTIFY) {
                mprLog(5, "EST: connection was closed gracefully\n");
                sp->flags |= MPR_SOCKET_EOF;
                return -1;
            } else if (rc == TROPICSSL_ERR_NET_CONN_RESET) {
                mprLog(5, "EST: connection reset");
                sp->flags |= MPR_SOCKET_EOF;
                return -1;
            }
        }
        break;
    }
    if (esp->ssl.in_left > 0) {
        sp->flags |= MPR_SOCKET_PENDING;
        mprRecallWaitHandlerByFd(sp->fd);
    }
    return rc;
}


/*
    Write data. Return the number of bytes written or -1 on errors.
 */
static ssize writeEst(MprSocket *sp, cvoid *buf, ssize len)
{
    EstSocket   *esp;
    ssize       totalWritten;
    int         rc;

    esp = (EstSocket*) sp->sslSocket;
    if (esp->introduced == 0 || len <= 0) {
        assure(0);
        return -1;
    }
    totalWritten = 0;

    do {
        rc = ssl_write(&esp->ssl, (uchar*) buf, (int) len);
        mprLog(7, "EST: written %d, requested len %d", rc, len);
        if (rc <= 0) {
            if (rc == TROPICSSL_ERR_NET_TRY_AGAIN) {                                                          
                continue;
            }
            if (rc == TROPICSSL_ERR_NET_CONN_RESET) {                                                         
                printf(" failed\n  ! peer closed the connection\n\n");                                         
                mprLog(0, "ssl_write peer closed");
                return -1;
            } else if (rc != TROPICSSL_ERR_NET_TRY_AGAIN) {                                                          
                mprLog(0, "ssl_write failed rc %d", rc);
                return -1;
            }
        } else {
            totalWritten += rc;
            buf = (void*) ((char*) buf + rc);
            len -= rc;
            mprLog(7, "EST: write: len %d, written %d, total %d", len, rc, totalWritten);
        }
    } while (len > 0);
    return totalWritten;
}


#if KEEP
/*
    Called to verify X509 client certificates
 */
static int verifyX509Certificate(int ok, X509_STORE_CTX *xContext)
{
    X509        *cert;
    SSL         *handle;
    EstSocket   *esp;
    MprSsl      *ssl;
    char        subject[260], issuer[260], peer[260];
    int         error, depth;
    
    subject[0] = issuer[0] = '\0';

    handle = (SSL*) X509_STORE_CTX_get_app_data(xContext);
    esp = (EstSocket*) SSL_get_app_data(handle);
    ssl = esp->sock->ssl;

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
        mprLog(3, "EST: Certificate verified: subject %s", subject);
        mprLog(4, "EST: Issuer: %s", issuer);
        mprLog(4, "EST: Peer: %s", peer);
    } else {
        mprLog(1, "EST: Certification failed: subject %s (more trace at level 4)", subject);
        mprLog(4, "EST: Issuer: %s", issuer);
        mprLog(4, "EST: Peer: %s", peer);
        mprLog(4, "EST: Error: %d: %s", error, X509_verify_cert_error_string(error));
    }
    return ok;
}
#endif


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
        if (ssl->timeout && (t - sp->start) > ssl->timeout) continue;
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
    mprLog(level, "%s", str);
}

#endif /* BIT_PACK_EST */

/*
    @copy   default

    Copyright (c) Embedthis Software LLC, 2003-2012. All Rights Reserved.

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
