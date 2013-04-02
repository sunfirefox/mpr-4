/**
    ssl.c -- Initialization for libmprssl. Load the SSL provider.

    Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************* Includes ***********************************/

#include    "mpr.h"

/********************************** Globals ***********************************/

/*
    See: http://www.iana.org/assignments/tls-parameters/tls-parameters.xml
*/
MprCipher mprCiphers[] = {
    { 0x0001, "SSL_RSA_WITH_NULL_MD5" },
    { 0x0002, "SSL_RSA_WITH_NULL_SHA" },
    { 0x0004, "TLS_RSA_WITH_RC4_128_MD5" },
    { 0x0005, "TLS_RSA_WITH_RC4_128_SHA" },
    { 0x0009, "SSL_RSA_WITH_DES_CBC_SHA" },
    { 0x000A, "SSL_RSA_WITH_3DES_EDE_CBC_SHA" },
    { 0x0015, "SSL_DHE_RSA_WITH_DES_CBC_SHA" },
    { 0x0016, "SSL_DHE_RSA_WITH_3DES_EDE_CBC_SHA" },
    { 0x001A, "SSL_DH_ANON_WITH_DES_CBC_SHA" },
    { 0x001B, "SSL_DH_ANON_WITH_3DES_EDE_CBC_SHA" },
    { 0x002F, "TLS_RSA_WITH_AES_128_CBC_SHA" },
    { 0x0033, "TLS_DHE_RSA_WITH_AES_128_CBC_SHA" },
    { 0x0034, "TLS_DH_ANON_WITH_AES_128_CBC_SHA" },
    { 0x0035, "TLS_RSA_WITH_AES_256_CBC_SHA" },
    { 0x0039, "TLS_DHE_RSA_WITH_AES_256_CBC_SHA" },
    { 0x003A, "TLS_DH_ANON_WITH_AES_256_CBC_SHA" },
    { 0x003B, "SSL_RSA_WITH_NULL_SHA256" },
    { 0x003C, "TLS_RSA_WITH_AES_128_CBC_SHA256" },
    { 0x003D, "TLS_RSA_WITH_AES_256_CBC_SHA256" },
    { 0x0041, "TLS_RSA_WITH_CAMELLIA_128_CBC_SHA" },
    { 0x0067, "TLS_DHE_RSA_WITH_AES_128_CBC_SHA256" },
    { 0x006B, "TLS_DHE_RSA_WITH_AES_256_CBC_SHA256" },
    { 0x006C, "TLS_DH_ANON_WITH_AES_128_CBC_SHA256" },
    { 0x006D, "TLS_DH_ANON_WITH_AES_256_CBC_SHA256" },
    { 0x0084, "TLS_DHE_RSA_WITH_CAMELLIA_256_CBC_SHA" },
    { 0x0088, "TLS_RSA_WITH_CAMELLIA_256_CBC_SHA" },
    { 0x008B, "TLS_PSK_WITH_3DES_EDE_CBC_SHA" },
    { 0x008C, "TLS_PSK_WITH_AES_128_CBC_SHA" },
    { 0x008D, "TLS_PSK_WITH_AES_256_CBC_SHA" },
    { 0x008F, "SSL_DHE_PSK_WITH_3DES_EDE_CBC_SHA" },
    { 0x0090, "TLS_DHE_PSK_WITH_AES_128_CBC_SHA" },
    { 0x0091, "TLS_DHE_PSK_WITH_AES_256_CBC_SHA" },
    { 0x0093, "TLS_RSA_PSK_WITH_3DES_EDE_CBC_SHA" },
    { 0x0094, "TLS_RSA_PSK_WITH_AES_128_CBC_SHA" },
    { 0x0095, "TLS_RSA_PSK_WITH_AES_256_CBC_SHA" },
    { 0xC001, "TLS_ECDH_ECDSA_WITH_NULL_SHA" },
    { 0xC003, "SSL_ECDH_ECDSA_WITH_3DES_EDE_CBC_SHA" },
    { 0xC004, "TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA" },
    { 0xC005, "TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA" },
    { 0xC006, "TLS_ECDHE_ECDSA_WITH_NULL_SHA" },
    { 0xC008, "SSL_ECDHE_ECDSA_WITH_3DES_EDE_CBC_SHA" },
    { 0xC009, "TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA" },
    { 0xC00A, "TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA" },
    { 0xC00B, "TLS_ECDH_RSA_WITH_NULL_SHA" },
    { 0xC00D, "SSL_ECDH_RSA_WITH_3DES_EDE_CBC_SHA" },
    { 0xC00E, "TLS_ECDH_RSA_WITH_AES_128_CBC_SHA" },
    { 0xC00F, "TLS_ECDH_RSA_WITH_AES_256_CBC_SHA" },
    { 0xC010, "TLS_ECDHE_RSA_WITH_NULL_SHA" },
    { 0xC012, "SSL_ECDHE_RSA_WITH_3DES_EDE_CBC_SHA" },
    { 0xC013, "TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA" },
    { 0xC014, "TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA" },
    { 0xC015, "TLS_ECDH_anon_WITH_NULL_SHA" },
    { 0xC017, "SSL_ECDH_anon_WITH_3DES_EDE_CBC_SHA" },
    { 0xC018, "TLS_ECDH_anon_WITH_AES_128_CBC_SHA" },
    { 0xC019, "TLS_ECDH_anon_WITH_AES_256_CBC_SHA " },
    { 0xC023, "TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256" },
    { 0xC024, "TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA384" },
    { 0xC025, "TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA256" },
    { 0xC026, "TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA384" },
    { 0xC027, "TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256" },
    { 0xC028, "TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA384" },
    { 0xC029, "TLS_ECDH_RSA_WITH_AES_128_CBC_SHA256" },
    { 0xC02A, "TLS_ECDH_RSA_WITH_AES_256_CBC_SHA384" },
    { 0xC02B, "TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256" },
    { 0xC02C, "TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384" },
    { 0xC02D, "TLS_ECDH_ECDSA_WITH_AES_128_GCM_SHA256" },
    { 0xC02E, "TLS_ECDH_ECDSA_WITH_AES_256_GCM_SHA384" },
    { 0xC02F, "TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256" },
    { 0xC030, "TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384" },
    { 0xC031, "TLS_ECDH_RSA_WITH_AES_128_GCM_SHA256" },
    { 0xC032, "TLS_ECDH_RSA_WITH_AES_256_GCM_SHA384" },
    { 0xFFF0, "TLS_ECDHE_ECDSA_WITH_AES_128_CCM_8" },
    { 0x0, 0 },
};

