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
$(pwd)/Astyle/bin/astyle.exe --options=$(pwd)/AStyle/file/linux.ini -R ./*.c,*.h --exclude=AStyle --formatted
```

**License**
---

This project is licensed under the GNU General Public License v2.0.

Some portions of the code are derived from the libnvme project,
which is licensed under the LGPL-2.1-or-later license.

In accordance with LGPL terms, this project redistributes those portions under the GPLv2.
