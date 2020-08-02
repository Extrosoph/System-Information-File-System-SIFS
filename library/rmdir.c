#include "sifs-internal.h"

// remove an existing directory from an existing volume
int SIFS_rmdir(const char *volumename, const char *dirname)
{
//	ENSURE THAT THE RECEIVED PARAMETER ARE VALID
	if(volumename == NULL && dirname == NULL)
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

//	GET THE PATH FROM THE DIRNAME
        char delimiter1 = '/';
	char* name = malloc(sizeof(char) * 100);
	strcpy(name, dirname);
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

//	OPEN THE EXISTED VOLUME FOR READ AND WRITE
	FILE *cv = fopen(volumename, "r+");

//	READING THE VOLUME HEADER AND THE BITMAP
	SIFS_VOLUME_HEADER volumeHeader;
	fread(&volumeHeader, sizeof volumeHeader,1 , cv);


	SIFS_BIT oldBitmap[(int)volumeHeader.nblocks];
	fread(&oldBitmap, (int)volumeHeader.nblocks, 1, cv);

//	READ AND  GET THE POSITION OF ALL THE DIRECTORY BLOCKS
	if(npaths == 1 || npaths == 0)
	{
		char delimiter2[2] = "/";
		char *path;
		path = strtok(name, delimiter2);

//	COUNT THE NUMBER OF DIRECTORY BLOCKS IN THE BITMAP EXCLUDING ROOTDIR
		int ndirblocks = 0;
		for(int i = 1; i < (int)volumeHeader.nblocks; i++)
		{
			if(oldBitmap[i] == SIFS_DIR)
			{
				ndirblocks++;
			}
		}

//	FIND THE POSITION OF ALL THE DIRECTORY BLOCKS EXCLUDING ROOTDIR
		int dirblockpos[ndirblocks];
		int j = 0;
		for(int i = 1; i < (int)volumeHeader.nblocks; i++)
		{
			if(oldBitmap[i] == SIFS_DIR)
			{
				dirblockpos[j] = i;
				j++;
			}
		}

//	READ THE ROOT DIRECTORY BLOCK
		SIFS_DIRBLOCK rootdir_block;
		fread(&rootdir_block, sizeof rootdir_block, 1, cv);

//	FIND THE REQUIRED DIRECTORY BLOCK TO BE REMOVED
		bool found1;
		SIFS_DIRBLOCK currentDirectory;
		for(int i = 0; i < ndirblocks; i++)
		{
			fseek(cv, sizeof volumeHeader + sizeof oldBitmap + (int)volumeHeader.blocksize * dirblockpos[i], SEEK_SET);
			fread(&currentDirectory, sizeof currentDirectory, 1, cv);
			for(int  j = 0; j < rootdir_block.nentries; j++)
			{
				if(strcmp(currentDirectory.name, path) == 0 && rootdir_block.entries[j].blockID == dirblockpos[i])
				{
					if(currentDirectory.nentries > 0)
					{
						SIFS_errno = SIFS_ENOTEMPTY;
						return 1;
					}
					else
					{

//	OVERWRITE THE CURRENT DIRECTORY TO AN EMPTY BLOCK
						char oneblock[(int)volumeHeader.blocksize];
						memset(oneblock, 0, sizeof oneblock);
						fseek(cv, sizeof volumeHeader + sizeof oldBitmap + (int)volumeHeader.blocksize * dirblockpos[i], SEEK_SET);
						fwrite(oneblock, sizeof oneblock, 1, cv);

//	REMOVE THE DIRECTORY BLOCK FROM THE BITMAP
						oldBitmap[dirblockpos[i]] = SIFS_UNUSED;
						fseek(cv, sizeof volumeHeader, SEEK_SET);
						fwrite(oldBitmap, sizeof oldBitmap, 1, cv);

//	CHANGE THE ATTRIBUTES IN THE ROOT DIRECTORY
						rootdir_block.entries[j].blockID = rootdir_block.entries[j+1].blockID;
	   					rootdir_block.entries[j].fileindex = SIFS_UNUSED;
						rootdir_block.modtime = time(NULL);
						rootdir_block.nentries--;
						fseek(cv, sizeof volumeHeader + sizeof oldBitmap, SEEK_SET);
						fwrite(&rootdir_block, sizeof rootdir_block, 1, cv);
						found1 = !found1;
						break;
					}
				}
			}
		}
		if(found1 == true)
		{
			SIFS_errno = SIFS_ENOTDIR;
			return 1;
		}
	}

	else
	{
//	GET THE NAMES OF THE PATH BEFORE LAST AND THE DIRECTORY NAME TO BE REMOVED
		char delimiter2[2] = "/";
		char pathLast[SIFS_MAX_NAME_LENGTH];
		char pathB4Last[SIFS_MAX_NAME_LENGTH];
		char *token;
		int b = 0;
		token  = strtok(name, delimiter2);
		while(b < (npaths - 1))
		{
			strcpy(pathB4Last, token);
			token = strtok(NULL, delimiter2);
			b++;
		}
		
		token = strtok(token, delimiter2);
		strcpy(pathLast, token);

//	COUNT THE NUMBER OF DIRECTORY BLOCKS IN THE BITMAP EXCLUDING ROOTDIR
		int ndirblock = 0;
		for(int i = 1; i < (int)volumeHeader.nblocks; i++)
		{
			if(oldBitmap[i] == SIFS_DIR)
			{
				ndirblock++;
			}
		}

//	FIND THE POSITION OF ALL THE DIRECTORY BLOCKS EXCLUDING ROOTDIR
		int dirblockpos[ndirblock];
		int j = 0;
		for(int i = 1; i < (int)volumeHeader.nblocks; i++)
		{
			if(oldBitmap[i] == SIFS_DIR)
			{
				dirblockpos[j] = i;
				j++;
			}
		}

//	READ THE ROOT DIRECTORY BLOCK
		SIFS_DIRBLOCK rootdir_block;
		fread(&rootdir_block, sizeof rootdir_block, 1, cv);

//	FIND THE DIRECTORY BLOCK TO BE REMOVED
		SIFS_DIRBLOCK currentDirectory;
		SIFS_DIRBLOCK tobeDeleted;
		bool found2;

		for(int i = 0; i < ndirblock; i++)
		{
			fseek(cv, sizeof volumeHeader + sizeof oldBitmap + (int)volumeHeader.blocksize * dirblockpos[i], SEEK_SET);
			fread(&currentDirectory, sizeof currentDirectory, 1, cv);
			if(strcmp(currentDirectory.name, pathB4Last) == 0)
			{
				for(int j = 0; j < ndirblock; j++)
				{
					fseek(cv, sizeof volumeHeader + sizeof oldBitmap + (int)volumeHeader.blocksize * dirblockpos[j], SEEK_SET);
					fread(&tobeDeleted, sizeof tobeDeleted, 1, cv);
					for(int k = 0; k < currentDirectory.nentries; k++)
					{
						if(strcmp(tobeDeleted.name, pathLast) == 0 && currentDirectory.entries[k].blockID == dirblockpos[j])
						{
							if(tobeDeleted.nentries > 0)
							{
								SIFS_errno = SIFS_ENOTEMPTY;
								return 1;
							}
							else
							{
//	OVERWRITE THE TO BE DELETED DIRECTORY WITH AN EMPTY BLOCK
								char oneblock[(int)volumeHeader.blocksize];
								memset(oneblock, 0, sizeof oneblock);
								fseek(cv, sizeof volumeHeader + sizeof oldBitmap + (int)volumeHeader.blocksize * dirblockpos[j], SEEK_SET);
								fwrite(oneblock, sizeof oneblock, 1, cv);

//	REMOVE THE DIRECTORY FROM THE BITMAP
								oldBitmap[dirblockpos[j]] = SIFS_UNUSED;
								fseek(cv, sizeof volumeHeader, SEEK_SET);
								fwrite(oldBitmap, sizeof oldBitmap, 1, cv);

//	CHANGE THE ATTRIBUTES IN THE CURRENT DIRECTORY
								currentDirectory.modtime = time(NULL);
								currentDirectory.entries[k].blockID = currentDirectory.entries[k+1].blockID;
								currentDirectory.entries[k].blockID = currentDirectory.entries[k+1].blockID;
								currentDirectory.entries[k+1].blockID = currentDirectory.entries[k+2].blockID;
								currentDirectory.entries[k+1].fileindex = currentDirectory.entries[k+2].fileindex;
								currentDirectory.nentries--;
								fseek(cv, sizeof volumeHeader + sizeof oldBitmap + (int)volumeHeader.blocksize * dirblockpos[i], SEEK_SET);
								fwrite(&currentDirectory, sizeof currentDirectory, 1, cv);
								found2 = !found2;
								break;
							}
						}
					}
				}
			}
		}
		
		if(found2 == true)
		{
			SIFS_errno = SIFS_ENOTDIR;
			return 1;
		}
	}

	fclose(cv);
	return 0;
}
