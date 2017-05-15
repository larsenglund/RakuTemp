const char* websocketPageHead = R"(
<!DOCTYPE html><html><head><title>RakuTemp</title>
<script src='Chart.bundle.min.js'></script>
<style>
html * {
  font-family: Arial;
}
</style>
<script>
var waitingForData = false;
var numData = 0;
var dataIdx = 0;
var numChunks = 0;
var chunkIdx = 0;
var numDataAdded = 0;
var MAX_SAMPLES = 2048;
var connection = new WebSocket('ws://'+location.hostname+':81/', ['arduino']);
connection.binaryType = 'arraybuffer';
var s1 = new Uint16Array(MAX_SAMPLES);
connection.onopen = function () {  
  connection.send('Connect ' + new Date()); 
}; 
connection.onerror = function (error) {    
  console.log('WebSocket Error ', error);
};
connection.onmessage = function (e) {  
  console.log('onmsg type: ', typeof(e.data));
  console.log('onmsg data: ', e.data);
  
  if (typeof(e.data) == 'object') {
    var data = e.data;
    var dv = new DataView(data);
    /*var idx = dataIdx - numData;
    if (idx < 0) idx += MAX_SAMPLES;
    for (var n=0; n<numData; n++) {
      var tmp = dv.getUint16(idx*2, true);
      console.log("idx " + idx + ": " + tmp);
      if (++idx >= MAX_SAMPLES) idx = 0;
    }*/
    for (var n=0; n<MAX_SAMPLES/2; n++) {
      s1[chunkIdx++] = dv.getUint16(n*2, true);
    }
    if (chunkIdx == MAX_SAMPLES) {
      console.log('All data received');
      var idx = dataIdx - numData;
      if (idx < 0) idx += MAX_SAMPLES;
      for (var n=0; n<numData; n++) {
        var tmp = s1[idx];
        console.log("idx " + idx + ": " + tmp);
        if (++idx >= MAX_SAMPLES) idx = 0;
      }
      waitingForData = false;
    }
  }
  
  if (typeof(e.data) == 'string') {
    var tmp = e.data.split(' ');
  
    if (tmp[0] == "D") {
      waitingForData = true;
      cfg.data.labels = [];
      cfg.data.datasets[0].data = [];
      dataIdx = parseInt(tmp[1]);
      numData = parseInt(tmp[2]);
    }
    /*else if (tmp.length == 1) {
      var _temp = parseInt(tmp[0]);
      cfg.data.labels.push(numDataAdded);
      cfg.data.datasets[0].data.push(_temp);
      numDataAdded++;
      if (numDataAdded == numData) {
        console.log('All data received!');
        myChart.update();
        waitingForData = false;
      }
    }*/
    else if (tmp.length == 2) {
      //console.log('Server: ', e.data);
      var _temp = parseInt(tmp[0]);
      var _millis = parseInt(tmp[1]);
      if (waitingForData) {
        
      }
      else {
        console.log(_temp);
        document.getElementById('a').innerHTML = _temp;
        //document.getElementById('b').value = _millis;
        if(cfg.data.datasets[0].data.length == MAX_SAMPLES){
              //myChart.removeData();
              cfg.data.datasets[0].data.shift();
              cfg.data.labels.shift();
        }
        //myChart.addData([temp], 'RakuTemp');
        //cfg.data.labels.push(_millis);
        cfg.data.labels.push(numDataAdded);
        cfg.data.datasets[0].data.push(_temp);
        numDataAdded++;
        myChart.update();
      }
    }
  }
};
</script>)";

const char* websocketPageBody = R"(
<center style='font-size: 200%;'>
RakuTemp <b><span id='a'></span></b><br/>
<canvas id='myChart' width='400' height='400'></canvas>
</center><br/>
<script>
var ctx = document.getElementById('myChart');
var cfg = {
    type: 'line',
    data: {
        labels: [],
        datasets: [{
            radius: 0,
            fillColor : 'rgba(252,233,79,0.5)',
            strokeColor : 'rgba(82,75,25,1)',
            label: 'RakuTemp',
            data: []
        }]
    },
    options: {
      legend: {
            display: false
         },
      scales:
        {
            xAxes: [{
                display: false
            }]
        }
    }
};
var myChart = new Chart(ctx, cfg);
</script>)";

