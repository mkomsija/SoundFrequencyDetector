#include "notes.h"
#include "math.h"
const uint16_t frequencies[77] = { 
	16,
	17,
	18,
	19,
	21,
	22,
	23,
	25,
	26,
	28,
	29,
	31,
	33,
	35,
	37,
	39,
	41,
	44,
	46,
	49,
	52,
	55,
	58,
	62,
	65,
	69,
	73,
	78,
	82,
	87,
	93,
	98,
	104,
	110,
	117,
	123,
	131,
	139,
	147,
	156,
	165,
	175,
	185,
	196,
	208,
	220,
	233,
	247,
	262,
	277,
	294,
	311,
	330,
	349,
	370,
	392,
	415,
	440,
	466,
	494,
	523,
	554,
	587,
	622,
	659,
	698,
	740,
	784,
	831,
	880,
	932,
	988,
	1047,
	1109,
	1175,
	1245,
	1319
};

const uint8_t notes[77][4] =
{		
"C0 ",
"C#0",
"D0	",
"D#0",
"E0	",
"F0	",
"F#0",
"G0	",
"G#0",
"A0	",
"A#0",
"B0	",
"C1	",
"C#1",
"D1	",
"D#1",
"E1	",
"F1	",
"F#1",
"G1	",
"G#1",
"A1	",
"A#1",
"B1	",
"C2	",
"C#2",
"D2	",
"D#2",
"E2	",
"F2	",
"F#2",
"G2	",
"G#2",
"A2	",
"A#2",
"B2	",
"C3	",
"C#3",
"D3	",
"D#3",
"E3	",
"F3	",
"F#3",
"G3	",
"G#3",
"A3	",
"A#3",
"B3	",
"C4	",
"C#4",
"D4	",
"D#4",
"E4	",
"F4	",
"F#4",
"G4	",
"G#4",
"A4	",
"A#4",
"B4	",
"C5	",
"C#5",
"D5	",
"D#5",
"E5	",
"F5	",
"F#5",
"G5	",
"G#5",
"A5	",
"A#5",
"B5	",
"C6	",
"C#6",
"D6	",
"D#6",
"E6	"
};


uint8_t getClosest(uint16_t smaller, uint16_t bigger, uint16_t foundFrequency, uint8_t mid, uint8_t option)
{
	if ( foundFrequency - smaller >= bigger - foundFrequency)
	{
		if (option == 1)
			return mid;
		else 
			return mid+1;
	}
	else
	{
		if (option == 1)
			return mid-1;
		else 
			return mid;
	}	
}

uint8_t mid = 0;

uint8_t findClosestNote(uint16_t foundFrequency)
{
	if (foundFrequency <= frequencies[0])
	{
		return 1;
	}
	if (foundFrequency >= frequencies[76])
	{
		return 76;
	}
	
	uint8_t i = 0;
	uint8_t j = 76;
	mid = 0;
	
	while(i < j)
	{
		mid = round(((float)i+j)/2);
		
		if (frequencies[mid] == foundFrequency)
		{
			return mid;
		}
		
		if (foundFrequency < frequencies[mid])
		{
			if ((mid > 0) && (foundFrequency >= frequencies[mid-1]))
			{
				return getClosest(frequencies[mid-1], frequencies[mid], foundFrequency, mid, 1);
			}
			j = mid-1;
		}
		else
		{	
			if ((mid < 76) && (foundFrequency < frequencies[mid+1]))
			{
				return getClosest(frequencies[mid], frequencies[mid+1], foundFrequency, mid, 2);
			}
			i = mid;
		}
	}
	return mid;
}
