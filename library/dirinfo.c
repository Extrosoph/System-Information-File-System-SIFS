#include "sifs-internal.h"


// get information about a requested directory
int SIFS_dirinfo(const char *volumename, const char *pathname,
                 char ***entrynames, uint32_t *nentries, time_t *modtime)
{
//	ENSURE THAT THE RECEIVE PARAMETER ARE VALID
	if(volumename == NULL || pathname == NULL || entrynames == NULL || nentries == 0 || modtime == NULL)
	{
		SIFS_errno = SIFS_EINVAL;
		return 1;
	}

//	ENSURE THAT THE REQUESTED VOLUME ALREADY EXIST
	if(access(volumename, F_OK) == -1)
	{
		SIFS_errno = SIFS_ENOVOL;
		return 1;
	}


//	GET THE NUMBER OF / FROM THE DIRNAME
        char delimiter1 = '/';
	char* name = malloc(sizeof(char) * 1000);
	strcpy(name, pathname);
	char* positionTemp = name;
	int npaths = 0;
	while(positionTemp != NULL)
	{
		positionTemp = strchr(positionTemp, delimiter1);
		if(positionTemp)
		{
			positionTemp++;
			npaths++;
		}
	}

//	char **entriesn = malloc(SIFS_MAX_NAME_LENGTH *  sizeof(char));
	
//	entriesn = malloc(sizeof(char*) * (SIFS_MAX_ENTRIES+1));
//	entriesn = malloc(SIFS_MAX_NAME_LENGTH * sizeof(char));


//	OBTAINING THE FILE NAME FROM THE DIRECTORY
	FILE *cv = fopen(volumename,"r+");

//	READING THE VOLUME HEADER AND THE BITMAP
	SIFS_VOLUME_HEADER volumeHeader;
	fread(&volumeHeader, sizeof volumeHeader,1 , cv);


	SIFS_BIT oldBitmap[(int)volumeHeader.nblocks];
	fread(&oldBitmap, (int)volumeHeader.nblocks, 1, cv);

/*	READ THE ROOT DIRECTORY BLOCK
	if(npaths == 1 && strcmp(name, "/") == 0)
	{
		SIFS_DIRBLOCK rootdir_block;
		fread(&rootdir_block, sizeof rootdir_block, 1, cv);
		*nentries = rootdir_block.nentries;
		*modtime = rootdir_block.modtime;
		SIFS_DIRBLOCK entries;
		for(int i = 0; i < rootdir_block.nentries; i++)
		{
			fseek(cv, sizeof volumeHeader + sizeof oldBitmap + (int)volumeHeader.blocksize  * rootdir_block.entries[i].blockID, SEEK_SET);
			fread(&entries, (int)volumeHeader.blocksize, 1, cv);	
			entriesn[i] = malloc(sizeof(char) * (SIFS_MAX_NAME_LENGTH));
			entriesn[i] = entries.name;
		}	
		*entrynames = entriesn;

		
		char delimiter2[2] = "/";
		char *path;
		path = strtok(name, delimiter2);
//	COUNT THE NUMBER OF DIRECTORY BLOCKS IN THE BITMAP
		int ndirblocks = 0;
		for(int i = 0; i < (int)volumeHeader.nblocks; i++)
		{
			if(oldBitmap[i] == SIFS_DIR)
			{
				ndirblocks++;
			}
		}

//	FIND THE POSITION OF ALL THE DIRECTORY BLOCKS
		int dirblockpos[ndirblocks];
		int j = 0;
		for(int i = 0; i < (int)volumeHeader.nblocks; i++)
		{
			if(oldBitmap[i] == SIFS_DIR)
			{ 
				dirblockpos[j] = i;
				j++;
			}
		}
		
	}*/

		 
	fclose(cv);
	return 0;
}
