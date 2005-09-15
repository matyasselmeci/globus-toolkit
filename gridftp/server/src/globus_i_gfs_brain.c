/*
 * Portions of this file Copyright 1999-2005 University of Chicago
 * Portions of this file Copyright 1999-2005 The University of Southern California.
 *
 * This file or a portion of this file is licensed under the
 * terms of the Globus Toolkit Public License, found at
 * http://www.globus.org/toolkit/download/license.html.
 * If you redistribute this file, with or without
 * modifications, you must include this notice in the file.
 */

#include "globus_i_gridftp_server.h"
#include "globus_i_gfs_ipc.h"

#define GFS_BRAIN_FIXED_SIZE 256

extern globus_i_gfs_community_t *       globus_i_gfs_ipc_community_default;

static globus_list_t *                  globus_l_brain_repo_list;
static globus_mutex_t                   globus_l_brain_mutex;
static globus_xio_server_t              globus_l_brain_server_handle;

static
globus_i_gfs_community_t *
globus_l_brain_find_community(
    const char *                        repo_name)
{
    globus_list_t *                     list;
    globus_i_gfs_community_t *          repo = NULL;
    globus_i_gfs_community_t *          tmp_repo = NULL;

    if(repo_name == NULL ||
        strcmp(repo_name, globus_i_gfs_ipc_community_default->name) == 0)
    {
        repo = globus_i_gfs_ipc_community_default;
    }
    else
    {
        list = globus_l_brain_repo_list;
        while(!globus_list_empty(list) && repo == NULL)
        {
            tmp_repo = globus_list_first(list);
            if(strcmp(tmp_repo->name, repo_name) == 0)
            {
                repo = tmp_repo;
            }
            list = globus_list_rest(list);
        }
    }

    return repo;
}

static
void
globus_l_brain_read_cb(
    globus_xio_handle_t                 handle,
    globus_result_t                     result,
    globus_byte_t *                     buffer,
    globus_size_t                       len,
    globus_size_t                       nbytes,
    globus_xio_data_descriptor_t        data_desc,
    void *                              user_arg)
{
    globus_i_gfs_community_t *          repo = NULL;
    int                                 i;
    char *                              start_str;
    char *                              repo_name = NULL;
    char *                              cs = NULL;

    globus_mutex_lock(&globus_l_brain_mutex);
    {
        if(result != GLOBUS_SUCCESS)
        {
            /* log an error */
            goto error;
        }

        /* verify message */
        start_str = buffer;
        for(i = 0; i < len; i++)
        {
            if(buffer[i] == '\0')
            {
                if(repo_name == NULL)
                {
                    repo_name = start_str;
                    start_str = &buffer[i+1];
                }
                else if(cs == NULL)
                {
                    cs = strdup(start_str);
                }
            }
            else if(!isprint(buffer[i]))
            {
                /* log an error */
                goto error;
            }
        }
        if(strcmp("", repo_name) == 0)
        {
            repo_name = NULL;
        }

        repo = globus_l_brain_find_community(repo_name);
        if(repo == NULL)
        {
            goto error;
        }
        repo->cs_count++;
        repo->cs = (char **)
            globus_libc_realloc(repo->cs, repo->cs_count * sizeof(char *));
        repo->cs[repo->cs_count-1]= cs;

error:
    globus_xio_register_close(
        handle,
        NULL,
        NULL,
        NULL);
    }
    globus_mutex_unlock(&globus_l_brain_mutex);

}


static
void
globus_l_brain_open_server_cb(
    globus_xio_handle_t                 handle,
    globus_result_t                     result,
    void *                              user_arg)
{
    globus_byte_t *                     buffer;
    globus_bool_t                       accept;

    if(result != GLOBUS_SUCCESS)
    {
        /* XXX log error */
        goto error_accept;
    }
    /* XXX todo verify we are ok with the sender */
    accept = GLOBUS_TRUE;

    buffer = globus_calloc(1, GFS_BRAIN_FIXED_SIZE);
    if(!accept)
    {
    }
    result = globus_xio_register_read(
        handle,
        buffer,
        GFS_BRAIN_FIXED_SIZE,
        GFS_BRAIN_FIXED_SIZE,
        NULL,
        globus_l_brain_read_cb,
        NULL);
    if(result != GLOBUS_SUCCESS)
    {
        goto error_read;
    }

    return;

error_read:
    globus_free(buffer);
error_accept:
    return;
}

static
void
globus_l_brain_add_server_accept_cb(
    globus_xio_server_t                 server,
    globus_xio_handle_t                 handle,
    globus_result_t                     result,
    void *                              user_arg)
{
    if(result != GLOBUS_SUCCESS)
    {
        /* XXX log error */
        goto error;
    }

   result = globus_xio_register_open(
        handle,
        NULL,
        NULL,
        globus_l_brain_open_server_cb,
        NULL);
    if(result != GLOBUS_SUCCESS)
    {
        /* XXX log error */
    }

error:
    result = globus_xio_server_register_accept(
        globus_l_brain_server_handle,
        globus_l_brain_add_server_accept_cb,
        NULL);
    if(result != GLOBUS_SUCCESS)
    {
        /* XXX log error */
    }
}

