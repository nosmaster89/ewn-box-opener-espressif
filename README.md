# Project Name: Erwin Official ESP Box Opener

## Table of Contents

- [Introduction](#introduction)
- [Installation](#installation)
- [Board Integration](#board-integration)
- [Usage](#usage)

## Introduction

Welcome to the Erwin project! This is the Offical ESP Box Opener project , get started at [Erwin Official](https://erwin.lol)!

## Installation

If you need help identifying the Type of board you have come to the discord Server and ask in the [install help](https://discord.com/channels/1276185861414977606/1290662713067962390) channel.

To install the project, follow these steps:

1. Select the correct release for your board in the Releases section.
2. Download the firmware binary.
3. Flash the firmware to your board using either:

   - The `esptool.py` tool.
   - PlatformIO.
   - [ESPHome](https://web.esphome.io/)

   If asked for a flash offset, use `0x0000`.

4. Reboot the board.
5. Connect to the board's Wi-Fi access point:
   - **SSID**: Erwin
   - **Password**: Erwin123
6. On connection, it should take you to the captive portal where you can configure the device. If the captive portal does not open, navigate to `erwin.local` or `192.168.4.1` in your browser.
7. Configure the device with your Wi-Fi credentials.
8. The device should now connect to your Wi-Fi network. You can now access the device at the IP address or [erwin.local](http://erwin.local) it received from your router.
9. You can now use the device to open your boxes!

## Board Integration

1. Fork the repository.
2. create a new branch for your board.
3. Add your board to the supported boards list in the README.md.
4. Complete the env: section in the platformio.ini file to match your board.
5. Create a pull request to add your board to the supported boards list.
