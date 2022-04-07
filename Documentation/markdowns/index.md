# Developers' Guide

## Requirements

* Git ([Download](https://git-scm.com/downloads))
* Visual Studio ([Download](https://visualstudio.microsoft.com/vs/whatsnew/))
* Python ([Download](https://www.python.org/downloads/))

{% hint style="warning" %}
Do not confuse [Visual Studio](https://visualstudio.microsoft.com) with [Visual Studio Code](https://code.visualstudio.com)
{% endhint %}

## Setting Up The Code

* First you need to do clone the [TerraForge3D](https://github.com/Jaysmito101/TerraForge3D) repository.

&#x20;Run :

```
git clone --recursive https://github.com/Jaysmito101/TerraForge3D.git
```

* Once it is cloned go to **scripts/** folder. And run the **Setup.bat.**
* Then you should see **TerraForge3D.sln** created in the root directory.

## Building the code

There are 2 ways to do this:

### With Visual Studio GUI

* Just open up the **TerraForge3D.sln** in Visual Studio
* Press **F5** or **CTRL+SHIFT+B** your built binary should be in **bin/Debug-windows-x86/TerraForge3D/TerraForge3D.exe**

### **From Command Line**

* Start up a developer command prompt (Press the windows button and search for **Developer Command Prompt**).
* Go to the TerraForge3D directory you cloned TerraForge3D in.
* Run:

```
SET Platform=
msbuild /m /p:PlatformTarget=x86 /p:Configuration=Debug TerraForge3D.sln
```

* Again your built exe should be in **bin/Debug-windows-x86/TerraForge3D/TerraForge3D.exe**

****

## Coding Conventions

* [PascalCase](https://techterms.com/definition/pascalcase) for Function/Class names.
* [CamelCase](https://en.wikipedia.org/wiki/Camel\_case) for variable names.
* You should add comments with the code you write.

## Submitting A  Pull Request

Once everything is done you should commit it to your fork and then create pull request. It will be reviewed and you may be asked to make any changes if needed else it will be merged with TerraForge3D master.

## Don'ts

* Do not run the **TerraForge3D.exe** in the binaries folder at all(run the binary you built in the **bin/Debug-windows-x86/TerraForge3D/TerraForge3D.exe** all necessary data will be copied there automatically on build).
* Do not change anything in the Binaries folder if it is not the only option.

## Help

For any queries or requests : [https://discord.gg/9ebyYwSktZ](https://discord.gg/9ebyYwSktZ)