/** Handle root or redirect to captive portal */
void handleRoot() {
  if (captivePortal()) { // If caprive portal redirect instead of displaying the page.
    return;
  }
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
  server.sendContent(
    "<html><head>"
  );
  server.sendContent(websocketPageHead);
  server.sendContent(
    "</head><body>"
  );
  server.sendContent(websocketPageBody);
  if (server.client().localIP() == apIP) {
    server.sendContent(String("<p>You are connected through the soft AP: ") + softAP_ssid + "</p>");
  } else {
    server.sendContent(String("<p>You are connected through the wifi network: ") + ssid + "</p>");
  }
  server.sendContent(
    "<p>You may want to <a href='/wifi'>config the wifi connection</a> or <a href='/update'>update the firmware</a></p>"
    "</body></html>"
  );
  server.client().stop(); // Stop is needed because we sent no content length
}

/** Redirect to captive portal if we got a request for another domain. Return true in that case so the page handler do not try to handle the request again. */
boolean captivePortal() {
  if (!isIp(server.hostHeader()) && server.hostHeader() != (String(myHostname)+".local")) {
    Serial.print("Request redirected to captive portal");
    server.sendHeader("Location", String("http://") + toStringIp(server.client().localIP()), true);
    server.send ( 302, "text/plain", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
    server.client().stop(); // Stop is needed because we sent no content length
    return true;
  }
  return false;
}

/** Wifi config page handler */
void handleWifi() {
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
  server.sendContent(
    "<html><head></head><body>"
    "<h1>Wifi config</h1>"
  );
  if (server.client().localIP() == apIP) {
    server.sendContent(String("<p>You are connected through the soft AP: ") + softAP_ssid + "</p>");
  } else {
    server.sendContent(String("<p>You are connected through the wifi network: ") + ssid + "</p>");
  }
  server.sendContent(
    "\r\n<br />"
    "<table><tr><th align='left'>SoftAP config</th></tr>"
  );
  server.sendContent(String() + "<tr><td>SSID " + String(softAP_ssid) + "</td></tr>");
  server.sendContent(String() + "<tr><td>IP " + toStringIp(WiFi.softAPIP()) + "</td></tr>");
  server.sendContent(
    "</table>"
    "\r\n<br />"
    "<table><tr><th align='left'>WLAN config</th></tr>"
  );
  server.sendContent(String() + "<tr><td>SSID " + String(ssid) + "</td></tr>");
  server.sendContent(String() + "<tr><td>IP " + toStringIp(WiFi.localIP()) + "</td></tr>");
  server.sendContent(
    "</table>"
    "\r\n<br />"
    "<table><tr><th align='left'>WLAN list (refresh if any missing)</th></tr>"
  );
  Serial.println("scan start");
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n > 0) {
    for (int i = 0; i < n; i++) {
      server.sendContent(String() + "\r\n<tr><td>SSID " + WiFi.SSID(i) + String((WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":" *") + " (" + WiFi.RSSI(i) + ")</td></tr>");
    }
  } else {
    server.sendContent(String() + "<tr><td>No WLAN found</td></tr>");
  }
  server.sendContent(
    "</table>"
    "\r\n<br /><form method='POST' action='wifisave'><h4>Connect to network:</h4>"
    "<input type='text' placeholder='network' name='n'/>"
    "<br /><input type='password' placeholder='password' name='p'/>"
    "<br /><input type='submit' value='Connect/Disconnect'/></form>"
    "<p>You may want to <a href='/'>return to the home page</a>.</p>"
    "</body></html>"
  );
  server.client().stop(); // Stop is needed because we sent no content length
}

/** Handle the WLAN save form and redirect to WLAN config page again */
void handleWifiSave() {
  Serial.println("wifi save");
  server.arg("n").toCharArray(ssid, sizeof(ssid) - 1);
  server.arg("p").toCharArray(password, sizeof(password) - 1);
  server.sendHeader("Location", "wifi", true);
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.send ( 302, "text/plain", "");  // Empty content inhibits Content-length header so we have to close the socket ourselves.
  server.client().stop(); // Stop is needed because we sent no content length
  saveCredentials();
  connect = strlen(ssid) > 0; // Request WLAN connect with new credentials if there is a SSID
}

void handleNotFound() {
  if (captivePortal()) { // If caprive portal redirect instead of displaying the error page.
    return;
  }
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for ( uint8_t i = 0; i < server.args(); i++ ) {
    message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
  }
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.send ( 404, "text/plain", message );
}

