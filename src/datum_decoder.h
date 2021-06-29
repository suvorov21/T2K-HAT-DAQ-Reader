/*******************************************************************************
                              PandaX-III / T2K-II
                              ___________________

 File:        datum_decoder.h

 Description: Header file for Datum Decoder


 Author:      D. Calvet,        denis.calvetATcea.fr
              

 History:
   April 2019 : created

   April 2019 : defined structure PrintFilter and added a pointer to such
   structure in the list of arguments for Item_Print()
   Added function Item_PrintFilter_Init()

   May 2019: added DT_PED_HISTO_MD_xx and IT_PED_HISTO_MD

   July 2019: added member EventSizeFound 

   October 2019: added member FrameSequenceNumber,
   defined DT_FRAME_SEQUENCE_NUMBER and IT_FRAME_SEQUENCE_NUMBER

   November 2019: added member DatumOkCount in DatumContext structure

   April 2020: added members DatumHistory[] and IncompleteItemCount
   in DatumContext structure

*******************************************************************************/

#ifndef _DATUM_DECODER_H
#define _DATUM_DECODER_H

#ifdef __cplusplus 
extern "C" {
#endif

/*******************************************************************************
Definition of constants
*******************************************************************************/
#define CHIP_TYPE_AGET  0
#define CHIP_TYPE_AFTER 1
#define PEDTHR_LIST_SIZE_AGET  72
#define PEDTHR_LIST_SIZE_AFTER 79

#define DATUM_HISTORY_SIZE 4

/*******************************************************************************
Definition of types and structures
*******************************************************************************/
typedef enum {
	DT_UNKNOWN_O,
	DT_ASCII_MSG_LENGTH,
	DT_ASCII_CHAR,
	DT_FRAME_LENGTH,
	DT_EVENT_NUMBER_MSB,
	DT_EVENT_NUMBER_LSB,
	DT_EVENT_TIME_STAMP_LSB,
	DT_EVENT_TIME_STAMP_MID,
	DT_EVENT_TIME_STAMP_MSB,
	DT_END_OF_EVENT_RSVD,
	DT_END_OF_EVENT_SIZE_LSB,
	DT_END_OF_EVENT_SIZE_MSB,
	DT_CHANNEL_DATA_CARD_CHIP_CHANNEL,
	DT_PED_HISTO_MD_CARD_CHIP_CHANNEL,
	DT_PED_HISTO_MD_MEAN_LSB,
	DT_PED_HISTO_MD_MEAN_MSB,
	DT_PED_HISTO_MD_DEV_LSB,
	DT_PED_HISTO_MD_DEV_MSB,
	DT_PED_THR_LIST_HEADER,
	DT_CHAN_PED_CORRECTION,
	DT_CHAN_ZERO_SUPPRESS_THRESHOLD,
	DT_FRAME_SEQUENCE_NUMBER
} DatumTypes;

typedef enum {
	IT_UNKNOWN                      = 0x00000001,
	IT_SHORT_MESSAGE                = 0x00000002,
	IT_LONG_MESSAGE                 = 0x00000004,
	IT_DATA_FRAME                   = 0x00000008,
	IT_MONITORING_FRAME             = 0x00000010,
	IT_CONFIGURATION_FRAME          = 0x00000020,
	IT_END_OF_FRAME                 = 0x00000040,
	IT_START_OF_EVENT               = 0x00000080,
	IT_CHANNEL_HIT_COUNT            = 0x00000100,
	IT_LAST_CELL_READ               = 0x00000200,
	IT_END_OF_EVENT                 = 0x00000400,
	IT_CHANNEL_HIT_HEADER           = 0x00000800,
	IT_TIME_BIN_INDEX               = 0x00001000,
	IT_ADC_SAMPLE                   = 0x00002000,
	IT_NULL_DATUM                   = 0x00004000,
	IT_START_OF_BUILT_EVENT         = 0x00008000,
	IT_END_OF_BUILT_EVENT           = 0x00010000,
	IT_PED_HISTO_MD                 = 0x00020000,
	IT_CHAN_PED_CORRECTION          = 0x00040000,
	IT_CHAN_ZERO_SUPPRESS_THRESHOLD = 0x00080000,
	IT_FRAME_SEQUENCE_NUMBER        = 0x00100000
} ItemTypes;

typedef struct _DatumContext {
	/* Public general information */
	short          DecoderMajorVersion;
	short          DecoderMinorVersion;
	char           *DecoderCompilationDate;
	char           *DecoderCompilationTime;
	/* Configuration settings */
	short          SampleIndexOffsetZS;
	/* Private fields */
	unsigned short DatumHistory[DATUM_HISTORY_SIZE];
	unsigned char  isDatumTypeImplicit;
	DatumTypes     DatumType;
	unsigned short FrameSizeExpected;
	unsigned short MessageExpectedLength;
	unsigned short MessageCurrentPtr;
	/* Public Interpreted Data */
	char           ErrorString[256];
	ItemTypes      ItemType;
	unsigned char  isItemComplete;
	char           MessageString[8192];
	unsigned short FrameSequenceNumber;
	unsigned short FramingVersion;
	unsigned short FrameSourceType;
	unsigned short FrameSourceId;
	unsigned short EventType;
	unsigned short SourceType;
	unsigned short SourceId;
	unsigned int   EventNumber;
	unsigned short EventTimeStampLsb;
	unsigned short EventTimeStampMid;
	unsigned short EventTimeStampMsb;
	unsigned int   EventSize;
	unsigned short CardIndex;
	unsigned short ChipIndex;
	unsigned short ChannelHitCount;
	unsigned short LastCellRead;
	unsigned short ChannelIndex;
	unsigned short TimeBinIndex;
	unsigned short AdcSample;
	short          RelativeSampleIndex;
	short          AbsoluteSampleIndex;
	unsigned int   PedestalMean;
	unsigned int   PedestalDev;
	short          PedestalCorrection;
	unsigned short ZeroSuppressThreshold;
	unsigned int   ChipType;
	/* Public Counters */
	unsigned int   DatumCount;
	unsigned int   DatumOkCount;
	unsigned int   IncompleteItemCount;
	unsigned int   StartOfEventFeCount;
	unsigned int   StartOfEventBeCount;
	unsigned int   EndOfEventFeCount;
	unsigned int   EndOfEventBeCount;
	unsigned int   ShortMessageCount;
	unsigned int   LongMessageCount;
	unsigned int   StartOfDataFrameCount;
	unsigned int   StartOfMonitoringFrameCount;
	unsigned int   StartOfConfigurationFrameCount;
	unsigned int   EndOfFrameCount;
	unsigned int   StartOfBuiltEventCount;
	unsigned int   EndOfBuiltEventCount;
	unsigned int   EventSizeFound;
} DatumContext;

typedef struct _PrintFilter {
	unsigned int   flags;
	unsigned short isCondensedFormat;
	unsigned short isAnyEvent;
	unsigned int   EventIndexMin;
	unsigned int   EventIndexMax;
	unsigned short isAnyCard;
	unsigned short CardIndexMin;
	unsigned short CardIndexMax;
	unsigned short isAnyChip;
	unsigned short ChipIndexMin;
	unsigned short ChipIndexMax;
	unsigned short isAnyChannel;
	unsigned short ChannelIndexMin;
	unsigned short ChannelIndexMax;
	unsigned short isAnySampleIndex;
	unsigned short AbsoluteSampleIndexMin;
	unsigned short AbsoluteSampleIndexMax;
	unsigned short isAnySampleAmpl;
	unsigned short SampleAmplMin;
	unsigned short SampleAmplMax;
} PrintFilter;

/*******************************************************************************
Function prototypes
*******************************************************************************/
void SourceTypeToString(unsigned short st, char *s);
void DatumContext_Init(DatumContext *dc, unsigned short sample_index_offset_zs);
int Datum_Decode(DatumContext *dc, unsigned short datum);
void Item_PrintFilter_Init(PrintFilter *pf);
int Item_Print(void *fp, DatumContext *dc, PrintFilter *pf);

#ifdef __cplusplus 
}
#endif

#endif
