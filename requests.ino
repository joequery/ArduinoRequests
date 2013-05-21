#include <SPI.h>
#include "Requests.h"

void t(int cond, const __FlashStringHelper *msg){
    if(!cond)
        Serial.print(F("FAILED: "));
    else
        Serial.print(F("OK: "));
    Serial.println(msg);
}

void setup(){
    Serial.begin(9600);

    /* ===============================================================
     * Constants to use throughout testing
     * ===============================================================*/
    byte macAddress[] = { 0x90, 0xA2, 0xDA, 0x0D, 0xD2, 0x9C };
    // Set the static IP address to use if the DHCP fails to assign
    IPAddress ip(10,0,10,157);
    EthernetClient client;

    struct HTTPConfig conf = {
        client, macAddress, ip, TIMEOUT_FAST
    };

    // Initialize ethernet from configuration
    init_ethernet(conf);
    delay(1000); // Give serial time to come up

    // Initialize httpobject
    struct HTTPObject g;
    char buf[500];
    g.body = buf;
    g.bufSize = sizeof buf;

    /* ===============================================================
    * Test GET Requests
    * ===============================================================*/
    Serial.println(F("\nTest GET Requests"));
    g.host = "httpbin.org";
    g.path = "/status/200";
     HTTPGetRequest(&g, conf);

    t(g.statusCode == 200, F("status code test 1"));
    t(g.bodyLength == 0, F("empty body response test"));

    // Test body results
    g.host = "s3.amazonaws.com";
    g.path = "/regexmisc/testing.txt";
    HTTPGetRequest(&g, conf);

    t(strcmp("testing", g.body) == 0, F("correct body content"));
    t(g.bodyLength == 7, F("correct body size"));
    t(g.statusCode == 200, F("status code test 2"));

    /* ===============================================================
    * Test POST Requests
    * ===============================================================*/
    Serial.println(F("\nTest POST Requests"));
    g.host = "httpbin.org";
    g.path = "/post";
    char *data = "key1=val1&key2=val2";
    HTTPPostRequest(&g, conf, data);

    t(g.bodyLength > 300, F("correct body size"));
    t(g.statusCode == 200, F("status code test"));


    HTTPPostRequest(&g, conf, NULL);
    t(g.statusCode == 200, F("NULL data works"));

}
void loop(){
}
