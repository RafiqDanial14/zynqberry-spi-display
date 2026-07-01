--Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
--Copyright 2022-2024 Advanced Micro Devices, Inc. All Rights Reserved.
----------------------------------------------------------------------------------
--Tool Version: Vivado v.2024.2 (win64) Build 5239630 Fri Nov 08 22:35:27 MST 2024
--Date        : Wed Jul  1 17:49:31 2026
--Host        : MSI running 64-bit major release  (build 9200)
--Command     : generate_target zsys_wrapper.bd
--Design      : zsys_wrapper
--Purpose     : IP block netlist
----------------------------------------------------------------------------------
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
library UNISIM;
use UNISIM.VCOMPONENTS.ALL;
entity zsys_wrapper is
  port (
    DDR_addr : inout STD_LOGIC_VECTOR ( 14 downto 0 );
    DDR_ba : inout STD_LOGIC_VECTOR ( 2 downto 0 );
    DDR_cas_n : inout STD_LOGIC;
    DDR_ck_n : inout STD_LOGIC;
    DDR_ck_p : inout STD_LOGIC;
    DDR_cke : inout STD_LOGIC;
    DDR_cs_n : inout STD_LOGIC;
    DDR_dm : inout STD_LOGIC_VECTOR ( 1 downto 0 );
    DDR_dq : inout STD_LOGIC_VECTOR ( 15 downto 0 );
    DDR_dqs_n : inout STD_LOGIC_VECTOR ( 1 downto 0 );
    DDR_dqs_p : inout STD_LOGIC_VECTOR ( 1 downto 0 );
    DDR_odt : inout STD_LOGIC;
    DDR_ras_n : inout STD_LOGIC;
    DDR_reset_n : inout STD_LOGIC;
    DDR_we_n : inout STD_LOGIC;
    FIXED_IO_ddr_vrn : inout STD_LOGIC;
    FIXED_IO_ddr_vrp : inout STD_LOGIC;
    FIXED_IO_mio : inout STD_LOGIC_VECTOR ( 31 downto 0 );
    FIXED_IO_ps_clk : inout STD_LOGIC;
    FIXED_IO_ps_porb : inout STD_LOGIC;
    FIXED_IO_ps_srstb : inout STD_LOGIC;
    GPIO_1_tri_i : in STD_LOGIC_VECTOR ( 1 downto 0 );
    GPIO_1_tri_t : out STD_LOGIC_VECTOR ( 1 downto 0 );
    GPIO_O_ext : out STD_LOGIC_VECTOR ( 1 downto 0 );
    PWM_L : out STD_LOGIC;
    PWM_R : out STD_LOGIC;
    SPI0_MISO_I_0 : in STD_LOGIC;
    SPI0_MOSI_O_0 : out STD_LOGIC;
    SPI0_SCLK_O_0 : out STD_LOGIC;
    SPI0_SS1_O_0 : out STD_LOGIC;
    SPI0_SS_O_0 : out STD_LOGIC
  );
end zsys_wrapper;

architecture STRUCTURE of zsys_wrapper is
  component zsys is
  port (
    DDR_cas_n : inout STD_LOGIC;
    DDR_cke : inout STD_LOGIC;
    DDR_ck_n : inout STD_LOGIC;
    DDR_ck_p : inout STD_LOGIC;
    DDR_cs_n : inout STD_LOGIC;
    DDR_reset_n : inout STD_LOGIC;
    DDR_odt : inout STD_LOGIC;
    DDR_ras_n : inout STD_LOGIC;
    DDR_we_n : inout STD_LOGIC;
    DDR_ba : inout STD_LOGIC_VECTOR ( 2 downto 0 );
    DDR_addr : inout STD_LOGIC_VECTOR ( 14 downto 0 );
    DDR_dm : inout STD_LOGIC_VECTOR ( 1 downto 0 );
    DDR_dq : inout STD_LOGIC_VECTOR ( 15 downto 0 );
    DDR_dqs_n : inout STD_LOGIC_VECTOR ( 1 downto 0 );
    DDR_dqs_p : inout STD_LOGIC_VECTOR ( 1 downto 0 );
    FIXED_IO_mio : inout STD_LOGIC_VECTOR ( 31 downto 0 );
    FIXED_IO_ddr_vrn : inout STD_LOGIC;
    FIXED_IO_ddr_vrp : inout STD_LOGIC;
    FIXED_IO_ps_srstb : inout STD_LOGIC;
    FIXED_IO_ps_clk : inout STD_LOGIC;
    FIXED_IO_ps_porb : inout STD_LOGIC;
    PWM_L : out STD_LOGIC;
    PWM_R : out STD_LOGIC;
    SPI0_MOSI_O_0 : out STD_LOGIC;
    SPI0_MISO_I_0 : in STD_LOGIC;
    SPI0_SCLK_O_0 : out STD_LOGIC;
    SPI0_SS_O_0 : out STD_LOGIC;
    SPI0_SS1_O_0 : out STD_LOGIC;
    GPIO_O_ext : out STD_LOGIC_VECTOR ( 1 downto 0 );
    GPIO_1_tri_i : in STD_LOGIC_VECTOR ( 1 downto 0 );
    GPIO_1_tri_t : out STD_LOGIC_VECTOR ( 1 downto 0 )
  );
  end component zsys;
begin
zsys_i: component zsys
     port map (
      DDR_addr(14 downto 0) => DDR_addr(14 downto 0),
      DDR_ba(2 downto 0) => DDR_ba(2 downto 0),
      DDR_cas_n => DDR_cas_n,
      DDR_ck_n => DDR_ck_n,
      DDR_ck_p => DDR_ck_p,
      DDR_cke => DDR_cke,
      DDR_cs_n => DDR_cs_n,
      DDR_dm(1 downto 0) => DDR_dm(1 downto 0),
      DDR_dq(15 downto 0) => DDR_dq(15 downto 0),
      DDR_dqs_n(1 downto 0) => DDR_dqs_n(1 downto 0),
      DDR_dqs_p(1 downto 0) => DDR_dqs_p(1 downto 0),
      DDR_odt => DDR_odt,
      DDR_ras_n => DDR_ras_n,
      DDR_reset_n => DDR_reset_n,
      DDR_we_n => DDR_we_n,
      FIXED_IO_ddr_vrn => FIXED_IO_ddr_vrn,
      FIXED_IO_ddr_vrp => FIXED_IO_ddr_vrp,
      FIXED_IO_mio(31 downto 0) => FIXED_IO_mio(31 downto 0),
      FIXED_IO_ps_clk => FIXED_IO_ps_clk,
      FIXED_IO_ps_porb => FIXED_IO_ps_porb,
      FIXED_IO_ps_srstb => FIXED_IO_ps_srstb,
      GPIO_1_tri_i(1 downto 0) => GPIO_1_tri_i(1 downto 0),
      GPIO_1_tri_t(1 downto 0) => GPIO_1_tri_t(1 downto 0),
      GPIO_O_ext(1 downto 0) => GPIO_O_ext(1 downto 0),
      PWM_L => PWM_L,
      PWM_R => PWM_R,
      SPI0_MISO_I_0 => SPI0_MISO_I_0,
      SPI0_MOSI_O_0 => SPI0_MOSI_O_0,
      SPI0_SCLK_O_0 => SPI0_SCLK_O_0,
      SPI0_SS1_O_0 => SPI0_SS1_O_0,
      SPI0_SS_O_0 => SPI0_SS_O_0
    );
end STRUCTURE;
