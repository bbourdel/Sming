/**
 * Please, note, that in order to run this sample you should recompile Sming with ENABLE_SSL=1.
 * The following three commands should be enough:
 *
 * cd Sming/Sming
 * make clean
 * make ENABLE_SSL=1
 */

#include <user_config.h>
#include <SmingCore/SmingCore.h>

#include "Network/WebClient.h"

// If you want, you can define WiFi settings globally in Eclipse Environment Variables
#ifndef WIFI_SSID
	#define WIFI_SSID "PleaseEnterSSID" // Put you SSID and Password here
	#define WIFI_PWD "PleaseEnterPass"
#endif

Timer procTimer;
WebClient downloadClient;

/* Debug SSL functions */
void displaySessionId(SSL *ssl)
{
    int i;
    const uint8_t *session_id = ssl_get_session_id(ssl);
    int sess_id_size = ssl_get_session_id_size(ssl);

    if (sess_id_size > 0)
    {
        debugf("-----BEGIN SSL SESSION PARAMETERS-----");
        for (i = 0; i < sess_id_size; i++)
        {
        	m_printf("%02x", session_id[i]);
        }

        debugf("\n-----END SSL SESSION PARAMETERS-----");
    }
}

/**
 * Display what cipher we are using
 */
void displayCipher(SSL *ssl)
{
	m_printf("CIPHER is ");
    switch (ssl_get_cipher_id(ssl))
    {
        case SSL_AES128_SHA:
        	m_printf("AES128-SHA");
            break;

        case SSL_AES256_SHA:
        	m_printf("AES256-SHA");
            break;

        case SSL_AES128_SHA256:
        	m_printf("SSL_AES128_SHA256");
            break;

        case SSL_AES256_SHA256:
        	m_printf("SSL_AES256_SHA256");
            break;

        default:
        	m_printf("Unknown - %d", ssl_get_cipher_id(ssl));
            break;
    }

    m_printf("\n");
}

void onDownload(HttpConnection& client, bool success)
{
	debugf("Got response code: %d", client.getResponseCode());
	debugf("Got content starting with: %s", client.getResponseString().substring(0, 50).c_str());
	debugf("Success: %d", success);

	SSL* ssl = client.getSsl();
	if (ssl) {
		const char *common_name = ssl_get_cert_dn(ssl,SSL_X509_CERT_COMMON_NAME);
		if (common_name) {
			debugf("Common Name:\t\t\t%s\n", common_name);
		}
		displayCipher(ssl);
		displaySessionId(ssl);
	}
}

void connectOk()
{
	const uint8_t googleSha1Fingerprint[] = { 0x07, 0xf0, 0xb0, 0x8d, 0x41, 0xfb, 0xee, 0x6b, 0x34, 0xfb,
			                                  0x9a, 0xd0, 0x9a, 0xa7, 0x73, 0xab, 0xcc, 0x8b, 0xb2, 0x64 };

	const uint8_t googlePublicKeyFingerprint[] = {
			0xe7, 0x06, 0x09, 0xc7, 0xef, 0xb0, 0x69, 0xe8, 0x0a, 0xeb, 0x21, 0x16, 0x4c, 0xd4, 0x2d, 0x86,
			0x65, 0x09, 0x62, 0x37, 0xeb, 0x75, 0x92, 0xaa, 0x10, 0x03, 0xe7, 0x99, 0x01, 0x9d, 0x9f, 0x0c
	};

	debugf("Connected. Got IP: %s", WifiStation.getIP().toString().c_str());

	/*
	 * Notice: If we use SSL we need to set the SSL settings only for the first request
	 * 		   and all consecutive requests to the same host:port will try to reuse those settings
	 */

	downloadClient.send(
			downloadClient.request("https://www.google.com/")
			->setSslOptions(SSL_SERVER_VERIFY_LATER)
			/*
			 * The line below shows how to trust only a certificate in which the public key matches the SHA256 fingerprint.
			 * When google changes the private key that they use in their certificate the SHA256 fingerprint should not match any longer.
			 */
			->pinCertificate(googlePublicKeyFingerprint, eSFT_PkSha256)
			/*
			 * The line below shows how to trust only a certificate that matches the SHA1 fingerprint.
			 * When google changes their certificate the SHA1 fingerprint should not match any longer.
			 */
			->pinCertificate(googleSha1Fingerprint, eSFT_CertSha1)
			->onRequestComplete(onDownload)
	);
	downloadClient.downloadString("https://www.google.com/services/", onDownload);
	downloadClient.downloadString("https://www.google.com/intl/en/policies/privacy/", onDownload);

	// The code above should make ONE tcp connection, ONE SSL handshake and then all requests should be pipelined to the
	// remote server taking care to speed the fetching of a page as fast as possible.

	// If we create a second web client instance it will create a new TCP connection and will try to reuse the SSL session id
	// from previous connections
	WebClient secondClient;
	secondClient.send(
			secondClient.request("https://www.google.com/")
			->setMethod(HTTP_POST)
			->setBody("q=sming+framework")
			->onRequestComplete(onDownload)
	);
}

void connectFail()
{
	debugf("I'm NOT CONNECTED!");
	WifiStation.waitConnection(connectOk, 10, connectFail); // Repeat and check again
}

void init()
{
	Serial.begin(SERIAL_BAUD_RATE);
	Serial.systemDebugOutput(true); // Allow debug print to serial
	Serial.println("Ready for SSL tests");

	// Setup the WIFI connection
	WifiStation.enable(true);
	WifiStation.config(WIFI_SSID, WIFI_PWD); // Put you SSID and Password here

	// Run our method when station was connected to AP (or not connected)
	WifiStation.waitConnection(connectOk, 30, connectFail); // We recommend 20+ seconds at start
}