static
globus_result_t
globus_l_brain_listen()
{
    char *                              contact_string;
    globus_result_t                     res;
    globus_xio_attr_t                   attr;
    int                                 port = 0;
    GlobusGFSName(globus_l_brain_listen);

    GlobusGFSDebugEnter();

    res = globus_xio_attr_init(&attr);
    if(res != GLOBUS_SUCCESS)
    {
        goto error_attr_init;
    }

    port =  globus_i_gfs_config_int("ipc_port");

    res = globus_xio_attr_cntl(
        attr,
        globus_i_gfs_tcp_driver,
        GLOBUS_XIO_TCP_SET_PORT,
        port);
    if(res != GLOBUS_SUCCESS)
    {
        goto error_attr;
    }
    res = globus_xio_server_create(
        &globus_l_brain_server_handle, attr, globus_i_gfs_ipc_xio_stack);
    if(res != GLOBUS_SUCCESS)
    {
        goto error_attr;
    }
    res = globus_xio_server_get_contact_string(
        globus_l_brain_server_handle,
        &contact_string);
    if(res != GLOBUS_SUCCESS)
    {
        goto error_server;
    }
    if(port == 0)
    {
        /* XXX log the port using */
    }

    res = globus_xio_server_register_accept(
        globus_l_brain_server_handle,
        globus_l_brain_add_server_accept_cb,
        NULL);
    if(res != GLOBUS_SUCCESS)
    {
        goto error_accept;
    }

    GlobusGFSDebugExit();

    return GLOBUS_SUCCESS;

error_accept:
    globus_free(contact_string);
error_server:
error_attr:
    globus_xio_attr_destroy(attr);
error_attr_init:

    return res;
}


globus_result_t
globus_i_gfs_brain_init()
{
    globus_result_t                     res;

    globus_mutex_init(&globus_l_brain_mutex, NULL);
    globus_l_brain_repo_list = globus_list_copy(
        globus_i_gfs_config_list("community"));

    globus_mutex_lock(&globus_l_brain_mutex);
    {
        if(globus_i_gfs_config_int("brain_listen"))
        {
            res = globus_l_brain_listen();
            if(res != GLOBUS_SUCCESS)
            {
                goto error;
            }
        }
    }
    globus_mutex_unlock(&globus_l_brain_mutex);

    return GLOBUS_SUCCESS;

error:
    globus_mutex_unlock(&globus_l_brain_mutex);

    return res;
}

void
globus_i_gfs_brain_stop()
{
    globus_mutex_destroy(&globus_l_brain_mutex);
}

globus_result_t
globus_gfs_brain_select_nodes(
    char ***                            out_contact_strings,
    int *                               out_array_length,
    const char *                        repo_name,
    globus_off_t                        filesize,
    int                                 min_count,
    int                                 max_count)
{
    int                                 best_count;
    int                                 count;
    char **                             cs;
    globus_result_t                     result;
    globus_i_gfs_community_t *          repo = NULL;
    GlobusGFSName(globus_gfs_brain_select_nodes);

    globus_mutex_lock(&globus_l_brain_mutex);
    {
        repo = globus_l_brain_find_community(repo_name);
        if(repo == NULL)
        {
            result = globus_error_put(GlobusGFSErrorObjParameter("repo_name"));
            goto error;
        }

        best_count = globus_i_gfs_config_int("repo_count");
        if(best_count > max_count || best_count <= 0)
        {
            best_count = max_count;
        }

        /* this is the tester brain dead approach */
        cs = globus_calloc(max_count, sizeof(char *));
        if(cs == NULL)
        {
            result = globus_error_put(GlobusGFSErrorObjMemory("cs"));
            goto error;
        }
        count = 0;
        while(count < best_count)
        {
            cs[count] = strdup(repo->cs[repo->next_ndx]);
            count++;
            repo->next_ndx++;
            if(repo->next_ndx >= repo->cs_count)
            {
                repo->next_ndx = 0;
            }
        }

        *out_contact_strings = cs;
        *out_array_length = count;
    }
    globus_mutex_unlock(&globus_l_brain_mutex);

    return GLOBUS_SUCCESS;

error:
    globus_mutex_unlock(&globus_l_brain_mutex);
    return result;
}

globus_result_t
globus_gfs_brain_release_node(
    char *                              contact_string,
    const char *                        repo_name,
    globus_gfs_brain_reason_t           reason)
{
    /* depending on reason we may remove from list or whatever */

    return GLOBUS_SUCCESS;
}
