/*
 *	Alan Abram - Stegosaur - Simple Steganography program.
 */
#include "XNZip.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef unsigned char byte;

// BMP headers don't pack as we would want
#pragma pack(1)
struct BITMAPFILEHEADER
{
	unsigned short _type;
	unsigned long _size;
	unsigned short _reserved1;
	unsigned short _reserved2;
	unsigned long _offBits;
};

struct BITMAPINFOHEADER
{
	unsigned long biSize;
	long biWidth;
	long biHeight;
	unsigned short biPlanes;
	unsigned short biBitCount;
	unsigned long biCompression;
	unsigned long biSizeImage;
	long biXPelsPerMeter;
	long biYPelsPerMeter;
	unsigned long biClrUsed;
	unsigned long biClrImportant;
};
//End BMP Headers

struct BMPHEADERS
{
	BITMAPFILEHEADER aFileHeader;
	BITMAPINFOHEADER aInfoHeader;
};

struct SplitByte
{
	byte _red : 3;
	byte _green : 2;
	byte _blue : 3;
};

struct ByteBitMask
{
	byte _0 : 1;
	byte _1 : 1;
	byte _2 : 1;
	byte _3 : 1;
	byte _4 : 1;
	byte _5 : 1;
	byte _6 : 1;
	byte _7 : 1;
};

struct RGB24
{
	union
	{
		byte _red;
		ByteBitMask _redMask;
	};
	union
	{
		byte _green;
		ByteBitMask _greenMask;
	};
	union
	{
		byte _blue;
		ByteBitMask _blueMask;
	};
};

struct ImageData
{
	union
	{
		byte* _data;
		BMPHEADERS* _headers;
	};
	long _dataLength;
};

struct EncodedDataHeader
{
	unsigned long _filesize;
	char _format[3];
	char _term;
};

int encode(const char* inputFile, ImageData& inputImageData)
{
	int status = 0;

	// Open the file we wish to encode into our image.
	FILE* srcFile = 0;
	fopen_s(&srcFile, inputFile, "rb");

	if (srcFile == 0)
	{
		printf("Failed to open %s, please ensure the file exists\n", inputFile);
		return -1;
	}

	printf("File %s opened for Encode\n", inputFile);

	fseek(srcFile, 0L, SEEK_END);
	long srcFileDataSize = ftell(srcFile);
	rewind(srcFile);

	byte* srcFileData = new byte[srcFileDataSize + 1];
	fread(srcFileData, srcFileDataSize, 1, srcFile);
	srcFileData[srcFileDataSize] = '\0';
	fclose(srcFile);

	// Allocate a larger buffer than required in case compression increases size.
	unsigned long ulSizeOut = srcFileDataSize * 2;

	byte* encodedDataBuffer = new byte[ulSizeOut];
	// Reserve some space for our header
	byte* compressedEncodeDataStart = encodedDataBuffer + sizeof(EncodedDataHeader);

	//Make header for our buffer
	EncodedDataHeader* encodedHeader = reinterpret_cast<EncodedDataHeader*>(encodedDataBuffer);

	//Compress Data into the buffer, after our "header"
	UTILITY::XNZip::Compress(srcFileData, srcFileDataSize, compressedEncodeDataStart, ulSizeOut, XNZIP_BEST); //Max Compression
	encodedHeader->_filesize = ulSizeOut;

	// Take the destination extension (limit 3) chars so we can assign the correct extension after
	int iLen = strlen(inputFile);
	encodedHeader->_format[0] = inputFile[iLen - 3];
	encodedHeader->_format[1] = inputFile[iLen - 2];
	encodedHeader->_format[2] = inputFile[iLen - 1];
	encodedHeader->_term = 0;

	ulSizeOut += sizeof(EncodedDataHeader);

	//Acquire Data
	byte* dataBuff = inputImageData._data + sizeof(BMPHEADERS);
	const int destinationPixelsAvailable =
		inputImageData._headers->aInfoHeader.biWidth * inputImageData._headers->aInfoHeader.biHeight;
	int inputBytesRemaining = sizeof(EncodedDataHeader) + ulSizeOut;

	// 1 RGB Pixel == 1 Byte of data
	if (inputBytesRemaining > destinationPixelsAvailable)
	{
		printf("Data to be encoded is greater than the available space");
		status = -2;
	}
	else
	{
		RGB24* dstRGBData = (RGB24*)dataBuff;
		SplitByte* encodeData = (SplitByte*)encodedDataBuffer;

		while (inputBytesRemaining > 0)
		{
			byte bR = encodeData->_red;

			ByteBitMask* srcRed = (ByteBitMask*)&bR;
			dstRGBData->_redMask._0 = srcRed->_0;
			dstRGBData->_redMask._1 = srcRed->_1;
			dstRGBData->_redMask._2 = srcRed->_2;

			byte bG = encodeData->_green;
			ByteBitMask* srcGreen = (ByteBitMask*)&bG;
			dstRGBData->_greenMask._0 = srcGreen->_0;
			dstRGBData->_greenMask._1 = srcGreen->_1;

			byte bB = encodeData->_blue;
			ByteBitMask* pbBe = (ByteBitMask*)&bB;
			dstRGBData->_blueMask._0 = pbBe->_0;
			dstRGBData->_blueMask._1 = pbBe->_1;
			dstRGBData->_blueMask._2 = pbBe->_2;

			dstRGBData++;
			encodeData++;
			inputBytesRemaining--;
		}

		const char* dstFilePath = "encode_out.bmp";
		FILE* outFile = 0;
		fopen_s(&outFile, dstFilePath, "wb");
		if (outFile)
		{
			fwrite(inputImageData._data, inputImageData._dataLength, 1, outFile);
			fclose(outFile);
		}
		else
		{
			printf("Failed to open %s for write\n", dstFilePath);
			status = -3;
		}
	}

	delete[] encodedDataBuffer;
	delete[] srcFileData;

	return status;
}

