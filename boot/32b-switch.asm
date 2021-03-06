                [bits 16]

switch_to_pm:   cli
                xor     ax, ax
                mov     ds, ax
                lgdt    [gdt_descriptor]
                mov     eax, cr0
                or      eax, 0x1
                mov     cr0, eax

                jmp     code_seg_c:init_pm


                [bits 32]

init_pm:        mov     ax, data_seg_c
                mov     ds, ax
                mov     ss, ax
                mov     es, ax
                mov     fs, ax
                mov     gs, ax

                mov     ebp, 0x5000000
                mov     esp, ebp

                jmp     begin_pm
