/*******************************************************************************
                      Minos -> PandaX-III / TDCM - Harpo / ARC
                      ________________________________________

 File:        frame.h

 Description: Feminos Frame Format


 Author:      D. Calvet,        denis.calvetATcea.fr
              

 History:
   June 2011 : created

   March 2012: changed file name mframe.h to frame.h

   September 2013: defined prefix PFX_SOBE_SIZE

   August 2016: defined new prefix for Start of Event and End of Event

   April 2017: added SRC_TYPE_FRONT_END and SRC_TYPE_BACK_END. The type of
   source is now included in the header of each frame to identify the source
   not only by its ID, but also by its type.

   January 2018: added PFX_LONG_ASCII_MSG to handle long ASCII messages (up to
   1 Jumbo frame, i.e. 8 KB)

   March 2018: added PFX_EXTD_CARD_CHIP_CHAN_HIT_IX and PFX_EXTD_CARD_CHIP_CHAN_HISTO
   These are used to support extended format with a 5-bit card index, a 4-bit
   chip index and 7-bit channel index

   October 2018: added PFX_EXTD_CARD_CHIP_LAST_CELL_READ.
   These are used to encode last SCA cell read with support of up to 16 chips
   per front-end card. Grouped the four FRAME_PRINT_LAST_CELL_READ in one common
   flag.
   Added FRAME_PRINT_LISTS_FOR_ARC so that generated commands have the syntax
   that works for the TDCM or for the ARC.
   Defined PFX_EXTD_PEDTHR_LIST and added PUT_EXTD_PEDTHR_LIST()

   January 2019: defined PFX_EXTD_CARD_CHIP_CHAN_H_MD

   May 2019: added function Frame_IsMFrame()

   October 2019: defined PFX_FRAME_SEQ_NB

*******************************************************************************/
#ifndef FRAME_H
#define FRAME_H

//
// Prefix Codes for 14-bit data content
//
#define PFX_14_BIT_CONTENT_MASK    0xC000 // Mask to select 2 MSB's of prefix
#define PFX_CARD_CHIP_CHAN_HIT_IX  0xC000 // Index of Card, Chip and Channel Hit
// "10" : available for future use
#define PFX_CARD_CHIP_CHAN_HISTO   0x4000 // Pedestal Histogram for given Card and Chip

#define PUT_CARD_CHIP_CHAN_HISTO(ca, as, ch) (PFX_CARD_CHIP_CHAN_HISTO | (((ca) & 0x1F) <<9) | (((as) & 0x3) <<7) | (((ch) & 0x7F) <<0))

//
// Prefix Codes for 12-bit data content
//
#define PFX_12_BIT_CONTENT_MASK    0xF000 // Mask to select 4 MSB's of prefix
#define PFX_ADC_SAMPLE             0x3000 // ADC sample
#define PFX_LAT_HISTO_BIN          0x2000 // latency or inter event time histogram bin

//
// Prefix Codes for 11-bit data content
//
#define PFX_11_BIT_CONTENT_MASK    0xF800 // Mask to select 5 MSB's of prefix
#define PFX_CHIP_LAST_CELL_READ    0x1800 // Chip index (2 bit) + last cell read (9 bits)

//
// Prefix Codes for 10-bit data content
//
// "000101" : available for future use
	
//
// Prefix Codes for 9-bit data content
//
#define PFX_9_BIT_CONTENT_MASK     0xFE00 // Mask to select 7 MSB's of prefix
#define PFX_TIME_BIN_IX            0x0E00 // Time-bin Index
#define PFX_HISTO_BIN_IX           0x0C00 // Histogram bin Index
#define PFX_PEDTHR_LIST            0x0A00 // List of pedestal or thresholds
#define PFX_START_OF_DFRAME        0x0800 // Start of Data Frame + 3 bit Version + 1 bit source type + 5 bit source
#define PFX_START_OF_MFRAME        0x0600 // Start of Monitoring Frame + 3 bit Version + 1 bit source type + 5 bit source
#define PFX_START_OF_CFRAME        0x0400 // Start of Configuration Frame + 3 bit Version + 1 bit source type + 5 bit source
#define PFX_CHIP_CHAN_HIT_CNT      0x1200 // Chip (2 bit) Channel hit count (7 bit)
#define PFX_FRAME_SEQ_NB           0x1000 // Sequence number for UDP frame

