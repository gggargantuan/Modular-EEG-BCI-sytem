# Modular-EEG-BCI-sytem

This repository contains the firmware for a BCI system based on modular EEG hardware.

The EEG device is designed as a low-cost, open-hardware module using commercially available off-the-shelf (COTS) components. in order to meet the core research requirements for brain-computer interface (BCI) studies while bypassing the high costs associated with clinical-grade equipment. Incorporating a precision instrumentation amplifier, the module provides a scalable and accessible starting point for work on neural commands and BCI signal processing. The system supports up to 8 channels with a resolution of 16 to 24 bits and a sampling rate of up to 1 kHz, featuring instrumentation amplifiers, active shielding and noise rejection, as well as wireless connectivity for real-time signal recording and visualization. 


"8ch_EEGmodule" contains the firmware for basic data acquisition.
All folders contaning "LSL" contains code for streaming the data via Lab Streaming Layer protocol, intended to use with specialized bio-signal processing software like OpenViBE.
