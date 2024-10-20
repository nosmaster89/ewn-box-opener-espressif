document.addEventListener('DOMContentLoaded', function () {
    fetch('/header')
        .then(response => response.text())
        .then(data => {
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
        })
        .catch(error => console.error('Error loading header:', error));
});

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