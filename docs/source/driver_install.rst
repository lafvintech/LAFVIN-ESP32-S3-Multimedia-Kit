.. _driver_install:

=========================
CH343 Driver Installation
=========================

The ESP32-S3 board uses a **CH343** USB-to-UART bridge chip. Depending on your operating system, you may need to install the driver for your computer to recognize the board.

Download CH343 Driver
=====================

The CH343 driver is included in the source code package under the ``0.CH343Driver`` folder:

* **Windows**: ``CH343SER.EXE``
* **macOS**: ``CH34XSER_MAC.ZIP``
* **Linux**: ``ch343ser_linux.zip``

Installation by Platform
========================

Windows
^^^^^^^

#. Double-click ``CH343SER.EXE`` to launch the installer.

#. Click **Install** and wait for the process to complete.

#. Once finished, connect your ESP32-S3 board. Open **Device Manager**, and under **Ports (COM & LPT)**, you should see a new COM port (e.g., ``USB-SERIAL CH343 (COMx)``).

macOS
^^^^^

#. Unzip ``CH34XSER_MAC.ZIP``.

#. Run the installer package (``.pkg`` file) and follow the on-screen instructions.

#. After installation, connect the board. Open **Terminal** and run::

      ls /dev/cu.*

   You should see a device like ``/dev/cu.usbserial-xxxx``.

Linux
^^^^^

#. Unzip ``ch343ser_linux.zip``.

#. Most modern Linux distributions include the CH343 driver in the kernel. If your board is not recognized, build and install the driver from the source package.

#. Connect the board and run::

      ls /dev/ttyUSB* /dev/ttyACM*

   The board should appear as ``/dev/ttyUSB0`` or ``/dev/ttyACM0``.

Verification
============

#. Connect the ESP32-S3 board to your computer via USB Type-C.

#. Open Arduino IDE and go to **Tools > Port**.

#. You should see the board's serial port listed (``COMx`` on Windows, ``/dev/cu.usbserial-xxxx`` on macOS, ``/dev/ttyUSB0`` or ``/dev/ttyACM0`` on Linux).

#. The port name should include "CH343" or "USB Serial". Select it and you're ready to upload code.
