# Modbus Communication for Intel 8051

This project implements Modbus communication for the Intel 8051 microcontroller using the C programming language. It was developed in the Keil 5 development environment, targeting the AT89C51RC2 processor. The project focused on slave functionalities, with Modbus Pool used for simulating the master program and generating the Longitudinal Redundancy Check (LRC).

## Features

- **Modbus Slave Implementation**: Communicates with Modbus masters, responds to function codes (such as read coils, read holding registers, etc.).
- **LRC (Longitudinal Redundancy Check)**: Implemented to ensure data integrity during communication.
- **Keil 5 IDE**: Developed and tested using Keil 5 with the AT89C51RC2 processor.
  
## Requirements

- **Keil 5 IDE**: For building and compiling the project.
- **AT89C51RC2 Processor**: Microcontroller used for the implementation.
- **Modbus Pool**: Software used to simulate the Modbus master and generate LRC.
  
## Getting Started

1. Clone this repository to your local machine:
    ```bash
    git clone https://github.com/Grkila/MODBUS-for-8051.git
    ```

2. Open the project in Keil 5:
    - Launch Keil uVision 5 and open the project file (`.uvproj`).

3. Build the project:
    - Compile the project by selecting "Build" from the menu in Keil.

4. Upload to the AT89C51RC2:
    - Flash the compiled code to the AT89C51RC2 microcontroller using a suitable programmer.

5. Simulate the master using Modbus Pool:
    - Configure Modbus Pool to communicate with the slave device.

### Modbus Slave

- The slave listens for Modbus requests and responds with the appropriate data.
- The slave supports various function codes like:
  - Read Coils 
  - Read Holding Registers 
  - On timer for outputs
  - error codes
  - LRC
  
### Modbus Master

- The master (simulated via Modbus Pool) sends requests to the slave for reading and writing registers or coils.


## Acknowledgements

- Thanks to Modbus Pool for providing the software to simulate the master.
- The AT89C51RC2 processor documentation was invaluable for understanding the hardware features.



