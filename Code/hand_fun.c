
#include "./include/res.h"


void socket_init(struct sockaddr_in * pcin )
{
	pcin->sin_family = AF_INET;
	pcin->sin_port =htons(SERV_PORT);
	pcin->sin_addr.s_addr= htonl(INADDR_ANY);
	bzero(pcin->sin_zero,8);
}




int  read_handle(struct board_info *temp,char *string)
{	
	char buf[BUFSIZE];
	int ret = -1;
	if(NULL==temp){
		return  -1;
	}
	if((ret = read(temp->fd,buf,BUFSIZE))< 0){
		perror("read board msg error");
		close(temp->fd);
		list_del(&temp->list);
		free(temp);
		return  -1;
	}else if( ret == 0){
		printf("XXXX  quit....\n");		
		close(temp->fd);
		list_del(&temp->list);
		free(temp);
		return  -1;
	}
	printf("read str is : %s\n",buf);
	strcpy(string,buf);
	//json_parse(buf,temp);
	return 0;

}



void json_parse(char *buf,struct board_info *temp)
{
	struct json_object *object,*board_id,*led_status,*work_status,*user_id,*cmd_type;;

	object = json_tokener_parse(buf);
#if  1
	const char *jsonstr= json_object_to_json_string(object);	
	printf("%s	====%s\n",__func__,jsonstr);
#endif 
	user_id = json_object_object_get(object,"user_id");
	board_id = json_object_object_get(object,"board_id");
	led_status = json_object_object_get(object,"led_status");
	work_status = json_object_object_get(object,"work_status");
	cmd_type = json_object_object_get(object,"cmd_type");
	
	
	temp->led_status = json_object_get_int(led_status);
	temp->work_status = json_object_get_int(work_status);
	temp->cmd_type =json_object_get_int(cmd_type);
	if(temp->cmd_type  == CONNECT)
		temp->user_id = json_object_get_int(user_id);
	temp->board_id = json_object_get_int(board_id);

	json_object_put(object);
}


void  json_array_packge(int fd)
{	
	struct board_info *temp;
	struct list_head *pos = NULL,*q = NULL;
	struct json_object *jsonarry;
	struct json_object *key;
	
	struct json_object *object,*board_id,*led_status,*work_status,*user_id,*cmd_type;
	jsonarry=json_object_new_array();

	list_for_each_safe(pos,q,&json_head.list){
		temp = list_entry(pos,struct board_info,list);
		
		object = json_object_new_object();
		board_id=json_object_new_int(temp->board_id );
		led_status=json_object_new_int(temp->led_status );
		work_status=json_object_new_int(temp->work_status );
		user_id=json_object_new_int(temp->user_id);
		cmd_type= json_object_new_int(temp->cmd_type);

		json_object_object_add(object,"board_id",board_id);
		json_object_object_add(object,"led_status",led_status);
		json_object_object_add(object,"work_status",work_status);
		json_object_object_add(object,"user_id",user_id);
		json_object_object_add(object,"cmd_type",cmd_type);
		
		json_object_array_add(jsonarry,object); 

		list_del(&temp->list);
		free(temp);	
	}
	const char* pstr= json_object_to_json_string(jsonarry);
	object = json_object_new_object();
	key = json_object_new_string(pstr);

	json_object_object_add(object,"menu",key);
	
	const char* tagetstr=json_object_to_json_string(object);
	printf("%s\n",tagetstr);
	send(fd,tagetstr,strlen(tagetstr),0);
	
}


void  send_json_packge(struct board_info *temp,int fd)
{
	struct json_object *object,*board_id,*led_status,*work_status,*user_id,*cmd_type;

	object = json_object_new_object();
	board_id=json_object_new_int(temp->board_id );
	led_status=json_object_new_int(temp->led_status );
	work_status=json_object_new_int(temp->work_status );
	user_id=json_object_new_int(temp->user_id);
	cmd_type= json_object_new_int(temp->cmd_type);

	json_object_object_add(object,"board_id",board_id);
	json_object_object_add(object,"led_status",led_status);
	json_object_object_add(object,"work_status",work_status);
	json_object_object_add(object,"user_id",user_id);
	json_object_object_add(object,"cmd_type",cmd_type);
	
	const char *jsonstr= json_object_to_json_string(object);
	
	send(fd,jsonstr,strlen(jsonstr),0);
}


void copy_to_list(struct board_info* temp)
{
	struct board_info* objtemp;
	objtemp = (struct board_info*)malloc(sizeof(struct board_info));
	objtemp->board_id = temp->board_id;
	objtemp->work_status= temp->work_status;
	objtemp->led_status= temp->led_status;
	list_add(&objtemp->list, &json_head.list);
}
void send_to_user_board_info(int board_id)
{	
	struct board_info *temp;
	struct list_head *pos = NULL,*q = NULL;
	struct board_info* objtemp;

	list_for_each_safe(pos,q,&board_head.list){
		temp = list_entry(pos,struct board_info,list);
		if(temp->board_id == board_id){
				temp->work_status=NORMAL;
				temp->led_status=OFF;
				//send_json_packge(temp,fd);
				copy_to_list(temp);
				printf("%s  %d\n",__func__,temp->board_id);
				return;
		}
	}
	
#if 1	
		objtemp = (struct board_info*)malloc(sizeof(struct board_info));
		objtemp->board_id=board_id;
		objtemp->work_status=FAULT;
		objtemp->cmd_type=READCMD;
		objtemp->led_status=OFF;
		list_add(&objtemp->list, &json_head.list);
#else
		
		send_json_packge(&objtemp,fd);
		printf("%s  %d\n",__func__,__LINE__);
#endif
}

