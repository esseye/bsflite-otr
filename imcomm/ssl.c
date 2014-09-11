#include <sys/socket.h>
#include <resolv.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#include "imcomm.h"

int ssl_verify_callback(int ok, X509_STORE_CTX *store) {
  if (!ok)
    fprintf(stderr, "[SSL] Error: %s\n", X509_verify_cert_error_string(store->error));
  return ok;
}

int ssl_verify_cert(X509 *cert) {
  int            result = -1;
  X509_STORE     *store = 0;
  X509_STORE_CTX *ctx = 0;
  X509_LOOKUP    *lookup = 0;
  
  store = X509_STORE_new(  );
  
  if (!store) return -1;

  X509_STORE_set_verify_cb_func(store, ssl_verify_callback);
  if (lookup = X509_STORE_add_lookup(store, X509_LOOKUP_file(  ))) {
    X509_LOOKUP_load_file(lookup, "/etc/ssl/certs/ca-bundle.crt", X509_FILETYPE_DEFAULT);
  }
  if ((ctx = X509_STORE_CTX_new(  )) != 0) {
    if (X509_STORE_CTX_init(ctx, store, cert, 0) == 1)
      result = (X509_verify_cert(ctx) == 1);
    X509_STORE_CTX_free(ctx);
  }
  X509_STORE_free(store);
  return(result);
}

SSL *ssl_init(int serversock) {
  BIO              *certbio = NULL;
  BIO               *outbio = NULL;
  X509                *cert = NULL;
  X509_NAME       *certname = NULL;
  X509_STORE		*store = NULL;
  X509_LOOKUP		*lookup = NULL;
  const SSL_METHOD *method;
  SSL_CTX *ctx;
  SSL *ssl;
  int ret, i;

  if(serversock == 0) {
    printf("[SSL] init got bad socket\n");
    return(NULL);
  }

  /* ---------------------------------------------------------- *
   * These function calls initialize openssl for correct work.  *
   * ---------------------------------------------------------- */
  OpenSSL_add_all_algorithms();
  ERR_load_BIO_strings();
  ERR_load_crypto_strings();
  SSL_load_error_strings();

  /* ---------------------------------------------------------- *
   * Create the Input/Output BIO's.                             *
   * ---------------------------------------------------------- */
  certbio = BIO_new(BIO_s_file());
  outbio  = BIO_new_fp(stdout, BIO_NOCLOSE);

  /* ---------------------------------------------------------- *
   * initialize SSL library and register algorithms             *
   * ---------------------------------------------------------- */
  if(SSL_library_init() < 0)
    BIO_printf(outbio, "[SSL] Could not initialize the OpenSSL library !\n");

  /* ---------------------------------------------------------- *
   * Set SSLv2 client hello, also announce SSLv3 and TLSv1      *
   * ---------------------------------------------------------- */
  method = SSLv23_client_method();

  /* ---------------------------------------------------------- *
   * Try to create a new SSL context                            *
   * ---------------------------------------------------------- */
  if ( (ctx = SSL_CTX_new(method)) == NULL)
    BIO_printf(outbio, "[SSL] Unable to create a new SSL context structure.\n");

  /* ---------------------------------------------------------- *
   * Disabling SSLv2 will leave v3 and TSLv1 for negotiation    *
   * ---------------------------------------------------------- */
  SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2);

  /* ---------------------------------------------------------- *
   * Create new SSL connection state object                     *
   * ---------------------------------------------------------- */
  ssl = SSL_new(ctx);

  SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);
  /* ---------------------------------------------------------- *
   * Attach the SSL session to the socket descriptor            *
   * ---------------------------------------------------------- */
  SSL_set_fd(ssl, serversock);

  /* ---------------------------------------------------------- *
   * Try to SSL-connect here, returns 1 for success             *
   * ---------------------------------------------------------- */
  if ( SSL_connect(ssl) != 1 )
    BIO_printf(outbio, "[SSL] Error: Could not build a SSL session\n");
  else
    BIO_printf(outbio, "[SSL] Successfully enabled SSL/TLS session\n");

  /* ---------------------------------------------------------- *
   * Get the remote certificate into the X509 structure         *
   * ---------------------------------------------------------- */
  cert = SSL_get_peer_certificate(ssl);
  if (cert == NULL)
    BIO_printf(outbio, "[SSL] Error: Could not get a certificate\n");
  else
    BIO_printf(outbio, "[SSL] Retrieved the server's certificate\n");

  /* ---------------------------------------------------------- *
   * extract various certificate information                    *
   * -----------------------------------------------------------*/
  certname = X509_NAME_new();
  certname = X509_get_subject_name(cert);

  /* ---------------------------------------------------------- *
   * display the cert subject here                              *
   * -----------------------------------------------------------*/
  int cert_valid = ssl_verify_cert(cert);
  BIO_printf(outbio, "[SSL] Displaying the certificate subject data:\n");
  X509_NAME_print_ex(outbio, certname, 0, 0);
  BIO_printf(outbio, "  %d\n", cert_valid);
  X509_STORE_free(store);

  /* ---------------------------------------------------------- *
   * Free the structures we don't need anymore                  *
   * -----------------------------------------------------------*/
  X509_free(cert);
  SSL_CTX_free(ctx);
  BIO_printf(outbio, "[SSL] Finished SSL/TLS connection with server\n");
  return(ssl);
}

int connect_ssl(void *handle, char *host, uint16_t port) {
        IMCOMM         *h = (IMCOMM *) handle;
        struct sockaddr_in sin;
        struct hostent *he = NULL;
        long            addy;
        struct in_addr  ina = {0};
        unsigned char   sockbuf[512];
        char           *sptr;
        int             received, retcode;

        printf("\n[SSL] Connecting to %s:%d.\n", host, port);

        if ((he = gethostbyname(host)) == NULL) {
                addy = inet_addr(host);
                ina.s_addr = addy;
        }
        if ((h->socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
                if (h->callbacks[IMCOMM_ERROR])
                        h->callbacks[IMCOMM_ERROR] (handle, IMCOMM_ERROR_PROXY,
                                                    PROXY_ERROR_CONNECT);

                return IMCOMM_RET_ERROR;
        }
        sin.sin_family = AF_INET;
        sin.sin_port = htons(port);

        if (he == NULL)
                sin.sin_addr = ina;
        else
                sin.sin_addr = *((struct in_addr *) he->h_addr);

        memset(&(sin.sin_zero), 0, 8);

        if (connect(h->socket, (struct sockaddr *) & sin, sizeof(struct sockaddr)) == -1) {
                if (h->callbacks[IMCOMM_ERROR])
                        h->callbacks[IMCOMM_ERROR] (handle, IMCOMM_ERROR_PROXY,
                                                    PROXY_ERROR_CONNECT);

                return IMCOMM_RET_ERROR;
        }
	h->ssl_handle = ssl_init(h->socket);
	h->use_ssl = 1;
}
