#include "globus_gridftp_server.h"
#include "globus_i_gfs_ipc.h"


typedef struct globus_l_gfs_remote_handle_s
{
    const char *                        user_id;
    globus_gfs_ipc_handle_t             ipc_handle;
} globus_l_gfs_remote_handle_t;


typedef struct globus_l_gfs_remote_ipc_bounce_s
{
    globus_gfs_operation_t              op;
    int                                 trev_arg;
    void *                              state;
    globus_l_gfs_remote_handle_t *      my_handle;
} globus_l_gfs_remote_ipc_bounce_t;

static
void
globus_l_gfs_remote_ipc_error_cb(
    globus_gfs_ipc_handle_t             ipc_handle,
    globus_result_t                     result,
    void *                              user_arg)
{
    globus_i_gfs_log_result(
        "IPC ERROR", result);
            
    return;
}


static
void
globus_l_gfs_ipc_finished_cb(
    globus_gfs_ipc_handle_t             ipc_handle,
    globus_result_t                     ipc_result,
    globus_gfs_ipc_reply_t *            reply,
    void *                              user_arg)
{
    globus_l_gfs_remote_ipc_bounce_t *  bounce_info;
    globus_result_t                     result;
    GlobusGFSName(globus_l_gfs_ipc_stat_cb);
    
    bounce_info = (globus_l_gfs_remote_ipc_bounce_t *)  user_arg;

    globus_gridftp_server_operation_finished(
        bounce_info->op,
        reply->result,
        reply);
   
   result = globus_gfs_ipc_handle_release(ipc_handle);
   if(result != GLOBUS_SUCCESS)
   {
       globus_i_gfs_log_result("IPC ERROR", result);
   }
   bounce_info->my_handle->ipc_handle = NULL;
    
   return;
}

static
void
globus_l_gfs_ipc_event_cb(
    globus_gfs_ipc_handle_t             ipc_handle,
    globus_result_t                     ipc_result,
    globus_gfs_ipc_event_reply_t *      reply,
    void *                              user_arg)
{
    globus_l_gfs_remote_ipc_bounce_t *  bounce_info;
    globus_result_t                     result;
    GlobusGFSName(globus_l_gfs_ipc_stat_cb);
    
    bounce_info = (globus_l_gfs_remote_ipc_bounce_t *)  user_arg;

    globus_gridftp_server_operation_event(
        bounce_info->op,
        GLOBUS_SUCCESS,
        reply);
   
/*
   result = globus_gfs_ipc_handle_release(ipc_handle);
   if(result != GLOBUS_SUCCESS)
   {
       globus_i_gfs_log_result("IPC ERROR", result);
   }
*/
   return;
}



static
globus_result_t
globus_l_gfs_remote_init_bounce_info(
    globus_l_gfs_remote_ipc_bounce_t ** bounce,
    globus_gfs_operation_t              op,
    void *                              state,
    globus_l_gfs_remote_handle_t *      my_handle)
{
    globus_l_gfs_remote_ipc_bounce_t *  bounce_info;
    globus_result_t                     result;
    GlobusGFSName(globus_l_gfs_remote_init_bounce_info);
    
    bounce_info = (globus_l_gfs_remote_ipc_bounce_t *)
        globus_malloc(sizeof(globus_l_gfs_remote_ipc_bounce_t));
    if(!bounce_info)
    {
        result = GlobusGFSErrorMemory("bounce_info");
        goto error_alloc;
    }
    
    bounce_info->op = op;
    bounce_info->state = state;
    bounce_info->my_handle = my_handle;

    *bounce = bounce_info;
    
    return GLOBUS_SUCCESS;

error_alloc:
    return result;
}
    

static
void
globus_l_gfs_remote_stat_kickout(
    globus_gfs_ipc_handle_t             ipc_handle,
    globus_result_t                     result,
    void *                              user_arg)
{
    globus_l_gfs_remote_ipc_bounce_t *  bounce_info;
    int                                 request_id;
    GlobusGFSName(globus_l_gfs_remote_stat_kickout);
    
    bounce_info = (globus_l_gfs_remote_ipc_bounce_t *)  user_arg;

    result = globus_gfs_ipc_request_stat(
        ipc_handle,
        &request_id,
        (globus_gfs_stat_info_t *) bounce_info->state,
        globus_l_gfs_ipc_finished_cb,
        bounce_info); 

    return;
}



