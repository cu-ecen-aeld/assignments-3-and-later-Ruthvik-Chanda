OOPS Error Message :

Unable to handle kernel NULL pointer dereference at virtual address 0000000000000000
Mem abort info:
  ESR = 0x96000045
  EC = 0x25: DABT (current EL), IL = 32 bits
  SET = 0, FnV = 0
  EA = 0, S1PTW = 0
  FSC = 0x05: level 1 translation fault
Data abort info:
  ISV = 0, ISS = 0x00000045
  CM = 0, WnR = 1
user pgtable: 4k pages, 39-bit VAs, pgdp=00000000420ea000
[0000000000000000] pgd=0000000000000000, p4d=0000000000000000, pud=0000000000000000
Internal error: Oops: 96000045 [#1] SMP
Modules linked in: hello(O) scull(O) faulty(O)
CPU: 0 PID: 149 Comm: sh Tainted: G           O      5.15.18 #1
Hardware name: linux,dummy-virt (DT)
pstate: 80000005 (Nzcv daif -PAN -UAO -TCO -DIT -SSBS BTYPE=--)
pc : faulty_write+0x14/0x20 [faulty]
lr : vfs_write+0xa8/0x2a0
sp : ffffffc008d0bd80
x29: ffffffc008d0bd80 x28: ffffff80020fb300 x27: 0000000000000000
x26: 0000000000000000 x25: 0000000000000000 x24: 0000000000000000
x23: 0000000040001000 x22: 0000000000000012 x21: 0000005581277a00
x20: 0000005581277a00 x19: ffffff8002125f00 x18: 0000000000000000
x17: 0000000000000000 x16: 0000000000000000 x15: 0000000000000000
x14: 0000000000000000 x13: 0000000000000000 x12: 0000000000000000
x11: 0000000000000000 x10: 0000000000000000 x9 : 0000000000000000
x8 : 0000000000000000 x7 : 0000000000000000 x6 : 0000000000000000
x5 : 0000000000000001 x4 : ffffffc0006f0000 x3 : ffffffc008d0bdf0
x2 : 0000000000000012 x1 : 0000000000000000 x0 : 0000000000000000
Call trace:
 faulty_write+0x14/0x20 [faulty]
 ksys_write+0x68/0x100
 __arm64_sys_write+0x20/0x30
 invoke_syscall+0x54/0x130
 el0_svc_common.constprop.0+0x44/0x100
 do_el0_svc+0x44/0xb0
 el0_svc+0x28/0x80
 el0t_64_sync_handler+0xa4/0x130
 el0t_64_sync+0x1a0/0x1a4
Code: d2800001 d2800000 d503233f d50323bf (b900003f) 
---[ end trace 23839af5989be4f9 ]---

Objdump:
0000000000000000 <faulty_write>:
   0:   d503245f        bti     c
   4:   d2800001        mov     x1, #0x0                        // #0
   8:   d2800000        mov     x0, #0x0                        // #0
   c:   d503233f        paciasp
  10:   d50323bf        autiasp
  14:   b900003f        str     wzr, [x1]
  18:   d65f03c0        ret
  1c:   d503201f        nop
  
The fault occurred at address 0x14, which indicates that it occurred at the instruction str wzr,[x1].

Analysis:
1. A 32-bit register called the ESR value in an ARM processor holds details about the reason why an exception occurred.
2. Dereferencing the NULL pointer fails due to an inability to handle a kernel NULL pointer dereference at virtual address 0000000000000000.
3. The result "EC = 0x25" denotes that a translation error was to blame for the exception. According to the "FSC" value, the value is 0x05, which denotes a level 1 translation defect. The "ISV" value specifies whether a data access or an instruction was to blame for the abort.
4. Oops notifications often render the system unusable. The system might need to restart, and that might not be sufficient to unload and reload the driver.
5. If the abort resulted from a cache maintenance operation is indicated by the "CM" value. A value of 0 signifies that there was no cache maintenance operation that resulted in the abort. Whether the abort was brought on by a write or a read operation is indicated by the "WnR" value. A number of 1 signifies that a write operation was what triggered the abort.
6. The call trail indicates that the problem appeared while faulty write was running.
It was discovered during the disassembly of the faulty.ko file that the value 0 was being written to the incorrect memory address 0x10.
There is therefore a chance of overwriting random memory and creating issues. As a result, the error might be fixed by restarting the qemu.



