# ModLoader

Console application for injecting mono assemblies and c++ dlls

### Features

Supports loading .net assemblies into both x86 / x64 processes.    
Supports loading c++ native dlls into both x86 / x64 processes.  

### Usage

create file config.cfg that contains commands for injector.  

First line must match a target process. Second line and further are commands. List of commands:

1.  “mono-dep path\_to\_file” - injecting .net dependency
2.  “mono-ex path\_to\_file namespace class method” - injecting .net assembly and executing static method
3.  “native path\_to\_file” - injecting native c++ dll

#### Example:

```plaintext
hollow_knight.exe
mono-dep 0Harmony.dll
mono-ex HollowKnight_v1.5.78_ModMenu.dll HollowKnightModMenu Mod Load
```
