#ifndef UTILITY_XNZip
#define	UTILITY_XNZip

namespace UTILITY
{

namespace XNZip
{

#define XNZIP_BEST (9)
#define XNZIP_FASTEST (1)

	void Compress( void * pSource, unsigned long uSourceSize, void * pDestination, unsigned long & uDestinationSizeOut, int iLevel = 5 );

	void UnCompress( void * pSource, unsigned long uSourceSize, void * pDestination, unsigned long & uDestinationSizeOut);

};

};	// ns UTILITY

#endif
