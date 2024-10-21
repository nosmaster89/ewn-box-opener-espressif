document.addEventListener('DOMContentLoaded', function () {
    fetch('/header')
        .then(response => response.text())
        .then(data => {
            let socket;
            // find any existing websocket



            if (typeof socket !== 'undefined' && socket.readyState === WebSocket.OPEN) {
                console.log("Using existing WebSocket in injected.js");

                socket.addEventListener('message', function (event) {
                    console.log("Message received in injected.js:", typeof event.data);
                    // convert the string to a JSON object
                    const e = JSON.parse(event.data);



                    const currentTime = e["current_time"];
                    if (currentTime) {
                        console.log("Updating time in injected.js:", currentTime);
                        // currenttime is a timestamp in seconds make it localtime 
                        const date = new Date(currentTime * 1000); // Convert from seconds to milliseconds
                        const ct = date.toLocaleString(); // Convert to local string
                        document.getElementById('time').innerHTML = ct;

                    }
                    else {
                        console.log("No current_time in message");
                    }
                });
            }
            else {
                console.log("Creating new WebSocket in injected.js");
                socket = new WebSocket('ws://' + window.location.hostname + '/ws');

                socket.addEventListener('message', function (event) {
                    console.log("Message received in injected.js:", event.data);
                });
            }


            document.getElementById('header').innerHTML = data;

            // Now that the header is loaded, we can safely call updateWiFiStatus
            console.log('Header loaded');
            const rssi = document.getElementById(`rssi`)
            if (rssi == null) {
                console.error('RSSI element not found');
                return;
            }
            var wifiSignal = parseInt(rssi.innerHTML); // RSSI value dynamically passed in
            updateWiFiStatus(wifiSignal);
            const time = document.getElementById(`time`)
            if (time == null) {
                console.error('Time element not found');
                return;
            }
            var timestamp = parseInt(time.innerHTML); // UTC timestamp dynamically passed in
            time.innerHTML = updateTime(timestamp);
        })
        .catch(error => console.error('Error loading header:', error));
});
function updateTime(time) {
    // time is a utc timestamp convert it to local time from the browser
    const date = new Date(time * 1000); // Convert from seconds to milliseconds
    return date.toLocaleString(); // Convert to local string
}

function updateWiFiStatus(rssi) {
    console.log('Updating WiFi status...');

    // Check if the elements exist now that the header is loaded
    const bars = document.querySelectorAll(`.wifi-bar`);



    var numberOfBars;
    var rssiNumber = parseInt(rssi);
    console.log('RSSI value as a number:', rssiNumber);

    // Determine the number of bars based on signal strength (RSSI)
    if (rssiNumber >= -50) {
        numberOfBars = 5; // Excellent signal
    } else if (rssiNumber >= -60) {
        numberOfBars = 4; // Good signal
    } else if (rssiNumber >= -70) {
        numberOfBars = 3; // Fair signal
    } else if (rssiNumber >= -80) {
        numberOfBars = 2; // Weak signal
    } else {
        numberOfBars = 1; // Very weak signal
    }
    console.log('Number of bars:', numberOfBars);
    console.log('Bars:', bars);

    // Update the bars' visibility based on signal strength
    for (var index = 0; index < bars.length; index++) {
        const bar = bars[index];
        if (index < numberOfBars) {
            bar.classList.add('connected');
        } else {
            bar.classList.remove('connected');
        }
        console.log('Bar', index, 'is connected:', index < numberOfBars);
    }

    // You can also update any RSSI value display here if needed
}