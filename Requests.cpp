/*
 * HTTP Requests Library for use with Arduino Uno. Inspired by the Python
 * Requests library.
 */
#include "Requests.h"

/*
 * Helper function prototypes
 */
static void init_HTTPResponse(struct HTTPObject*);
static byte connect_with_retry(EthernetClient*,byte ,const char *,uint16_t);
static void extract_client_response(struct HTTPObject *h, EthernetClient client);
static void skip_http_headers(EthernetClient client);
static byte finished_reading_headers(char * const c, EthernetClient client);
static void extract_status_code(struct HTTPObject *h, EthernetClient client);


/*
 * Initialize ethernet settings, including timeout
 */
void init_ethernet(struct HTTPConfig conf){
    if (Ethernet.begin(conf.macAddress) == 0){
        Ethernet.begin(conf.macAddress, conf.ip);
    }
    // Set client.connect() timeouts
    W5100.setRetransmissionCount(conf.failSpeed);
}


void
HTTPGetRequest(struct HTTPObject *h, struct HTTPConfig conf){
    init_HTTPResponse(h);
    const int BUFSIZE = 100;
    char buf[BUFSIZE];

    if (connect_with_retry(&conf.client, MAX_RETRIES, h->host, 80)){
        // Make an HTTP request:
        snprintf(buf, BUFSIZE, "GET %s HTTP/1.1", h->path);
        conf.client.println(buf);

        snprintf(buf, BUFSIZE, "Host: %s", h->host);
        conf.client.println(buf);
        conf.client.println("Connection: close");
        conf.client.println();

        extract_client_response(h, conf.client);
    }
}


void
HTTPPostRequest(struct HTTPObject *h, struct HTTPConfig conf, char *params){
    init_HTTPResponse(h);
    char buf[100];
    size_t len = 0;

    if(params != NULL)
       len = strlen(params);

    if (connect_with_retry(&conf.client, MAX_RETRIES, h->host, 80)){
        // Make an HTTP request:
        snprintf(buf, 100, "POST %s HTTP/1.1", h->path);
        conf.client.println(buf);

        snprintf(buf, 100, "Host: %s", h->host);
        conf.client.println(buf);

        conf.client.println("User-Agent: Arduino/1.0");
        conf.client.println("Connection: close");
        conf.client.println("Content-Type: application/x-www-form-urlencoded;");

        if(len > 0){
            conf.client.print("Content-Length: ");
            conf.client.println(len);
            conf.client.println();
            conf.client.println(params);
        }
        conf.client.println();

        extract_client_response(h, conf.client);
    }
}

/*
 * Helper method definitons
 */

/*
 * Try connecting the client *c to the *host at most `max` number of times.
 * Return 1 if a connection is made, 0 otherwise. We use a pointer to the
 * client here because otherwise connect seems to only persist through the
 * life of the function it was called in.
 */
static byte 
connect_with_retry(EthernetClient *c, byte max, const char *host, uint16_t port){
    unsigned int i;
    for(i=0; i<max; i++){
        if (c->connect(host, port) != 0)
            return 1;
    }
    return 0;
}

static void
init_HTTPResponse(struct HTTPObject *h){
    /* An initial body size of -1 helps us determine if no request was made
     * (due to connection) or no response was provided.
     */
    h->bodyLength = -1;
    h->body[0] = '\0';
    h->statusCode = 0;
}

// Helper function for determining if we having finished reading headers (and
// are now to the body of the HTTP response)
static byte 
finished_reading_headers(char * const c, EthernetClient client){
    // \r\n\r\n designates the end of the header and start of the body.
    // http://stackoverflow.com/a/11254057 (RFC2616 Sec4.1 'Message Types')
    char CRLF[] = {'\r', '\n', '\r', '\n'};
    static unsigned short j = 0;

    if(*c == CRLF[j])
        j++;
    else
        j = 0;

    // If we're at the end of the CRLF array, we're at the end of the headers.
    if(j == sizeof CRLF){
        return 1;
    }
    return 0;
}

/*
 * Read through all the headers of the HTTP Response stored in the client
 */
static void 
skip_http_headers(EthernetClient client){
    char c;
    byte allHeadersRead = 0;
    while(!allHeadersRead && client.connected()){
        if(client.available()){
            c = client.read();
            allHeadersRead = finished_reading_headers(&c, client);
        }
    }
}

static void 
extract_status_code(struct HTTPObject *h, EthernetClient client){
    unsigned int k,multiplier,j;
    j=0;
    char c;
    const byte STATUS_CODE_START_INDEX = 9;
    const byte STATUS_CODE_END_INDEX = 11;

    while(j<=STATUS_CODE_END_INDEX && client.connected()){
        if(client.available()){
            // We want to extract the 200 from "HTTP/1.1 200 OK". The 
            // status code starts at index 9 in the string and ends at 11
            c = client.read();
            if(j<STATUS_CODE_START_INDEX){
                j++;
            }
            else if(j>=STATUS_CODE_START_INDEX && j<=STATUS_CODE_END_INDEX){
                // code = 10^2 * str[9] + 10^1 * str[10] + 10^0 * str[11]
                // Do ghetto exponentiation here.
                multiplier = 1;
                for(k = 1; k <= 2 - (j - STATUS_CODE_START_INDEX); k++){
                    multiplier *= 10;
                }
                // c - '0' => char to int
                h->statusCode += (c - '0') * multiplier;
                j++;
            }
        }
    }
}

void extract_body(struct HTTPObject *h, EthernetClient client){
    char c;
    unsigned int bodyIdx = 0;

    while(bodyIdx < h->bufSize - 1 && client.connected()){
        if(client.available()){
            c = client.read();
            h->body[bodyIdx++] = c;
        }
    }
    h->body[bodyIdx] = '\0';
    h->bodyLength = bodyIdx;
}

/*
 * Read client response from an HTTP request into a buffer. Return the number of
 * characters written to buf (EXCLUDING null character);
 */
void
extract_client_response(struct HTTPObject *h, EthernetClient client){
    extract_status_code(h, client);
    skip_http_headers(client);
    extract_body(h, client);

    // Clean up the connection
    client.flush();
    client.stop();
}
