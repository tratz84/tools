/*
 * main.c
 *
 *  Created on: 16 aug. 2012
 *      Author: Tratz
 */


#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>

#include <libssh/libssh.h>
#include <libssh/sftp.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iniparser.h>
#include "dirsync.h"


int main(int argc, char *argv[]) {
	ssh_session my_session;
	sftp_session my_sftp;

	struct configdata *c;

	if (argc != 2) {
		fprintf(stderr, "Error: Usage: %s <ini-config>\n", argv[0]);
		exit(-1);
	}


	c = load_ini_config(argv[1]);

	// create ssh session
	my_session = create_ssh_session(c->host, c->username, c->password);

	// create sftp
	my_sftp = create_sftp_session(my_session);


	// sync files
	sync_my_files(my_session, my_sftp, c);

	printf("Finished\n");


	sftp_free(my_sftp);

	ssh_disconnect(my_session);
	ssh_free(my_session);


	return 0;
}

sftp_session create_sftp_session(ssh_session my_session)
{
	sftp_session sftp;
	int rc;

	sftp = sftp_new(my_session);
	if (sftp == NULL)
	{
		fprintf(stderr, "Error allocating SFTP session: %s",
							ssh_get_error(my_session));
		exit(-2);
	}

	rc = sftp_init(sftp);
	if (rc != SSH_OK)
	{
		fprintf(stderr, "Error initializing SFTP session: %s",
							(char*)sftp_get_error(sftp));
		sftp_free(sftp);
		exit(-2);
	}

	return sftp;
}



ssh_session create_ssh_session(char *host, char *username, char *password)
{
	ssh_session my_session;
	int rc;

	my_session = ssh_new();

	if (my_session == NULL) {
		printf("Failed to create ssh-object\n");
		exit(-1);
	}

	ssh_options_set(my_session, SSH_OPTIONS_HOST, host);
	ssh_options_set(my_session, SSH_OPTIONS_USER, username);

	rc = ssh_connect(my_session);
	if (rc != SSH_OK)
	{
		printf("Error to connect to \"%s\": %s", host, ssh_get_error(my_session));
		ssh_free(my_session);
		exit(-1);
	}


	rc = ssh_userauth_password(my_session, NULL, password);

	if (rc != SSH_AUTH_SUCCESS)
	{
		printf("Authentication failed, invalid username/password");
		ssh_disconnect(my_session);
		ssh_free(my_session);
		exit(-1);
	}

	return my_session;
}


int list_dir(ssh_session my_ssh_session, sftp_session my_sftp_session, char *path)
{
	sftp_dir dir;
	sftp_attributes a;
	int rc;

	dir = sftp_opendir(my_sftp_session, path);
	if (!dir)
	{
		fprintf(stderr, "Error opening directory: %s\n", (char*)ssh_get_error(my_ssh_session));
		return SSH_ERROR;
	}

	while ((a = sftp_readdir(my_sftp_session, dir)) != NULL)
	{
		printf("file: %s\n", a->name);
	}

	if (!sftp_dir_eof(dir))
	{
		fprintf(stderr, "Can't list directory: %s\n",
							ssh_get_error(my_ssh_session));
		sftp_closedir(dir);
		return SSH_ERROR;
	}


	rc = sftp_closedir(dir);
	if (rc != SSH_OK) {
		fprintf(stderr, "Can't close directory: %s\n", ssh_get_error(my_ssh_session));
		return rc;
	}

	return 1;
}



struct configdata* load_ini_config(char *path)
{
	dictionary *ini;

	ini = iniparser_load(path);
	if (ini == NULL) {
		fprintf(stderr, "Can't parse file: %s\n", path);
		exit(-1);
	}

	struct configdata *c = malloc(sizeof(struct configdata));

	// host
	if (iniparser_find_entry(ini, ":host")) {
		c->host = iniparser_getstring(ini, ":host", "");
	} else {
		fprintf(stderr, "Error: host not found in ini-file\n");
		exit(-1);
	}

	// port
	if (iniparser_find_entry(ini, ":port")) {
		c->port = iniparser_getint(ini, ":port", 22);
	} else {
		c->port = 22;
	}
	if (c->port <= 0)
		c->port = 22;


	// username
	if (iniparser_find_entry(ini, ":username")) {
		c->username = iniparser_getstring(ini, ":username", "");
	} else {
		fprintf(stderr, "Error: username not found in ini-file\n");
		exit(-1);
	}