/************************************ Code ************************************/
/*
    Module initialization entry point
 */
PUBLIC int mprSslInit(void *unused, MprModule *module)
{
#if BIT_PACK_SSL
    assert(module);

    /*
        Order matters. The last enabled stack becomes the default.
     */
#if BIT_PACK_MATRIXSSL
    if (mprCreateMatrixSslModule() < 0) {
        return MPR_ERR_CANT_OPEN;
    }
    MPR->socketService->defaultProvider = sclone("matrixssl");
#endif
#if BIT_PACK_NANOSSL
    if (mprCreateNanoSslModule() < 0) {
        return MPR_ERR_CANT_OPEN;
    }
    MPR->socketService->defaultProvider = sclone("nanossl");
#endif
#if BIT_PACK_OPENSSL
    if (mprCreateOpenSslModule() < 0) {
        return MPR_ERR_CANT_OPEN;
    }
    MPR->socketService->defaultProvider = sclone("openssl");
#endif
#if BIT_PACK_EST
    if (mprCreateEstModule() < 0) {
        return MPR_ERR_CANT_OPEN;
    }
    MPR->socketService->defaultProvider = sclone("est");
#endif
    return 0;
#else
    return MPR_ERR_BAD_STATE;
#endif /* BLD_PACK_SSL */
}


PUBLIC cchar *mprGetSslCipherName(int code) 
{
    MprCipher   *cp;

    for (cp = mprCiphers; cp->name; cp++) {
        if (cp->code == code) {
            return cp->name;
        }
    }
    return 0;
}


PUBLIC int mprGetSslCipherCode(cchar *cipher) 
{
    MprCipher   *cp;

    for (cp = mprCiphers; cp->name; cp++) {
        if (smatch(cp->name, cipher)) {
            return cp->code;
        }
    }
    return 0;
}

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