#define PUT_HISTO_BIN_IX(bi)        (PFX_HISTO_BIN_IX | ((bi) & 0x1FF))
#define PUT_PEDTHR_LIST(f, a, m, t) (PFX_PEDTHR_LIST | (((f) & 0x1F)<<4) | (((a) & 0x3)<<2) | (((m) & 0x1)<<1) | (((t) & 0x1)<<0))
#define PUT_FRAME_SEQ_NB(nb)        (PFX_FRAME_SEQ_NB | ((nb) & 0x1FF))
#define GET_FRAME_SEQ_NB(fr)        ((fr) & 0x1FF)

//
// Prefix Codes for 8-bit data content
//
#define PFX_8_BIT_CONTENT_MASK      0xFF00 // Mask to select 8 MSB's of prefix
#define PFX_ASCII_MSG_LEN           0x0100 // ASCII message length
#define PFX_START_OF_EVENT          0x0300 // Start Of Event + 2 bit event type + 1 bit source type + 5 bit source ID

//
// Prefix Codes for 6-bit data content
//
#define PFX_6_BIT_CONTENT_MASK      0xFFC0 // Mask to select 10 MSB's of prefix
#define PFX_END_OF_EVENT            0x02C0 // End Of Event + 1 bit source type + 5 bit source ID
#define PFX_BERT_STAT               0x0280 // RX BERT Statistics + 1 bit source + 5 bit ID
// "0000 0010 01xx xxxx" : available for future use
// "0000 0010 00xx xxxx" : available for future use

//
// Prefix Codes for 4-bit data content
//
#define PFX_4_BIT_CONTENT_MASK            0xFFF0 // Mask to select 12 MSB's of prefix
#define PFX_START_OF_EVENT_MINOS          0x00F0 // Start of Event + 1 bit free + Event Trigger Type
#define PFX_END_OF_EVENT_MINOS            0x00E0 // End of Event + 4 MSB of size
#define PFX_EXTD_CARD_CHIP_LAST_CELL_READ 0x00D0 // Last Cell Read + 4 bit Card/Chip index followed by 16-bit last cell value
//#define 0x00C0 // available for future use
//#define 0x00B0 // available for future use
// "000000001010" : available for future use
// "000000001001" : available for future use
// "000000001000" : available for future use
 	
//
// Prefix Codes for 2-bit data content
//
#define PFX_2_BIT_CONTENT_MASK      0xFFFC // Mask to select 14 MSB's of prefix
#define PFX_CH_HIT_CNT_HISTO        0x007C // Channel Hit Count Histogram
// "00000000011110" : available for future use	
// "00000000011100" : available for future use	
// "00000000011011" : available for future use	
// "00000000011010" : available for future use	
// "00000000011001" : available for future use	
// "00000000011000" : available for future use	

//
// Prefix Codes for 1-bit data content
//
#define PFX_1_BIT_CONTENT_MASK      0xFFFE // Mask to select 15 MSB's of prefix
// "000000000011111" : available for future use
// "000000000011110" : available for future use	
// "000000000011101" : available for future use	
// "000000000011100" : available for future use	
// "000000000001111" : available for future use	
// "000000000001110" : available for future use	
// "000000000001101" : available for future use	
// "000000000001100" : available for future use	
// "000000000001011" : available for future use	
// "000000000001010" : available for future use
	
