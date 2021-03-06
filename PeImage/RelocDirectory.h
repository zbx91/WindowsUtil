#pragma once
#include "PeImageFwd.h"
#include "DirectoryBase.h"
namespace PeDecoder
{
	// UNDONE: 
	class RelocDirectory :
		public Detail::DirectoryBase<DataDirectoryEntryType::BaseReloc>
	{
	public:
		typedef RelocDirectoryIterator iterator;
		RelocDirectory(PeImage& pe, TDataPtr ptr, TSizePtr sizePtr);
		~RelocDirectory() = default;
		iterator begin();
		iterator end();
	};
	
}  // namespace PeDecoder
