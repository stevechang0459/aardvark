**Aardvark**
===

Aardvark I2C Host Adapter Application

**Reference**
---

Aardvark Software API

<https://www.totalphase.com/products/aardvark-software-api/>

Aardvark I2C/SPI Host Adapter User Manual

<https://www.totalphase.com/support/articles/200468316-aardvark-i2c-spi-host-adapter-user-manual/>

**Compile the source code**
---

```bash
# Create dependency files for the source code
make depall
# Compile the source code
make
```

**Format the source code with Linux coding style**
---

```bash
# AStyle
$(pwd)/Astyle/bin/astyle.exe --options=$(pwd)/_astylerc -R ./*.c,*.h --exclude=AStyle --formatted
```
