#  Configuration of DAC TAS2563

**Official datasheet:**
🔗 [https://www.ti.com/lit/gpn/tas2563](https://www.ti.com/lit/gpn/tas2563)

---

## Overview

This example demonstrates how to output a **1 kHz sine wave** using the **ESP32-S3** microcontroller and the **TAS2563** digital-to-analog converter (DAC).

Before running the example, make sure that:

* The **I²C interface** is properly configured.
* The DAC is initialized through the `TAS2563_Init()` function.
* The DAC has an **SDZ pin**, which controls switching between the **normal operating mode** and a **limited functionality mode**.
To enable the device, a **high logic level** must be applied to this pin. (In this example, the SDZ pin is connected to **GPIO_NUM_21**. If you are also using the **interrupt pin (IRQ)** and there is **no external pull-up resistor**, you can enable the **internal pull-up** through a DAC register.)

---

## DAC Initialization Description (`TAS2563_Init`)

### 1. Page Selection

```c
tas2563_write_reg(0, 0);
```

Selecting **page 0**, as it contains most of the control registers used during initialization.

---

### 2. Device Reset

```c
tas2563_write_reg(0x01, 0x01);
```

Performs a **software reset**, returning the DAC to its initial (shutdown) state.
This guarantees a known default configuration of all internal registers.

---

### 3. Enter Software Shutdown

```c
tas2563_write_reg(0x02, 0x0E);
```

Places the device in **software shutdown mode**.
This mode is used to safely modify clock and data format settings.

---

### 4. Input and Interface Configuration

| Register | Value  | Description                                                                          |
| -------- | ------ | ------------------------------------------------------------------------------------ |
| `0x04`   | `0x08` | Enables internal IRQ pull-up                                                         |
| `0x06`   | `0x09` | Sets sample rate to 44.1 / 48 kHz, enables auto TDM rate detection, FSYNC active low |
| `0x07`   | `0x00` | Standard I²S mode (left channel first)                                               |

---

### 5. Channel Mixing Configuration

```c
tas2563_write_reg(0x08, 0x30);
```

Configures **stereo downmixing**, combining `(L + R) / 2` into a single mono output channel.

> 💡 Alternative settings:
> `0x10` — Mono left channel
> `0x20` — Mono right channel

---

### 6. Enable DAC (Power ON)

```c
tas2563_write_reg(0x02, 0x00);
```

Exits software shutdown mode, **enabling the amplifier and DSP**.

---

### 7. DSP Gain Adjustment

Registers **0x0C–0x0F** (on **page 2**) define the **32-bit digital gain coefficient** used for volume control.

Example configuration:

```c
tas2563_write_reg(0, 2);   // Select page 2
tas2563_write_reg(0x0C, 0x16);
tas2563_write_reg(0x0D, 0x4E);
tas2563_write_reg(0x0E, 0xFB);
tas2563_write_reg(0x0F, 0xD6);
```

The value `0x164EFBD6` provides **moderate gain without distortion**.
For detailed information on adjusting digital volume, refer to TI’s discussion:
🔗 [Adjusting Digital Volume Control Using I²C (TI E2E Forum)](https://e2e.ti.com/support/audio-group/audio/f/audio-forum/928138/faq-tas2563-adjusting-digital-volume-control-using-i2c)

---

## Notes

* Always perform a software reset before reconfiguring the TAS2563.
* Avoid changing I²S or TDM formats while the device is active — use software shutdown mode for that.
* Refer to the TI documentation for additional register descriptions and power management details.

---
