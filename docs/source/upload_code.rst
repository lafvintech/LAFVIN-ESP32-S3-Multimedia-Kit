.. _upload_code:

========================
Upload Code to the Board
========================

This section explains how to open the example sketches and upload them to your ESP32-S3 board.

Download the Source Code
========================

The complete source code for all tutorials is available on GitHub:

* :download:`Download Source Code (ZIP) <https://github.com/lafvintech/LAFVIN-ESP32-S3-Multimedia-Kit/archive/refs/heads/main.zip>`

Alternatively, you can clone the repository using Git::

   git clone https://github.com/lafvintech/LAFVIN-ESP32-S3-Multimedia-Kit.git

The source code is organized as follows::

   课程代码文件/
   └── LAFVIN-ESP32-S3-Multimedia-Kit-main/
       ├── 0.CH343Driver/        # USB driver
       ├── 1.Arduino/            # 9 Arduino fundamental projects
       │   ├── 1.WS2812/
       │   ├── 2.ADC_Battery/
       │   ├── 3.CameraWebServer/
       │   ├── 4.SDMMC/
       │   ├── 5.1_Play_SDMMC/
       │   ├── 5.2_Play_Online/
       │   ├── 6.HeartRate/
       │   ├── 7.TFT_Watch/
       │   └── 8.Touch/
       ├── 2.LVGL/               # 15 LVGL advanced projects
       │   ├── 01_LVGL_Test/
       │   ├── ...
       │   └── 15_LVGL_All_In_One/
       └── lib/                  # Required libraries

Open a Project
==============

#. Launch Arduino IDE.

#. Go to **File > Open** (or press ``Ctrl+O``).

#. Navigate to the project folder (e.g., ``1.Arduino/1.WS2812``) and open the ``.ino`` file.

Configure Board and Port
========================

#. Connect your ESP32-S3 board to your computer via USB Type-C.

#. Select the board: **Tools > Board > ESP32 Arduino > ESP32S3 Dev Module**.

#. Select the port: **Tools > Port > USB-SERIAL CH343 (COMx)**.

Upload the Code
===============

#. Click the **Upload** button (right arrow icon) in the toolbar.

#. The IDE will compile the sketch and upload it to the board. You can monitor the progress in the output panel at the bottom.

#. When the upload completes, you should see **"Done uploading."** and the board will automatically reset and start running the new program.

Monitor Serial Output
=====================

Many examples print debug information to the Serial Monitor. To view it:

#. Go to **Tools > Serial Monitor** (or press ``Ctrl+Shift+M``).

#. Set the baud rate to **115200** (or the rate specified in the sketch).

#. You should see the program's output in the Serial Monitor window.

Troubleshooting
===============

Common issues and solutions:

* **Board not detected**: Make sure the CH343 driver is installed (see :ref:`driver_install`). Try a different USB cable or port.

* **Upload fails**: Hold the **BOOT** button on the board, press and release the **RST** button, then release **BOOT** to enter download mode. Click Upload again.

* **Compilation errors**: Ensure all required libraries are installed (see :ref:`environment_setup`). Check that you selected the correct board type.

* **Serial Monitor shows garbage**: Make sure the baud rate in Serial Monitor matches the baud rate set in the sketch (``Serial.begin(baudrate)``).
