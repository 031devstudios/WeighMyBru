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
        background: rgb(58, 58, 58);
        box-shadow: none;
        transition: all 0.3s ease;
        border-radius: 10px
      }
      .main-content {
        margin-left: 240px;
        flex: 1;
        padding: 30px 20px 20px 20px;
        background: rgba(58, 58, 58);
        min-height: 100vh;
      }
      .container {
        display: block;
        position: fixed; /* or absolute */
        top: 5%;
        left: 50%;
        width: 400px;
        height: 250px;
        justify-content: center;
        background-color:rgb(160, 160, 160);
        border-radius: 18px;
        padding: 20px 60px;
        box-shadow: 0 0 6px rgba(0, 0, 0, 0.5);
      }
      h1 {
        font-size: 1.5rem;
        color: #00878F;
        font-family: consolas;
        text-shadow: 1px 1px 0.5px rgba(0, 0, 0, 1);
      }
      /* Existing styles for buttons, etc. */
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
      <a href="/" class="nav-link" id="dashboard-link">Dashboard</a>
      <a href="/settings" class="nav-link active" id="settings-link" onclick="return forceSettingsReload(event)">Settings</a>
      <a href="/calibration" class="nav-link" id="calibration-link">Calibration</a>
      <a href="/updates" class="nav-link" id="updates-link">Updates</a>
    </div>
    <div class="main-content">
    <div class="container">
      <div id="dashboard" class="page-section">
        <h1>Dashboard</h1>
        <div id="weightDisplay" style="font-size: 2.5rem; color: #00878F; margin: 40px 0; font-family: consolas;text-shadow: 1px 1px 0.5px rgba(0, 0, 0, 1);"></div>
        <button type="button" onclick="tareScale()">TARE</button>
        <br /><br />
        <p style="font-size: 20px; color: #800000" id="conv"></p>
      </div>
      <div id="settings" class="page-section" style="display:none">
        <h1>Settings</h1>
        <label for="resolutionSelect">Resolution:</label>
        <select id="resolutionSelect" onchange="setResolution(this.value)">
          <option value="0">No decimal</option>
          <option value="1">1 decimal place</option>
          <option value="2">2 decimal places</option>
        </select>
        <br><br>
        <label for="units">Display units:</label>
        <select name="units" id="units" onchange="onUnitsChange()">
          <option value="g">g</option>
          <option value="oz">oz</option>
          <option value="kg">kg</option>
          <option value="lb">lb</option>
        </select>
        <br><br>
        <label for="maxCapacity">Loadcell Full Load Capacity:</label>
        <input type="number" id="maxCapacity" min="1" step="any" style="width:100px;"> g
      </div>
      <div id="updates" class="page-section" style="display:none">
        <h1>Updates</h1>
        <p>Current Firmware Version: <span id="fwver">Loading...</span></p>
        <p>Updates page content goes here.</p>
      </div>
    </div>
    </div>
    <script>
      function setUnitsSelector(units) {
        var unitsSelect = document.getElementById('units');
        if (unitsSelect) {
          for (var i = 0; i < unitsSelect.options.length; i++) {
            if (unitsSelect.options[i].value === units) {
              unitsSelect.selectedIndex = i;
              break;
            }
          }
        }
      }
      function setUnitsHandler() {
        var unitsSelect = document.getElementById('units');
        if (unitsSelect) {
          unitsSelect.onchange = function() {
            var unit = unitsSelect.value;
            localStorage.setItem('units', unit);
            fetch('/setunits?units=' + unit);
            Convert.lastUnit = undefined; // force update
            updateWeightDisplay();
          };
        }
      }
      function loadUnitsAndSetSelector(cb) {
        fetch('/getunits').then(r => r.text()).then(function(units) {
          if (!units) units = localStorage.getItem('units') || 'g';
          setUnitsSelector(units);
          setUnitsHandler();
          localStorage.setItem('units', units); // Always sync localStorage to match selector
          if (cb) cb();
        }).catch(function() {
          var units = localStorage.getItem('units') || 'g';
          setUnitsSelector(units);
          setUnitsHandler();
          if (cb) cb();
        });
      }

      // SPA navigation: if user clicks Calibration, redirect to /calibration for full page load
      function showPage(page, el) {
        if(page === 'calibration') {
          window.location.href = '/calibration';
          return;
        }
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
        if(page === 'settings') {
          var res = localStorage.getItem('resolution') || '0';
          var resSelect = document.getElementById('resolutionSelect');
          if (resSelect) resSelect.value = res;
          loadUnitsAndSetSelector(function() {
            Convert.lastUnit = undefined;
            updateWeightDisplay();
          });
          setMaxCapacityHandler();
        } else {
          updateWeightDisplay();
        }
      }

      var getWeighingResults = 0; //--> Variable to get weighing results in grams from ESP32.
      var displayResolution = 0;

      // Helper: get displayResolution from localStorage or default
      function getDisplayResolution() {
        var res = localStorage.getItem('resolution');
        if (res === null || res === undefined) return 0;
        return parseInt(res);
      }

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

            // Always use the raw value from the JSON, do not round or parseFloat early
            getWeighingResults = obj.weight_In_g;

            if (Number(getWeighingResults) >= 0)
              Circular_Progress_Bar_Val = Number(getWeighingResults);

            updateWeightDisplay();
          },
          false
        );
      }

      // Call the Convert() function.
      Convert();

      //////////////////////////////////////////////////////////////////////
      //---Convert weighing results from gram units to oz and kg units.---//
      //////////////////////////////////////////////////////////////////////
      // Helper: convert grams to selected unit and return {value, unit}
      function getConvertedWeightAndUnit() {
        var unit = localStorage.getItem('units') || 'g';
        var val;
        if (unit == "oz") {
          val = Number(getWeighingResults) / 28.34952;
        } else if (unit == "kg") {
          val = Number(getWeighingResults) / 1000;
        } else if (unit == "lb") {
          val = Number(getWeighingResults) / 453.59237;
        } else {
          val = Number(getWeighingResults);
        }
        return { value: val, unit: unit };
      }

      function updateWeightDisplay() {
        var conv = getConvertedWeightAndUnit();
        var fixedVal = conv.value.toFixed(getDisplayResolution());
        document.getElementById("weightDisplay").innerHTML = fixedVal + " " + conv.unit;
      }

      function Convert() {
        var conv = getConvertedWeightAndUnit();
        var fixedVal = conv.value.toFixed(getDisplayResolution());
        document.getElementById("conv").innerHTML = ""; // Clear the old display
        updateWeightDisplay();
      }
      function displayUnit() {
        var x = document.getElementById("units");
        if (!x) return '';
        var i = x.selectedIndex;
        return x.options[i].value;
      }

      function tareScale() {
        var xhr = new XMLHttpRequest();
        xhr.open("POST", "/tare", true);
        xhr.send();
      }

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

      function setResolution(val) {
        displayResolution = parseInt(val);
        localStorage.setItem('resolution', displayResolution);
        Convert();
      }
      function updateDisplay() {
        Convert();
      }

      // Add this function to handle unit changes live
      function onUnitsChange() {
        Convert.lastUnit = undefined; // force update
        Convert();
      }

      // Add maxCapacity persistence and logic
      function setMaxCapacityHandler() {
        var maxInput = document.getElementById('maxCapacity');
        if (maxInput) {
          maxInput.value = localStorage.getItem('maxCapacity') || '300';
          maxInput.onchange = function() {
            var val = maxInput.value;
            if (val && !isNaN(val) && Number(val) > 0) {
              localStorage.setItem('maxCapacity', val);
            }
          };
        }
      }

      // On load, set theme and resolution from localStorage and show correct section
      window.addEventListener('DOMContentLoaded', function() {
        var theme = localStorage.getItem('theme') || (window.matchMedia('(prefers-color-scheme: dark)').matches ? 'dark' : 'light');
        setTheme(theme);
        var select = document.getElementById('themeSelect');
        if (select) select.value = theme;
        // Only set up Dashboard section by default
        var dashboardSection = document.getElementById('dashboard');
        var settingsSection = document.getElementById('settings');
        dashboardSection.style.display = '';
        settingsSection.style.display = 'none';
        // Set up Dashboard as active by default
        document.getElementById('dashboard-link').classList.add('active');
        document.getElementById('settings-link').classList.remove('active');
        // Always fetch settings from backend on load if on /settings
        if (window.location.pathname === '/settings') {
          fetchSettingsFromBackend(function() {
            var res = localStorage.getItem('resolution') || '0';
            displayResolution = parseInt(res);
            var resSelect = document.getElementById('resolutionSelect');
            if (resSelect) resSelect.value = res;
            loadUnitsAndSetSelector(function() {
              Convert.lastUnit = undefined;
              updateWeightDisplay();
            });
            setMaxCapacityHandler();
            settingsSection.style.display = '';
            dashboardSection.style.display = 'none';
            document.getElementById('settings-link').classList.add('active');
            document.getElementById('dashboard-link').classList.remove('active');
          });
        } else {
          // Set up settings fields from localStorage
          var res = localStorage.getItem('resolution') || '0';
          displayResolution = parseInt(res);
          var resSelect = document.getElementById('resolutionSelect');
          if (resSelect) resSelect.value = res;
          loadUnitsAndSetSelector(function() {
            Convert.lastUnit = undefined;
            updateWeightDisplay();
          });
          setMaxCapacityHandler();
        }
        setTimeout(function() {
          if (document.getElementById('calibration') && document.getElementById('calibration').style.display !== 'none') {
            fetchCurrentCF();
          }
        }, 100);
      });

      // SPA navigation: if user clicks Settings, show settings section and update fields
      document.getElementById('settings-link').addEventListener('click', function(e) {
        e.preventDefault();
        document.getElementById('dashboard').style.display = 'none';
        document.getElementById('settings').style.display = '';
        this.classList.add('active');
        document.getElementById('dashboard-link').classList.remove('active');
        // Always fetch settings from backend
        fetchSettingsFromBackend(function() {
          var res = localStorage.getItem('resolution') || '0';
          displayResolution = parseInt(res);
          var resSelect = document.getElementById('resolutionSelect');
          if (resSelect) resSelect.value = res;
          loadUnitsAndSetSelector(function() {
            Convert.lastUnit = undefined;
            updateWeightDisplay();
          });
          setMaxCapacityHandler();
        });
      });

      // SPA navigation: if user clicks Dashboard, show dashboard section
      document.getElementById('dashboard-link').addEventListener('click', function(e) {
        e.preventDefault();
        document.getElementById('dashboard').style.display = '';
        document.getElementById('settings').style.display = 'none';
        this.classList.add('active');
        document.getElementById('settings-link').classList.remove('active');
        updateWeightDisplay();
      });

      // On load, show the correct section based on which nav-link is active
      window.addEventListener('DOMContentLoaded', function() {
        var dashboardLink = document.getElementById('dashboard-link');
        var settingsLink = document.getElementById('settings-link');
        var dashboardSection = document.getElementById('dashboard');
        var settingsSection = document.getElementById('settings');
        if (dashboardLink && dashboardLink.classList.contains('active')) {
          dashboardSection.style.display = '';
          settingsSection.style.display = 'none';
        } else if (settingsLink && settingsLink.classList.contains('active')) {
          dashboardSection.style.display = 'none';
          settingsSection.style.display = '';
        }
      });

      function forceSettingsReload(e) {
        if (window.location.pathname !== '/settings') {
          // Always do a full page load to /settings
          window.location.href = '/settings';
          e.preventDefault();
          return false;
        }
        // If already on /settings, allow default
        return true;
      }

      // Always fetch settings from backend when showing settings page
      function fetchSettingsFromBackend(cb) {
        fetch('/settings?_=' + Date.now())
          .then(r => r.json())
          .then(function(data) {
            if (data) {
              if (data.resolution !== undefined) {
                localStorage.setItem('resolution', data.resolution);
                var resSelect = document.getElementById('resolutionSelect');
                if (resSelect) resSelect.value = data.resolution;
              }
              if (data.units) {
                localStorage.setItem('units', data.units);
                setUnitsSelector(data.units);
              }
              if (data.maxCapacity) {
                localStorage.setItem('maxCapacity', data.maxCapacity);
                var maxInput = document.getElementById('maxCapacity');
                if (maxInput) maxInput.value = data.maxCapacity;
              }
            }
            if (cb) cb();
          })
          .catch(function() { if (cb) cb(); });
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
        background: rgb(58, 58, 58);
        box-shadow: none;
        transition: all 0.3s ease;
        border-radius: 10px
      }
      .main-content {
        margin-left: 240px;
        flex: 1;
        padding: 30px 20px 20px 20px;
        background: rgba(58, 58, 58);
        min-height: 100vh;
      }
      .container {
        display: inline-block;              /* or omit entirely since it's default */
        background-color:rgb(160, 160, 160);
        border-radius: 18px;
        padding: 20px 60px;
        box-shadow: 0 0 6px rgba(0, 0, 0, 0.5);
      }
      h1, h2 {
        font-size: 1.5rem;
        color: #00878F;
        font-family: consolas;
        margin-bottom: 20px;
        text-shadow: 1px 1px 0.5px rgba(0, 0, 0, 1);
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
      <a href="/" class="nav-link" id="dashboard-link">Dashboard</a>
      <a href="/settings" class="nav-link" id="settings-link">Settings</a>
      <a href="/calibration" class="nav-link active" id="calibration-link">Calibration</a>
      <a href="/updates" class="nav-link" id="updates-link">Updates</a>
    </div>
    <div class="main-content">
    <div class="container">
      <h2>Scale Calibration</h2>
      <div style="margin-bottom: 30px; text-align:left; max-width:500px; margin-left:auto; margin-right:auto;">
        <div>Remove all objects from the scale and click <b>Zero (Tare)</b>.</div>
        <div style="margin-top:10px;">Place a known weight on the scale, enter its value in <b>Known weight in grams</b> field, and click <b>Set Calibration</b>.</div>
        <div style="margin-top:20px;">Once calibration has completed, copy and paste the <b>Current Calibration Factor</b> into the <b>Manual Calibration Factor</b> field and click <b>Save Calibration Factor</b>.</div>
      </div>
      <br><br>
      <br><br>
    <div class="container">
      <button onclick="tare()">Zero (Tare)</button>
      <br><br>
      <input type="number" id="knownWeight" placeholder="Known weight (grams)" style="width: 200px;">
      <br><br>
      <button onclick="calibrate()">Set Calibration</button>
      <br><br>
      <input type="number" id="manualCF" placeholder="Manual calibration factor" style="width: 200px;">
      <br><br>
      <button onclick="setManualCF()">Save Calibration Factor</button>
      <br><br>
      <div><b>Current Calibration Factor:</b> <span id="currentCF">Loading...</span></div>
      <div class="status" id="status"></div>
      <script>
    </div>
        // Fetch and display calibration factor immediately on page load
        fetch('/getcf?_=' + Date.now())
          .then(r => r.text())
          .then(function(t) {
            var cfSpan = document.getElementById('currentCF');
            var val = (t || '').trim();
            if (cfSpan) {
              // Always show the value as returned, even if 0
              cfSpan.innerText = val;
            }
          })
          .catch(function() {
            var cfSpan = document.getElementById('currentCF');
            if (cfSpan) cfSpan.innerText = 'Unavailable';
          });
      </script>
      </div>
    </div>
    <script>
      function tare() {
        var status = document.getElementById('status');
        if (status) status.innerText = "Taring...";
        fetch('/tare').then r => r.text()).then(t => {
          if (status) status.innerText = t;
        });
      }
      function calibrate() {
        let weight = document.getElementById('knownWeight').value;
        var status = document.getElementById('status');
        if (!weight) {
          if (status) status.innerText = "Please enter a known weight.";
          return;
        }
        if (status) status.innerText = "Calibrating, please wait...";
        fetch('/calibrate?weight=' + weight)
          .then(r => r.text())
          .then(t => { if (status) status.innerText = t; });
      }
      function fetchCurrentCF() {
        fetch('/getcf?_=' + Date.now()) // prevent caching
          .then(function(r) { return r.text(); })
          .then(function(t) {
            var cfSpan = document.getElementById('currentCF');
            if (cfSpan) {
              var val = (t || '').trim();
              if (val && !isNaN(Number(val)) && Math.abs(Number(val)) > 0.000001) {
                cfSpan.innerText = val;
              } else if (val === '0' || val === '0.000000' || Math.abs(Number(val)) <= 0.000001) {
                cfSpan.innerText = 'Not calibrated';
              } else {
                cfSpan.innerText = 'Unavailable';
              }
            }
          })
          .catch(function() {
            var cfSpan = document.getElementById('currentCF');
            if (cfSpan) cfSpan.innerText = 'Unavailable';
          });
      }
      function setManualCF() {
        let cf = document.getElementById('manualCF').value;
        var status = document.getElementById('status');
        if (!cf) {
          if (status) status.innerText = "Please enter a calibration factor.";
          return;
        }
        if (status) status.innerText = "Saving calibration factor...";
        fetch('/setcf?cf=' + encodeURIComponent(cf))
          .then(r => r.text())
          .then(t => {
            if (status) status.innerText = t;
            setTimeout(function() {
              fetchCurrentCF();
            }, 500);
          })
          .catch(function() {
            if (status) status.innerText = 'Error saving calibration factor.';
          });
      }

      // Fetch current calibration factor on page load and when calibration page is shown
      window.onload = function() { fetchCurrentCF(); };
    </script>
  </body>
