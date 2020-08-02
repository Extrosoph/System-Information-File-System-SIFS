#include "sifs-internal.h"




// make a new directory within an existing volume
int SIFS_mkdir(const char *volumename, const char *dirname)
{
//	ENSURE THAT RECEIVED PARAMETER ARE VALID
	if(volumename == NULL || dirname == NULL)
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
	
//	CHECK IF THE NAME IS EMPTY
	if(strcmp(name,"/") == 0)
	{
		SIFS_errno = SIFS_EINVAL;
		return 1;
	}

//	OPEN THE EXISTED VOLUME FOR READ AND WRITE
	FILE *cv = fopen(volumename, "r+");

//	READING THE VOLUME HEADER AND THE BITMAP
	SIFS_VOLUME_HEADER volumeHeader;
	fread(&volumeHeader, sizeof volumeHeader,1 , cv);


	SIFS_BIT oldBitmap[(int)volumeHeader.nblocks];
	fread(&oldBitmap, (int)volumeHeader.nblocks, 1, cv);

//	CHECK IF VOLUME STILL HAVE ONE BLOCK SPACE LEFT
	int unused = 0;
	for(int i = 0; i < (int)volumeHeader.nblocks; i++)
	{
		if(oldBitmap[i] == SIFS_UNUSED)
		{
			unused++;
		}
	}
	if(unused == 0)
	{
		SIFS_errno = SIFS_ENOMEM;
		return 1;
	}

//	READ, EDIT AND WRITE THE NEW BITMAP INTO THE VOLUME
	if(npaths == 1 || npaths == 0)
	{
		char delimiter2[2] = "/";
		char *path;
		path = strtok(name, delimiter2);

		int block;
		for(block = 0; block < (int)volumeHeader.nblocks; block++)
		{
			if(oldBitmap[block] == SIFS_UNUSED)
			{
				oldBitmap[block] = SIFS_DIR;
				break;
			}
		}

		fseek(cv, sizeof volumeHeader, SEEK_SET);
		fwrite(&oldBitmap, sizeof oldBitmap, 1, cv);

//	READ, EDIT AND WRITE THE NEW ROOT DIRECOTRY INTO THE VOLUME
		SIFS_DIRBLOCK rootdir_block;
		fread(&rootdir_block, sizeof rootdir_block, 1, cv);
		char oneblock[(int)volumeHeader.blocksize];
		
		if(rootdir_block.nentries == 0)
		{
			rootdir_block.nentries = 1;
			rootdir_block.entries[0].blockID = block;
			rootdir_block.entries[0].fileindex = SIFS_UNUSED;
		}
		else
		{
			if(rootdir_block.nentries >= SIFS_MAX_ENTRIES)
			{
				SIFS_errno = SIFS_EMAXENTRY;
				return 1;
			}
			for(int k = 0; k < rootdir_block.nentries; k++)
			{
				SIFS_DIRBLOCK check;
				fseek(cv, sizeof volumeHeader + sizeof oldBitmap + (int)volumeHeader.blocksize * rootdir_block.entries[k].blockID, SEEK_SET);
				fread(&check, sizeof check, 1, cv);
				if(strcmp(check.name, path) == 0)
				{
					SIFS_errno = SIFS_EEXIST;
					return 1;
				}
			}
			rootdir_block.nentries++;
			rootdir_block.entries[rootdir_block.nentries-1].blockID = block;
			rootdir_block.entries[rootdir_block.nentries-1].fileindex = SIFS_DIR;
		}

		memcpy(oneblock, &rootdir_block, sizeof rootdir_block);
		fseek(cv, (sizeof volumeHeader + sizeof oldBitmap), SEEK_SET);
		fwrite(oneblock, sizeof oneblock, 1, cv);

//	READ ,EDIT AND WRITE THE NEW DIRECTROY INTO THE VOLUME
		SIFS_DIRBLOCK currentDirectory;
		fseek(cv, (sizeof volumeHeader + sizeof oldBitmap + (int)volumeHeader.blocksize * (block - 1)), SEEK_SET);
		fread(&oneblock, (int)volumeHeader.blocksize, 1, cv);

		strcpy(currentDirectory.name, path);
		currentDirectory.modtime = time(NULL);
		currentDirectory.nentries = 0;

		memcpy(oneblock, &currentDirectory, sizeof currentDirectory);

		fwrite(oneblock, sizeof oneblock, 1, cv);

	}
	else
	{
//READ THE DIRNAME AND FIGURE OUT THE PATH
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

//	COUNT THE NUMBER OF DIRECTORY BLOCKS IN THE BITMAP EXCLUDING ROOTDIR
		int ndirblocks = 0;
		for(int i = 0; i < (int)volumeHeader.nblocks; i++)
		{
			if(oldBitmap[i] == SIFS_DIR)
			{
				ndirblocks++;
			}
		}

//	FIGURE OUT ALL THE POSITION OF THE DIRECTORIES INCLUDING ROOT
		int dirblocks[ndirblocks];
		int j = 0;
		for(int i = 0; i < (int)volumeHeader.nblocks; i++)
		{
			if(oldBitmap[i] ==  SIFS_DIR)
			{
				dirblocks[j] = i;
				j++;
			}
		}

//	PLACE IN THE NEW DIRECTORY
		int block;
		for(block = 0; block < (int)volumeHeader.nblocks; block++)
		{
			if(oldBitmap[block] == SIFS_UNUSED)
			{
				oldBitmap[block] = SIFS_DIR;
				break;
			}
		}
	

		fseek(cv, sizeof volumeHeader, SEEK_SET);
		fwrite(&oldBitmap, sizeof oldBitmap, 1, cv);

//	READ THE DIRECtORY WITH THE ENTRYNAMES
		SIFS_DIRBLOCK rootdir_block;
		fread(&rootdir_block, sizeof rootdir_block, 1, cv);
		char currentBlock[(int)volumeHeader.blocksize];
		char newBlock[(int)volumeHeader.blocksize];
		SIFS_DIRBLOCK newDirectory;
		token = strtok(token, delimiter2);
		strcpy(pathLast, token);

		for(int i = 0; i < ndirblocks; i++)
		{
			SIFS_DIRBLOCK currentDirectory;
			fseek(cv, sizeof volumeHeader + sizeof oldBitmap + (int)volumeHeader.blocksize * dirblocks[i] , SEEK_SET);
			fread(&currentDirectory, sizeof currentDirectory, 1, cv);

			if(strcmp(currentDirectory.name, pathB4Last) == 0)
			{
				if(currentDirectory.nentries == 0)
				{
					currentDirectory.entries[currentDirectory.nentries].blockID = block;
					currentDirectory.entries[currentDirectory.nentries].fileindex = SIFS_UNUSED;
					currentDirectory.nentries++;
					fseek(cv, sizeof volumeHeader + sizeof oldBitmap + (int)volumeHeader.blocksize * dirblocks[i], SEEK_SET);
					memcpy(currentBlock, &currentDirectory, sizeof currentDirectory);
					fwrite(currentBlock, sizeof currentBlock, 1, cv);
					break;
				}
				else
				{
					if(currentDirectory.nentries >= SIFS_MAX_ENTRIES)
					{
						SIFS_errno = SIFS_EMAXENTRY;
						return 1;
					}
					for(int k = 0; k < currentDirectory.nentries; k++)
					{
						SIFS_DIRBLOCK check;
						fseek(cv, sizeof volumeHeader + sizeof oldBitmap + (int)volumeHeader.blocksize * currentDirectory.entries[k].blockID, SEEK_SET);
						fread(&check, sizeof check, 1, cv);
						if(strcmp(check.name, pathLast) == 0)
						{
							SIFS_errno = SIFS_EEXIST;
							return 1;
						}
					}
					currentDirectory.entries[currentDirectory.nentries].blockID = block;
					currentDirectory.entries[currentDirectory.nentries].fileindex = SIFS_UNUSED;
					currentDirectory.nentries++;
					fseek(cv, sizeof volumeHeader + sizeof oldBitmap + (int)volumeHeader.blocksize * dirblocks[i], SEEK_SET);
					memcpy(currentBlock, &currentDirectory, sizeof currentDirectory);
					fwrite(currentBlock, sizeof currentBlock, 1, cv);
					break;
				}
			}			
		}

		fseek(cv, sizeof volumeHeader + sizeof oldBitmap + (int)volumeHeader.blocksize * block, SEEK_SET);
		strcpy(newDirectory.name, pathLast);
		newDirectory.modtime = time(NULL);
		newDirectory.nentries = 0;

		memcpy(newBlock, &newDirectory, sizeof newDirectory);
		fwrite(newBlock, sizeof newBlock, 1, cv);

	}

	fclose(cv);
	return 0;
}

