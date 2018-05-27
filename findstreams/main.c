/*
 * main.c
 *
 *  Created on: 26 mei 2018
 *      Author: tvanw
 */

#include <windows.h>
#include <stdio.h>


int ADSCount(char *path, int printFiles) {
	void *buffer;
	int err;
	int cnt=0;

	wchar_t wpath[512];

	MultiByteToWideChar( CP_ACP, 0, path, -1, wpath, 512 );

	HANDLE file;
	WIN32_FIND_STREAM_DATA stream;
	file = FindFirstStreamW(wpath, FindStreamInfoStandard, &stream, 0);
	if (file == INVALID_HANDLE_VALUE) {
		return cnt;
	}

	// first stream found :)
	cnt++;

	if (printFiles) {
		wprintf(L"\t%-50s\t%d\n",  stream.cStreamName, stream.StreamSize.QuadPart);
	}

	while(1) {
		err = FindNextStreamW(file, &stream);
		if (err == 0) {
//			if (GetLastError() == ERROR_HANDLE_EOF)
//				printf("\nNo more streams available\n");
//			else
//				printf("error = 0x%x\n", GetLastError());

			break;
		} else {
			if (printFiles) {
				// printf("StreamSize   = 0x%x\n", stream.StreamSize.QuadPart);
				wprintf(L"\t%-50s\t%d\n",   stream.cStreamName, stream.StreamSize.QuadPart);
			}

			cnt++;
		}
		break;

	}

	FindClose(file);

	return cnt;
}

void findAlternateDatastreams(char *path) {

	WIN32_FIND_DATA ffd;
	HANDLE hFind;

	int lenpath = strlen(path);

	char querypath[1024];
	strcpy(querypath, path);
	strcpy(querypath+strlen(querypath), "\\*.*");

	char fullpath[1024];

	int lastError;
    LPVOID errorMsg=0;
    char *p;
    int r;


	hFind = FindFirstFile(querypath, &ffd);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		lastError = GetLastError();

	    r = FormatMessage(
	        FORMAT_MESSAGE_ALLOCATE_BUFFER |
	        FORMAT_MESSAGE_FROM_SYSTEM |
	        FORMAT_MESSAGE_IGNORE_INSERTS,
	        NULL,
	        GetLastError(),
	        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
	        (LPTSTR) &errorMsg,
	        0, NULL );


	    if (r > 2) {
	    	p = errorMsg;
	    	p += strlen(errorMsg)-2;
	    	*p = '\0';

	    	printf ("Unable to find path, %s (%d, %s)\n", path, GetLastError(), errorMsg);
	    }

		LocalFree(errorMsg);

		return;
	}

	strcpy(fullpath, path);

	do {
		if (strcmp(ffd.cFileName, ".") == 0 || strcmp(ffd.cFileName, "..") == 0) continue;

		strcpy(fullpath+lenpath, "\\");
		strcpy(fullpath+lenpath+1, ffd.cFileName);

//		printf("Checking file: %s - %x\n", fullpath, ffd.dwFileAttributes);

		if (ADSCount( fullpath, 0 ) > 1) {
			printf("%s\n", fullpath);
			ADSCount( fullpath, 1 );
		}


		// no recursive for reparse points or symbolic links
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
			continue;

		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			// recursive..
			findAlternateDatastreams( fullpath );
		}
	}   while (FindNextFile(hFind, &ffd) != 0);


	FindClose( hFind );

}

int main(int argc, char *argv[]) {
	int count = 0;
	int pathLen;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s <path>\n", argv[0]);
		exit(0);
	}

	// remove ending slashes
	pathLen = strlen(argv[1]);
	while (*(argv[1] + (pathLen - 1)) == '/' || *(argv[1] + (pathLen - 1)) == '\\') {
		*(argv[1]+pathLen-1) = '\0';
		pathLen = strlen(argv[1]);
	}

	findAlternateDatastreams( argv[1] );

	return 0;
}




