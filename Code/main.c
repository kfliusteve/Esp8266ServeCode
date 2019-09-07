#include "./include/res.h"



#define  sql_no_back(mysql,str)  sqlite3_exec(mysql,str,NULL,NULL,NULL);

#define YES "yes"

#define NO "no"

struct user_info client_user_info_head;
struct user_info server_user_info_head;
struct user_password_data passwd_list_head;



int max_fd = -1;
fd_set event_set;	
char usernamebuff[NAME_BUFSIZE];
int device_id = -1;
void socket_init(struct sockaddr_in * pcin)
{
	pcin->sin_family = AF_INET;
	pcin->sin_port =htons(SERV_PORT);
	pcin->sin_addr.s_addr= htonl(INADDR_ANY);
	bzero(pcin->sin_zero,8);
}
 

int get_value_from_read_buf(char id[],char buf[])
{
	struct json_object *object,*identity;
	object = json_tokener_parse(buf);
	identity = json_object_object_get(object,id);	
	return json_object_get_int(identity);
}

int  get_user_type_buf(char buf[])
{	
	//dprintf("%s--:%s\n",__func__,buf);
	int ret = get_value_from_read_buf("user_type",buf);
	return ret;
}


int get_ctrl_obj_tpye(char *buf)
{	
	//dprintf("%s--:%s\n",__func__,buf);
	return get_value_from_read_buf("device_id",buf);
}
	
void get_str_from_buf(char id[],char buf[],char passwd_buf[])
{
	//dprintf("%s--:%s\n",__func__,buf);
	struct json_object *object,*password;
	object = json_tokener_parse(buf);
	password = json_object_object_get(object,id);	
	const char *j_buf=json_object_get_string(password);
	if(j_buf != NULL)
		strcpy(passwd_buf,j_buf);
} 


int info_is_in_vaild_list(char *info)
{

	struct list_head *pos = NULL,*q = NULL;
	struct user_password_data *temp = NULL;

	list_for_each_safe(pos,q,&passwd_list_head.list ){
	temp = list_entry(pos,struct user_password_data,list);
		if(!strcmp(info,temp->mac_data)){
			return 0;
		}
	}
	return -1;
}

enum_user_type  user_identify(int fd)
{
	char buf[BUFSIZE];
	char read_buf[BUFSIZE+7];
	char dst_buf[128];
	int ret_user = -1;
	
	memset(dst_buf,0,128);
	memset(buf,0,BUFSIZE);
	memset(read_buf,0,BUFSIZE+7);
	
	if(read(fd,read_buf,BUFSIZE) < 0){
		perror("%s---:read error\n");
		return INVAILD_TYPE;
	}

	dprintf("read buf is : %s\n",read_buf);
	
	if(strncmp(read_buf,"lkfjson",7)){
		perror("%s---:json str is error\n");
		return INVAILD_TYPE;
	}

	strncpy(buf,&read_buf[7],BUFSIZE);

	dprintf("read buf is 22222\n");
	ret_user = get_user_type_buf(buf);

	if(ret_user < 0){
		dprintf("no such id,get info error!\n");
		return INVAILD_TYPE;
	}
	
	if(ret_user == USER_SERVER_TYPE){
		device_id = get_ctrl_obj_tpye(buf);
		return SERVER_USER;
	}else if(ret_user == USER_CLIENT_TYPE){
		get_str_from_buf(PASSWORD_KEY,buf,dst_buf);		
		if(!info_is_in_vaild_list(dst_buf)){
			memset(usernamebuff,0,NAME_BUFSIZE);
			get_str_from_buf("user_name",buf,usernamebuff);
			return CLIENT_USER;
		}else{
			send(fd,CON_RETURN_PIN_ERROR,strlen(CON_RETURN_PIN_ERROR),0);
			dprintf("password is not correctly !\n");
			return INVAILD_TYPE; 
		}

	}else{
		return INVAILD_TYPE;
	}
}


void add_user_to_list(int fd)
{
	enum_user_type ret_type;
	struct user_info* new_user=NULL;

	dprintf("start....!\n");
	
	ret_type = user_identify(fd);
	if(ret_type != INVAILD_TYPE){
			/*---------------------------*/
			new_user= (struct user_info*)malloc(sizeof(struct user_info));
			if(NULL == new_user){
				perror("malloc error");
				exit(1);
			}	
			new_user->fd = fd;
			new_user->user_type = ret_type;
			if(ret_type == CLIENT_USER){
				new_user->ctrl_cmd_sending_flag = 0;
				send(fd,CON_RETURN_PIN,strlen(CON_RETURN_PIN),0);//password is valid add to list
				list_add_tail(&new_user->list,&client_user_info_head.list);
				strcpy(new_user->user->user_name,usernamebuff);
			}else if (ret_type == SERVER_USER){
				list_add_tail(&new_user->list,&server_user_info_head.list);
				new_user->ctrl_obj_type = device_id;
			}
			printf("new devices connect........!\n");
			/*---------------------------*/
		
	}else{
		close(fd);
	}

}


