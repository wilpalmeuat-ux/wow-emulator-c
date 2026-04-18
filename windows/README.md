# WoW 3.3.5a Emulator — Windows Build Guide

## Prerequisites

### 1. Visual Studio 2019 or 2022
Download from: https://visualstudio.microsoft.com/

During installation, select:
- **Desktop development with C++**
- **Windows 11 SDK** (or Windows 10 SDK)

### 2. MySQL Connector/C 8.0
Download from: https://dev.mysql.com/downloads/connector/c/

Install to the default path: `C:\mysql`

If installed elsewhere, update `build.bat` and `install.bat` with your path.

### 3. MySQL Server 8.0 (to run the database)
Download from: https://dev.mysql.com/downloads/mysql/

Install MySQL Server 8.0. You need the server, not just the Connector/C library.

---

## Build

### Option A: Visual Studio GUI
1. Open `windows/WoWEmulator.sln` in Visual Studio
2. Set configuration to **Release** and **x64**
3. Build → Build Solution (Ctrl+Shift+B)

### Option B: Command Line
1. Open "x64 Native Tools Command Prompt for VS 2022"
2. Navigate to `windows\build.bat`
3. Run `build.bat`

---

## Install

1. Run `install.bat` as Administrator
2. Follow the prompts
3. MySQL Connector/C will be checked automatically
4. If not found, the installer will offer to download it

---

## Database Setup

1. Make sure MySQL Server is running (default port 3306)
2. Create the database:

```cmd
mysql -u root -p < C:\WoWEmulator\configs\create_db.sql
```

3. Edit `C:\WoWEmulator\configs\worldserver.conf` and set your MySQL password:

```ini
[database]
Type = "mysql"
Host = "127.0.0.1"
Port = 3306
User = "root"
Password = "YOUR_PASSWORD_HERE"
Database = "wow_emulator"
```

---

## Run

```
C:\WoWEmulator\run.bat
```

Or double-click `wow-emulator.exe` in the `C:\WoWEmulator` folder.

---

## Connect Client

In your `realmlist.wtf` on the client:

```
set realmlist 127.0.0.1:8085
```

---

## Default Account

- Username: **test**
- Password: **testpassword**

---

## WSS Scripting

The emulator uses a Word Statement Scripting system. Scripts are `.wss` files in the `scripts/` folder. Example:

```
set boss_health = 10000
when creature spawns do
    print "A boss has appeared!"
    set boss_health = 10000
end
when player attacks do
    set boss_health = boss_health - 50
    if boss_health is less than 5000 then
        broadcast "Phase 2!"
    end
end
```
