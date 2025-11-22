# Code-Block-Validation

# Code Block Validation – Qt C++ GUI Application

This repository contains a Qt-based C++ application for validating code blocks. Below is a step-by-step guide to install Qt, open the project, and run it—especially helpful for Windows users.

---

## 🛠️ Prerequisites

- Windows 10 or 11 (recommended)
- Internet connection
- Basic familiarity with C++ and Qt (optional)

---

## 📥 Step 1: Install Qt

1. Go to [https://www.qt.io/download](https://www.qt.io/download)
2. Download the **Qt Online Installer** (free for open-source use).
3. Run the installer and sign in or create a free Qt account.
4. In the component selector, choose:
   - **Qt 6.x.x** (or **Qt 5.15.x**)
   - **MinGW 11.2.0 64-bit** (includes compiler)
   - **Qt Creator** (IDE)
5. Complete the installation.

----

## 📂 Step 2: Get the Code

**Option A – Clone with Git:**
```bash
git clone https://github.com/RinJ07/Code-Block-Validation.git
```

**Option B – Download ZIP:**

- On GitHub, click Code → Download ZIP
- Extract the folder to a location like Documents/Code-Block-Validation

---
## Step 3: Open & Run in Qt Creator ##
-Open Qt Creator
-Go to File → Open File or Project
-Select the .pro file in your project folder
   _(If there’s no .pro file, see Appendix below)_
-Click the green Run (▶) button or press Ctrl+R

---
## Appendix: Create a .pro File (If Missing) ## 
If the project doesn’t include a .pro file, create one named **CodeBlockValidation.pro** in the root folder:

```c++
QT += core widgets
CONFIG += c++17
TEMPLATE = app
SOURCES += main.cpp \
           mainwindow.cpp
HEADERS += mainwindow.h
FORMS += mainwindow.ui

```
⚠️ Adjust the file list to match your actual source files.

---
🆘 Need Help?
Use Qt Creator (not VS Code) for easiest setup.
No extra libraries needed—Qt includes everything.
In Qt Creator, ensure your build kit uses the MinGW compiler you installed.
---

