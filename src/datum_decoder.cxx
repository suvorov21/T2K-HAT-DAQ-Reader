/*******************************************************************************
                              PandaX-III / T2K-II
                              ___________________

 File:        datum_decoder.c

 Description: Datum Decoder

 This decoder interprets elementary datum recorded by the DAQ client program
 of the TDCM. Each elementary datum is composed of two bytes. Little-endian
 byte ordering is assumed.

 At startup, the DatumContext_Init() function is called.
 Then, the function Datum_Decode() is called sequentially for every datum to
 interpret. The interpretation of a datum may or may not depend from the
 interpretation of the previous ones. If the interpretation of the current
 datum depends on one or several of the previous ones, the type of the
 current datum is implicitly known. If the interpretation of the current
 datum does not depend on previous ones, the datum itself must contain one
 of the prefix codes defined in the data format, otherwise an error occurs.
 Depending on which prefix is identified, the following datum may have
 an implicit type or not.

 After the required number of datum have been decoded, a valid item becomes
 available. The caller of Datum_Decode() must query the field isItemComplete
 of the DatumContext structure after each iteration to determine if a new
 valid item is ready or not. If a valid item is available, the type of the
 item is given in structure member ItemType, and the value of the item itself
 is placed in the relevant members of the DatumContext structure. A valid
 item must be copied elsewhere before Datum_Decode() is called for the next
 datum. In case of error, the Datum_Decode() function returns a negative
 error code and the reason of the failure is stored in the string
 ErrorString[] member of the DatumContext structure.

 Completed items can be printed out by calling Item_Print() with the
 appropriate flags being set to select which types of item shall be printed.


 Author:      D. Calvet,        denis.calvetATcea.fr


 History:

   April 2019 : (version 1.0) created

   April 2019 : (version 1.1) added various filtering capability to Item_Print()

   May 2019 : (version 1.2) added the capability to decode pedestal histograms
   in condensed format, i.e. pedestal mean and rms.

   May 2019 : (version 1.3) added the capability to decode lists of pedestal
   correction values and zero suppression thresholds

   July 2019: (version 1.4) added member EventSizeFound which indicates the
   actual size (in bytes) found for this event. In contrast, member EventSize
   indicates the size of the event that is indicated in the End Of Event marker.
   Both fields should coincide, but there might be a small difference. A large
   difference indicates a possible loss of frame that can make the EventSizeFound
   smaller than EventSize, or larger if some End Of Event were lost and the data
   from more than one event are incorrectly grouped together.

   October 2019: (version 1.5) added decoding of PFX_FRAME_SEQ_NB

   November 2019: (version 1.6) added field DatumOkCount in DatumContext
   structure. Previously, DatumCount was incremented only if datum decoding
   succedeed. Now DatumCount is incremented on every datum being processed,
   and the field DatumOkCount is only incremented if the decoding of the
   datum was sucessful

   February 2020: (version 1.7) partially developed the decoding of the Midas
   T2K format. This part is not operational at this stage.

   April 2020: (version 1.8) improved the decoding of T2K Midas files. Not yet
   final.

   Mai 2020: (version 1.9) fixed bug for decoding ASCII strings when message
   length is odd. Decoding strings must be done character by character instead
   of doing it by pairs of characters. This ensures that the size accumulated
   when building up a decoded string can become equal to the expected size,
   and not pass beyond it by one unit when message size is odd.

*******************************************************************************/

#include "datum_decoder.h"
#include "frame.h"
#include <stdio.h>

/*******************************************************************************
Manage Major/Minor version numbering manually
*******************************************************************************/
#define DECODER_VERSION_MAJOR 1
#define DECODER_VERSION_MINOR 9
char decoder_date[] = __DATE__;
char decoder_time[] = __TIME__;
/******************************************************************************/

/*******************************************************************************
SourceTypeToString
*******************************************************************************/
void SourceTypeToString(unsigned short st, char *s)
{

	if (st == SRC_TYPE_FRONT_END)
	{
		sprintf(s, "FE");
	}
	else if (st == SRC_TYPE_BACK_END)
	{
		sprintf(s, "BE");
	}
	else
	{
		sprintf(s, "??");
	}
}

/*******************************************************************************
DatumContext_Init
*******************************************************************************/
void DatumContext_Init(DatumContext *dc, unsigned short sample_index_offset_zs)
{
	int i;

	/* Public general information */
	dc->DecoderMajorVersion            = DECODER_VERSION_MAJOR;
	dc->DecoderMinorVersion            = DECODER_VERSION_MINOR;
	dc->DecoderCompilationDate         = &decoder_date[0];
	dc->DecoderCompilationTime         = &decoder_time[0];
	/* Configuration settings */
	dc->SampleIndexOffsetZS = sample_index_offset_zs;
	/* Private fields */
	for (i = 0; i < DATUM_HISTORY_SIZE; i++)
	{
		dc->DatumHistory[i] = 0xFFFF;
	}
	dc->isDatumTypeImplicit            = 0;
	dc->DatumType                      = DT_UNKNOWN_O;
	dc->FrameSizeExpected              = 0;
	dc->MessageExpectedLength          = 0;
	dc->MessageCurrentPtr              = 0;
	/* Public Interpreted Data */
	sprintf(&(dc->ErrorString[0]), "");
	dc->ItemType                       = IT_UNKNOWN;
	dc->isItemComplete                 = 0;
	sprintf(&(dc->MessageString[0]), "");
	dc->FramingVersion                 = 0;
	dc->FrameSourceType                = 0;
	dc->FrameSourceId                  = 0;
	dc->EventType                      = 0;
	dc->SourceType                     = 0;
	dc->SourceId                       = 0;
	dc->EventNumber                    = 0xFFFFFFFF;
	dc->EventTimeStampLsb              = 0xFFFF;
	dc->EventTimeStampMid              = 0xFFFF;
	dc->EventTimeStampMsb              = 0xFFFF;
	dc->EventSize                      = 0;
	dc->CardIndex                      = 0;
	dc->ChipIndex                      = 0;
	dc->ChannelHitCount                = 0;
	dc->LastCellRead                   = 0;
	dc->ChannelIndex                   = 0;
	dc->TimeBinIndex                   = 0;
	dc->AdcSample                      = 0x0000;
	dc->RelativeSampleIndex            = -1;
	dc->AbsoluteSampleIndex            = -1;
	dc->PedestalMean                   = 0;
	dc->PedestalDev                    = 0;
	dc->PedestalCorrection             = 0;
	dc->ZeroSuppressThreshold          = 0;
	dc->ChipType                       = -1;
	/* Public Counters */
	dc->DatumCount                     = 0;
	dc->DatumOkCount                   = 0;
	dc->IncompleteItemCount            = 0;
	dc->StartOfEventFeCount            = 0;
	dc->StartOfEventBeCount            = 0;
	dc->EndOfEventFeCount              = 0;
	dc->EndOfEventBeCount              = 0;
	dc->ShortMessageCount              = 0;
	dc->LongMessageCount               = 0;
	dc->StartOfDataFrameCount          = 0;
	dc->StartOfMonitoringFrameCount    = 0;
	dc->StartOfConfigurationFrameCount = 0;
	dc->EndOfFrameCount                = 0;
	dc->StartOfBuiltEventCount         = 0;
	dc->EndOfBuiltEventCount           = 0;
	dc->EventSizeFound                 = 0;
}

