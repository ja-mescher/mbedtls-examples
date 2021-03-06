/**
* \brief This tool is testing a connection to aws iot
*
* \copyright (c) 2017 Microchip Technology Inc. and its subsidiaries.
*            You may use this software and any derivatives exclusively with
*            Microchip products.
*
* \page License
*
* (c) 2017 Microchip Technology Inc. and its subsidiaries. You may use this
* software and any derivatives exclusively with Microchip products.
*
* THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
* EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
* WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
* PARTICULAR PURPOSE, OR ITS INTERACTION WITH MICROCHIP PRODUCTS, COMBINATION
* WITH ANY OTHER PRODUCTS, OR USE IN ANY APPLICATION.
*
* IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
* INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
* WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
* BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
* FULLEST EXTENT ALLOWED BY LAW, MICROCHIPS TOTAL LIABILITY ON ALL CLAIMS IN
* ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
* THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
*
* MICROCHIP PROVIDES THIS SOFTWARE CONDITIONALLY UPON YOUR ACCEPTANCE OF THESE
* TERMS.
*/


/* Configuration defines for the example application */



/* This is mbedtls boilerplate for library configuration */
#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

/* System Includes */
#include "stdio.h"
#include "stdlib.h"

/* From mbedtls */
#include "mbedtls/platform.h"
#include "mbedtls/debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"
#include "mbedtls/pk.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/ecdh.h"

/* From Cryptoauthlib */
#include "cryptoauthlib.h"
#include "atcacert/atcacert_client.h"
#include "mbedtls/atca_mbedtls_wrap.h"

/* Local Includes */
#include "cert_chain.h"

/* This is the value used to secure the pre-master secret generated by ECDH on 
    the device since it is being transfered over the bus */
uint8_t atca_io_protection_key[32] = {
    0x37, 0x80, 0xe6, 0x3d, 0x49, 0x68, 0xad, 0xe5,
    0xd8, 0x22, 0xc0, 0x13, 0xfc, 0xc3, 0x23, 0x84,
    0x5d, 0x1b, 0x56, 0x9f, 0xe7, 0x05, 0xb6, 0x00,
    0x06, 0xfe, 0xec, 0x14, 0x5a, 0x0d, 0xb1, 0xe3
};


static void my_debug(void *ctx, int level,
    const char *file, int line,
    const char *str)
{
    ((void)level);

    printf("%s:%04d: %s", file, line, str);
}

unsigned char hash[32] = {
    0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea, 0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23,
    0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c, 0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad
};

static int my_verify(void *data, mbedtls_x509_crt *crt, int depth, uint32_t *flags)
{
    //char buf[1024];
    //((void)data);

    //mbedtls_printf("\nVerify requested for (Depth %d):\n", depth);
    //mbedtls_x509_crt_info(buf, sizeof(buf) - 1, "", crt);
    //mbedtls_printf("%s", buf);

    //if ((*flags) == 0)
    //    mbedtls_printf("  This certificate has no flags\n");
    //else
    //{
    //    mbedtls_x509_crt_verify_info(buf, sizeof(buf), "  ! ", *flags);
    //    mbedtls_printf("%s\n", buf);
    //}

    return(0);
}

int atca_tests(void)
{
    ATCA_STATUS status;
    mbedtls_x509_crt cacert;
    mbedtls_pk_context pkey;
    mbedtls_x509_crt cert;
    int ret;
    mbedtls_ctr_drbg_context ctr_drbg;
    unsigned char buf[MBEDTLS_MPI_MAX_SIZE];
    size_t olen = 0;
    uint32_t flags;


    /* Start a session with the device */
    if (ATCA_SUCCESS != (status = atcab_init(&cfg_ateccx08a_kithid_default)))
    {
        printf("Failed to init: %d\n", status);
        goto exit;
    }

    /* ECDSA Sign/Verify */
    /* Convert to an mbedtls key */
    if (0 != atca_mbedtls_pk_init(&pkey, 0))
    {
        printf("Failed to parse key from device\n");
        goto exit;
    }

    printf("  . Generating ECDSA Signature...");
    if ((ret = mbedtls_pk_sign(&pkey, MBEDTLS_MD_SHA256, hash, 0, buf, &olen,
        mbedtls_ctr_drbg_random, &ctr_drbg)) != 0)
    {
        printf(" failed\n  ! mbedtls_pk_sign returned -0x%04x\n", -ret);
        goto exit;
    }
    printf(" ok\n");

    printf("  . Verifying ECDSA Signature...");
    if ((ret = mbedtls_pk_verify(&pkey, MBEDTLS_MD_SHA256, hash, 0,
        buf, olen)) != 0)
    {
        printf(" failed\n  ! mbedtls_pk_verify returned -0x%04x\n", -ret);
        goto exit;
    }
    printf(" ok\n");

    mbedtls_x509_crt_init(&cacert);
    mbedtls_x509_crt_init(&cert);

    /* Extract the device certificate and convert to mbedtls cert */
    if (0 != atca_mbedtls_cert_init(&cert, &g_cert_def_2_device))
    {
        printf("Failed to parse cert from device\n");
        goto exit;
    }

    /* Extract the signer certificate, convert, then attach to the chain */
    if (0 != atca_mbedtls_cert_init(&cacert, &g_cert_def_1_signer))
    {
        printf("Failed to parse cert from device\n");
        goto exit;
    }

    printf("  . Verifying X.509 certificate...");

    if ((ret = mbedtls_x509_crt_verify(&cert, &cacert, NULL, NULL, &flags,
        my_verify, NULL)) != 0)
    {
        char vrfy_buf[512];
        printf(" failed\n");
        mbedtls_x509_crt_verify_info(vrfy_buf, sizeof(vrfy_buf), "  ! ", flags);
        printf("%s\n", vrfy_buf);
        goto exit;
    }
    else
    {
        printf(" ok\n");
    }

exit:
    return 0;
}

int main(int argc, char *argv[])
{
    return atca_tests();
}
