/*
  Source taken from: https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266HTTPUpdateServer
  Creditis to all autors !!!!

  Modifcations by tschaban https://github.com/tschaban
*/

#include <Arduino.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include "ota.h"
#include "sonoff-configuration.h"



const char* ESP8266HTTPUpdateServer::_serverIndex =
  R"(<html><body><form method='POST' action='' enctype='multipart/form-data'>
                  <input type='file' name='update'>
                  <input type='submit' value='Update firmware'>
               </form>
         </body></html>)";
const char* ESP8266HTTPUpdateServer::_failedResponse = R"(Update Failed!)";
const char* ESP8266HTTPUpdateServer::_successResponse = "<META http-equiv=\"refresh\" content=\"10;URL=/\"><h3>Upgrade was successful!</h3><p>Please wait while Sonoff is rebooting</p>";


ESP8266HTTPUpdateServer::ESP8266HTTPUpdateServer(bool serial_debug)
{
  _serial_output = serial_debug;
  _server = NULL;
  _username = NULL;
  _password = NULL;
  _authenticated = false;
}

void ESP8266HTTPUpdateServer::setup(ESP8266WebServer *server, const char * path, const char * username, const char * password)
{
  _server = server;
  _username = (char *)username;
  _password = (char *)password;

  // handler for the /update form page
  _server->on(path, HTTP_GET, [&]() {
    if (_username != NULL && _password != NULL && !_server->authenticate(_username, _password))
      return _server->requestAuthentication();

    String page =
      "<h3>Firmware upgrade</h3>"
      "<p style='margin-top:40px;'><strong>Do not disconnect Sonoff from power source during upgrade.</strong></p>"
      "<p style='margin-bottom:40px;'>After upgrade Sonoff will be automatically restarted. </p>"
      "<form style='margin-bottom: 100px;' method='POST' action='' enctype='multipart/form-data'>"
      "<input type='file' name='update'>"
      "<input type='submit' value='Upgrade'>"
      "</form>";

    _server->send(200, "text/html", PAGE_HEADER + page + PAGE_FOOTER);

  });

  // handler for the /update form POST (once file upload finishes)
  _server->on(path, HTTP_POST, [&]() {
    if (!_authenticated)
      return _server->requestAuthentication();
    _server->send(200, "text/html", Update.hasError() ? _failedResponse : _successResponse);
    ESP.restart();
  }, [&]() {
    // handler for the file upload, get's the sketch bytes, and writes
    // them through the Update object
    HTTPUpload& upload = _server->upload();
    if (upload.status == UPLOAD_FILE_START) {
      if (_serial_output)
        Serial.setDebugOutput(true);

      _authenticated = (_username == NULL || _password == NULL || _server->authenticate(_username, _password));
      if (!_authenticated) {
        if (_serial_output)
          Serial.printf("Unauthenticated Update\n");
        return;
      }

      WiFiUDP::stopAll();
      if (_serial_output)
        Serial.printf("Update: %s\n", upload.filename.c_str());
      uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
      if (!Update.begin(maxSketchSpace)) { //start with max available size
        if (_serial_output) Update.printError(Serial);
      }
    } else if (_authenticated && upload.status == UPLOAD_FILE_WRITE) {
      if (_serial_output) Serial.printf(".");
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        if (_serial_output) Update.printError(Serial);

      }
    } else if (_authenticated && upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
        if (_serial_output) Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      } else {
        if (_serial_output) Update.printError(Serial);
      }
      if (_serial_output) Serial.setDebugOutput(false);
    } else if (_authenticated && upload.status == UPLOAD_FILE_ABORTED) {
      Update.end();
      if (_serial_output) Serial.println("Update was aborted");
    }
    delay(0);
  });
}
