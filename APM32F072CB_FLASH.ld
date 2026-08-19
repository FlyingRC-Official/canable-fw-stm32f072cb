ENTRY(Reset_Handler)

_rom_base = 0x08000000;
_rom_size = 128K;
_ram_base = 0x20000000;
_ram_size = 16K;
_heap_size = 0x400;
_stack_size = 0x800;
_end_stack = ORIGIN(RAM) + LENGTH(RAM);

MEMORY
{
    FLASH (rx)  : ORIGIN = _rom_base, LENGTH = _rom_size
    RAM   (xrw) : ORIGIN = _ram_base, LENGTH = _ram_size
}

SECTIONS
{
    .apm32_isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.apm32_isr_vector))
        . = ALIGN(4);
    } > FLASH

    .text :
    {
        . = ALIGN(4);
        *(.text .text*)
        *(.glue_7 .glue_7t)
        KEEP(*(.init))
        KEEP(*(.fini))
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    .rodata :
    {
        . = ALIGN(4);
        *(.rodata .rodata*)
        . = ALIGN(4);
    } > FLASH

    .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) } > FLASH
    .ARM :
    {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } > FLASH

    .preinit_array :
    {
        PROVIDE_HIDDEN(__preinit_array_start = .);
        KEEP(*(.preinit_array*))
        PROVIDE_HIDDEN(__preinit_array_end = .);
    } > FLASH
    .init_array :
    {
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array*))
        PROVIDE_HIDDEN(__init_array_end = .);
    } > FLASH
    .fini_array :
    {
        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array*))
        PROVIDE_HIDDEN(__fini_array_end = .);
    } > FLASH

    _start_address_init_data = LOADADDR(.data);
    .data :
    {
        . = ALIGN(4);
        _start_address_data = .;
        *(.data .data*)
        . = ALIGN(4);
        _end_address_data = .;
    } > RAM AT > FLASH

    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        _start_address_bss = .;
        __bss_start__ = .;
        *(.bss .bss*)
        *(COMMON)
        . = ALIGN(4);
        _end_address_bss = .;
        __bss_end__ = .;
    } > RAM

    ._user_heap_stack (NOLOAD) :
    {
        . = ALIGN(8);
        PROVIDE(end = .);
        PROVIDE(_end = .);
        _heap_start = .;
        . += _heap_size;
        _heap_end = .;
        . += _stack_size;
        . = ALIGN(8);
    } > RAM

    ASSERT((_end_address_bss + _heap_size + _stack_size) <= _end_stack,
           "RAM overflow: static data, heap and stack exceed 16 KiB")
    .ARM.attributes 0 : { *(.ARM.attributes) }
}