//
// Prefix Codes for 0-bit data content
//
#define PFX_0_BIT_CONTENT_MASK         0xFFFF // Mask to select 16 MSB's of prefix
// "0000000000010011" : available for future use
#define PFX_EXTD_CARD_CHIP_CHAN_H_MD   0x0012 // Header for pedestal histogram of one channel - mean and std deviation
#define PFX_EXTD_CARD_CHIP_CHAN_HIT_IX 0x0011 // Header for data of one channel
#define PFX_EXTD_CARD_CHIP_CHAN_HISTO  0x0010 // Header for pedestal histogram of one channel
#define PFX_END_OF_FRAME               0x000F // End of Frame (any type)
#define PFX_DEADTIME_HSTAT_BINS        0x000E // Deadtime statistics and histogram
#define PFX_PEDESTAL_HSTAT             0x000D // Pedestal histogram statistics
#define PFX_PEDESTAL_H_MD              0x000C // Pedestal histogram Mean and Deviation
#define PFX_SHISTO_BINS                0x000B // Hit S-curve histogram
#define PFX_CMD_STATISTICS             0x000A // Command server statistics
#define PFX_START_OF_BUILT_EVENT       0x0009 // Start of built event
#define PFX_END_OF_BUILT_EVENT         0x0008 // End of built event
#define PFX_EVPERIOD_HSTAT_BINS        0x0007 // Inter Event Time statistics and histogram
#define PFX_SOBE_SIZE                  0x0006 // Start of built event + Size
#define PFX_LONG_ASCII_MSG             0x0005 // Long ASCII message + Size (16-bit)
#define PFX_EXTD_PEDTHR_LIST           0x0004 // Extended Pedestal Threshold List
// "0000000000000011" : available for future use
// "0000000000000010" : available for future use
// "0000000000000001" : available for future use
#define PFX_NULL_CONTENT               0x0000 // Null content

#define PUT_EXTD_PEDTHR_LIST(f, a, m, t) ((((f) & 0x1F)<<6) | (((a) & 0xF)<<2) | (((m) & 0x1)<<1) | (((t) & 0x1)<<0))
#define GET_EXTD_PEDTHR_LIST_FEM(w)      (((w) & 0x07C0) >>  6)
#define GET_EXTD_PEDTHR_LIST_ASIC(w)     (((w) & 0x003C) >>  2)
#define GET_EXTD_PEDTHR_LIST_MODE(w)     (((w) & 0x0002) >>  1)
#define GET_EXTD_PEDTHR_LIST_TYPE(w)     (((w) & 0x0001) >>  0)

//
// Macros to extract 14-bit data content
//
#define GET_CARD_IX(w)  (((w) & 0x3E00) >>  9)
#define GET_CHIP_IX(w)  (((w) & 0x0180) >>  7)
#define GET_CHAN_IX(w)  (((w) & 0x007F) >>  0)

//
// Macros to extract 12-bit data content
//
#define GET_ADC_DATA(w)              (((w) & 0x0FFF) >>  0)
#define GET_LAT_HISTO_BIN(w)         (((w) & 0x0FFF) >>  0)
#define PUT_LAT_HISTO_BIN(w)         (PFX_LAT_HISTO_BIN | (((w) & 0x0FFF) >>  0))

//
// Macros to extract 11-bit data content
//
#define GET_LAST_CELL_READ(w)        (((w) & 0x01FF) >>  0)
#define GET_CHIP_IX_LCR(w)           (((w) & 0x0600) >>  9)

//
// Macros to extract 9-bit data content
//
#define GET_TIME_BIN(w)                (((w) & 0x01FF) >>  0)
#define GET_HISTO_BIN(w)               (((w) & 0x01FF) >>  0)
#define GET_PEDTHR_LIST_FEM(w)         (((w) & 0x01F0) >>  4)
#define GET_PEDTHR_LIST_ASIC(w)        (((w) & 0x000C) >>  2)
#define GET_PEDTHR_LIST_MODE(w)        (((w) & 0x0002) >>  1)
#define GET_PEDTHR_LIST_TYPE(w)        (((w) & 0x0001) >>  0)
#define PUT_VERSION_ST_SID(w, v, t, i) (((w) & 0xFE00) | (((v) & 0x0007) <<  6) | (((t) & 0x0001) <<  5) | (((i) & 0x001F) <<  0))
#define GET_VERSION_FRAMING(w)         (((w) & 0x01C0) >>  6)
#define GET_SOURCE_TYPE(w)             (((w) & 0x0020) >>  5)
#define GET_SOURCE_ID(w)               (((w) & 0x001F) >>  0)
#define GET_CHAN_HIT_CNT(w)            (((w) & 0x007F) >>  0)
#define GET_CHIP_IX_CHC(w)             (((w) & 0x0180) >>  7)

