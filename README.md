# ModLoader
Mono Framework Interaction / Injection Library for .NET (C++/CLI).

## Features
- Core is in pure C++ for reliabilty and performance
- Native MonoProcess class is exposed to .NET throught a custom managed wrapper
- Allows making dependency injection
- Allows easy injection into Mono namespace
## Usage
```c#
Process target = Process.GetProcessesByName("EtG")[0];
using (MonoProcess mp = new MonoProcess(target.Id))
{
    int status = mp.LoadDependencyFrom("C:\\Users\\Sacracia\\Harmony.2.2.2.0\\net35\\0Harmony.dll");
    if (status == 0)
    {
        status = mp.LoadModFrom("ETGCheatMenu.dll", "ETGCheatMenu", "Loader", "Init");
        if (status == 0)
        {
            Console.WriteLine("Success!");
        }
    }
}
```
## Download
You can find the most recent releases here: https://github.com/Sacracia/ModLoader/releases