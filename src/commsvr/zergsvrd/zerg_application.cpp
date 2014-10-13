
#include "zerg_predefine.h"
#include "zerg_application.h"
#include "zerg_console_handler.h"
#include "zerg_udp_ctrl_handler.h"
#include "zerg_tcp_ctrl_handler.h"
#include "zerg_ip_restrict.h"
#include "zerg_comm_manager.h"
#include "zerg_configure.h"
#include "zerg_stat_define.h"

/****************************************************************************************************
class  Zerg_Service_App
****************************************************************************************************/

//���ӵ�staticʵ��

//����Ҫ͵͵����
Zerg_Service_App::Zerg_Service_App():
    zerg_comm_mgr_(NULL),
    conf_timestamp_(0)
{
}

Zerg_Service_App::~Zerg_Service_App()
{
}

//����������������
int Zerg_Service_App::on_start(int argc, const char *argv[])
{
    int ret = 0;

    ret = Comm_Svrd_Appliction::on_start(argc, argv);
    if (ret != 0)
    {
        return ret;
    }

    size_t max_accept = 0, max_connect = 0, max_peer = 0;
    TCP_Svc_Handler::get_max_peer_num(max_accept, max_connect);
    max_peer = max_accept + max_connect + 16;


    //���÷��ͽ��ջ��������
    ZBuffer_Storage::instance()->init_buflist_by_hdlnum(max_peer);

    //Zerg_Comm_Manager�����ò��ֿ��Զ�̬�ظ�����
    Zerg_IPRestrict_Mgr::instance();
    //ͨ�Ź�������ʼ��
    zerg_comm_mgr_ = Zerg_Comm_Manager::instance();


    //
    ZCE_LOGMSG(RS_INFO, "[zergsvr] ReloadDynamicConfig Succ.Ooooo!Some people believe that God created the world,but.");

    //-----------------------------------------------------------------------------------------------
    //��ʼ����̬����
    ret = TCP_Svc_Handler::init_all_static_data();

    if ( ret != 0 )
    {
        return ret;
    }
    ret = UDP_Svc_Handler::init_all_static_data();
    if ( ret != 0 )
    {
        return ret;
    }

    //
    ret = zerg_comm_mgr_->init_allpeer();
    if (ret != 0)
    {
        return ret;
    }


    ZCE_LOGMSG(RS_INFO, "[zergsvr] init_instance Succ.Have Fun.!!!");
    //���̼�أ������ã�����˵������ڳ����ʼ��������������Ա�֤�������ڴ�ĳ�ʼ�����������,

    return 0;
}

int Zerg_Service_App::on_exit()
{
    ZCE_LOGMSG(RS_INFO, "[zergsvr] exit_instance Succ.Have Fun.!!!");

    //�ͷ����еľ�̬��Դ���ر����еľ��
    TCP_Svc_Handler::uninit_all_staticdata();

    //������������ʵ��
    Zerg_Comm_Manager::clean_instance();

    //��������
    Zerg_IPRestrict_Mgr::clean_instance();

    //
    ZBuffer_Storage::instance()->close();

    
    //�����û�����˳�����
    Comm_Svrd_Appliction::on_exit();

    return 0;
}


