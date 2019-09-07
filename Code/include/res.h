#ifndef __RES_H__
#define __RES_H__

#include "list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sqlite3.h>
#include <sys/types.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

//#include "json/json.h"
//#include </usr/local/include/json/json.h>
#include</usr/include/json-c/json.h>
#define NAME_BUFSIZE 32



typedef int bool;

#define false 0
#define true 1


typedef enum enum_user_type{
	 INVAILD_TYPE,
	 CLIENT_USER,
	 SERVER_USER
}enum_user_type;


typedef enum ctrl_object_type{
	 USER_MANAGER=0x51,
	 DOOR,
	 COMPUTE,
	 LIGHT,
	 WATCH_TV
}ctrl_object_type;

typedef enum led_statues_type{
	 LED_OFF=0xD0,
	 LED_ON,
	 INVAILED
}led_statues_type;



typedef struct led_info{
	int led_status;
	int location;
}*pled_info;

typedef struct water_heart{
	int action_status;
}*pwater_heart;

typedef struct tv_info{
	int control_action;
	int key_val;
}*ptv_info;

typedef struct user_info{
	//bool ctrl_cmd_sending;
	struct led_info *p_led_info;
	struct water_heart  *water_heart;
	struct list_head  list;
	struct tv_info *tv_info_option;
	int fd;
	int user_type;//client or server
	struct user_password_data *user;
	int ctrl_obj_type;
	int ctrl_cmd_sending_flag;
}*p_user_info;

typedef struct user_password_data{
	struct list_head  list;
	char mac_data[NAME_BUFSIZE];
	char user_name[16];
	bool isroot;
}*pwd_data;




#define SERV_PORT  6735
//#define SERV_IP_ADDR "192.168.7.139"
#define QUIT_STR "quit"

#define PASSWORD "linux"
#define TV_STRING "watch_tv"

#define PASSWORD_KEY "MacAdress"


#define CON_RETURN_PIN "allow_ack"  // succes return string
#define CON_RETURN_PIN_ERROR "no_allow_ack"

#define USER_IDENTIFY_CLIENT_STR "identity_client_id"
#define USER_IDENTIFY_SERVER_STR "identity_server_id"


#define  BUFSIZE 4096
#define ON  0x11
#define OFF  0x22
#define  NORMAL 0xBB
#define  FAULT    0xAA

#define  READCMD 0x40
#define  CTRLCMD 0x41
#define  RETURNSTATUS 0x42
#define  CONNECT 0x43
#define USER_CLIENT_TYPE 0xA1
#define USER_SERVER_TYPE 0xA2

#if 1
#define dprintf(fmt,args...) printf("%s:"fmt,__func__,##args)
#else
#define dprintf(fmt,args...)
#endif


#endif

