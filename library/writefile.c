#include "sifs-internal.h"

// add a copy of a new file to an existing volume
int SIFS_writefile(const char *volumename, const char *pathname,
		   void *data, size_t nbytes)
{
//	ENSURE THAT RECIEVED PARAMETER ARE VALID
	if(volumename == NULL || pathname == NULL || nbytes == 0)
	{
		SIFS_errno = SIFS_EINVAL;
		return 1;
	}

//	OPEN THE EXISTED VOLUME FOR READ AND WRITE
	FILE *cv = fopen(volumename, "r+");

//	READING THE VOLUME HEADER
	SIFS_VOLUME_HEADER volumeHeader;
	fread(&volumeHeader, sizeof volumeHeader, 1, cv);

//	READING THE BITMAP
	SIFS_BIT oldBitmap[(int)volumeHeader.nblocks];
	fread(&oldBitmap, (int)volumeHeader.nblocks, 1, cv);
    return 1;
}
