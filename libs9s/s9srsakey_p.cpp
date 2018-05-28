/*
 * Severalnines Tools
 * Copyright (C) 2016-2018 Severalnines AB
 *
 * This file is part of s9s-tools.
 *
 * s9s-tools is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * s9s-tools is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with s9s-tools. If not, see <http://www.gnu.org/licenses/>.
 */
#include "s9srsakey_p.h"

#include <openssl/bn.h>
#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/crypto.h>

S9sRsaKeyPrivate::S9sRsaKeyPrivate() :
    m_rsa(0),
    m_referenceCounter(1)
{
}

S9sRsaKeyPrivate::~S9sRsaKeyPrivate()
{
    release();
}

/**
 * Release all the loaded keys, data..
 */
void
S9sRsaKeyPrivate::release()
{
    if (m_rsa)
        RSA_free(m_rsa);
    m_rsa = 0;
    m_errorString = "";
}

void
S9sRsaKeyPrivate::ref()
{
    ++m_referenceCounter;
}

int
S9sRsaKeyPrivate::unRef()
{
    return --m_referenceCounter;
}

/**
 * Loads an RSA (private) key from a file.
 */
bool
S9sRsaKeyPrivate::loadFromFile(
        const S9sString     &path)
{
    release();

    BIO *bio = BIO_new_file(STR(path), "r");
    if (bio == 0)
    {
        m_errorString = "BIO_new_file failure, not enough memory?";
        return false;
    }

    m_rsa = 0;
    m_rsa = PEM_read_bio_RSAPrivateKey (bio, &m_rsa, 0, 0);
    BIO_free_all(bio);
    bio = 0;

    if (m_rsa == 0)
    {
        m_errorString = "PEM_read_bio_RSAPrivateKey failure.";
        return false;
    }

    return true;
}

/**
 * Generates a new 2048 bit strong RSA keypair.
 *
 * Please note: it must be enough cryptographic enthropy on the system (VM's
 * might need tools like 'haveged') otherwise this method may hang for a longer
 * period.
 */
bool
S9sRsaKeyPrivate::generateKeyPair()
{
    release();

    int      keySize = 2048;
    BIGNUM   *bigNum = BN_new();

    m_rsa = RSA_new ();

    BN_set_word (bigNum, (BN_ULONG) RSA_F4);
    if (RSA_generate_key_ex(m_rsa, keySize, bigNum, 0) == 0)
    {
        // error, not enough enthrophy?
        m_errorString = "RSA_generate_key_ex failure, not enough entrophy?";

        release();
        BN_free(bigNum);

        // key generation have failed
        return false;
    }   

    // this can be freed now
    BN_free(bigNum);

    return true;
}

/**
 * Save the generated (or loaded) key into the specified files
 */
bool
S9sRsaKeyPrivate::saveKeys(
        const S9sString &privateKeyPath,
        const S9sString &publicKeyPath)
{
    BIO *bio = 0;

    if (!isValid())
    {
        m_errorString = "No valid key loaded/generated.";
        return false;
    }

    // Private key

    bio = BIO_new_file(STR(privateKeyPath), "w");
    if (!bio) 
    {
        m_errorString.sprintf ("'%s' can't be opened for writing.",
                STR(privateKeyPath));
        return false;
    }

    // just save it unencrypted/unprotected
    PEM_write_bio_RSAPrivateKey(bio, m_rsa, 0, 0, 0, 0, 0);

    (void) BIO_flush(bio);
    BIO_free_all(bio);

    // Public key

    bio = BIO_new_file(STR(publicKeyPath), "w");
    if (!bio)
    {
        m_errorString.sprintf ("'%s' can't be opened for writing.",
                STR(publicKeyPath));
        return false;
    }

    PEM_write_bio_RSAPublicKey(bio, m_rsa);

    (void) BIO_flush(bio);
    BIO_free_all(bio);

    return true;
}

/**
 * Computes an RSA-SHA256 signature and returns it
 * in base64 encoded format (so it can be sent on RPC)
 */
bool
S9sRsaKeyPrivate::signRsaSha256(
        const S9sString     &input,
        S9sString           &signature)
{
    if (!isValid())
    {
        m_errorString = "No valid key loaded/generated.";
        return false;
    }

	// Set up an OpenSSL EVP context for hashing & signing

    EVP_PKEY *evpKey = EVP_PKEY_new();
    EVP_PKEY_set1_RSA(evpKey, m_rsa);

    EVP_MD_CTX *context = EVP_MD_CTX_create ();
    EVP_SignInit_ex (context, EVP_sha256(), NULL);
    EVP_SignUpdate (context, STR(input), input.size());

    uint siglen = 0u;
    EVP_SignFinal (context, NULL, &siglen, evpKey);
    unsigned char* sigstr = new unsigned char[siglen+1];
    EVP_SignFinal (context, sigstr, &siglen, evpKey);

	EVP_MD_CTX_destroy (context);
    EVP_PKEY_free (evpKey);

	BIO *bio, *base64;
	base64 = BIO_new(BIO_f_base64());
    // to have a one-liner base64 string
    BIO_set_flags(base64, BIO_FLAGS_BASE64_NO_NL);
    bio = BIO_new(BIO_s_mem());
    base64 = BIO_push(base64, bio);

    // write the signature into the Base64 encoder
    BIO_write (base64, sigstr, siglen);
    (void) BIO_flush (base64);
    delete[] sigstr;

    BUF_MEM *bioMem = 0;
    BIO_get_mem_ptr(base64, &bioMem);
    // push the data into signature
    signature = std::string(bioMem->data, bioMem->length);

    // cleanup
    BIO_free_all(base64);

    return true;
}