static
globus_result_t
globus_l_gfs_remote_stat(
    globus_gfs_operation_t              op,
    globus_gfs_stat_info_t *            stat_info,
    void *                              user_arg)
{
    globus_l_gfs_remote_ipc_bounce_t *  bounce_info;
    globus_result_t                     result;
    globus_l_gfs_remote_handle_t *      my_handle;
    GlobusGFSName(globus_l_gfs_remote_stat);
    
    my_handle = (globus_l_gfs_remote_handle_t *) user_arg;
    
    result = globus_l_gfs_remote_init_bounce_info(
        &bounce_info, op, stat_info, my_handle);
            
    result = globus_gfs_ipc_handle_get(
        my_handle->user_id,
        stat_info->pathname,
        &globus_gfs_ipc_default_iface,
        globus_l_gfs_remote_stat_kickout,
        bounce_info,
        globus_l_gfs_remote_ipc_error_cb,
        bounce_info);        
                    
    return result;
}

static
void
globus_l_gfs_remote_command_kickout(
    globus_gfs_ipc_handle_t             ipc_handle,
    globus_result_t                     result,
    void *                              user_arg)
{
    globus_l_gfs_remote_ipc_bounce_t *  bounce_info;
    int                                 request_id;
    GlobusGFSName(globus_l_gfs_remote_command_kickout);
    
    bounce_info = (globus_l_gfs_remote_ipc_bounce_t *)  user_arg;

    result = globus_gfs_ipc_request_command(
        ipc_handle,
        &request_id,
        (globus_gfs_command_info_t *) bounce_info->state,
        globus_l_gfs_ipc_finished_cb,
        bounce_info); 

    return;
}

static
globus_result_t
globus_l_gfs_remote_command(
    globus_gfs_operation_t              op,
    globus_gfs_command_info_t *         command_info,
    void *                              user_arg)
{
    globus_l_gfs_remote_ipc_bounce_t *  bounce_info;
    globus_result_t                     result;
    globus_l_gfs_remote_handle_t *      my_handle;
    GlobusGFSName(globus_l_gfs_remote_command);
    
    my_handle = (globus_l_gfs_remote_handle_t *) user_arg;
    
    result = globus_l_gfs_remote_init_bounce_info(
        &bounce_info, op, command_info, my_handle);
            
    result = globus_gfs_ipc_handle_get(
        my_handle->user_id,
        command_info->pathname,
        &globus_gfs_ipc_default_iface,
        globus_l_gfs_remote_command_kickout,
        bounce_info,
        globus_l_gfs_remote_ipc_error_cb,
        bounce_info);        
                    
    return result;
}

static
void
globus_l_gfs_remote_list_kickout(
    globus_gfs_ipc_handle_t             ipc_handle,
    globus_result_t                     result,
    void *                              user_arg)
{
    globus_l_gfs_remote_ipc_bounce_t *  bounce_info;
    int                                 request_id;
    GlobusGFSName(globus_l_gfs_remote_list_kickout);
    
    bounce_info = (globus_l_gfs_remote_ipc_bounce_t *)  user_arg;

    result = globus_gfs_ipc_request_list(
        ipc_handle,
        &request_id,
        (globus_gfs_transfer_info_t *) bounce_info->state,
        globus_l_gfs_ipc_finished_cb,
        globus_l_gfs_ipc_event_cb,
        bounce_info); 

    return;
}

static
globus_result_t
globus_l_gfs_remote_list(
    globus_gfs_operation_t              op,
    globus_gfs_transfer_info_t *        transfer_info,
    void *                              user_arg)
{
    globus_l_gfs_remote_ipc_bounce_t *  bounce_info;
    globus_result_t                     result;
    globus_l_gfs_remote_handle_t *      my_handle;
    GlobusGFSName(globus_l_gfs_remote_list);
    
    my_handle = (globus_l_gfs_remote_handle_t *) user_arg;
    
    result = globus_l_gfs_remote_init_bounce_info(
        &bounce_info, op, transfer_info, my_handle);
            
    result = globus_gfs_ipc_handle_get(
        my_handle->user_id,
        transfer_info->pathname,
        &globus_gfs_ipc_default_iface,
        globus_l_gfs_remote_list_kickout,
        bounce_info,
        globus_l_gfs_remote_ipc_error_cb,
        bounce_info);        
                    
    return result;
}

static
void
globus_l_gfs_remote_recv_kickout(
    globus_gfs_ipc_handle_t             ipc_handle,
    globus_result_t                     result,
    void *                              user_arg)
{
    globus_l_gfs_remote_ipc_bounce_t *  bounce_info;
    int                                 request_id;
    GlobusGFSName(globus_l_gfs_remote_recv_kickout);
    
    bounce_info = (globus_l_gfs_remote_ipc_bounce_t *)  user_arg;

    result = globus_gfs_ipc_request_recv(
        ipc_handle,
        &request_id,
        (globus_gfs_transfer_info_t *) bounce_info->state,
        globus_l_gfs_ipc_finished_cb,
        globus_l_gfs_ipc_event_cb,
        bounce_info); 
        
    bounce_info->my_handle->ipc_handle = ipc_handle;
    
    return;
}

