# SteelSeriesSvc
![version: 1.0](https://img.shields.io/badge/version-1.0-blue.svg)
![license: GNU GPL v2](https://img.shields.io/badge/license-GNU_GPL_v2-brightgreen.svg)
![works: on my machine](https://img.shields.io/badge/works-on_my_machine-brightgreen.svg)
#### SteelSeries Integration Service
> 🔆 Tints MSI SteelSeries keyboards with Windows accent color.

## What's this?
This is a tiny utility that synchronizes the Windows accent/Aero color and your SteelSeries keyboard tint in
real time.

It's actually a Windows program that runs (preferably) as a service.

## Quick start
1. [Download this](https://github.com/my-nick-is-taken/SteelSeriesSvc) and extract **in any folder not directly writable
by you(e.g. `C:\Program Files`, `C:\Windows`, etc.)**.
2. Open an elevated command prompt and run `SteelSeriesSvc install`.
3. Start the service on `services.msc` or run `net start SteelSeriesSvc`. Alternatively, you can just log out
   and in again.

The service is set to run automatically with Windows. You can change some settings by using the standard `sc` utility
or using the `services.msc` snap-in.

## Compatibility
This software has only been tested on the latest Windows 10 April build, but will theoretically work in any
Windows OS starting from Windows 7.

Only the MSI laptop keyboard has been tested and I'm not sure it will work with other SteelSeries keyboards,
as I don't know if they share the same HID features or hardware IDs. Pull requests welcome.

Also the PID/VID matching is "a bit" hacky, but it _Works On My Machine™_. Again, pull requests more than welcome.

## Security
By default, the service runs as the `.\LocalSystem` account. This is probably okay, but if you are overly
paranoid there's a way you can work around that. More on that just right below.

## Usage and Installation
[Grab the latest release](https://github.com/my-nick-is-taken/SteelSeriesSvc/releases) and extract it to somewhere kinda safe.
Like, you don't want the executable to be in a user writable location, because it will be executed as `SYSTEM`
by default. Not good.

### As a standalone program
```
> cd <your safe directory where the .exe file is>
> SteelSeriesSvc start
```

That's it. It will keep running until you issue a `Ctrl-C` or close the console window.

### As A Service™
Open an elevated command prompt and
```
> cd <the directory containing the .exe file>
> SteelSeriesSvc install
> net start SteelSeriesSvc
```

Congrats! You just installed and started the service!
It runs automatically so you don't have to worry about changing its start type or anything.

#### Running as your user account
Because the service runs as `SYSTEM` user, it has "some extra" privileges that may affect security under some
circumstances (i.e. the .exe file is replaced by something else)
You can use `services.msc` to change the user account the service runs on or type the following command in an
elevated prompt:
```
> sc config SteelSeriesSvc obj=<your user name> password=<optional>
```

Restart the service and it should be up and running, with no extra privileges.

### Uninstalling the service
Execute the following command in an elevated prompt:
```
> sc delete SteelSeriesSvc
```

Voilà! No more integration. You can now safely remove the .exe file, too.

## Building
In a Native Tools Command Prompt, Visual Studio Command Prompt or equivalent:
```
> git clone https://github.com/my-nick-is-taken/SteelSeriesSvc
> cd SteelSeriesSvc
> MSBuild /p:Configuration=Release
```

Or just clone and open the `SteelSeriesSvc.sln` file and build from Visual Studio.

## Open-source code
This software has no external dependencies, and is subject to the terms of the GNU GPL v2 license.
You can find a copy of the license terms in the `LICENSE.md` file.

```
SteelSeriesSvc -- Tints MSI SteelSeries keyboards with Windows accent color
Copyright (c) 2018 Angel P.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
```