/*******************************************************************************
 Datum_Decode
*******************************************************************************/
int Datum_Decode(DatumContext *dc, unsigned short datum)
{
	int i;

	// Update datum history
	for (i = (DATUM_HISTORY_SIZE-1); i >= 1; i--)
	{
		dc->DatumHistory[i] = dc->DatumHistory[i-1];
	}
	dc->DatumHistory[0] = datum;

	// Increment the datum count
	dc->DatumCount = dc->DatumCount + 1;

	// Is datum meaning implicit?
	if (dc->isDatumTypeImplicit)
	{
		if (dc->DatumType == DT_UNKNOWN_O)
		{
			sprintf(&(dc->ErrorString[0]), "Datum(%d) Implicit_Type: %04d : interpretation unknown.", dc->DatumCount, datum);
			return(-1);
		}
		else if (dc->DatumType == DT_ASCII_CHAR)
		{
			// Get the first character from the datum
			dc->MessageString[dc->MessageCurrentPtr] = (char)((datum & 0x00FF) >> 0);
			dc->MessageCurrentPtr++;
			// Expected message size may be reached after the first half of the datum when message size is odd
			if (dc->MessageCurrentPtr == dc->MessageExpectedLength)
			{
				dc->isDatumTypeImplicit = 0;
				dc->DatumType = DT_UNKNOWN_O;
				dc->isItemComplete = 1;
			}
			else
			{
				// Get the second character from the datum
				dc->MessageString[dc->MessageCurrentPtr] = (char)((datum & 0xFF00) >> 8);
				dc->MessageCurrentPtr++;
				// Expected message size may be reached at this time when message size is even
				if (dc->MessageCurrentPtr == dc->MessageExpectedLength)
				{
					dc->isDatumTypeImplicit = 0;
					dc->DatumType = DT_UNKNOWN_O;
					dc->isItemComplete = 1;
				}
			}
		}
		else if (dc->DatumType == DT_ASCII_MSG_LENGTH)
		{
			dc->MessageExpectedLength = datum;
			dc->isDatumTypeImplicit   = 1;
			dc->DatumType             = DT_ASCII_CHAR;
			dc->MessageCurrentPtr     = 0;
			sprintf(&(dc->MessageString[0]), "");
			dc->isItemComplete        = 0;
			dc->LongMessageCount++;
		}
		else if (dc->DatumType == DT_FRAME_LENGTH)
		{
			dc->FrameSizeExpected   = datum;
			dc->isDatumTypeImplicit = 0;
			dc->DatumType           = DT_UNKNOWN_O;
			dc->isItemComplete      = 1;
		}
		else if (dc->DatumType == DT_EVENT_TIME_STAMP_LSB)
		{
			dc->EventTimeStampLsb = datum;
			dc->DatumType = DT_EVENT_TIME_STAMP_MID;
			dc->EventSizeFound = dc->EventSizeFound + sizeof(unsigned short);
		}
		else if (dc->DatumType == DT_EVENT_TIME_STAMP_MID)
		{
			dc->EventTimeStampMid = datum;
			dc->DatumType   = DT_EVENT_TIME_STAMP_MSB;
			dc->EventSizeFound = dc->EventSizeFound + sizeof(unsigned short);
		}
		else if (dc->DatumType == DT_EVENT_TIME_STAMP_MSB)
		{
			dc->EventTimeStampMsb = datum;
			dc->DatumType = DT_EVENT_NUMBER_LSB;
			dc->EventSizeFound = dc->EventSizeFound + sizeof(unsigned short);
		}
		else if (dc->DatumType == DT_EVENT_NUMBER_LSB)
		{
			dc->EventNumber = (unsigned int) datum;
			dc->DatumType = DT_EVENT_NUMBER_MSB;
			dc->EventSizeFound = dc->EventSizeFound + sizeof(unsigned short);
		}
		else if (dc->DatumType == DT_EVENT_NUMBER_MSB)
		{
			dc->EventNumber         = (((unsigned int)(datum)) << 16) | dc->EventNumber;
			dc->isDatumTypeImplicit = 0;
			dc->DatumType           = DT_UNKNOWN_O;
			dc->isItemComplete      = 1;
			dc->EventSizeFound      = dc->EventSizeFound + sizeof(unsigned short);
		}
		else if (dc->DatumType == DT_END_OF_EVENT_RSVD)
		{
			dc->DatumType = DT_END_OF_EVENT_SIZE_LSB;
			dc->EventSizeFound = dc->EventSizeFound + sizeof(unsigned short);
		}
		else if (dc->DatumType == DT_END_OF_EVENT_SIZE_LSB)
		{
			dc->EventSize = (unsigned int)datum;
			dc->DatumType = DT_END_OF_EVENT_SIZE_MSB;
			dc->EventSizeFound = dc->EventSizeFound + sizeof(unsigned short);
		}
		else if (dc->DatumType == DT_END_OF_EVENT_SIZE_MSB)
		{
			dc->EventSize           = (((unsigned int)(datum)) << 16) | dc->EventSize;
			dc->isDatumTypeImplicit = 0;
			dc->DatumType           = DT_UNKNOWN_O;
			dc->isItemComplete      = 1;
			dc->EventSizeFound      = dc->EventSizeFound + sizeof(unsigned short);
		}
		else if (dc->DatumType == DT_CHANNEL_DATA_CARD_CHIP_CHANNEL)
		{
			dc->CardIndex           = GET_EXTD_CARD_IX(datum);
			dc->ChipIndex           = GET_EXTD_CHIP_IX(datum);
			dc->ChannelIndex        = GET_EXTD_CHAN_IX(datum);
			dc->isDatumTypeImplicit = 0;
			dc->DatumType           = DT_UNKNOWN_O;
			dc->RelativeSampleIndex = -1;
			dc->AbsoluteSampleIndex = -1;
			dc->isItemComplete      = 1;
			dc->EventSizeFound = dc->EventSizeFound + sizeof(unsigned short);
		}
		else if (dc->DatumType == DT_PED_HISTO_MD_CARD_CHIP_CHANNEL)
		{
			dc->CardIndex           = GET_EXTD_CARD_IX(datum);
			dc->ChipIndex           = GET_EXTD_CHIP_IX(datum);
			dc->ChannelIndex        = GET_EXTD_CHAN_IX(datum);
			dc->isDatumTypeImplicit = 1;
			dc->DatumType           = DT_PED_HISTO_MD_MEAN_LSB;
			dc->PedestalMean        = 0;
			dc->PedestalDev         = 0;
			dc->isItemComplete      = 0;
		}
		else if (dc->DatumType == DT_PED_HISTO_MD_MEAN_LSB)
		{
			dc->isDatumTypeImplicit = 1;
			dc->DatumType           = DT_PED_HISTO_MD_MEAN_MSB;
			dc->PedestalMean        = (unsigned int) datum;
			dc->isItemComplete      = 0;
		}
		else if (dc->DatumType == DT_PED_HISTO_MD_MEAN_MSB)
		{
			dc->isDatumTypeImplicit = 1;
			dc->DatumType           = DT_PED_HISTO_MD_DEV_LSB;
			dc->PedestalMean        = (((unsigned int) datum) << 16) | (dc->PedestalMean);
			dc->isItemComplete      = 0;
		}
		else if (dc->DatumType == DT_PED_HISTO_MD_DEV_LSB)
		{
			dc->isDatumTypeImplicit = 1;
			dc->DatumType           = DT_PED_HISTO_MD_DEV_MSB;
			dc->PedestalDev         = (unsigned int) datum;
			dc->isItemComplete      = 0;
		}
		else if (dc->DatumType == DT_PED_HISTO_MD_DEV_MSB)
		{
			dc->isDatumTypeImplicit = 0;
			dc->DatumType           = DT_UNKNOWN_O;
			dc->PedestalDev         = (((unsigned int)datum) << 16) | (dc->PedestalDev);
			dc->isItemComplete      = 1;
		}
		else if (dc->DatumType == DT_PED_THR_LIST_HEADER)
		{
			dc->isDatumTypeImplicit = 1;
			// Determine the type of list, pedestal correction or zero suppression threshold
			if (GET_EXTD_PEDTHR_LIST_TYPE(datum) == 0) // Pedestal Correction List
			{
				dc->ItemType  = IT_CHAN_PED_CORRECTION;
				dc->DatumType = DT_CHAN_PED_CORRECTION;
			}
			else // Zero Suppression List
			{
				dc->ItemType  = IT_CHAN_ZERO_SUPPRESS_THRESHOLD;
				dc->DatumType = DT_CHAN_ZERO_SUPPRESS_THRESHOLD;
			}
			// Determine the type of chip, AGET or AFTER
			if (GET_EXTD_PEDTHR_LIST_MODE(datum) == 0) // AGET
			{
				dc->ChipType = CHIP_TYPE_AGET;
			}
			else // AFTER
			{
				dc->ChipType = CHIP_TYPE_AFTER;
			}
			dc->CardIndex      = GET_EXTD_PEDTHR_LIST_FEM(datum);
			dc->ChipIndex      = GET_EXTD_PEDTHR_LIST_ASIC(datum);
			dc->ChannelIndex   = -1;
			dc->isItemComplete = 0;
		}
		else if (dc->DatumType == DT_CHAN_PED_CORRECTION)
		{
			dc->ChannelIndex       = dc->ChannelIndex + 1;
			dc->PedestalCorrection = (short) datum;
			dc->isItemComplete     = 1;
			if (dc->ChipType == CHIP_TYPE_AGET)
			{
				if (dc->ChannelIndex == (PEDTHR_LIST_SIZE_AGET-1))
				{
					dc->isDatumTypeImplicit = 0;
				}
			}
			else
			{
				if (dc->ChannelIndex == (PEDTHR_LIST_SIZE_AFTER - 1))
				{
					dc->isDatumTypeImplicit = 0;
				}
			}
		}
		else if (dc->DatumType == DT_CHAN_ZERO_SUPPRESS_THRESHOLD)
		{
			dc->ChannelIndex          = dc->ChannelIndex + 1;
			dc->ZeroSuppressThreshold = (short)datum;
			dc->isItemComplete        = 1;
			if (dc->ChipType == CHIP_TYPE_AGET)
			{
				if (dc->ChannelIndex == (PEDTHR_LIST_SIZE_AGET - 1))
				{
					dc->isDatumTypeImplicit = 0;
				}
			}
			else
			{
				if (dc->ChannelIndex == (PEDTHR_LIST_SIZE_AFTER - 1))
				{
					dc->isDatumTypeImplicit = 0;
				}
			}
		}
		else
		{
			sprintf(&(dc->ErrorString[0]), "Datum(%d) = 0x%04x Implicit_Type: 0x%04x: interpretation unknown.", dc->DatumCount, datum, dc->DatumType);
			return(-1);
		}
	}
	// Datum type is not implicit, so it must be decoded from its prefix
	else
	{
		dc->isItemComplete = 0;

		// Is it a prefix for 14-bit content?
		if ((datum & PFX_14_BIT_CONTENT_MASK) == PFX_CARD_CHIP_CHAN_HIT_IX)
		{
			dc->CardIndex           = GET_CARD_IX(datum);
			dc->ChipIndex           = GET_CHIP_IX(datum);
			dc->ChannelIndex        = GET_CHAN_IX(datum);
			dc->ItemType            = IT_CHANNEL_HIT_HEADER;
			dc->isDatumTypeImplicit = 0;
			dc->DatumType           = DT_UNKNOWN_O;
			dc->RelativeSampleIndex = -1;
			dc->AbsoluteSampleIndex = -1;
			dc->isItemComplete      = 1;
			dc->EventSizeFound = dc->EventSizeFound + sizeof(unsigned short);
		}
		else if ((datum & PFX_14_BIT_CONTENT_MASK) == PFX_CARD_CHIP_CHAN_HISTO)
		{
			sprintf(&(dc->ErrorString[0]), "Datum(%d) PFX_CARD_CHIP_CHAN_HISTO : interpretation not implemented.", dc->DatumCount);
			return(-1);
		}
		// Is it a prefix for 12-bit content?
		else if ((datum & PFX_12_BIT_CONTENT_MASK) == PFX_ADC_SAMPLE)
		{
			dc->AdcSample           = GET_ADC_DATA(datum);
			dc->RelativeSampleIndex = dc->RelativeSampleIndex + 1;
			dc->AbsoluteSampleIndex = dc->AbsoluteSampleIndex + 1;
			dc->ItemType            = IT_ADC_SAMPLE;
			dc->isItemComplete      = 1;
			dc->EventSizeFound = dc->EventSizeFound + sizeof(unsigned short);
		}
		else if ((datum & PFX_12_BIT_CONTENT_MASK) == PFX_LAT_HISTO_BIN)
		{
			sprintf(&(dc->ErrorString[0]), "Datum(%d) PFX_LAT_HISTO_BIN : interpretation not implemented.", dc->DatumCount);
			return(-1);
		}
		// Is it a prefix for 11-bit content?
		else if ((datum & PFX_11_BIT_CONTENT_MASK) == PFX_CHIP_LAST_CELL_READ)
		{
			dc->ChipIndex           = GET_CHIP_IX_LCR(datum);
			dc->LastCellRead        = GET_LAST_CELL_READ(datum);
			dc->ItemType            = IT_LAST_CELL_READ;
			dc->isItemComplete      = 1;
			dc->isDatumTypeImplicit = 0;
			dc->DatumType = DT_UNKNOWN_O;
			dc->EventSizeFound = dc->EventSizeFound + sizeof(unsigned short);
		}
		// Is it a prefix for 9-bit content?
		else if ((datum & PFX_9_BIT_CONTENT_MASK) == PFX_TIME_BIN_IX)
		{
			dc->TimeBinIndex         = GET_TIME_BIN(datum);
			dc->ItemType             = IT_TIME_BIN_INDEX;
			dc->RelativeSampleIndex  = -1;
			dc->AbsoluteSampleIndex  = dc->TimeBinIndex - 1 - dc->SampleIndexOffsetZS;
			dc->isItemComplete       = 1;
			dc->EventSizeFound = dc->EventSizeFound + sizeof(unsigned short);
		}
		else if ((datum & PFX_9_BIT_CONTENT_MASK) == PFX_HISTO_BIN_IX)
		{
			sprintf(&(dc->ErrorString[0]), "Datum(%d) PFX_HISTO_BIN_IX : interpretation not implemented.", dc->DatumCount);
			return(-1);
		}
		else if ((datum & PFX_9_BIT_CONTENT_MASK) == PFX_PEDTHR_LIST)
		{
			sprintf(&(dc->ErrorString[0]), "Datum(%d) PFX_PEDTHR_LIST : interpretation not implemented.", dc->DatumCount);
			return(-1);
		}
		else if ((datum & PFX_9_BIT_CONTENT_MASK) == PFX_START_OF_DFRAME)
		{
			dc->ItemType            = IT_DATA_FRAME;
			dc->FramingVersion      = GET_VERSION_FRAMING(datum);
			dc->FrameSourceType     = GET_SOURCE_TYPE(datum);
			dc->FrameSourceId       = GET_SOURCE_ID(datum);
			dc->isDatumTypeImplicit = 1;
			dc->DatumType           = DT_FRAME_LENGTH;
			dc->StartOfDataFrameCount++;
		}
		else if ((datum & PFX_9_BIT_CONTENT_MASK) == PFX_START_OF_MFRAME)
		{
			dc->ItemType            = IT_MONITORING_FRAME;
			dc->FramingVersion      = GET_VERSION_FRAMING(datum);
			dc->FrameSourceType     = GET_SOURCE_TYPE(datum);
			dc->FrameSourceId       = GET_SOURCE_ID(datum);
			dc->isDatumTypeImplicit = 1;
			dc->DatumType           = DT_FRAME_LENGTH;
			dc->StartOfMonitoringFrameCount++;
		}
		else if ((datum & PFX_9_BIT_CONTENT_MASK) == PFX_START_OF_CFRAME)
		{
			dc->ItemType            = IT_CONFIGURATION_FRAME;
			dc->FramingVersion      = GET_VERSION_FRAMING(datum);
			dc->FrameSourceType     = GET_SOURCE_TYPE(datum);
			dc->FrameSourceId       = GET_SOURCE_ID(datum);
			dc->isDatumTypeImplicit = 1;
			dc->DatumType           = DT_FRAME_LENGTH;
			dc->StartOfConfigurationFrameCount++;
		}
		else if ((datum & PFX_9_BIT_CONTENT_MASK) == PFX_CHIP_CHAN_HIT_CNT)
		{
			dc->ChipIndex           = GET_CHIP_IX_CHC(datum);
			dc->ChannelHitCount     = GET_CHAN_HIT_CNT(datum);
			dc->ItemType            = IT_CHANNEL_HIT_COUNT;
			dc->isItemComplete      = 1;
			dc->isDatumTypeImplicit = 0;
			dc->DatumType           = DT_UNKNOWN_O;
			dc->EventSizeFound = dc->EventSizeFound + sizeof(unsigned short);
		}
		else if ((datum & PFX_9_BIT_CONTENT_MASK) == PFX_FRAME_SEQ_NB)
		{
			dc->ItemType            = IT_FRAME_SEQUENCE_NUMBER;
			dc->FrameSequenceNumber = GET_FRAME_SEQ_NB(datum);
			dc->isDatumTypeImplicit = 0;
			dc->isItemComplete      = 1;
			dc->DatumType           = DT_FRAME_SEQUENCE_NUMBER;
		}

		// Is it a prefix for 8-bit content?
		else if ((datum & PFX_8_BIT_CONTENT_MASK) == PFX_ASCII_MSG_LEN)
		{
			dc->ItemType              = IT_SHORT_MESSAGE;
			dc->isDatumTypeImplicit   = 1;
			dc->DatumType             = DT_ASCII_CHAR;
			dc->MessageExpectedLength = GET_ASCII_LEN(datum);
			dc->MessageCurrentPtr     = 0;
			sprintf(&(dc->MessageString[0]), "");
			dc->ShortMessageCount++;
		}
		else if ((datum & PFX_8_BIT_CONTENT_MASK) == PFX_START_OF_EVENT)
		{
			dc->ItemType            = IT_START_OF_EVENT;
			dc->EventType           = GET_SOE_EV_TYPE(datum);
			dc->SourceType          = GET_SOE_SOURCE_TYPE(datum);
			dc->SourceId            = GET_SOE_SOURCE_ID(datum);
			dc->isDatumTypeImplicit = 1;
			dc->DatumType           = DT_EVENT_TIME_STAMP_LSB;
			dc->EventNumber         = 0xFFFFFFFF;
			dc->EventTimeStampLsb   = 0xFFFF;
			dc->EventTimeStampMid   = 0xFFFF;
			dc->EventTimeStampMsb   = 0xFFFF;
			dc->EventSize           = 0;
			if (dc->SourceType == SRC_TYPE_FRONT_END)
			{
				dc->StartOfEventFeCount++;
				dc->EventSizeFound = dc->EventSizeFound + sizeof(unsigned short);
			}
			else if (dc->SourceType == SRC_TYPE_BACK_END)
			{
				dc->StartOfEventBeCount++;
				dc->EventSizeFound = sizeof(unsigned short);
			}
			else
			{
				// should not have any other source of Start Of Event
			}
		}

		// Is it a prefix for 6-bit content?
		else if ((datum & PFX_6_BIT_CONTENT_MASK) == PFX_END_OF_EVENT)
		{
			dc->ItemType            = IT_END_OF_EVENT;
			dc->SourceType          = GET_EOE_SOURCE_TYPE(datum);
			dc->SourceId            = GET_EOE_SOURCE_ID(datum);
			dc->isDatumTypeImplicit = 1;
			dc->DatumType           = DT_END_OF_EVENT_RSVD;
			if (dc->SourceType == SRC_TYPE_FRONT_END)
			{
				dc->EndOfEventFeCount++;
			}
			else if (dc->SourceType == SRC_TYPE_BACK_END)
			{
				dc->EndOfEventBeCount++;
			}
			else
			{
				// should not have any other source of Start Of Event
			}
			dc->EventSizeFound = dc->EventSizeFound + sizeof(unsigned short);
		}
		else if ((datum & PFX_6_BIT_CONTENT_MASK) == PFX_BERT_STAT)
		{
			sprintf(&(dc->ErrorString[0]), "Datum(%d) PFX_BERT_STAT : interpretation not implemented.", dc->DatumCount);
			return(-1);
		}

		// Is it a prefix for 4-bit content?
		else if ((datum & PFX_4_BIT_CONTENT_MASK) == PFX_START_OF_EVENT_MINOS)
		{
			sprintf(&(dc->ErrorString[0]), "Datum(%d) PFX_START_OF_EVENT_MINOS : interpretation not implemented.", dc->DatumCount);
			return(-1);
		}
		else if ((datum & PFX_4_BIT_CONTENT_MASK) == PFX_END_OF_EVENT_MINOS)
		{
			sprintf(&(dc->ErrorString[0]), "Datum(%d) PFX_END_OF_EVENT_MINOS : interpretation not implemented.", dc->DatumCount);
			return(-1);
		}
		else if ((datum & PFX_4_BIT_CONTENT_MASK) == PFX_EXTD_CARD_CHIP_LAST_CELL_READ)
		{
			sprintf(&(dc->ErrorString[0]), "Datum(%d) PFX_EXTD_CARD_CHIP_LAST_CELL_READ : interpretation not implemented.", dc->DatumCount);
			return(-1);
		}

		// Is it a prefix for 2-bit content?
		else if ((datum & PFX_2_BIT_CONTENT_MASK) == PFX_CH_HIT_CNT_HISTO)
		{
			sprintf(&(dc->ErrorString[0]), "Datum(%d) PFX_CH_HIT_CNT_HISTO : interpretation not implemented.", dc->DatumCount);
			return(-1);
		}

		// Is it a prefix for 1-bit content?


		// Is it a prefix for 0-bit content?
		else if ((datum & PFX_0_BIT_CONTENT_MASK) == PFX_EXTD_CARD_CHIP_CHAN_H_MD)
		{
			dc->ItemType            = IT_PED_HISTO_MD;
			dc->isDatumTypeImplicit = 1;
			dc->DatumType           = DT_PED_HISTO_MD_CARD_CHIP_CHANNEL;
		}
		else if ((datum & PFX_0_BIT_CONTENT_MASK) == PFX_EXTD_CARD_CHIP_CHAN_HIT_IX)
		{
			dc->ItemType            = IT_CHANNEL_HIT_HEADER;
			dc->isDatumTypeImplicit = 1;
			dc->DatumType           = DT_CHANNEL_DATA_CARD_CHIP_CHANNEL;
			dc->EventSizeFound = dc->EventSizeFound + sizeof(unsigned short);
		}
		else if ((datum & PFX_0_BIT_CONTENT_MASK) == PFX_EXTD_CARD_CHIP_CHAN_HISTO)
		{
			sprintf(&(dc->ErrorString[0]), "Datum(%d) PFX_EXTD_CARD_CHIP_CHAN_HISTO : interpretation not implemented.", dc->DatumCount);
			return(-1);
		}
		else if ((datum & PFX_0_BIT_CONTENT_MASK) == PFX_END_OF_FRAME)
		{
			dc->ItemType            = IT_END_OF_FRAME;
			dc->isItemComplete      = 1;
			dc->isDatumTypeImplicit = 0;
			dc->DatumType           = DT_UNKNOWN_O;
		}
		else if (datum == PFX_NULL_CONTENT)
		{
			dc->ItemType = IT_NULL_DATUM;
			dc->isItemComplete = 1;
			dc->isDatumTypeImplicit = 0;
			dc->DatumType = DT_UNKNOWN_O;
			dc->EventSizeFound = dc->EventSizeFound + sizeof(unsigned short);
		}
		else if ((datum == PFX_DEADTIME_HSTAT_BINS) || (datum == PFX_EVPERIOD_HSTAT_BINS))
		{
			sprintf(&(dc->ErrorString[0]), "Datum(%d) PFX_DEADTIME_HSTAT_BINS PFX_EVPERIOD_HSTAT_BINS : interpretation not implemented.", dc->DatumCount);
			return(-1);
		}
		else if (datum == PFX_PEDESTAL_HSTAT)
		{
			sprintf(&(dc->ErrorString[0]), "Datum(%d) PFX_PEDESTAL_HSTAT : interpretation not implemented.", dc->DatumCount);
			return(-1);
		}
		else if (datum == PFX_PEDESTAL_H_MD)
		{
			sprintf(&(dc->ErrorString[0]), "Datum(%d) PFX_PEDESTAL_H_MD : interpretation not implemented.", dc->DatumCount);
			return(-1);
		}
		else if (datum == PFX_SHISTO_BINS)
		{
			sprintf(&(dc->ErrorString[0]), "Datum(%d) PFX_SHISTO_BINS : interpretation not implemented.", dc->DatumCount);
			return(-1);
		}
		else if (datum == PFX_CMD_STATISTICS)
		{
			sprintf(&(dc->ErrorString[0]), "Datum(%d) PFX_CMD_STATISTICS : interpretation not implemented.", dc->DatumCount);
			return(-1);
		}
		else if (datum == PFX_START_OF_BUILT_EVENT)
		{
			dc->ItemType            = IT_START_OF_BUILT_EVENT;
			dc->isItemComplete      = 1;
			dc->isDatumTypeImplicit = 0;
			dc->DatumType           = DT_UNKNOWN_O;
			dc->StartOfBuiltEventCount++;
		}
		else if (datum == PFX_END_OF_BUILT_EVENT)
		{
			dc->ItemType            = IT_END_OF_BUILT_EVENT;
			dc->isItemComplete      = 1;
			dc->isDatumTypeImplicit = 0;
			dc->DatumType           = DT_UNKNOWN_O;
			dc->EndOfBuiltEventCount++;
		}
		else if (datum == PFX_SOBE_SIZE)
		{
			sprintf(&(dc->ErrorString[0]), "Datum(%d) PFX_SOBE_SIZE : interpretation not implemented.", dc->DatumCount);
			return(-1);
		}
		else if (datum == PFX_LONG_ASCII_MSG)
		{
			dc->ItemType            = IT_LONG_MESSAGE;
			dc->isDatumTypeImplicit = 1;
			dc->DatumType           = DT_ASCII_MSG_LENGTH;
		}
		else if (datum == PFX_EXTD_PEDTHR_LIST)
		{
			dc->isDatumTypeImplicit = 1;
			dc->DatumType           = DT_PED_THR_LIST_HEADER;
		}
		// No interpretable data
		else
		{
			sprintf(&(dc->ErrorString[0]), "Datum(%d) 0x%04x: found no matching prefix", dc->DatumCount, datum);
			return(-1);
		}
	}

	// Datum was sucessfully decoded
	dc->DatumOkCount = dc->DatumOkCount + 1;

	return(0);
}