static
globus_result_t
globus_l_gfs_remote_recv(
    globus_gfs_operation_t              op,
    globus_gfs_transfer_info_t *        transfer_info,
    void *                              user_arg)
{
    globus_l_gfs_remote_ipc_bounce_t *  bounce_info;
    globus_result_t                     result;
    globus_l_gfs_remote_handle_t *      my_handle;
    GlobusGFSName(globus_l_gfs_remote_recv);
    
    my_handle = (globus_l_gfs_remote_handle_t *) user_arg;
    
    result = globus_l_gfs_remote_init_bounce_info(
        &bounce_info, op, transfer_info, my_handle);
            
    result = globus_gfs_ipc_handle_get(
        my_handle->user_id,
        transfer_info->pathname,
        &globus_gfs_ipc_default_iface,
        globus_l_gfs_remote_recv_kickout,
        bounce_info,
        globus_l_gfs_remote_ipc_error_cb,
        bounce_info);        
                    
    return result;
}

static
void
globus_l_gfs_remote_send_kickout(
    globus_gfs_ipc_handle_t             ipc_handle,
    globus_result_t                     result,
    void *                              user_arg)
{
    globus_l_gfs_remote_ipc_bounce_t *  bounce_info;
    int                                 request_id;
    GlobusGFSName(globus_l_gfs_remote_send_kickout);
    
    bounce_info = (globus_l_gfs_remote_ipc_bounce_t *)  user_arg;

    result = globus_gfs_ipc_request_send(
        ipc_handle,
        &request_id,
        (globus_gfs_transfer_info_t *) bounce_info->state,
        globus_l_gfs_ipc_finished_cb,
        globus_l_gfs_ipc_event_cb,
        bounce_info); 

    bounce_info->my_handle->ipc_handle = ipc_handle;
    
    return;
}

static
globus_result_t
globus_l_gfs_remote_send(
    globus_gfs_operation_t              op,
    globus_gfs_transfer_info_t *        transfer_info,
    void *                              user_arg)
{
    globus_l_gfs_remote_ipc_bounce_t *  bounce_info;
    globus_result_t                     result;
    globus_l_gfs_remote_handle_t *      my_handle;
    GlobusGFSName(globus_l_gfs_remote_send);
    
    my_handle = (globus_l_gfs_remote_handle_t *) user_arg;
    
    result = globus_l_gfs_remote_init_bounce_info(
        &bounce_info, op, transfer_info, my_handle);
            
    result = globus_gfs_ipc_handle_get(
        my_handle->user_id,
        transfer_info->pathname,
        &globus_gfs_ipc_default_iface,
        globus_l_gfs_remote_send_kickout,
        bounce_info,
        globus_l_gfs_remote_ipc_error_cb,
        bounce_info);        
                    
    return result;
}

static
void
globus_l_gfs_remote_active_kickout(
    globus_gfs_ipc_handle_t             ipc_handle,
    globus_result_t                     result,
    void *                              user_arg)
{
    globus_l_gfs_remote_ipc_bounce_t *  bounce_info;
    int                                 request_id;
    GlobusGFSName(globus_l_gfs_remote_active_kickout);
    
    bounce_info = (globus_l_gfs_remote_ipc_bounce_t *)  user_arg;

    result = globus_gfs_ipc_request_active_data(
        ipc_handle,
        &request_id,
        (globus_gfs_data_info_t *) bounce_info->state,
        globus_l_gfs_ipc_finished_cb,
        bounce_info); 

    return;
}

static
globus_result_t
globus_l_gfs_remote_active(
    globus_gfs_operation_t              op,
    globus_gfs_data_info_t *            data_info,
    void *                              user_arg)
{
    globus_l_gfs_remote_ipc_bounce_t *  bounce_info;
    globus_result_t                     result;
    globus_l_gfs_remote_handle_t *      my_handle;
    GlobusGFSName(globus_l_gfs_remote_active);
    
    my_handle = (globus_l_gfs_remote_handle_t *) user_arg;
    
    result = globus_l_gfs_remote_init_bounce_info(
        &bounce_info, op, data_info, my_handle);
            
    result = globus_gfs_ipc_handle_get(
        my_handle->user_id,
        NULL,
        &globus_gfs_ipc_default_iface,
        globus_l_gfs_remote_active_kickout,
        bounce_info,
        globus_l_gfs_remote_ipc_error_cb,
        bounce_info);        
                    
    return result;
}

static
void
globus_l_gfs_remote_passive_kickout(
    globus_gfs_ipc_handle_t             ipc_handle,
    globus_result_t                     result,
    void *                              user_arg)
{
    globus_l_gfs_remote_ipc_bounce_t *  bounce_info;
    int                                 request_id;
    GlobusGFSName(globus_l_gfs_remote_passive_kickout);
    
    bounce_info = (globus_l_gfs_remote_ipc_bounce_t *)  user_arg;

    result = globus_gfs_ipc_request_passive_data(
        ipc_handle,
        &request_id,
        (globus_gfs_data_info_t *) bounce_info->state,
        globus_l_gfs_ipc_finished_cb,
        bounce_info); 

    return;
}

