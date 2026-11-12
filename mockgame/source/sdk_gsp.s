@ GSPGPU RegisterInterruptRelayQueue as the Nintendo SDK compiles it (the words CTRPluginFramework searches a game's
@ code for to hook the GSP interrupt queue). libctru inlines its own version, so the mock game carries this copy and
@ calls it once: Result sdk_gsp_register(Handle *session, Handle event, u32 flags, Handle *outSharedMem, u32 *outThreadId)
    .arm
    .section .text.sdk_gsp_register, "ax", %progbits
    .global sdk_gsp_register
    .type sdk_gsp_register, %function
    .align 2
sdk_gsp_register:
    .word 0xE92D4070, 0xE1A06003, 0xE59D5010, 0xEE1D3F70, 0xE2834080, 0xE59F3034, 0xE3A0C000
    .word 0xE584100C, 0xE9841004, 0xE5843000, 0xE5900000, 0xEF000032, 0xE2101102, 0x4A000004
    .word 0xE5940008, 0xE5850000, 0xE5940010, 0xE5860000, 0xE5940004, 0xE8BD8070, 0x00130042
    .size sdk_gsp_register, . - sdk_gsp_register

@ The game's "present a framebuffer" entry as CTRPluginFramework finds it (STMFD {R4-R12,LR}, then its known SDK
@ instruction sequence within 0x400 bytes): the plugin's OSD hooks this function and draws into the framebuffer that
@ is about to be shown. The mock calls it before every buffer swap:
@   void sdk_present(u32 isBottom, int nextBank, void *leftFb, void *rightFb, int stride, int format, int swap)
@ (the SDK words are only searched for, never executed: they are jumped over)
    .section .text.sdk_present, "ax", %progbits
    .global sdk_present
    .type sdk_present, %function
    .align 2
sdk_present:
    stmfd sp!, {r4-r12, lr}
    mov r4, r4
    mov r4, r4
    mov r4, r4
    b 1f
    .word 0xE1833000, 0xE2044CFF, 0xE3C33CFF, 0xE1833004, 0xE1824F93
1:  mov r0, #0
    ldmfd sp!, {r4-r12, pc}
    .size sdk_present, . - sdk_present