//���к���,�����򲻵���,�����˳�,Ϊ�˼ӿ췢�͵��ٶȣ��Զ����������˲�ͬ��΢��������Ҫ�ĺ���,����Ҳ���,
int Zerg_Service_App::on_run()
{

    //
    const size_t NORMAL_MAX_ONCE_SEND_FRAME = 4096;
    //
    const size_t SENDBUSY_MAX_ONCE_SEND_FRAME = 12288;

    //
    const size_t SEND_BUSY_JUDGE_STANDARD = NORMAL_MAX_ONCE_SEND_FRAME / 2;
    //
    const size_t SEND_IDLE_JUDGE_STANDARD = 128;

    //
    const time_t IDLE_REACTOR_WAIT_USEC = 1000;
    //
    const time_t NORMAL_REACTOR_WAIT_USEC = 100;
    //
    const time_t BUSY_REACTOR_WAIT_USEC = 20;
    //
    const time_t SEND_BUSY_REACTOR_WAIT_USEC = 0;

    //
    const size_t DEFAULT_IO_FIRST_RATIO = 32;

    size_t num_io_event = 0, num_send_frame = 0, want_send_frame = NORMAL_MAX_ONCE_SEND_FRAME;

    ZCE_LOGMSG(RS_INFO, "[zergsvr] Zerg_Service_App::run_instance start.");

    //microsecond
    ZCE_Reactor *preactor = ZCE_Reactor::instance();
    ZCE_Timer_Queue *p_timer_queue = ZCE_Timer_Queue::instance();

    ZCE_Time_Value run_interval(0, IDLE_REACTOR_WAIT_USEC);

    for (size_t i = 0; app_run_; ++i)
    {

        //����Ƚ�æ��
        if ( num_send_frame >= SEND_BUSY_JUDGE_STANDARD )
        {
            //��������Ƿ��ͱȽ�æ��
            if (num_io_event == 0)
            {
                //ȡ�÷�����������
                run_interval.usec(SEND_BUSY_REACTOR_WAIT_USEC);
                want_send_frame = SENDBUSY_MAX_ONCE_SEND_FRAME;
            }
            else
            {
                run_interval.usec(BUSY_REACTOR_WAIT_USEC);
                want_send_frame = NORMAL_MAX_ONCE_SEND_FRAME;
            }
        }
        //����Ƚ�����
        else if ( num_send_frame < SEND_IDLE_JUDGE_STANDARD )
        {
            if (num_io_event == 0)
            {
                run_interval.usec(IDLE_REACTOR_WAIT_USEC);
                want_send_frame = NORMAL_MAX_ONCE_SEND_FRAME;
            }
            else
            {
                run_interval.usec(NORMAL_REACTOR_WAIT_USEC);
                want_send_frame = NORMAL_MAX_ONCE_SEND_FRAME;
            }
        }
        else
        {
            run_interval.usec(BUSY_REACTOR_WAIT_USEC);
            want_send_frame = NORMAL_MAX_ONCE_SEND_FRAME;
        }

        //
        preactor->handle_events(&run_interval, &num_io_event);

        //ÿ�ζ��������ʼ��ZCE_Time_Value����,��Ҫ����.
        zerg_comm_mgr_->popall_sendpipe_write(want_send_frame, num_send_frame);

        //������Ͷ��к�æ���ٽ���һ�η���
        if ((num_send_frame > SEND_BUSY_JUDGE_STANDARD && num_io_event == 0) || num_send_frame >= NORMAL_MAX_ONCE_SEND_FRAME )
        {
            zerg_comm_mgr_->popall_sendpipe_write(want_send_frame, num_send_frame);
        }

        //ż������һ�¶�ʱ��
        if (i % DEFAULT_IO_FIRST_RATIO == 0)
        {
            p_timer_queue->expire();
        }
    }

    ZCE_LOGMSG(RS_INFO, "[zergsvr] Zerg_Service_App::run_instance end.");

    return 0;
}


// ��������
int Zerg_Service_App::load_config()
{
    int ret = 0;
    Zerg_Server_Config *config = reinterpret_cast<Zerg_Server_Config *>(config_base_);
    ret = Zerg_IPRestrict_Mgr::instance()->get_config(config);
    if (0 != ret)
    {
        return ret;
    }

    //ͨ�Ź�������ȡ�����ļ�
    ret = Zerg_Comm_Manager::instance()->get_config(config);
    if (0 != ret)
    {
        return ret;
    }

    ret = TCP_Svc_Handler::get_config(config);
    if (0 != ret)
    {
        return ret;
    }

    ret = UDP_Svc_Handler::get_config(config);
    if (0 != ret)
    {
        return ret;
    }


    return 0;
}

////
//int Zerg_Service_App::reload()
//{
//    int ret = reload_daynamic_config();
//
//    if ( ret != 0 )
//    {
//        ZCE_LOGMSG(RS_INFO,"[zergsvr] reload config fail,ret = %d.", ret);
//        return ret;
//    }
//
//    TCP_Svc_Handler::adjust_svc_handler_pool();
//
//    return 0;
//}

//���¼��������ļ�
//int Zerg_Service_App::reload_daynamic_config()
//{
//
//
//    //��̬�޸���־�ļ���
//    set_log_priority(zerg_config->log_config_.log_level_);
//
//
//
//    return 0;
//}

bool Zerg_Service_App::if_proxy()
{
    unsigned short self_svc_type = self_svc_id_.services_type_;
    if (self_svc_type == SVC_PROXY_SERVER)
    {
        return true;
    }
    return false;
}
