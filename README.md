# Aardvark
Aardvark C Application Practice

Format the source code with Linux coding style
---
```bash
# MinGW
$(pwd)/Astyle/bin/astyle.exe --options=$(pwd)/_astylerc -R ./*.c,*.h --exclude=AStyle --formatted
```