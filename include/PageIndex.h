#ifndef PAGE_INDEX_H
#define PAGE_INDEX_H

const char MAIN_page[] PROGMEM = R"=====(

<!DOCTYPE html>
<html>
  <head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <title>Weigh My Bru</title>
    <style>
      html {
        font-family: Arial;
        display: inline-block;
        margin: 0px auto;
        text-align: center;
      }
      body {
        margin: 0;
        display: flex;
        min-height: 100vh;
      }
      .sidebar {
        width: 240px;
        background: #00878F;
        color: #fff;
        display: flex;
        flex-direction: column;
        align-items: flex-start;
        padding-top: 30px;
        min-height: 100vh;
        position: fixed;
        left: 0;
        top: 0;
        z-index: 2;
      }
      .sidebar h2 {
        font-size: 1.6rem;
        margin: 0 0 30px 30px;
        font-family: consolas;
        color: #fff;
      }
      .nav-link {
        display: block;
        color: #fff;
        text-decoration: none;
        padding: 15px 0 15px 30px;
        width: 100%;
        font-size: 1.1rem;
        transition: background 0.2s;
        box-sizing: border-box;
        border-radius: 0 25px 25px 0;
      }
      .nav-link:hover, .nav-link.active {
        background: #005e63;
        box-shadow: none;
      }
      .main-content {
        margin-left: 240px;
        flex: 1;
        padding: 30px 20px 20px 20px;
        background: #f7f7f7;
        min-height: 100vh;
      }
      h1 {
        font-size: 1.5rem;
        color: #00878F;
        font-family: consolas;
      }
      /* Existing styles for canvas, buttons, etc. */
      canvas {
        border: 2px solid #dcdcdc;
        border-radius: 25px;
      }
      button {
        background: #00878F;
        color: #fff;
        border: none;
        border-radius: 5px;
        padding: 10px 30px;
        font-size: 1rem;
        cursor: pointer;
        margin-top: 10px;
      }
      button:hover {
        background: #005e63;
      }
      select {
        padding: 5px 10px;
        border-radius: 5px;
        border: 1px solid #ccc;
      }
    </style>
  </head>

  <body>
    <div class="sidebar">
      <h2>Weigh My Bru</h2>
      <a href="/" class="nav-link active" id="dashboard-link">Dashboard</a>
      <a href="#" class="nav-link" onclick="showPage('settings', this)">Settings</a>
      <a href="/calibration" class="nav-link" id="calibration-link">Calibration</a>
      <a href="#" class="nav-link" onclick="showPage('updates', this)">Updates</a>
    </div>
    <div class="main-content">
      <div id="dashboard" class="page-section">
        <h1>Dashboard</h1>
        <canvas
          id="myCanvas"
          width="220"
          height="200"
        ></canvas>
        <p>Min = 0 g &nbsp;&nbsp;&nbsp; Max = 300 g</p>
        <button type="button" onclick="tareScale()">TARE</button>
        <br /><br />
        <label for="units">Convert to :</label>
        <select name="units" id="units" onchange="Convert()">
          <option value="kg">oz</option>
          <option value="oz">kg</option>
        </select>
        <p style="font-size: 20px; color: #800000" id="conv"></p>
      </div>
      <div id="settings" class="page-section" style="display:none">
        <h1>Settings</h1>
        <p>Settings page content goes here.</p>
      </div>
      <div id="calibration" class="page-section" style="display:none">
        <h1>Calibration</h1>
        <p>Calibration page content goes here.</p>
      </div>
      <div id="updates" class="page-section" style="display:none">
        <h1>Updates</h1>
        <p>Current Firmware Version: <span id="fwver">Loading...</span></p>
        <p>Updates page content goes here.</p>
      </div>
    </div>
    <script>
      function showPage(page, el) {
        var sections = document.getElementsByClassName('page-section');
        for (var i = 0; i < sections.length; i++) {
          sections[i].style.display = 'none';
        }
        document.getElementById(page).style.display = '';
        var links = document.getElementsByClassName('nav-link');
        for (var i = 0; i < links.length; i++) {
          links[i].classList.remove('active');
        }
        el.classList.add('active');
        if(page === 'updates') fetchFirmwareVersion();
      }

      var getWeighingResults = 0; //--> Variable to get weighing results in grams from ESP32.
      var Circular_Progress_Bar_Val = 0;

      //////////////////////////////////////////////////////
      //---Processes the data received from the ESP32.---///
      //////////////////////////////////////////////////////
      if (!!window.EventSource) {
        var source = new EventSource("/events");

        source.addEventListener(
          "open",
          function (e) {
            console.log("Events Connected");
          },
          false
        );

        source.addEventListener(
          "error",
          function (e) {
            if (e.target.readyState != EventSource.OPEN) {
              console.log("Events Disconnected");
            }
          },
          false
        );

        source.addEventListener(
          "message",
          function (e) {
            console.log("message", e.data);
          },
          false
        );

        source.addEventListener(
          "allDataJSON",
          function (e) {
            console.log("allDataJSON", e.data);

            var obj = JSON.parse(e.data);

            getWeighingResults = obj.weight_In_g;

            if (getWeighingResults >= 0)
              Circular_Progress_Bar_Val = getWeighingResults;

            Convert();
          },
          false
        );
      }

      // Call the Convert() function.
      Convert();

      //////////////////////////////////////////////////////////////////////
      //---Convert weighing results from gram units to oz and kg units.---//
      //////////////////////////////////////////////////////////////////////
      function Convert() {
        var x = document.getElementById("units");
        var i = x.selectedIndex;

        if (x.options[i].text == "oz") {
          var oz_unit = getWeighingResults / 28.34952;
          oz_unit = oz_unit.toFixed(2);
          document.getElementById("conv").innerHTML = oz_unit + " oz";
        }

        if (x.options[i].text == "kg") {
          var kg_unit = getWeighingResults / 1000;
          kg_unit = kg_unit.toFixed(3);
          document.getElementById("conv").innerHTML = kg_unit + " kg";
        }
      }

      ////////////////////////////////////////////////////////////////////////////
      //---Displays the weighing value and displays a circular progress bar.---///
      ////////////////////////////////////////////////////////////////////////////
      // Source: https://www.tothenew.com/blog/tutorial-to-create-a-circular-progress-bar-with-canvas/
      var canvas = document.getElementById("myCanvas");
      var context = canvas.getContext("2d");
      var start = 4.7;
      var cw = context.canvas.width / 2;
      var ch = context.canvas.height / 2;
      var diff;

      var cnt = 0;
      var bar = setInterval(progressBar, 10);
      function progressBar() {
        var Circular_Progress_Bar_Val_Rslt = map(
          Circular_Progress_Bar_Val,
          0,
          300,
          0,
          100
        );
        Circular_Progress_Bar_Val_Rslt = Math.round(Circular_Progress_Bar_Val_Rslt);
        diff = (cnt / 100) * Math.PI * 2;
        context.clearRect(0, 0, 400, 200);
        context.beginPath();
        context.arc(cw, ch, 80, 0, 2 * Math.PI, false);
        context.fillStyle = "#FFF";
        context.fill();
        context.strokeStyle = "#f0f0f0";
        context.lineWidth = 11;
        context.stroke();
        context.fillStyle = "#000";
        context.strokeStyle = "#0583F2";
        context.textAlign = "center";
        context.lineWidth = 13;
        context.font = "20pt Verdana";
        context.beginPath();
        context.arc(cw, ch, 80, start, diff + start, false);
        context.stroke();
        context.fillStyle = "#8C7965";
        context.fillText(getWeighingResults + " g", cw, ch + 10);

        if (cnt < Circular_Progress_Bar_Val_Rslt) {
          cnt++;
        }

        if (cnt > Circular_Progress_Bar_Val_Rslt) {
          cnt--;
        }
      }
      //------------------------------------------------------------

      //------------------------------------------------------------Scale from weighed value to Progress bar value.
      // The weighing results are min = 0 and max = 300 in grams (max value is 300 because of your load cell) scaled to Progress bar values min = 0 and max = 100.
      // Source: https://www.arduino.cc/reference/en/language/functions/math/map/ , https://forum.jquery.com/topic/jquery-map-like-arduino-map
      function map(x, in_min, in_max, out_min, out_max) {
        return ((x - in_min) * (out_max - out_min)) / (in_max - in_min) + out_min;
      }
      //------------------------------------------------------------

      //------------------------------------------------------------ XMLHttpRequest to submit data.
      function tareScale() {
        var xhr = new XMLHttpRequest();
        xhr.open("POST", "/tare", true);
        xhr.send();
      }
      //------------------------------------------------------------

      // Fetch firmware version from the server and display it
      function fetchFirmwareVersion() {
        var xhr = new XMLHttpRequest();
        xhr.open("GET", "/fwver", true);
        xhr.onreadystatechange = function() {
          if (xhr.readyState == 4 && xhr.status == 200) {
            document.getElementById("fwver").innerText = xhr.responseText;
          }
        };
        xhr.send();
      }
    </script>
  </body>
