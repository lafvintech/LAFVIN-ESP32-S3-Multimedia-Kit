.. _environment_setup:

===================
Environment Setup
===================

This section guides you through setting up the development environment: installing Arduino IDE, adding ESP32 board support, and installing the required libraries.

Install Arduino IDE
===================

**Step 1: Download Arduino IDE**

Visit the `Arduino Software page <https://www.arduino.cc/en/software>`_ and download the latest Arduino IDE for your operating system (Windows, macOS, or Linux).

**Step 2: Install**

Windows
^^^^^^^

#. Double-click the ``arduino-ide_xxxx.exe`` installer.
#. Accept the License Agreement.
#. Select installation options and choose an install location (avoid the system drive if possible).
#. Complete the installation.

macOS
^^^^^

Double-click the ``arduino_ide_xxxx.dmg`` file and drag **Arduino IDE.app** into the **Applications** folder.

Linux
^^^^^

Refer to the official `Linux Installation Guide <https://docs.arduino.cc/software/ide-v2/tutorials/getting-started/ide-v2-downloading-and-installing#linux>`_.

**Step 3: First Launch**

When you first launch Arduino IDE, it will automatically install built-in libraries and board support files. Allow any firewall prompts for device driver installation.

Install ESP32 Board Support
===========================

#. Open Arduino IDE, go to **File > Preferences** (or **Arduino IDE > Settings** on macOS).

#. In the **Additional Boards Manager URLs** field, paste the following URL::

      https://espressif.github.io/arduino-esp32/package_esp32_index.json

#. Click **OK** to save.

#. Go to **Tools > Board > Boards Manager**, search for **"esp32"**.

#. Find **"esp32 by Espressif Systems"** and click **Install**.

#. Wait for the installation to complete.

Select the Correct Board
========================

#. Connect your ESP32-S3 board to your computer via USB Type-C.

#. In Arduino IDE, go to **Tools > Board > ESP32 Arduino**, and select **ESP32S3 Dev Module**.

#. Configure the following settings under the **Tools** menu:

   =============================== =====================
   Setting                         Value
   =============================== =====================
   USB CDC On Boot                 Enabled
   USB Mode                        Hardware CDC and JTAG
   USB Firmware MSC On Boot        Disabled
   Upload Mode                     UART0 / Hardware CDC
   PSRAM                           OPI PSRAM
   Flash Size                      16MB (128Mb)
   Partition Scheme                16M Flash (3M APP/9.9M FATFS)
   CPU Frequency                   240MHz (WiFi)
   =============================== =====================

Install Required Libraries
==========================

Open **Tools > Manage Libraries** (or **Sketch > Include Library > Manage Libraries**) and install the following libraries:

.. list-table::
   :header-rows: 1
   :widths: 30 40

   * - Library
     - Purpose
   * - **TFT_eSPI** by Bodmer
     - TFT LCD display driver
   * - **lvgl** by kisvegabor
     - Light and Versatile Graphics Library
   * - **FT6336U** by LAFVIN
     - FT6336U touch panel driver
   * - **ESP32-audioI2S** by schreibfaul1
     - I2S audio playback
   * - **SparkFun MAX3010x** by SparkFun
     - Heart rate & pulse oximeter sensor

.. note::
   Make sure you install the exact library names listed above. Some libraries may have similar names from different authors.

.. important::
   After installing **TFT_eSPI**, you need to configure the ``User_Setup.h`` file to match the display pins used by this kit. The required configuration file is provided in the source code package.
