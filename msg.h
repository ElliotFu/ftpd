/**
 * @file msg.h
 * @brief FTP code message
 * 
 */
#ifndef MSG_H
#define MSG_H

#define MSG_CONNECTION_READY "150 Data connection already open; transfer starting\r\n"

#define MSG_COMMON_SUCCESS "200 Command okay\r\n"
#define MSG_NEW_USER "220 Service ready for new user\r\n"
#define MSG_PASV_SUCCESS "227 Entering Passive Mode. (%s)\r\n"
#define MSG_LOGIN_SUCCESS "230 User logged in, proceed\r\n"
#define MSG_FILE_SUCCESS "250 Requested file action okay, completed\r\n"
#define MSG_CUR_PATH "257 \"%s\" is your working directory\r\n"
#define MSG_MKD_SUCCESS "257 making directory OK\r\n"

#define MSG_REQUIRE_PASS "331 User name okay, need password\r\n"
#define MSG_REQUIRE_USER "332 Need account for login\r\n"

#define MSG_CLOSE "421 Service not available, closing control connection\r\n"
#define MSG_DATA_LINK_FAIL "425 Can't open data connection\r\n"
#define MSG_CONNECTION_CLOSED "426 Connection closed; transfer aborted\r\n"
#define MSG_LOGIN_FAIL "430 Invalid username or password\r\n"

#define MSG_INVALID_COMMAND "500 Syntax error, command unrecognized\r\n"
#define MSG_FAILED "500 Command failed\r\n"
#define MSG_INVALID_PARAM "501 Syntax error in parameters or argument\r\n"
#define MSG_BAD_SEQUENCE "503 Bad sequence of commands\r\n"
#define MSG_NOT_LOGIN "530 Not logged in\r\n"

#endif