/**
 * @file cs35l41_system_types.h
 *
 * @brief CS35L41 Driver module system configuration typedefs and enums
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2020 All Rights Reserved, http://www.cirrus.com/
 *
 * This code and information are provided 'as-is' without warranty of any
 * kind, either expressed or implied, including but not limited to the
 * implied warranties of merchantability and/or fitness for a particular
 * purpose.
 *
 */

#ifndef CS35L41_SYSTEM_TYPES_H
#define CS35L41_SYSTEM_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include <stdint.h>
#include <stdbool.h>

/***********************************************************************************************************************
 * LITERALS & CONSTANTS
 **********************************************************************************************************************/

/**
 * Data Routing value to indicate 'disabled'
 *
 */
#define CS35L41_INPUT_SRC_DISABLE                       (0x00)

/**
 * Weak-FET amp drive threshold to indicate disabled
 *
 */
#define CS35L41_WKFET_AMP_THLD_DISABLED                 (0x0)

/***********************************************************************************************************************
 * MACROS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * ENUMS, STRUCTS, UNIONS, TYPEDEFS
 **********************************************************************************************************************/
/**
 * Configuration of amplifier audio hardware
 */
typedef struct
{
    uint8_t dout_hiz_ctrl;  ///< ASP TX data pin Hi-Z control.  See datasheet Section 7.15.4
    bool is_master_mode;    ///< (True) Set ASP in Master Mode
    bool fsync_inv;         ///< (True) Invert polarity of FSYNC
    bool bclk_inv;          ///< (True) Invert polarity of BCLK
    bool amp_dre_en;        ///< (True) Enable Amplifier DRE
    bool ng_enable;         ///< (True) Enable Noise Gate
    uint8_t ng_thld;        ///< Noise Gate threshold.  See datasheet Section 7.19.3
    uint8_t ng_delay;       ///< Noise Gate delay.    See datasheet Section 7.19.3
    uint8_t amp_gain_pcm;   ///< Amplifier analog gain for PCM input path.  See datasheet Section 7.20.1
    uint8_t amp_ramp_pcm;   ///< Amplifier PCM audio digital soft-ramp rate.  See datasheet Section 7.17.1
} cs35l41_audio_hw_config_t;

/**
 * Configuration of amplifier Audio Serial Port (ASP)
 */
typedef struct
{
    bool is_i2s;        ///< (True) Port is in I2S mode; (False) Port is in DSPA mode
    uint8_t rx1_slot;   ///< Slot position for RX Channel 1
    uint8_t rx2_slot;   ///< Slot position for RX Channel 2
    uint8_t tx1_slot;   ///< Slot position for TX Channel 1
    uint8_t tx2_slot;   ///< Slot position for TX Channel 2
    uint8_t tx3_slot;   ///< Slot position for TX Channel 3
    uint8_t tx4_slot;   ///< Slot position for TX Channel 4
    uint8_t tx_wl;      ///< TX active data width (in number of BCLK cycles)
    uint8_t tx_width;   ///< TX slot width (in number of BCLK cycles)
    uint8_t rx_wl;      ///< RX active data width (in number of BCLK cycles)
    uint8_t rx_width;   ///< RX slot width (in number of BCLK cycles)
} cs35l41_asp_config_t;

/**
 * Routing of audio data to Amplifier DAC, DSP, and ASP TX channels
 *
 * @see CS35L41_INPUT_SRC_
 */
typedef struct
{
    uint8_t dac_src;        ///< Amplifier DAC audio mixer source
    uint8_t dsp_rx1_src;    ///< DSP RX Channel 1 audio mixer source
    uint8_t dsp_rx2_src;    ///< DSP RX Channel 2 audio mixer source
    uint8_t asp_tx1_src;    ///< ASP TX Channel 1 audio mixer source
    uint8_t asp_tx2_src;    ///< ASP TX Channel 2 audio mixer source
    uint8_t asp_tx3_src;    ///< ASP TX Channel 3 audio mixer source
    uint8_t asp_tx4_src;    ///< ASP TX Channel 4 audio mixer source
} cs35l41_routing_config_t;

/**
 * Configuration of internal clocking.
 */
typedef struct
{
    uint8_t refclk_sel;     ///< Clock source for REFCLK @see CS35L41_PLL_REFLCLK_SEL_
    uint32_t sclk;          ///< BCLK (or SCLK) frequency in Hz
    uint32_t refclk_freq;   ///< REFCLK frequency in Hz
    uint32_t global_fs;     ///< FSYNC frequency in Hz
} cs35l41_clock_config_t;

/**
 * Collection of audio-related configurations
 */
typedef struct
{
    cs35l41_audio_hw_config_t hw;
    cs35l41_asp_config_t asp;
    cs35l41_routing_config_t routing;
    cs35l41_clock_config_t clock;
    uint16_t volume;                    ///< Volume to be applied at reset
} cs35l41_audio_config_t;

/**
 * Amplifier-related configurations
 */
typedef struct
{
    uint16_t boost_inductor_value_nh;   ///< Boost inductor value in nH
    uint16_t boost_capacitor_value_uf;  ///< Boost capacitor value in uF
    uint16_t boost_ipeak_ma;            ///< Boost peak current in mA
    uint8_t bst_ctl;                    ///< Boost converter target voltage.  See datasheet Section 7.11.1
    uint8_t temp_warn_thld;             ///< Amplifier overtemperature warning threshold.  See datasheet Section 7.13.1
    bool classh_enable;                 ///< (True) Enable Class H functionality
    uint8_t bst_ctl_sel;                ///< Boost converter control source selection.  See datasheet Section 7.11.2
    bool bst_ctl_lim_en;                ///< Class H boost control max limit.  See datasheet Section 7.11.2
    uint8_t ch_mem_depth;               ///< Class H algorithm memory depth.  See datasheet Section 7.19.1
    uint8_t ch_hd_rm;                   ///< Class H algorithm headroom.  See datasheet Section 7.19.1
    uint8_t ch_rel_rate;                ///< Class H release rate.  See datasheet Section 7.19.1
    uint8_t wkfet_amp_delay;            ///< Weak-FET entry delay.  See datasheet Section 7.19.2
    uint8_t wkfet_amp_thld;             ///< Weak-FET amplifier drive threshold.  See datasheet Section 7.19.2
} cs35l41_amp_config_t;

/**
 * Driver configuration data structure
 *
 * @see cs35l41_functions_t member configure
 */
typedef struct
{
    cs35l41_audio_config_t audio_config;                ///< Amplifier audio-related configuration
    cs35l41_amp_config_t amp_config;                    ///< Amplifier amp-related configuration
} cs35l41_syscfg_t;

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/

/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // CS35L41_SYSTEM_TYPES_H