//
// Macros to act on 8-bit data content
//
#define GET_ASCII_LEN(w)        (((w) & 0x00FF) >>  0)
#define PUT_ASCII_LEN(w)        (PFX_ASCII_MSG_LEN | ((w) & 0x00FF))
#define GET_SOE_EV_TYPE(w)      (((w) & 0x00C0) >>  6)
#define GET_SOE_SOURCE_TYPE(w)  (((w) & 0x0020) >>  5)
#define GET_SOE_SOURCE_ID(w)    (((w) & 0x001F) >>  0)

//
// Macros to act on 6-bit data content
//
#define GET_EOE_SOURCE_TYPE(w)  (((w) & 0x0020) >>  5)
#define GET_EOE_SOURCE_ID(w)    (((w) & 0x001F) >>  0)
#define GET_BERT_STAT_SOURCE_TYPE(w)  (((w) & 0x0020) >>  5)
#define GET_BERT_STAT_SOURCE_ID(w)    (((w) & 0x001F) >>  0)

//
// Macros to act on 4-bit data content
//
#define GET_EVENT_TYPE(w)                    (((w) & 0x0007) >>  0)
#define GET_EOE_SIZE(w)                      (((w) & 0x000F) >>  0)
#define GET_EXTD_CARD_CHIP_LAST_CELL_READ(w) (((w) & 0x000F) >>  0)

//
// Macros to extract 2-bit data content
//
#define GET_CH_HIT_CNT_HISTO_CHIP_IX(w) (((w) & 0x0003) >>  0)
#define PUT_CH_HIT_CNT_HISTO_CHIP_IX(w) (PFX_CH_HIT_CNT_HISTO | ((w) & 0x0003))

//
// Macros to work with extended card/chip/channel format
//
#define PUT_EXTD_CARD_CHIP_CHAN(ca, as, ch) ((((ca) & 0x1F) <<11) | (((as) & 0xF) <<7) | (((ch) & 0x7F) <<0))
#define GET_EXTD_CARD_IX(w)  (((w) & 0xF800) >> 11)
#define GET_EXTD_CHIP_IX(w)  (((w) & 0x0780) >>  7)
#define GET_EXTD_CHAN_IX(w)  (((w) & 0x007F) >>  0)

#define CURRENT_FRAMING_VERSION 0

// Definition of types of source
#define SRC_TYPE_FRONT_END 0
#define SRC_TYPE_BACK_END  1

// Definition of verboseness flags used by MFrame_Print
#define FRAME_PRINT_ALL              0x00000001
#define FRAME_PRINT_SIZE             0x00000002
#define FRAME_PRINT_HIT_CH           0x00000004
#define FRAME_PRINT_HIT_CNT          0x00000008
#define FRAME_PRINT_CHAN_DATA        0x00000010
#define FRAME_PRINT_HISTO_BINS       0x00000020
#define FRAME_PRINT_ASCII            0x00000040
#define FRAME_PRINT_FRBND            0x00000080
#define FRAME_PRINT_EVBND            0x00000100
#define FRAME_PRINT_NULLW            0x00000200
#define FRAME_PRINT_HISTO_STAT       0x00000400
#define FRAME_PRINT_LISTS            0x00000800
#define FRAME_PRINT_LAST_CELL_READ   0x00001000
#define FRAME_PRINT_EBBND            0x00002000
#define FRAME_PRINT_LISTS_FOR_ARC    0x00004000


void Frame_Print(void *fp, void *fr, int fr_sz, unsigned int vflg);
int  Frame_IsCFrame(void *fr, short *err_code);
int  Frame_IsDFrame(void *fr);
int  Frame_IsMFrame(void *fr);
int  Frame_IsMsgStat(void *fr);
int  Frame_IsDFrame_EndOfEvent(void *fr);
int  Frame_GetEventTyNbTs(void *fr,
	unsigned short *ev_ty,
	unsigned int   *ev_nb,
	unsigned short *ev_tsl,
	unsigned short *ev_tsm,
	unsigned short *ev_tsh);
void sourcetype2str(unsigned short st, char *s);

#endif