void led_status_control(char *buf)
{
	//fix led control logic
	led_statues_type led_status = get_value_from_read_buf("cmd_ctrl_led",buf);
	//led_locationt led_status = 
}

void  send_cmd_to_object(const char *buf,ctrl_object_type obj_type)
{
	//int ir_cmd = get_value_from_read_buf("tv_control_cmd",buf);
	//dprintf("watch_tv_cmd is :%d\n",ir_cmd);

	
	struct list_head *pos = NULL,*q = NULL;
	struct user_info *temp = NULL;
	list_for_each_safe(pos,q,&server_user_info_head.list){
		temp = list_entry(pos,struct user_info,list);
		if(temp->ctrl_obj_type == obj_type){
			dprintf("send power on  compute msg...\n");
			send(temp->fd,buf,strlen(buf),0);
		}
	}
}
void del_client_event(struct user_info *user_info,char r_buf[])
{
	ctrl_object_type object_ret = get_value_from_read_buf("ctrl_object",r_buf);
	user_info->ctrl_cmd_sending_flag = 1;
	send_cmd_to_object(r_buf,object_ret);
	
}	

void send_ack_to_client(struct user_info *user_info)
{
	 dprintf("--->\n");
	struct list_head *pos = NULL,*q = NULL;
	struct user_info *temp = NULL;
	list_for_each_safe(pos,q,&client_user_info_head.list){
		temp = list_entry(pos,struct user_info,list);
		if(temp->ctrl_cmd_sending_flag){		
			send(temp->fd,"cmd_linux_ack",strlen("cmd_linux_ack"),0);
			dprintf("sucess!\n");
		}
	}
	
}

void handle_event(struct user_info *user_info)
{
	
	int ret_r=-1;
	char buf[BUFSIZE];
	char del_json_buf[BUFSIZE-7];
	
	memset(buf,0,BUFSIZE);
	memset(del_json_buf,0,BUFSIZE-7);
	

	ret_r = read(user_info->fd,buf,BUFSIZE);
	if( ret_r < 0){
		perror("read error!");
		return;
	}else if(ret_r == 0){
		dprintf("%s:  quit....\n",user_info->user->user_name);		
		close(user_info->fd);
		list_del(&user_info->list);
		free(user_info);
		return;
	}
	
	dprintf(":---->%s\n",buf);
	dprintf("user_tpye = %d\n",user_info->user_type);
	
	if(user_info->user_type == CLIENT_USER){
		dprintf("--%s req\n",user_info->user->user_name);
		if(strncmp(buf,"lkfjson",7)){
			dprintf("%s---: is not json str\n",__func__);
			return ;
		}
		
		strncpy(del_json_buf,&buf[7],BUFSIZE-7);
		dprintf("del json buf is %s:\n",del_json_buf);
		del_client_event(user_info,del_json_buf);
	}else if(user_info->user_type == SERVER_USER){
	    //fix server handler
	    if(strncmp(buf,"cmd_linux_ack",strlen("cmd_linux_ack")) == 0)
			send_ack_to_client(user_info);
	}
}

void add_fd_to_fdset()
{
	//struct user_info *new_user = NULL;
	struct list_head *pos = NULL,*q = NULL;
	struct user_info *temp = NULL;

	//add client to list
	list_for_each_safe(pos,q,&client_user_info_head.list){
		temp = list_entry(pos,struct user_info,list);
		FD_SET(temp->fd,&event_set);
		if(temp->fd > max_fd)
			max_fd = temp->fd;
	}

	//add sercver to list
	list_for_each_safe(pos,q,&server_user_info_head.list){
		temp = list_entry(pos,struct user_info,list);
		FD_SET(temp->fd,&event_set);
		if(temp->fd > max_fd)
			max_fd = temp->fd;
	}

}


void for_each_server_client_fd()
{	

	struct list_head *pos = NULL,*q = NULL;
	struct user_info *temp = NULL;
	
	list_for_each_safe(pos,q,&client_user_info_head.list ){
		temp = list_entry(pos,struct user_info,list);
		if(FD_ISSET(temp->fd,&event_set)){	
	   		handle_event(temp);				
		}
	}

	list_for_each_safe(pos,q,&server_user_info_head.list ){
		temp = list_entry(pos,struct user_info,list);
		if(FD_ISSET(temp->fd,&event_set)){
	   		handle_event(temp);	
		}
	}

}




