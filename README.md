# VMPImportFixer

VMPImportFixer is a tool aimed to resolve import calls in a VMProtect'd (3.x) binary.

# Information

VMPImportFixer attempts to resolve all obfuscated API calls in a VMProtect'd binary. A binary which has VMProtect's "Import Protection" option enabled will have all it's `CALL NEAR` instructions replaced with near relative call instructions (see [ImportCallObfuscator](https://github.com/mike1k/ImportCallObfuscator) for a similar method of obfuscating imports).

VMProtect usually has two different variations of import calls which seem to be chosen at random once the binary is protected. The first, being `push reg; call func`, and the other being `call func; ret/int3`.

![call/int3](https://i.imgur.com/X15Aps6.png)
![push/call](https://i.imgur.com/cgA8ecy.png)

Following these calls lead into the VMProtect section, which, by default is named `.vmp0`. Each stub can vary in complexity and size, however the concept is generally the same. Through a series of arithmetic which is used to calculate the real import address, the final operation usually sets `[rsp]`/`[esp]` to the import address before the final RET instruction.

Based on the variant of the call (`push reg; call func` or `call func; int3/ret`), the stub may increment the return address. This use of the extra byte and return address incrementing is used to break various decompilers from properly analyzing a function due to the decompiler not recognizing that the byte will be skipped over in runtime.

With this information combined, I decided to write a tool over the day that solves these calls. I was not happy with public implementations due to various reasons. One was closed-source, and seemed to be limited to a debugger, and the other lifts these stubs into a IL which seems impractical. I decided to go the emulation route as this trivially tackles the problem and supports both X86 and X86-64 flawlessly.

VMPImportFixer is an all-in-one tool; it will support X86 processes regardless of being in a X64 context. This means that there is no need for architecture dependent versions of the binary.

# Usage 

```
Usage:  VMPImportFixer
  -p            (required) process name/process id
  -mod:         (optional) name of module to dump.
  -section:     (optional) VMP section name to use if changed from default (VMP allows custom names)
```

# Examples
<details>
  <summary>Images</summary>

* Before
![b1](https://i.imgur.com/wzraZfe.png)
* After
![a1](https://i.imgur.com/E12Gnxc.png)

* Before
![b2](https://i.imgur.com/eKdCdtm.png)
* After
![a2](https://i.imgur.com/acPdGVt.png)
</details>

# TODO

* Add support for loading binaries off the disk into a state where it can be monitored at specific stages (such as unpacking) then fixed.
* Add relocation handling on X86 binaries.
* Kernel support.

# Dependencies
* [pepp](https://github.com/mike1k/pepp)
* [Unicorn](https://github.com/unicorn-engine/unicorn)
* [Zydis](https://github.com/zyantific/zydis)
* [spdlog](https://github.com/gabime/spdlog)

# Credits

[mrexodia](https://github.com/mrexodia) for his contribution to [HookHunter](https://github.com/mike1k/HookHunter) regarding `ReadMemory` inside the `Process` class.