</html>
)=====";

const char MAIN_updates_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
  <head>
    <meta charset=\"UTF-8\" />
    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\" />
    <title>Updates</title>
    <style>
      html {
        font-family: Arial;
        display: inline-block;
        margin: 0px auto;
        text-align: center;
      }
      body {
        margin: 0;
        background:rgb(209, 209, 209);
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
        background: rgb(58, 58, 58);
        box-shadow: none;
        transition: all 0.3s ease;
      }
      .main-content {
        margin-left: 240px;
        flex: 1;
        padding: 30px 20px 20px 20px;
        background:rgb(58, 58, 58);
        min-height: 100vh;
      }
      .container {
        display: inline-block;
        background-color:rgb(160, 160, 160);
        border-radius: 12px;
        padding: 25px 60px;
        box-shadow: 0 0 12px rgba(0, 0, 0, 0.5);
      }
      h1 {
        color: #00878F;
        font-family: consolas;
        text-shadow: 1px 1px 0.5px rgba(0, 0, 0, 1);
      }
    </style>
  </head>
  <body>
    <div class="sidebar">
      <h2>Weigh My Bru</h2>
      <a href="/" class="nav-link" id="dashboard-link">Dashboard</a>
      <a href="/settings" class="nav-link" id="settings-link">Settings</a>
      <a href="/calibration" class="nav-link" id="calibration-link">Calibration</a>
      <a href="/updates" class="nav-link active" id="updates-link">Updates</a>
    </div>
    <div class="main-content">
    <div class="container">
      <h1>Updates</h1>
      <p>Current Firmware Version: <span id="fwver">Loading...</span></p>
    </div>
  </div>
    <script>
      document.addEventListener('DOMContentLoaded', function() {
        fetch('/fwver')
          .then(r => r.text())
          .then(ver => { document.getElementById('fwver').innerText = ver; });
      });
    </script>
  </body>
</html>
)=====";

#endif
