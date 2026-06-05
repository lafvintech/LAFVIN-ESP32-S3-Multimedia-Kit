.. _preparation:

===========
Preparation
===========

This section covers the setup steps you should complete before starting the Arduino and LVGL tutorials.

The recommended order is:

1. Download the source code package and firmware files
2. Install the CH343 USB driver so your computer can detect the board
3. Install Arduino IDE and configure ESP32-S3 board support
4. Install the required local libraries from the provided ``lib/`` folder

After completing these steps, you can return to the main tutorial chapters and start uploading examples to the board.

.. note::
   Use a USB Type-C **data cable**. A charging-only cable can power the board but cannot be used for programming or serial communication.

.. toctree::
   :maxdepth: 2

   download_code
   install_driver
   arduino_ins
   lib_ins
