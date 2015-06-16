/**
 * Copyright (c) UT-Battelle, LLC. 2014-2015. ALL RIGHTS RESERVED.
 * Copyright (C) Mellanox Technologies Ltd. 2001-2015.  ALL RIGHTS RESERVED.
 * $COPYRIGHT$
 * $HEADER$
 */

#include "sysv_iface.h"
#include "sysv_pd.h"

#include <ucs/debug/memtrack.h>
#include <ucs/type/class.h>


static ucs_status_t uct_sysv_mem_alloc(uct_pd_h pd, size_t *length_p,
                                       void **address_p,
                                       uct_mem_h *memh_p UCS_MEMTRACK_ARG)
{
    ucs_status_t rc;
    uct_sysv_lkey_t *key_hndl = NULL;
    int shmid = 0;

    if (0 == *length_p) {
        ucs_error("Unexpected length %zu", *length_p);
        return UCS_ERR_INVALID_PARAM;
    }

    key_hndl = ucs_malloc(sizeof(*key_hndl), "key_hndl");
    if (NULL == key_hndl) {
        ucs_error("Failed to allocate memory for key_hndl");
        return UCS_ERR_NO_MEMORY;
    }

    rc = ucs_sysv_alloc(length_p, address_p, 0, &shmid UCS_MEMTRACK_VAL);
    if (rc != UCS_OK) {
        ucs_error("Failed to attach %zu bytes", *length_p);
        ucs_free(key_hndl);
        return rc;
    }

    key_hndl->shmid = shmid;
    key_hndl->owner_ptr = *address_p;

    ucs_debug("Memory registration address_p %p, len %lu, keys [%d %p]",
              *address_p, *length_p, key_hndl->shmid, key_hndl->owner_ptr);
    *memh_p = key_hndl;
    ucs_memtrack_allocated(address_p, length_p UCS_MEMTRACK_VAL);
    return UCS_OK;
}

static ucs_status_t uct_sysv_mem_free(uct_pd_h pd, uct_mem_h memh)
{
    /* this releases the key allocated in uct_sysv_mem_alloc */

    uct_sysv_lkey_t *key_hndl = memh;
    ucs_sysv_free(key_hndl->owner_ptr);  /* detach shared segment */
    ucs_free(key_hndl);

    return UCS_OK;
}

static ucs_status_t uct_sysv_pd_query(uct_pd_h pd, uct_pd_attr_t *pd_attr)
{
    pd_attr->cap.flags         = UCT_PD_FLAG_ALLOC;
    pd_attr->cap.max_alloc     = ULONG_MAX;
    pd_attr->cap.max_reg       = 0;
    memset(&pd_attr->local_cpus, 0xff, sizeof(pd_attr->local_cpus));
    return UCS_OK;
}

static ucs_status_t uct_sysv_mkey_pack(uct_pd_h pd, uct_mem_h memh,
                                       void *rkey_buffer)
{
    /* user is responsible to free rkey_buffer */
    uct_sysv_rkey_t *rkey = rkey_buffer;
    uct_sysv_lkey_t *key_hndl = memh;

    rkey->shmid     = key_hndl->shmid;
    rkey->owner_ptr = (uintptr_t)key_hndl->owner_ptr;

    ucs_trace("packed rkey: shmid %d owner_ptr %"PRIxPTR,
              rkey->shmid, rkey->owner_ptr);
    return UCS_OK;
}

static void uct_sysv_rkey_release(uct_rkey_t rkey, void *handle)
{
    /* detach shared segment */
    shmdt(handle);
}

ucs_status_t uct_sysv_rkey_unpack(const void *rkey_buffer, uct_rkey_t *rkey_p,
                                  void **handle_p)
{
    /* user is responsible to free rkey_buffer */
    const uct_sysv_rkey_t *rkey = rkey_buffer;
    int shmid;
    void *client_ptr;

    ucs_trace("unpacking rkey: shmid %d owner_ptr %"PRIxPTR,
              rkey->shmid, rkey->owner_ptr);

    /* Attach segment */
    /* FIXME would like to extend ucs_sysv_alloc to do this? */
    shmid = rkey->shmid;
    client_ptr = shmat(shmid, NULL, 0);
    /* Check if attachment was successful */
    if (client_ptr == (void*)-1) {
        if (errno == ENOMEM) {
            return UCS_ERR_NO_MEMORY;
        } else if (RUNNING_ON_VALGRIND && (errno == EINVAL)) {
            return UCS_ERR_NO_MEMORY;
        } else {
            ucs_error("shmat(shmid=%d) returned unexpected error: %m", shmid);
            return UCS_ERR_SHMEM_SEGMENT;
        }
    }

    /* store the offset of the addresses, this can be used directly to translate
     * the remote VA to local VA of the attached segment */
    *handle_p = client_ptr;
    *rkey_p   = (uintptr_t)client_ptr - rkey->owner_ptr;
    return UCS_OK;
}

static ucs_status_t uct_sysv_query_pd_resources(uct_pd_resource_desc_t **resources_p,
                                                unsigned *num_resources_p)
{
    return uct_single_pd_resource(&uct_sysv_pd, resources_p, num_resources_p);
}

static ucs_status_t uct_sysv_pd_open(const char *pd_name, uct_pd_h *pd_p)
{
    static uct_pd_ops_t pd_ops = {
        .close        = (void*)ucs_empty_function,
        .query        = uct_sysv_pd_query,
        .mem_alloc    = uct_sysv_mem_alloc,
        .mem_free     = uct_sysv_mem_free,
        .mkey_pack    = uct_sysv_mkey_pack,
    };
    static uct_pd_t pd = {
        .ops          = &pd_ops,
        .component    = &uct_sysv_pd
    };

    *pd_p = &pd;
    return UCS_OK;
}

UCT_PD_COMPONENT_DEFINE(uct_sysv_pd, UCT_SYSV_TL_NAME,
                        uct_sysv_query_pd_resources, uct_sysv_pd_open,
                        sizeof(uct_sysv_rkey_t), uct_sysv_rkey_unpack,
                        uct_sysv_rkey_release)