int decode(ImageData& inputImageData)
{
	int status = 0;

	const BMPHEADERS* bmpHeader = reinterpret_cast<BMPHEADERS*>(inputImageData._data);

	int numElementsToRead = bmpHeader->aInfoHeader.biWidth * bmpHeader->aInfoHeader.biHeight;

	RGB24* srcRGBData = (RGB24*)(inputImageData._data + sizeof(BMPHEADERS));

	byte* buffer = new byte[numElementsToRead + 1];
	byte* currentByte = buffer;

	while (numElementsToRead > 0)
	{
		byte rebuiltByte;
		ByteBitMask* rebuiltByteMask = (ByteBitMask*)&rebuiltByte;

		ByteBitMask* redMask = (ByteBitMask*)&srcRGBData->_red;
		ByteBitMask* greenMask = (ByteBitMask*)&srcRGBData->_green;
		ByteBitMask* blueMask = (ByteBitMask*)&srcRGBData->_blue;

		rebuiltByteMask->_0 = redMask->_0;
		rebuiltByteMask->_1 = redMask->_1;
		rebuiltByteMask->_2 = redMask->_2;

		rebuiltByteMask->_3 = greenMask->_0;
		rebuiltByteMask->_4 = greenMask->_1;

		rebuiltByteMask->_5 = blueMask->_0;
		rebuiltByteMask->_6 = blueMask->_1;
		rebuiltByteMask->_7 = blueMask->_2;

		*currentByte = rebuiltByte;

		srcRGBData++;
		currentByte++;
		numElementsToRead--;
	}

	EncodedDataHeader* decodedHeader = reinterpret_cast<EncodedDataHeader*>(buffer);

	unsigned long uLengthIn = decodedHeader->_filesize;
	byte* pStartBuffer = buffer + sizeof(EncodedDataHeader);

	unsigned long uLength = uLengthIn * 2;
	byte* decodeTmpBuffer = new byte[uLength];

	UTILITY::XNZip::UnCompress(pStartBuffer, uLengthIn, decodeTmpBuffer, uLength);

	char cOutput[16];
	sprintf_s(cOutput, sizeof(cOutput), "decode.%s", decodedHeader->_format);

	FILE* outFile = 0;
	fopen_s(&outFile, cOutput, "wb");
	if (outFile)
	{
		fwrite(decodeTmpBuffer, uLength, 1, outFile);
		fclose(outFile);
	}
	else
	{
		status = -2;
	}
	delete[] decodeTmpBuffer;
	delete[] buffer;
	return status;
};


int main(int argc, const char* argv[])
{
	if (argc < 2)
	{
		printf(
			"Format is either \n -encode <Encode into> <File to be encoded>\n or -decode <file>");
		return -1;
	}

	const char* inputCommand = argv[1];
	const char* fileToEncodeIntoPath = argv[2];
	const char* fileToEncodePath = argv[3];

	bool shouldEncode = false;

	if (_stricmp(inputCommand, "-encode") != 0 && _stricmp(inputCommand, "-decode") != 0)
	{
		printf("Invalid switch, please use -encode or -decode or drop a file on to encode\n");
		return -1;
	}
	else
	{
		shouldEncode = (_stricmp(inputCommand, "-encode") == 0);
	}

	//// Image file to encode
	FILE* fileToEncodeInto = 0;
	fopen_s(&fileToEncodeInto, fileToEncodeIntoPath, "rb");

	if (fileToEncodeInto == 0)
	{
		printf("Failed to open %s, please ensure the file exists\n", fileToEncodeIntoPath);
		return -1;
	}

	printf("File %s opened for Encode\n", fileToEncodeIntoPath);

	ImageData imgData;

	fseek(fileToEncodeInto, 0L, SEEK_END);
	imgData._dataLength = ftell(fileToEncodeInto);
	rewind(fileToEncodeInto);

	imgData._data = new byte[imgData._dataLength];
	fread(imgData._data, imgData._dataLength, 1, fileToEncodeInto);
	fclose(fileToEncodeInto);

	int returnVal;
	if (shouldEncode)
	{
		returnVal = encode(fileToEncodePath, imgData);
	}
	else
	{
		returnVal = decode(imgData);
	}

	delete[] imgData._data;

	return returnVal;
}