void send_info_to_board(struct board_info *packge)
{
	struct board_info *temp;
	struct list_head *pos = NULL,*q = NULL;


	list_for_each_safe(pos,q,&board_head.list){
		temp = list_entry(pos,struct board_info,list);
		if(temp->board_id== packge->board_id  &&  temp->user_id ==0){
				printf("%d \n",temp->board_id);
				printf("%d \n",temp->user_id);
				send_json_packge(packge,temp->fd);
				printf("%s  %d \n",__func__,__LINE__);
		}
	}

}

void add_info_to_board(struct board_info *temp)
{	
	//printf("%s  %d   bid = %d  uid = %d\n",__func__,__LINE__,temp->board_id,temp->user_id);
	
	if(temp->board_id  > 0){
		temp->user_id=0;
	}else if (temp->user_id >0){
		temp->board_id=0;
	}
}

void find_fd_and_send(int target_id,struct board_info *packge)
{
	struct board_info *temp;
	struct list_head *pos = NULL,*q = NULL;


	list_for_each_safe(pos,q,&board_head.list){
		temp = list_entry(pos,struct board_info,list);
		if(temp->user_id== target_id){
				send_json_packge(packge,temp->fd);
				printf("%s  %d\n",__func__,__LINE__);
		}
	}	
}
void send_returnstatus_to_user(struct board_info *temp)
{
	struct mytable *local_temp;
	struct list_head *pos = NULL,*q = NULL;
	list_for_each_safe(pos,q,&table_head.list){
			local_temp = list_entry(pos,struct mytable,list);
			if(local_temp->board_id  == temp->board_id){
				find_fd_and_send(local_temp->user_id,temp);
			}
	}
}

void send_board_info(struct board_info *temp)
{	
	struct mytable *local_temp;
	struct list_head *pos = NULL,*q = NULL;
	list_for_each_safe(pos,q,&table_head.list){
			local_temp = list_entry(pos,struct mytable,list);
#if 0
			if(local_temp->user_id ==  temp->user_id){
					const char* pstr = json_packge(local_temp);
					//printf("%s\n",pstr);
					send(temp->fd,pstr,strlen(pstr),0);
			}

#if 1	
			printf("%s  ===== %d\n",__func__,local_temp->board_id);
			printf("%s  ===== %d\n",__func__,local_temp->user_id);
#endif	
			if(local_temp->user_id ==  temp->user_id){
					send_to_user_board_info(local_temp->board_id);
					printf("%s  %d\n",__func__,__LINE__);
			}
#endif			
	}

	//json_array_packge(temp->fd);
}




sqlite3* create_sqlite(void)
{
	struct sqlite3 *mysql;

	sqlite3_open("./mytest.db",&mysql);
	sql_no_back(mysql,"Create table menu(user_id integer,board_id integer);");

	for_each_board_id(mysql);
	if(is_table_list(80001)  || is_table_list(80002)){			
			return  mysql;
	}
		
	sql_no_back(mysql,"insert into menu(user_id,board_id) values(10001,80001);");
	sql_no_back(mysql,"insert into menu(user_id,board_id) values(10001,80002);");
	for_each_board_id(mysql);
	printf("data base create sucess!\n");

	return  mysql;
	
}

int  is_table_list(int id)
{	
	struct mytable* temp;
	struct list_head *pos = NULL,*q = NULL;
	int tmp =0;
	list_for_each_safe(pos,q,&table_head.list){
			temp = list_entry(pos,struct mytable,list);
			if(temp->board_id  == id){
					tmp = 1;
		}
	}
	return tmp;		
}

void  for_each_board_id(struct sqlite3 *mysql)
{	
	int nrow,ncolum,i;
	char **result;
	char* errmsg;
	struct mytable* temp;
	int res1,res2;
	sqlite3_get_table(mysql,"select  * from menu ",&result,&nrow,&ncolum,&errmsg);
			
	//index = 0;		

	for(i = 1;i <= nrow ;i++){
			printf("%s:%s\n",result[i*2],result[i*2+1]);
			res1 = atoi(result[i*2]);
			res2=atoi(result[i*2+1]);
			if(!is_table_list(res2)){
				temp =(struct mytable*)malloc(sizeof(struct mytable));
				if(NULL == temp){
					perror("malloc for table failed!");
					return;
				}
				temp->board_id=  res2;
				temp->user_id=res1;
				list_add(&temp->list, &table_head.list);
			}
	}
}

