#include<cstdio>
#include<iostream>
#include<string>
#include<mutex>
#include<jsoncpp/json/json.h>
#include<mysql/mysql.h>

#define MYSQL_HOST "127.0.0.1"
#define MYSQL_USER "root"
#define MYSQL_PASS ""
#define MYSQL_NAME "vod_system"

#define CHECK_RET(q)  if((q)==false) {return false;}

namespace vod_system
{
	static std::mutex g_mutex; //互斥锁，实例化对象
	//初始化句柄
	static MYSQL *MysqlInit()
	{
		MYSQL *mysql = mysql_init(NULL);
		if (mysql == NULL)
		{
			printf("mysql init failed!\n");
			return NULL;
		}
		//连接服务器
		if (mysql_real_connect(mysql, MYSQL_HOST, MYSQL_USER, MYSQL_PASS,
			MYSQL_NAME, 0, NULL, 0)==NULL)
		{
			printf("connect mysql server failed!\n");
			mysql_close(mysql);
			return NULL;
		}
		//设置字符编码
		if (mysql_set_character_set(mysql, "utf8") != 0)
		{
			printf("mysql set character failed!\n");
			mysql_close(mysql);
			return NULL;
		}
		return mysql;
	}
	static void MysqlRelease(MYSQL *mysql)
	{
		if (mysql)
		{
			mysql_close(mysql);
		}
		return;
	}
	static bool MysqlQuery(MYSQL *mysql, const std::string &sql)
	{
		int ret = mysql_query(mysql, sql.c_str());
		if (ret != 0)
		{
			printf("mysql query sql:[%s] failed: %s\n", sql.c_str(), mysql_error(mysql));
			return false;
		}
		return true;
	}
	class TableVod
	{
	public:
		TableVod(MYSQL *mysql) : _mysql(mysql){  }
		bool Insert(const Json::Value &video)
		{
			//id,name,vdesc,vsize,vurl,purl,ultime
#define INSERT_VOD "insert teble_vod values(null,'%s','%s','%ld','%s','%s',now());"
			char tmp[4096];
			sprintf(tmp, INSERT_VOD, video["name"].asCString(),
				video["vdesc"].asCString(), video["vsize"].asInt64(),
				video["vurl"].asCString(), video["purl"].asCString());
			CHECK_RET(MysqlQuery(_mysql, tmp));
			return true;
		}

		bool Update(const int video_id, const Json::Value &video)
		{
#define UPDATE_VOD "update table_vod set name='%s',vdesc='%s' where id=%d;"
			char tmp[4096] = { 0 };
			sprintf(tmp,UPDATE_VOD,video["name"].asCString(),video["vdesc"].asCString(),video_id);
			CHECK_RET(MysqlQuery(_mysql, tmp));
			return true;
		}

		bool Delete(const int video_id)
		{
#define DELETE_VOD "delete from table_vod  where id = %d;"
			char tmp[4096] = { 0 };
			sprintf(tmp, DELETE_VOD, video_id);
			CHECK_RET(MysqlQuery(_mysql, tmp));
			return true;
		}

		bool GetAll(Json::Value *videos)
		{
#define GETALL_VOD "select *from table_vod;"

			g_mutex.lock();
			CHECK_RET(MysqlQuery(_mysql, GETALL_VOD));
			MYSQL_RES *res = mysql_store_result(_mysql);
			g_mutex.unlock();

			if (res == NULL)
			{
				printf("get all store result failed! %s\n", mysql_error(_mysql));
				return false;
			}
			int num_row = mysql_num_rows(res);
			for (int i = 0; i < num_row; i++)
			{
				MYSQL_ROW row = mysql_fetch_row(res);
				Json::Value video;
				video["id"] = std::stoi(row[0]);
				video["name"] = row[1];
				video["vdesc"] = row[2];
				video["vsize"] = (Json::Value::Int64)std::stol(row[3]);
				video["vurl"] = row[4];
				video["purl"] = row[5];
				video["ultime"] = row[6];
				videos->append(video);//将每一个对象的数据添加到数组中
			}
			mysql_free_result(res);
			return true;
		}

		bool GetOne(const int video_id, Json::Value *video)
		{
#define GETONE_VOD "select * from table_vod where id=%d;"
			char tmp[4096] = { 0 };
			sprintf(tmp,GETONE_VOD,video_id);
			
			g_mutex.lock();
			CHECK_RET(MysqlQuery(_mysql, tmp));
			MYSQL_RES * res = mysql_store_result(_mysql);
			g_mutex.unlock();

			if (res == NULL)
			{
				printf("get one store result failed!\n");
				return false;
			}
			int num_row = mysql_num_rows(res);
			if (num_row != 1)
			{
				printf("get one result count error!\n");
				return false;
			}
			for (int i = 0; i < num_row; i++)
			{
				MYSQL_ROW row = mysql_fetch_row(res);
				(*video)["id"] = std::stoi(row[0]);
				(*video)["name"] = row[1];
				(*video)["vdesc"] = row[2];
				(*video)["vsize"] = (Json::Value::Int64)std::stol(row[3]);
				(*video)["vurl"] = row[4];
				(*video)["purl"] = row[5];
				(*video)["ultime"] = row[6];
			}
			mysql_free_result(res);
			return true;
		}
	private:
		MYSQL *_mysql;
	};
}
