# RHCP (Robotic Hand Control Panel)

Sanctuary AI's internal GUI for interacting with [Phoenix hand V3.x](https://sanctuaryai.atlassian.net/wiki/spaces/EMBD/pages/1115357218/WBH+V3). RHCP's features include positional encoder and tactile sensor visualization, as well as on-the-fly encoder programming over CAN. 

This GUI was created with [ImGui](https://github.com/ocornut/imgui) and [ImPlot](https://github.com/epezent/implot), and can be run on any flavour of Linux. At the moment, this application cannot be run on windows.


## Installation

Clone this repository to any location on your computer.

If you have not already done so, follow the instructions for installing the [PCAN Linux drivers](https://www.peak-system.com/fileadmin/media/linux/index.htm). 

Finally, install the two graphics library backends required by ImGui. 

```
sudo apt update
sudo apt install libgl1-mesa-dev
sudo apt install libglfw3-dev
```

## Usage

> The syntax of the examples below assume you are running ```app``` in the base project directory.

Once the PCAN is plugged in, set up the device using the following command.

```
sudo ip link set can0 up type can bitrate 1000000 dbitrate 2000000 fd on restart-ms 100
```

To start the GUI, run the `app` executable, located in the `bin` directory. 
```
    $./bin/app
```

Currently supported command line arguments are listed below.  

* To specify an FDCAN interface, use `-d <interface name>`. Default is `can0`
    ```
    $ ./bin/app -d can1
    ```
    
> Note: If you have not set up a CAN device, the GUI will prompt you to do so. If your PCAN is set up correctly (quickly blinking green LED), the GUI's visualizations for the hands' various sensors will appear.




