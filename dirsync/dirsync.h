/*
 * dirsync.h
 *
 *  Created on: 16 aug. 2012
 *      Author: Tratz
 */

#ifndef DIRSYNC_H_
#define DIRSYNC_H_

#define DIRSYNC_MAX_UPLOAD_BUF 4096

struct configdata {
	char *username;
	char *password;
	int port;
	char *host;
	char *remote_dir;
	char *local_dir;
	int override;
};




ssh_session create_ssh_session(char *host, char *username, char *password);
sftp_session create_sftp_session(ssh_session my_session);
int list_dir(ssh_session my_ssh_session, sftp_session my_sftp_session, char *path);
struct configdata* load_ini_config(char *path);
int sync_my_files(ssh_session my_ssh_session, sftp_session s, struct configdata *d);
int sync_file(ssh_session my_ssh_session, sftp_session s, struct configdata *d, char *file);



#endif /* DIRSYNC_H_ */
