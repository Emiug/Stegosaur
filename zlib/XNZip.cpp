#include "XNZip.h"
#include "zlib.h"
#include <string>
#include <algorithm>

namespace UTILITY
{
	namespace XNZip
	{
		void Compress( void * pSource, unsigned long uSourceSize, void * pDestination, unsigned long & uDestinationSize, int iLevel )
		{
			int ret;

			const long BlockSizeCompress=0x8000;

			z_stream zcpr;

			long lOrigToDo = uSourceSize;
			long lOrigDone = 0;

			int step=0;
			memset(&zcpr,0,sizeof(z_stream));

			deflateInit(&zcpr,iLevel);

			zcpr.next_in = (Bytef*)pSource;
			zcpr.next_out = (Bytef*)pDestination;

			do
			{
				long all_read_before = zcpr.total_in;
				zcpr.avail_in = std::min(lOrigToDo,BlockSizeCompress);
				zcpr.avail_out = BlockSizeCompress;
				ret=deflate(&zcpr,(zcpr.avail_in==lOrigToDo) ? Z_FINISH : Z_SYNC_FLUSH);
				lOrigDone += (zcpr.total_in-all_read_before);
				lOrigToDo -= (zcpr.total_in-all_read_before);
				step++;

			} while (ret==Z_OK);

			uDestinationSize = zcpr.total_out;
			deflateEnd(&zcpr);
		}

		void UnCompress( void * pSource, unsigned long uSourceSize, void * pDestination, unsigned long & uDestinationSizeOut )
		{
			 long BlockSizeUncompress=0x8000;
			z_stream zcpr;
			int ret=Z_OK;
			long lOrigToDo = uSourceSize;
			long lOrigDone = 0;
			int step=0;
			memset(&zcpr,0,sizeof(z_stream));
			inflateInit(&zcpr);

			zcpr.next_in = (Bytef*)pSource;
			zcpr.next_out = (Bytef*)pDestination;

			do
			{
				long all_read_before = zcpr.total_in;
				zcpr.avail_in = std::min(lOrigToDo,BlockSizeUncompress);
				zcpr.avail_out = BlockSizeUncompress;
				ret=inflate(&zcpr,Z_SYNC_FLUSH);
				lOrigDone += (zcpr.total_in-all_read_before);
				lOrigToDo -= (zcpr.total_in-all_read_before);
				step++;
			} while (ret==Z_OK);

			uDestinationSizeOut=zcpr.total_out;
			inflateEnd(&zcpr);
		}
	};

};	// ns UTILITY

