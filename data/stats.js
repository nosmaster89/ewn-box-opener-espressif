// stats.js

function convertUTCToLocal(utcTimestamp) {
    const date = new Date(utcTimestamp * 1000); // Convert from seconds to milliseconds
    return date.toLocaleString(); // Convert to local string
}

function populateStats() {
    const devnet = "Your DevNet Value"; // Replace with actual value
    const ip = "192.168.1.1"; // Replace with actual value
    const uptime = Math.floor(millis() / 1000 / 60 / 60); // Replace with actual uptime
    const lastRequestTime = 1697430000; // Example UTC timestamp in seconds
    const lastRequestStatus = "Success"; // Replace with actual status
    const apiKey = "your_api_key"; // Replace with actual API key
    const version = "1.0"; // Replace with actual version

    document.getElementById('devnet').innerText = devnet;
    document.getElementById('ip').innerText = ip;
    document.getElementById('uptime').innerText = uptime;
    document.getElementById('lastRequestTime').innerText = convertUTCToLocal(lastRequestTime);
    document.getElementById('lastRequestStatus').innerText = lastRequestStatus;
    document.getElementById('apiKey').innerText = apiKey;
    document.getElementById('version').innerText = version;
}

window.onload = populateStats;