/*******************************************************************************
Item_PrintFilter_Init
*******************************************************************************/
void Item_PrintFilter_Init(PrintFilter *pf)
{
	pf->flags                  = 0x0;
	pf->isCondensedFormat      = 0;
	pf->isAnyEvent             = 1;
	pf->EventIndexMin          = 0;
	pf->EventIndexMax          = 0;
	pf->isAnyCard              = 1;
	pf->CardIndexMin           = 0;
	pf->CardIndexMax           = 0;
	pf->isAnyChip              = 1;
	pf->ChipIndexMin           = 0;
	pf->ChipIndexMax           = 0;
	pf->isAnyChannel           = 1;
	pf->ChannelIndexMin        = 0;
	pf->ChannelIndexMax        = 0;
	pf->isAnySampleIndex       = 1;
	pf->AbsoluteSampleIndexMin = 0;
	pf->AbsoluteSampleIndexMax = 0;
	pf->isAnySampleAmpl        = 1;
	pf->SampleAmplMin          = 0;
	pf->SampleAmplMax          = 0;
}

/*******************************************************************************
Item_Print
*******************************************************************************/
int Item_Print(void *fp, DatumContext *dc, PrintFilter *pf)
{
	char source_type[4];
	float ped_mean;
	float ped_dev;

	if (dc->isItemComplete)
	{
		switch (dc->ItemType)
		{
		case IT_UNKNOWN:
			if (pf->flags & IT_UNKNOWN)
			{
				fprintf((FILE *)fp, "Unknown item\n");
			}
			break;

		case IT_SHORT_MESSAGE:
			if (pf->flags & IT_SHORT_MESSAGE)
			{
				fprintf((FILE *)fp, "%s\n", dc->MessageString);
			}
			break;

		case IT_LONG_MESSAGE:
			if (pf->flags & IT_LONG_MESSAGE)
			{
				fprintf((FILE *)fp, "%s\n", dc->MessageString);
			}
			break;


		case IT_DATA_FRAME:
			if (pf->flags & IT_DATA_FRAME)
			{
				SourceTypeToString(dc->FrameSourceType, &source_type[0]);
				fprintf((FILE *)fp, "--- Start of Data Frame (V.%01d) %s %02d (%4d bytes) --\n", dc->FramingVersion, &source_type[0], dc->FrameSourceId, dc->FrameSizeExpected);
			}
			break;

		case IT_MONITORING_FRAME:
			if (pf->flags & IT_MONITORING_FRAME)
			{
				SourceTypeToString(dc->FrameSourceType, &source_type[0]);
				fprintf((FILE *)fp, "--- Start of Monitoring Frame (V.%01d) %s %02d (%4d bytes) --\n", dc->FramingVersion, &source_type[0], dc->FrameSourceId, dc->FrameSizeExpected);
			}
			break;

		case IT_END_OF_FRAME:
			if (pf->flags & IT_END_OF_FRAME)
			{
				fprintf((FILE *)fp, "----- End of Frame -----\n");
			}
			break;

		case IT_START_OF_EVENT:
			if (pf->flags & IT_START_OF_EVENT)
			{
				if (
					((pf->isAnyEvent) || ((dc->EventNumber >= pf->EventIndexMin) && (dc->EventNumber <= pf->EventIndexMax)))
					)
				{
					SourceTypeToString(dc->SourceType, &source_type[0]);
					fprintf((FILE *)fp, "-- Start of Event (Type %01d From %s %02d) --\n", dc->EventType, &source_type[0], dc->SourceId);
					fprintf((FILE *)fp, "Time 0x%04x 0x%04x 0x%04x\n", dc->EventTimeStampMsb, dc->EventTimeStampMid, dc->EventTimeStampLsb);
					fprintf((FILE *)fp, "Event_Number 0x%08x (%d)\n", dc->EventNumber, dc->EventNumber);
				}
			}
			break;

		case IT_CHANNEL_HIT_COUNT:
			if (pf->flags & IT_CHANNEL_HIT_COUNT)
			{
				if (
					   ((pf->isAnyEvent) || ((dc->EventNumber >= pf->EventIndexMin) && (dc->EventNumber <= pf->EventIndexMax)))
					&& ((pf->isAnyCard)  || ((dc->CardIndex   >= pf->CardIndexMin)  && (dc->CardIndex   <= pf->CardIndexMax)))
					&& ((pf->isAnyChip)  || ((dc->ChipIndex   >= pf->ChipIndexMin)  && (dc->ChipIndex   <= pf->ChipIndexMax)))
					)
				{
					fprintf((FILE *)fp, "Chip %2d Channel_Hit_Count %2d\n", dc->ChipIndex, dc->ChannelHitCount);
				}
			}
			break;

		case IT_LAST_CELL_READ:
			if (pf->flags & IT_LAST_CELL_READ)
			{
				if (
					   ((pf->isAnyEvent) || ((dc->EventNumber >= pf->EventIndexMin) && (dc->EventNumber <= pf->EventIndexMax)))
					&& ((pf->isAnyCard)  || ((dc->CardIndex   >= pf->CardIndexMin)  && (dc->CardIndex   <= pf->CardIndexMax)))
					&& ((pf->isAnyChip)  || ((dc->ChipIndex   >= pf->ChipIndexMin)  && (dc->ChipIndex   <= pf->ChipIndexMax)))
					)
				{
					fprintf((FILE *)fp, "Chip %2d Last_Cell_Read 0x%03x (%3d) \n", dc->ChipIndex, dc->LastCellRead, dc->LastCellRead);
				}
			}
			break;


		case IT_END_OF_EVENT:
			if (pf->flags & IT_END_OF_EVENT)
			{
				if (
					((pf->isAnyEvent) || ((dc->EventNumber >= pf->EventIndexMin) && (dc->EventNumber <= pf->EventIndexMax)))
					)
				{
					SourceTypeToString(dc->FrameSourceType, &source_type[0]);
					fprintf((FILE *)fp, "----- End of Event ----- (from %s %02d - size %d bytes)\n", &source_type[0], dc->SourceId, dc->EventSize);
				}
			}
			break;

		case IT_CHANNEL_HIT_HEADER:
			if (pf->flags & IT_CHANNEL_HIT_HEADER)
			{
				if (
					   ((pf->isAnyEvent)   || ((dc->EventNumber  >= pf->EventIndexMin)   && (dc->EventNumber  <= pf->EventIndexMax)))
					&& ((pf->isAnyCard)    || ((dc->CardIndex    >= pf->CardIndexMin)    && (dc->CardIndex    <= pf->CardIndexMax)))
					&& ((pf->isAnyChip)    || ((dc->ChipIndex    >= pf->ChipIndexMin)    && (dc->ChipIndex    <= pf->ChipIndexMax)))
					&& ((pf->isAnyChannel) || ((dc->ChannelIndex >= pf->ChannelIndexMin) && (dc->ChannelIndex <= pf->ChannelIndexMax)))
					)
				{
					fprintf((FILE *)fp, "Card %02d Chip %02d Channel %02d\n", dc->CardIndex, dc->ChipIndex, dc->ChannelIndex);
				}
			}
			break;

		case IT_TIME_BIN_INDEX:
			if (pf->flags & IT_TIME_BIN_INDEX)
			{
				if (
					   ((pf->isAnyEvent)       || ((dc->EventNumber  >= pf->EventIndexMin)          && (dc->EventNumber  <= pf->EventIndexMax)))
					&& ((pf->isAnyCard)        || ((dc->CardIndex    >= pf->CardIndexMin)           && (dc->CardIndex    <= pf->CardIndexMax)))
					&& ((pf->isAnyChip)        || ((dc->ChipIndex    >= pf->ChipIndexMin)           && (dc->ChipIndex    <= pf->ChipIndexMax)))
					&& ((pf->isAnyChannel)     || ((dc->ChannelIndex >= pf->ChannelIndexMin)        && (dc->ChannelIndex <= pf->ChannelIndexMax)))
					&& ((pf->isAnySampleIndex) || ((dc->TimeBinIndex >= pf->AbsoluteSampleIndexMin) && (dc->TimeBinIndex <= pf->AbsoluteSampleIndexMax)))
					)
				{
					fprintf((FILE *)fp, "Time_Bin: %03d\n", dc->TimeBinIndex);
				}
			}
			break;

		case IT_ADC_SAMPLE:
			if (pf->flags & IT_ADC_SAMPLE)
			{
				if (
					   ((pf->isAnyEvent)       || ((dc->EventNumber         >= pf->EventIndexMin)          && (dc->EventNumber         <= pf->EventIndexMax)))
					&& ((pf->isAnyCard)        || ((dc->CardIndex           >= pf->CardIndexMin)           && (dc->CardIndex           <= pf->CardIndexMax)))
					&& ((pf->isAnyChip)        || ((dc->ChipIndex           >= pf->ChipIndexMin)           && (dc->ChipIndex           <= pf->ChipIndexMax)))
					&& ((pf->isAnyChannel)     || ((dc->ChannelIndex        >= pf->ChannelIndexMin)        && (dc->ChannelIndex        <= pf->ChannelIndexMax)))
					&& ((pf->isAnySampleIndex) || ((dc->AbsoluteSampleIndex >= pf->AbsoluteSampleIndexMin) && (dc->AbsoluteSampleIndex <= pf->AbsoluteSampleIndexMax)))
					&& ((pf->isAnySampleAmpl)  || ((dc->AdcSample           >= pf->SampleAmplMin)          && (dc->AdcSample           <= pf->SampleAmplMax)))
					)
				{
					if (pf->isCondensedFormat)
					{
						fprintf((FILE *)fp, "%03d %4d\n", dc->AbsoluteSampleIndex, dc->AdcSample);
					}
					else
					{
						fprintf((FILE *)fp, "Relative_Index: %03d Absolute_Index: %03d Amplitude: 0x%04x (%4d)\n", dc->RelativeSampleIndex, dc->AbsoluteSampleIndex, dc->AdcSample, dc->AdcSample);
					}
				}
			}
			break;

		case IT_NULL_DATUM:
			if (pf->flags & IT_NULL_DATUM)
			{
				fprintf((FILE *)fp, "(null datum)\n");
			}
			break;

		case IT_START_OF_BUILT_EVENT:
			if (pf->flags & IT_START_OF_BUILT_EVENT)
			{
				fprintf((FILE *)fp, "***** Start of Built Event *****\n");
			}
			break;

		case IT_END_OF_BUILT_EVENT:
			if (pf->flags & IT_END_OF_BUILT_EVENT)
			{
				fprintf((FILE *)fp, "***** End of Built Event *****\n");
			}
			break;

		case IT_PED_HISTO_MD:
			if (pf->flags & IT_PED_HISTO_MD)
			{
				ped_mean = ((float) dc->PedestalMean) / 100.0;
				ped_dev  = ((float) dc->PedestalDev)  / 100.0;
				fprintf((FILE *)fp, "Card %02d Chip %02d Channel %02d Mean/Std_dev : %.2f  %.2f\n", dc->CardIndex, dc->ChipIndex, dc->ChannelIndex, ped_mean, ped_dev);
			}
			break;

		case IT_CHAN_PED_CORRECTION:
			if (pf->flags & IT_CHAN_PED_CORRECTION)
			{
				fprintf((FILE *)fp, "Card %02d Chip %02d Channel %02d Ped_Correct %+03d\n", dc->CardIndex, dc->ChipIndex, dc->ChannelIndex, dc->PedestalCorrection);
			}
			break;

		case IT_CHAN_ZERO_SUPPRESS_THRESHOLD:
			if (pf->flags & IT_CHAN_PED_CORRECTION)
			{
				fprintf((FILE *)fp, "Card %02d Chip %02d Channel %02d Zero_Sup_Thr %03d\n", dc->CardIndex, dc->ChipIndex, dc->ChannelIndex, dc->ZeroSuppressThreshold);
			}
			break;

		case IT_FRAME_SEQUENCE_NUMBER:
			if (pf->flags & IT_DATA_FRAME)
			{
				fprintf((FILE *)fp, "--- UDP Frame Sequence Number 0x%03x\n", dc->FrameSequenceNumber);
			}
			break;

		default:
			fprintf((FILE *)fp, "Error: unknown item 0x%08x\n", dc->ItemType);
			return(-1);
			break;
		}
	}
	return(0);
}