	// password
	if (iniparser_find_entry(ini, ":password")) {
		c->password = iniparser_getstring(ini, ":password", "");
	} else {
		fprintf(stderr, "Error: password not found in ini-file\n");
		exit(-1);
	}


	// override files ?
	if (iniparser_find_entry(ini, ":override")) {
		c->override = iniparser_getboolean(ini, ":override", 0);
	} else {
		c->override = 0;
	}

	// remote dir
	if (iniparser_find_entry(ini, ":remote_directory")) {
		c->remote_dir = iniparser_getstring(ini, ":remote_directory", "");
	} else {
		fprintf(stderr, "Error: remote_directory not found in ini-file\n");
		exit(-1);
	}

	// local dir
	if (iniparser_find_entry(ini, ":local_directory")) {
		c->local_dir = iniparser_getstring(ini, ":local_directory", "");
	} else {
		fprintf(stderr, "Error: local_directory not in ini-file\n");
		exit(-1);
	}

	return c;
}



int sync_my_files(ssh_session my_ssh_session, sftp_session s, struct configdata *d)
{
    WIN32_FIND_DATA fdFile;
    HANDLE hFind = NULL;

    char sPath[2048];

    sftp_attributes remote_attributes;

    remote_attributes = sftp_stat(s, d->remote_dir);
    if (remote_attributes == NULL || remote_attributes->type != SSH_FILEXFER_TYPE_DIRECTORY)
    {
    	if (remote_attributes == NULL)
    		printf("Remote directory does not exists: %s\n", d->remote_dir);
    	else
    		printf("Remote path is not a directory: %s\n", d->remote_dir);
    	sftp_attributes_free(remote_attributes);
    	return -1;
    }
    sftp_attributes_free(remote_attributes);




    //Specify a file mask. *.* = We want everything!
    sprintf(sPath, "%s\\*.*", d->local_dir);


	// list local files
    if((hFind = FindFirstFile(sPath, &fdFile)) == INVALID_HANDLE_VALUE)
    {
        printf("Local path not found: %s\n", d->local_dir);
        return -1;
    }

    do
    {
    	// skip directories
    	if(fdFile.dwFileAttributes &FILE_ATTRIBUTE_DIRECTORY)
    		continue;

		// copy 2 remote
    	sync_file(my_ssh_session, s, d, fdFile.cFileName);

    }
    while(FindNextFile(hFind, &fdFile)); //Find the next file.

    FindClose(hFind); //Always, Always, clean things up!

    return 1;
}


int sync_file(ssh_session my_ssh_session, sftp_session s, struct configdata *d, char *file)
{
	char remote_path[2048];											// path to remote file
	char local_path[2048];											// path to local file
	char buf[DIRSYNC_MAX_UPLOAD_BUF];								// buf

	int accesstype = O_WRONLY | O_CREAT | O_TRUNC;					// access type for REMOTE file

	sftp_attributes file_attributes;								// attributes to check if file exists (in case of override=0)

	FILE *fp_local;
	size_t read_count;

	sftp_file fp_remote;

	sprintf(remote_path, "%s/%s", d->remote_dir, file);
	sprintf(local_path, "%s\\%s", d->local_dir, file);

	// no override? => check if file exists
	if (d->override == FALSE) {
		file_attributes = sftp_stat(s, remote_path);
		if (file_attributes != NULL) {								// exists? skip!
			sftp_attributes_free(file_attributes);
			printf("Skipping: %s (already on server, override=0)\n", file);
			return -1;
		}
		sftp_attributes_free(file_attributes);
	}

	// open remote file
	fp_remote = sftp_open(s, remote_path, accesstype, 0755);
	if (fp_remote == NULL)
	{
		printf("Failed: %s (creating remote file failed)\n", file);
		return -1;
	}

	// open local file
	fp_local = fopen(local_path, "rb");
	if (fp_local == NULL)
	{
		printf("Failed: %s (opening local file failed)\n", file);
		sftp_close(fp_remote);
		return -1;
	}

	printf("Copying file: %s\n", file);

	while(feof(fp_local) == 0) {
		read_count = fread(buf, 1, DIRSYNC_MAX_UPLOAD_BUF, fp_local);

		if (read_count > 0) {
			if (sftp_write(fp_remote, buf, read_count) < 0)
			{
				printf("Failed: %s (writing error, connection error?)\n", file);
				break;
			}
		} else {
			printf("Failed: %s (read error, ? ?)\n", file);
			break;
		}
	}

	sftp_close(fp_remote);
	fclose(fp_local);

	return 1;
}






