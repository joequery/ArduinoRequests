/*
 * HTTP Requests Library for use with Arduino Uno. Inspired by the Python
 * Requests library.
 */
#ifndef ARD_REQUESTS_H
#define ARD_REQUESTS_H

#include <Ethernet.h>
#include <utility/w5100.h>

#define MAX_RETRIES 5

enum ConnectionTimeout { 
    TIMEOUT_FAST = 2, 
    TIMEOUT_MED  = 4,
    TIMEOUT_SLOW = 6
};

struct HTTPObject{
    char *host;
    char *path;
    char *body;
    size_t bufSize;
    unsigned int statusCode;
    int bodyLength;
};

struct HTTPConfig{
    EthernetClient client;
    byte *macAddress;
    IPAddress ip;
    ConnectionTimeout failSpeed;
};

void
HTTPGetRequest(struct HTTPObject *h, struct HTTPConfig conf);

void
HTTPPostRequest(struct HTTPObject *h, struct HTTPConfig conf, char *data);

void
init_ethernet(struct HTTPConfig conf);

#endif