static bool is_in_sqlite_db(struct sqlite3 *mysql,char *mac)
{
	int nrow,ncolum,i;
	char **result;
	char* errmsg;

	sqlite3_get_table(mysql,"select  * from menu ",&result,&nrow,&ncolum,&errmsg);

	for(i = 1;i <= nrow ;i++){
		dprintf("db %d data is %s  , %s , %s\n",i,result[i*3],result[i*3+1],result[i*3+2]);
		if(!strncmp(mac,result[i*3+1],strlen(mac))){
			dprintf("%s is already in data base!\n!",mac);
			return true;
		}
	}
	
	return  false;
}

static void  passwd_list_init(struct sqlite3 *mysql)
{	
	int nrow,ncolum,i;
	char **result;
	char* errmsg;
	pwd_data pdata= NULL;

	sqlite3_get_table(mysql,"select  * from menu ",&result,&nrow,&ncolum,&errmsg);

	pdata = (pwd_data)malloc(sizeof(struct user_password_data));
	if(pdata){
		for(i = 1;i <= nrow ;i++){
			dprintf("db %d data is %s  , %s , %s\n",i,result[i*3],result[i*3+1],result[i*3+2]);
			strcpy(pdata->mac_data,result[i*3+1]);
			strcpy(pdata->user_name,result[i*3]);
			list_add_tail(&pdata->list,&passwd_list_head.list);
		}
	}

}



void insert_user_info(char *name,char *mac,char *isroot,sqlite3 *mysql)
{
	char cmd_text[128] = {0};

	if(is_in_sqlite_db(mysql,mac)){
		return;
	}
	sprintf(cmd_text,"insert into menu(USER_NAME,BOARD_MAC,IS_ROOT) values('%s','%s','%s');",name,mac,isroot);
	if(mysql){
		sql_no_back(mysql,cmd_text);
		dprintf("sucess!!!!\n");
	}
}

sqlite3* create_sqlite(void)
{
	struct sqlite3 *mysql;
	char MAC_STR[20]= "48:2c:a0:7f:df:3b";//my mi 8 wifi mac addres

	sqlite3_open("./mytest.db",&mysql);
	sql_no_back(mysql,"Create table menu(USER_NAME TEXT,BOARD_MAC TEXT,IS_ROOT TEXT);");

	insert_user_info("kfliu",MAC_STR,YES,mysql);

	return  mysql;
	
}


int main(void)
{
	int listen_fd = -1, new_fd = -1;
	bool flag = false;

	struct sockaddr_in sin,cin;
	socklen_t slen = sizeof(cin);
	char MAC_STR[20]= "48:2c:a0:7f:df:4d";//my mi 8 wifi mac addres
		

	INIT_LIST_HEAD(&client_user_info_head.list);
	INIT_LIST_HEAD(&server_user_info_head.list);
	INIT_LIST_HEAD(&passwd_list_head.list);
	
	
	//INIT_LIST_HEAD(&json_head.list);
	
	if( (listen_fd = socket(AF_INET, SOCK_STREAM, 0))< 0) {
		perror("server socket error");
		exit(1);
	}



	int b_reuse = 1; 
	setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &b_reuse, sizeof(int));

	socket_init(&sin);

	struct sqlite3 *mysql = create_sqlite();

	if(mysql){
		insert_user_info("Steve",MAC_STR,"yes",mysql);
		passwd_list_init(mysql);
	}


	if(bind(listen_fd,(struct sockaddr *)&sin,sizeof(sin))  != 0){
		perror("bind failed");
		exit(1);
	}
	
	listen(listen_fd,5);

	dprintf("server start......!\n");

	while(1){
		FD_ZERO(&event_set);
		FD_SET(listen_fd,&event_set);
		max_fd = listen_fd;
		/*--------------------------------*/
		add_fd_to_fdset();

		printf("maxfd=%d\n",max_fd);
		select(max_fd+1,&event_set,NULL,NULL,NULL);
		
		if(FD_ISSET(listen_fd,&event_set)){
			if((new_fd = accept(listen_fd,(struct sockaddr *)&cin,&slen)) < 0 ){
				perror("accept error");
				continue;
			}
			add_user_to_list(new_fd);
		}

		for_each_server_client_fd();
		//sleep(2);
		
	}
	
	close(listen_fd);
	//sqlite3_close(mysql);
	return 0;
}



















