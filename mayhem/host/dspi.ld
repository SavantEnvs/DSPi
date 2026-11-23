SECTIONS
{
  .dspi_state_data : ALIGN(64) { __dspi_state_data_start = .; *(.data.dspi_state) __dspi_state_data_end = .; }
  .dspi_state_bss (NOLOAD) : ALIGN(64) { __dspi_state_bss_start = .; *(.bss.dspi_state) __dspi_state_bss_end = .; }
}
INSERT AFTER .bss;
