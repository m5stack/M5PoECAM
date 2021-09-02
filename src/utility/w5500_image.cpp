#include <SPI.h>
#include <Ethernet.h>
#include "http_parser.h"
#include "esp_camera.h"
#include "camera_index.h"

#define STEAM_URL  "/stream"
#define TEST_URL "/"

byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};

IPAddress poe_addr;

EthernetServer server(80);

// used to image stream 
#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

bool led_flag = true;

void jpegStream(EthernetClient* client) {
  camera_fb_t * fb = NULL;
  Serial.println("Image stream satrt");
  client->println("HTTP/1.1 200 OK");
  client->printf("Content-Type: %s\r\n", _STREAM_CONTENT_TYPE);
  client->println("Content-Disposition: inline; filename=capture.jpg");
  client->println("Access-Control-Allow-Origin: *");
  client->println();
  for (;;) {
    int64_t fr_start = esp_timer_get_time();
    fb = esp_camera_fb_get();
    if (fb == NULL) {
      delay(100);
      continue ;
    }
    client->print(_STREAM_BOUNDARY);
    client->printf(_STREAM_PART, fb);
    int32_t to_sends = fb->len;
    int32_t now_sends = 0;
    uint8_t *out_buf = fb->buf;
    uint32_t packet_len = 8 * 1024;
    while (to_sends > 0) {
      now_sends = to_sends > packet_len ? packet_len : to_sends;
      if (client->write(out_buf, now_sends) == 0) {
        goto client_exit;
      }
      out_buf += now_sends;
      to_sends -= packet_len;
    }
    int64_t fr_end = esp_timer_get_time();
    uint32_t speed = 1000 * 8 * fb->len / (fr_end - fr_start);
    Serial.printf("JPG: %uB %ums, speed: %u kbps/s \r\n", (uint32_t)(fb->len), (uint32_t)((fr_end - fr_start)/1000), speed);
    esp_camera_fb_return(fb);
    fb = NULL;

    // if (digitalRead(37))
    // {
    //   digitalWrite(0, 1);
    // }
    // else{
    //   digitalWrite(0, 0);
    // }
  }

client_exit:
  if (fb != NULL) {
    esp_camera_fb_return(fb);
  }
  client->stop();
  Serial.printf("Image stream end\r\n");
}

void respond404(EthernetClient* client) {
  Serial.print("Respond 404 start\r\n");
  char string_out[] = "Page not found\r\n";
  client->setConnectionTimeout(100);
  client->println("HTTP/1.1 404 OK");
  client->println("Content-Type: text/plain");
  client->println("Connection: close");
  client->printf("Content-Length: %d\r\n", strlen(string_out));
  client->println();
  client->write(string_out, strlen(string_out));
  delay(1);
  client->stop();
  Serial.print("Respond 404 End\r\n");
}

void respondTest(EthernetClient* client) {
  Serial.print("Respond Test start\r\n");
  char string_out[] = "Cool W5500\r\n";
  client->setConnectionTimeout(100);
  client->println("HTTP/1.1 200 OK");
  client->println("Content-Type: text/plain");
  client->println("Connection: close");
  client->printf("Content-Length: %d\r\n", strlen(string_out));
  client->println();
  client->write(string_out, strlen(string_out));
  delay(1);
  client->stop();
  Serial.print("Respond Test End\r\n");
}

int on_message_begin(http_parser* _) {
  (void)_;
  Serial.printf("\n***MESSAGE BEGIN***\r\n\r\n");
  return 0;
}

int on_message_complete(http_parser* _) {
  (void)_;
  Serial.printf("\n***MESSAGE COMPLETE***\r\n");
  return 0;
}

int on_url(http_parser* parser, const char* at, size_t length) {
  EthernetClient* client = (EthernetClient*)parser->data;
  if (length == strlen("/stream") && strncmp(at, "/stream", length) == 0) {
    jpegStream(client);
  } else if (length == strlen("/") && strncmp(at, "/", length) == 0) {
    respondTest(client);
  } else {
    respond404(client);
  }

  Serial.printf("Url: %.*s\r\n", (int)length, at);
  return 0;
}

void w5500ImageTask(void *arg) {
  (void)arg;
  server.begin();
  Serial.printf("Poe Listening url: \r\n");
  Serial.printf("%s%s -> image stream \r\n", poe_addr.toString().c_str(), STEAM_URL);

  enum http_parser_type file_type = HTTP_BOTH;
  http_parser_settings settings;
  memset(&settings, 0, sizeof(settings));
  settings.on_message_begin = on_message_begin;
  settings.on_message_complete = on_message_complete;
  settings.on_url = on_url;
  settings.on_header_field = NULL;
  settings.on_header_value = NULL;
  settings.on_headers_complete = NULL;
  settings.on_body = NULL;
  http_parser parser;
  http_parser_init(&parser, file_type);
  uint8_t buffer[2048];
  for (;;) {
    EthernetClient client = server.available();
    if (client) {
      // read client data
      uint32_t len = client.available();
      // ignore to long http header
      if (len > 2048) {
        client.stop();
        continue ;
      }
      len = client.readBytes(buffer, len);
      parser.data = &client;
      // deal http data
      http_parser_execute(&parser, &settings, (char*)buffer, len);
    }
    delay(10);
  }
}

