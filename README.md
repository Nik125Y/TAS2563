# Configuration of DAC TAS2563

Official datasheet TAS2563
https://www.ti.com/lit/gpn/tas2563

This example demonstrates outputting a 1 kHz sine wave. It uses an ESP32-S3 microcontroller. To run the example, you need to configure the I2C configuration and initialize the DAC.
DAC Initialization Description (finction TAS2563_Init):

Sending value 0 to register 0 selects page 0, as it contains most of the registers we need.

Writing 0x01 = 0x01 resets the DAC, returning it to its initial state (Software Shutdown), ensuring that all registers are in their initial state.

Writing 0x02 = 0x0E puts the device into software shutdown.
This is necessary to safely change the clock signal and data format configuration.
- Input and Interface Configuration
0x04 = 0x08 - Enables the internal IRQ pullup.
0x06 = 0x09 - sets the sampling frequency to 44.1/48 kHz, auto detection of TDM sample rate, and FSYNC active level to low.

0x07 = 0x00 - selects standard I²S mode (left channel first).

- Channel mixing configuration
0x08 = 0x30 - sets the stereo processing mode, mixing (L+R)/2 into one channel;

(Note: you can also select only the left (0x10) or right (0x20) channel)
- DAC enable (Power ON)
0x02 = 0x00 - after configuration is complete, this entry enables power to the amplifier and DSP.

- DSP gain adjustment for audio control
Registers 0x0C–0x0F set the digital gain. They are located on page 2.
To control the sound, you can change the values ​​in these registers.
This example uses the 32-bit value 0x164EFBD6, which corresponds to moderate gain without distortion. For more information you can see also this discussion https://e2e.ti.com/support/audio-group/audio/f/audio-forum/928138/faq-tas2563-adjusting-digital-volume-control-using-i2c
