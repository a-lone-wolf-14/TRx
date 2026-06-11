# TRx

A lightweight Python communication suite designed for data transmission (Tx) and reception (Rx). This repository provides core scripts to establish a communication link and debug controller inputs, specifically tailored for robotic applications and autonomous systems.

## Overview

The **TRx** project facilitates a simple transmitter-receiver architecture. It is built to handle real-time data flow, making it suitable for projects involving remote-controlled systems or sensor data telemetry.

### Key Components:
- **`Tx.py`**: The Transmitter script. Responsible for packaging and sending data over a specified communication protocol.
- **`Rx.py`**: The Receiver script. Handles the reception of incoming data packets and prepares them for processing.
- **`controller_debug.py`**: A utility tool for testing and debugging controller mappings and input signals to ensure reliable communication before deployment.

##  Installation

**Clone the repository:**
```bash
git clone [https://github.com/a-lone-wolf-14/TRx.git](https://github.com/a-lone-wolf-14/TRx.git)
cd TRx
```

## Quick Start
**Recommended:** Before running the script on either Linux or Windows, run ```controller_debug.py``` file to know the axes and the buttons being changed according to their respective numbers mapped to the controller.
To run any script, use this command:
```bash
python <filename.py>
```
or
```bash
python3 <filename.py>
```

Let's say that you are using python 3.xx but you also have multiple versions installed where all the versions do not have the dependencies installed for running these files smoothly. To run the python file on a specific version:

```bash
py -3.xx <filename.py>
```
