

int SplitString(char* str, char* dest[SPLIT_STRING_ARRAY_SIZE], char* seps)
{
	int index = 0;
	
	char *token = NULL;
	char *nextToken = NULL;
	
	token = strtok_s(str, seps, &nextToken);
	while( token != NULL )
	{
#if DEBUG				
		OutputDebugStringA(token);
		OutputDebugStringA("\n");
#endif		

		dest[index++] = token;
		
		token = strtok_s(NULL, seps, &nextToken);
	}
	return index;
}

void GenerateRandomString(char* destination, unsigned int length)
{
	char choices[] = { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z','a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z',0 };
	size_t choicesLength = strlen(choices);
	for ( unsigned int i = 0 ; i < length ; i++ )
	{
		*destination++ = choices[rand() % choicesLength];
	}
}