int token_on_body(http_parser* parser, const char* at, size_t length) {
  asprintf((char **)parser->data, "%.*s", length - 2, at + 1);
  return 0;
}

bool w5500GotToken(const char* url, char **data, uint32_t* len) {
  const char* url_detail;
  url_detail = strpbrk(url, "//");
  if (url_detail == NULL) {
    url_detail = url;
  } else {
    url_detail += 2;
  }


  char* post_path;
  post_path = strchr(url_detail, '/');
  
  char* host;
  asprintf(&host, "%.*s", post_path - url_detail, url_detail);
  
  EthernetClient client;
  if (!client.connect(host, 80)) {
    free(host);
    Serial.printf("Connect server error\r\n");
    return false;
  }
  
  char *params = NULL;
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  asprintf(&params, "{\"mac\": \"%02x:%02x:%02x:%02x:%02x:%02x\"}", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  client.printf("POST %s HTTP/1.1\r\n", post_path);
  client.printf("HOST: %s\r\n", host);
  client.printf("Connection: close\r\n");
  client.printf("Content-Length: %d\r\n", strlen(params));
  client.printf("Cache-Control: no-cache\r\n");
  client.printf("Content-Type: text/plain\r\n");
  client.println();
  client.write(params, strlen(params));

  // Wait tcp status to close wait
  while (client.status() == 0x17) {
    delay(10);
  }

  enum http_parser_type file_type = HTTP_RESPONSE;
  http_parser_settings settings;
  memset(&settings, 0, sizeof(settings));
  settings.on_message_begin = NULL;
  settings.on_message_complete = NULL;
  settings.on_url = NULL;
  settings.on_header_field = NULL;
  settings.on_header_value = NULL;
  settings.on_headers_complete = NULL;
  settings.on_body = token_on_body;
  http_parser parser;
  http_parser_init(&parser, file_type);

  parser.data = data;
  uint8_t buffer[2048];
  uint32_t length = client.available();
  bool result = false;
  if (length < 2048) {
    client.readBytes(buffer, length);
    if (http_parser_execute(&parser, &settings, (char*)buffer, length) == length) {
      result = true;
      *len = strlen(*data);
    }
  }
  client.stop();
  free(params);
  free(host);
  return result;
}

void w5500PostImage(const char* url, const char* tok, const uint8_t *data, uint32_t len, uint32_t voltage) {
  if (data == NULL) {
    return ;
  }

  const char* url_detail;
  url_detail = strpbrk(url, "//");
  if (url_detail == NULL) {
    url_detail = url;
  } else {
    url_detail += 2;
  }

  char* post_path;
  post_path = strchr(url_detail, '/');
  
  char* host;
  asprintf(&host, "%.*s", post_path - url_detail, url_detail);

  EthernetClient client;
  if (!client.connect(host, 80)) {
    free(host);
    Serial.printf("Connect server error\r\n");
    return ;
  }
  
  int32_t to_sends = len;
  int32_t now_sends = 0;
  const uint8_t *out_buf = data;
  uint32_t packet_len = 8 * 1024;

  client.printf("POST %s?tok=%s&voltage=%d HTTP/1.0\r\n", post_path, tok, voltage);
  client.printf("HOST: %s\r\n", host);
  client.printf("Connection: close\r\n");
  client.printf("Cache-Control: no-cache\r\n");
  client.printf("Content-Length: %d\r\n", len);
  client.printf("Content-Type: application/binary\r\n");
  client.println();

  while (to_sends > 0) {
    now_sends = to_sends > packet_len ? packet_len : to_sends;
    if (client.write(out_buf, now_sends) == 0) {
      goto client_exit;
    }
    out_buf += now_sends;
    to_sends -= packet_len;
  }

client_exit:
  delay(10);
  free(host);
  client.stop();
}

void w5500Init() {
  SPI.begin(23, 38, 13, -1);
  Ethernet.init(4);
  
  // wait poe connect 
  for (;;) {
    auto link = Ethernet.linkStatus();
    if (link == Unknown) {

      Serial.printf("W5500 maybe error\r\n");
    } else if (link == LinkON) {
      break ;
    } else {
      Serial.printf("Poe wait connect\r\n");
    }
    delay(500);
  }
  Serial.printf("Poe connect successed\r\n");
  Ethernet.begin(mac);

  Serial.print("Poe got local ip at ");
  poe_addr = Ethernet.localIP();
  Serial.println(poe_addr);
}

void w5500ImageLoop() {
  w5500ImageTask(NULL);
}