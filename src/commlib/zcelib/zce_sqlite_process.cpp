
#include "zce_predefine.h"
#include "zce_trace_log_debug.h"
#include "zce_sqlite_process.h"

//对于SQLITE的最低版本限制
#if SQLITE_VERSION_NUMBER >= 3005000

//=========================================================================================

/******************************************************************************************
SQLite3_DB_Handler SQLite3DB Handler 连接处理一个SQLite3数据库的Handler
******************************************************************************************/
ZCE_SQLite_DB_Handler::ZCE_SQLite_DB_Handler():
    sqlite3_handler_(NULL)
{
}

ZCE_SQLite_DB_Handler::~ZCE_SQLite_DB_Handler()
{
    close_database();
}

//const char* db_file ,数据库名称文件路径,接口要求UTF8编码，
//int == 0表示成功，否则失败
int ZCE_SQLite_DB_Handler::open_database(const char *db_file, bool create_db)
{
    int flags = SQLITE_OPEN_READWRITE;
    if (create_db)
    {
        flags |= SQLITE_OPEN_CREATE;
    }

    int ret = ::sqlite3_open_v2(db_file,
                                &sqlite3_handler_,
                                flags,
                                NULL);
    if (ret != SQLITE_OK )
    {
        ZCE_LOGMSG(RS_ERROR, "[zcelib] sqlite3_open_v2 fail:[%d][%s]",
                   error_code(),
                   error_message());
        return -1;
    }

    return 0;

}



//以只读的方式打开一个数据库
//这个特性要3.5以后的版本才可以用。
int ZCE_SQLite_DB_Handler::open_readonly_db(const char *db_file)
{

    int ret = ::sqlite3_open_v2(db_file,
                                &sqlite3_handler_,
                                SQLITE_OPEN_READONLY,
                                NULL);
    //
    if (ret != SQLITE_OK )
    {
        ZCE_LOGMSG(RS_ERROR, "[zcelib] sqlite3_open_v2 open readonly table fail:[%d][%s]",
                   error_code(),
                   error_message());
        return -1;
    }

    return 0;

}


//关闭数据库。
void ZCE_SQLite_DB_Handler::close_database()
{
    if (sqlite3_handler_)
    {
        ::sqlite3_close_v2(sqlite3_handler_);
        sqlite3_handler_ = NULL;
    }
}

//错误语句Str
const char *ZCE_SQLite_DB_Handler::error_message()
{
    return ::sqlite3_errmsg(sqlite3_handler_);
}

//DB返回的错误ID
int ZCE_SQLite_DB_Handler::error_code()
{
    return ::sqlite3_errcode(sqlite3_handler_);
}

//开始一个事务
int ZCE_SQLite_DB_Handler::begin_transaction()
{
    int ret = 0;
    char *err_msg = NULL;
    ret = ::sqlite3_exec(sqlite3_handler_,
                         "BEGIN TRANSACTION;",
                         NULL,
                         NULL,
                         &err_msg);

    if (ret == SQLITE_OK)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

//提交一个事务
int ZCE_SQLite_DB_Handler::commit_transction()
{
    int ret = 0;
    char *err_msg = NULL;
    ret = ::sqlite3_exec(sqlite3_handler_,
                         "COMMIT TRANSACTION;",
                         NULL,
                         NULL,
                         &err_msg);

    if (ret == SQLITE_OK)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

//将同步选项关闭，可以适当的提高insert的速度，但是为了安全起见，建议不要使用
int ZCE_SQLite_DB_Handler::turn_off_synch()
{
    int ret = 0;
    char *err_msg = NULL;
    ret = ::sqlite3_exec(sqlite3_handler_,
                         "PRAGMA synchronous=OFF;",
                         NULL,
                         NULL,
                         &err_msg);

    if (ret == SQLITE_OK)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

//执行SQL 查询，取得结果
int ZCE_SQLite_DB_Handler::get_table(const char *sql_string,
                                     ZCE_SQLite_Result *result)
{
    int ret = SQLITE_OK;
    ret = ::sqlite3_get_table(sqlite3_handler_, sql_string,
                              &(result->result_),
                              &(result->row_),
                              &(result->column_),
                              &(result->err_msg_));
    if (ret != SQLITE_OK)
    {
        ZCE_LOGMSG(RS_ERROR, "[zcelib] sqlite3_get_table execute fail:[%d][%s]",
                   ret,
                   result->err_msg_);
    }
    return 0;
}

//=========================================================================================

ZCE_SQLite_Result::ZCE_SQLite_Result()
{

}


ZCE_SQLite_Result::~ZCE_SQLite_Result()
{

}

//释放结果集合
void ZCE_SQLite_Result::free_result()
{
    if (err_msg_)
    {
        ::sqlite3_free(err_msg_);
    }
    if (result_)
    {
        ::sqlite3_free_table(result_);
    }

    column_ = 0;
    row_ = 0;
}



#endif //#if SQLITE_VERSION_NUMBER >= 3005000