static
globus_result_t
globus_l_gfs_remote_passive(
    globus_gfs_operation_t              op,
    globus_gfs_data_info_t *            data_info,
    void *                              user_arg)
{
    globus_l_gfs_remote_ipc_bounce_t *  bounce_info;
    globus_result_t                     result;
    globus_l_gfs_remote_handle_t *      my_handle;
    GlobusGFSName(globus_l_gfs_remote_passive);
    
    my_handle = (globus_l_gfs_remote_handle_t *) user_arg;
    
    result = globus_l_gfs_remote_init_bounce_info(
        &bounce_info, op, data_info, my_handle);
            
    result = globus_gfs_ipc_handle_get(
        my_handle->user_id,
        NULL,
        &globus_gfs_ipc_default_iface,
        globus_l_gfs_remote_passive_kickout,
        bounce_info,
        globus_l_gfs_remote_ipc_error_cb,
        bounce_info);        
                    
    return result;
}


static
void
globus_l_gfs_remote_data_destroy_kickout(
    globus_gfs_ipc_handle_t             ipc_handle,
    globus_result_t                     result,
    void *                              user_arg)
{
    globus_l_gfs_remote_ipc_bounce_t *  bounce_info;
    GlobusGFSName(globus_l_gfs_remote_data_destroy_kickout);
    
    bounce_info = (globus_l_gfs_remote_ipc_bounce_t *)  user_arg;

    result = globus_gfs_ipc_request_data_destroy(
        ipc_handle,
        (int) bounce_info->state); 

    return;
}

static
void
globus_l_gfs_remote_data_destroy(
    int                                 data_handle_id,
    void *                              user_arg)
{
    globus_l_gfs_remote_ipc_bounce_t *  bounce_info;
    globus_result_t                     result;
    globus_l_gfs_remote_handle_t *      my_handle;
    GlobusGFSName(globus_l_gfs_remote_data_destroy);
    
    my_handle = (globus_l_gfs_remote_handle_t *) user_arg;
    
    result = globus_l_gfs_remote_init_bounce_info(
        &bounce_info, NULL, (void *) data_handle_id, my_handle);
            
    result = globus_gfs_ipc_handle_get(
        my_handle->user_id,
        NULL,
        &globus_gfs_ipc_default_iface,
        globus_l_gfs_remote_data_destroy_kickout,
        bounce_info,
        globus_l_gfs_remote_ipc_error_cb,
        bounce_info);        
                    
    return;
}


static
void
globus_l_gfs_remote_trev(
    int                                 transfer_id,
    int                                 event_type,
    void *                              user_arg)
{
    globus_result_t                     result;
    globus_l_gfs_remote_handle_t *      my_handle;
    GlobusGFSName(globus_l_gfs_remote_trev);
    
    my_handle = (globus_l_gfs_remote_handle_t *) user_arg;
    
    /* lookup handle based on transfer id here */
    result = globus_gfs_ipc_request_transfer_event(
        my_handle->ipc_handle,
        transfer_id,
        event_type);
                                 
    return;
}


static
void
globus_l_gfs_remote_set_cred(
    globus_gfs_operation_t              op,
    gss_cred_id_t                       cred_thing,
    void *                              user_arg)
{
    GlobusGFSName(globus_l_gfs_remote_set_cred);
}



globus_result_t
globus_l_gfs_remote_init(
    const char *                        user_id,
    void **                             out_user_arg)
{
    globus_result_t                     result = GLOBUS_SUCCESS;
    globus_l_gfs_remote_handle_t *      my_handle;
    
    my_handle = (globus_l_gfs_remote_handle_t *) 
        globus_calloc(1, sizeof(globus_l_gfs_remote_handle_t));
        
    my_handle->user_id = globus_libc_strdup(user_id);
    my_handle->ipc_handle = NULL;
    
    *out_user_arg = my_handle;
    
    return result;
}
                                                                                
void
globus_l_gfs_remote_destory(
    void *                              user_arg)
{
}


globus_gfs_storage_iface_t              globus_gfs_remote_dsi_iface = 
{
    globus_l_gfs_remote_init,
    globus_l_gfs_remote_destory,
    globus_l_gfs_remote_list,
    globus_l_gfs_remote_send,
    globus_l_gfs_remote_recv,
    globus_l_gfs_remote_trev,
    globus_l_gfs_remote_active,
    globus_l_gfs_remote_passive,
    globus_l_gfs_remote_data_destroy,
    globus_l_gfs_remote_command, 
    globus_l_gfs_remote_stat,
    globus_l_gfs_remote_set_cred
};