</html>

)=====";

const char MAIN_calibration_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
  <head>
    <meta charset=\"UTF-8\" />
    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\" />
    <title>Scale Calibration</title>
    <style>
      html {
        font-family: Arial;
        display: inline-block;
        margin: 0px auto;
        text-align: center;
      }
      body {
        margin: 0;
        display: flex;
        min-height: 100vh;
      }
      .sidebar {
        width: 240px;
        background: #00878F;
        color: #fff;
        display: flex;
        flex-direction: column;
        align-items: flex-start;
        padding-top: 30px;
        min-height: 100vh;
        position: fixed;
        left: 0;
        top: 0;
        z-index: 2;
      }
      .sidebar h2 {
        font-size: 1.6rem;
        margin: 0 0 30px 30px;
        font-family: consolas;
        color: #fff;
      }
      .nav-link {
        display: block;
        color: #fff;
        text-decoration: none;
        padding: 15px 0 15px 30px;
        width: 100%;
        font-size: 1.1rem;
        transition: background 0.2s;
        box-sizing: border-box;
        border-radius: 0 25px 25px 0;
      }
      .nav-link:hover, .nav-link.active {
        background: #005e63;
        box-shadow: none;
      }
      .main-content {
        margin-left: 240px;
        flex: 1;
        padding: 30px 20px 20px 20px;
        background: #f7f7f7;
        min-height: 100vh;
        text-align: left;
      }
      h1, h2 {
        font-size: 1.5rem;
        color: #00878F;
        font-family: consolas;
        margin-bottom: 20px;
      }
      button {
        background: #00878F;
        color: #fff;
        border: none;
        border-radius: 5px;
        padding: 10px 30px;
        font-size: 1rem;
        cursor: pointer;
        margin-top: 10px;
      }
      button:hover {
        background: #005e63;
      }
      input[type=number] {
        padding: 8px;
        border-radius: 5px;
        border: 1px solid #ccc;
        font-size: 1rem;
        margin-right: 10px;
        width: 180px;
      }
      .status {
        margin-top: 20px;
        color: green;
        font-size: 1.1rem;
      }
      ol {
        margin-bottom: 30px;
      }
    </style>
  </head>
  <body>
    <div class="sidebar">
      <h2>Weigh My Bru</h2>
      <a href="/" class="nav-link">Dashboard</a>
      <a href="#" class="nav-link">Settings</a>
      <a href="/calibration" class="nav-link active">Calibration</a>
      <a href="#" class="nav-link">Updates</a>
    </div>
    <div class="main-content">
      <h2>Scale Calibration</h2>
      <ol>
        <li>Step 1: Remove all objects from the scale and click <b>Zero (Tare)</b>.</li>
        <li>Step 2: Place a known weight on the scale, enter its value below, and click <b>Set Calibration</b>.</li>
      </ol>
      <button onclick="tare()">Zero (Tare)</button>
      <br><br>
      <input type="number" id="knownWeight" placeholder="Known weight in grams">
      <button onclick="calibrate()">Set Calibration</button>
      <br><br>
      <input type="number" id="manualCF" placeholder="Manual calibration factor">
      <button onclick="setManualCF()">Save Calibration Factor</button>
      <br><br>
      <div><b>Current Calibration Factor:</b> <span id="currentCF">Loading...</span></div>
      <div class="status" id="status"></div>
    </div>
    <script>
      function tare() {
        document.getElementById('status').innerText = "Taring...";
        fetch('/tare').then(r => r.text()).then(t => {
          document.getElementById('status').innerText = t;
        });
      }
      function calibrate() {
        let weight = document.getElementById('knownWeight').value;
        if (!weight) {
          document.getElementById('status').innerText = "Please enter a known weight.";
          return;
        }
        document.getElementById('status').innerText = "Calibrating, please wait...";
        fetch('/calibrate?weight=' + weight)
          .then(r => r.text())
          .then(t => { document.getElementById('status').innerText = t; });
      }
      function fetchCurrentCF() {
        fetch('/getcf')
          .then(r => r.text())
          .then(t => { document.getElementById('currentCF').innerText = t; });
      }
      // Fetch current calibration factor on page load
      window.onload = function() { fetchCurrentCF(); };
      function setManualCF() {
        let cf = document.getElementById('manualCF').value;
        if (!cf) {
          document.getElementById('status').innerText = "Please enter a calibration factor.";
          return;
        }
        document.getElementById('status').innerText = "Saving calibration factor...";
        fetch('/setcf?cf=' + cf)
          .then(r => r.text())
          .then(t => {
            document.getElementById('status').innerText = t;
            fetchCurrentCF();
          });
      }
    </script>
  </body>
</html>
)=====";

#endif
