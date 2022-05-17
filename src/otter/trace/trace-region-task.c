#include <stdlib.h>
#include <pthread.h>
#include "otter/trace-lookup-macros.h"
#include "otter/trace-attributes.h"
#include "otter/trace-structs.h"
#include "otter/trace-region-task.h"
#include "otter/trace-unique-refs.h"
#include "otter/trace-check-error-code.h"
#include "otter/queue.h"
#include "otter/stack.h"
#include "otter/char_ref_registry.hpp"

/* Defined in trace-archive.c */
extern OTF2_StringRef attr_name_ref[n_attr_defined][2];
extern OTF2_StringRef attr_label_ref[n_attr_label_defined];

trace_region_def_t *
trace_new_task_region(
    trace_location_def_t  *loc, 
    trace_region_def_t    *parent_task_region, 
    unique_id_t            id,
    otter_task_flag_t      flags,
    int                    has_dependences,
    otter_src_location_t  *src_location)
{
    /* Create a region representing a task. Add to the location's region
       definition queue. */

    /* A task maintains a stack of the active regions encountered during its
       execution up to a task-switch event, which is restored to the executing
       thread when the task is resumed */

    LOG_INFO_IF((parent_task_region == NULL),
        "[t=%lu] parent task region is null", loc->id);

    LOG_DEBUG_IF((src_location), "got src_location(file=%s, func=%s, line=%d)\n", src_location->file, src_location->func, src_location->line);

    // if (src_location) {
    //     fprintf(stderr, "[%s] got src_location(file=%s, func=%s, line=%d)\n",
    //         __func__,
    //         src_location->file,
    //         src_location->func,
    //         src_location->line
    //     );
    // }

    trace_region_def_t *new = malloc(sizeof(*new));
    *new = (trace_region_def_t) {
        .ref = get_unique_rgn_ref(),
        .role = OTF2_REGION_ROLE_TASK,
        .attributes = OTF2_AttributeList_New(),
        .type = trace_region_task,
        .rgn_stack = stack_create(),
        .attr.task = {
            .id              = id,
            .type            = flags & 0xF,
            .flags           = flags,
            .has_dependences = has_dependences,
            .parent_id   = parent_task_region != NULL ? 
                parent_task_region->attr.task.id   : OTF2_UNDEFINED_UINT64,
            .parent_type = parent_task_region != NULL ? 
                parent_task_region->attr.task.type : OTF2_UNDEFINED_UINT32,
            .task_status     = 0 /* no status */,

            .source_file_name_ref = src_location ? char_ref_registry_insert(get_global_str_registry(), src_location->file) : 0,
            .source_func_name_ref = src_location ? char_ref_registry_insert(get_global_str_registry(), src_location->func) : 0,
            .source_line_number = src_location ? src_location->line : 0,
        }
    };
    new->encountering_task_id = new->attr.task.parent_id;

    LOG_DEBUG("[t=%lu] created region %u for task %lu at %p",
        loc->id, new->ref, new->attr.task.id, new);

    /* Add region definition to location's region definition queue */
    queue_push(loc->rgn_defs, (data_item_t) {.ptr = new});

    return new;
}

void
trace_destroy_task_region(trace_region_def_t *rgn)
{
    LOG_WARN_IF(
        (!(rgn->attr.task.task_status == otter_task_complete 
            || rgn->attr.task.task_status == otter_task_cancel)),
        "destroying task region before task-complete/task-cancel");
    LOG_DEBUG("region %p destroying attribute list %p", rgn, rgn->attributes);
    OTF2_AttributeList_Delete(rgn->attributes);
    LOG_DEBUG("region %p destroying active regions stack %p", rgn, rgn->rgn_stack);
    stack_destroy(rgn->rgn_stack, false, NULL);
    LOG_DEBUG("region %p", rgn);
    free(rgn);
}

void
trace_add_task_attributes(trace_region_def_t *rgn)
{
    OTF2_ErrorCode r = OTF2_SUCCESS;
    r = OTF2_AttributeList_AddUint64(rgn->attributes, attr_unique_id,
        rgn->attr.task.id);
    CHECK_OTF2_ERROR_CODE(r);
    r = OTF2_AttributeList_AddStringRef(rgn->attributes, attr_task_type,
        TASK_TYPE_TO_STR_REF(rgn->attr.task.type));
    CHECK_OTF2_ERROR_CODE(r);
    r = OTF2_AttributeList_AddUint32(rgn->attributes, attr_task_flags,
        rgn->attr.task.flags);
    CHECK_OTF2_ERROR_CODE(r);
    r = OTF2_AttributeList_AddUint64(rgn->attributes, attr_parent_task_id,
        rgn->attr.task.parent_id);
    CHECK_OTF2_ERROR_CODE(r);
    r = OTF2_AttributeList_AddStringRef(rgn->attributes, attr_parent_task_type,
        TASK_TYPE_TO_STR_REF(rgn->attr.task.parent_type));
    CHECK_OTF2_ERROR_CODE(r);
    r = OTF2_AttributeList_AddUint8(rgn->attributes, attr_task_has_dependences,
        rgn->attr.task.has_dependences);
    CHECK_OTF2_ERROR_CODE(r);
    r = OTF2_AttributeList_AddUint8(rgn->attributes, attr_task_is_undeferred,
        rgn->attr.task.flags & ompt_task_undeferred);
    CHECK_OTF2_ERROR_CODE(r);
    r = OTF2_AttributeList_AddUint8(rgn->attributes, attr_task_is_untied,
        rgn->attr.task.flags & ompt_task_untied);
    CHECK_OTF2_ERROR_CODE(r);
    r = OTF2_AttributeList_AddUint8(rgn->attributes, attr_task_is_final,
        rgn->attr.task.flags & ompt_task_final);
    CHECK_OTF2_ERROR_CODE(r);
    r = OTF2_AttributeList_AddUint8(rgn->attributes, attr_task_is_mergeable,
        rgn->attr.task.flags & ompt_task_mergeable);
    CHECK_OTF2_ERROR_CODE(r);
    r = OTF2_AttributeList_AddUint8(rgn->attributes, attr_task_is_merged,
        rgn->attr.task.flags & ompt_task_merged);
    CHECK_OTF2_ERROR_CODE(r);
    r = OTF2_AttributeList_AddStringRef(rgn->attributes, attr_prior_task_status,
        TASK_STATUS_TO_STR_REF(rgn->attr.task.task_status));
    CHECK_OTF2_ERROR_CODE(r);

    // Add source location if defined for this task
    if (rgn->attr.task.source_file_name_ref != 0)
    {
        r = OTF2_AttributeList_AddUint32(rgn->attributes, attr_source_line_number,
        rgn->attr.task.source_line_number);
        CHECK_OTF2_ERROR_CODE(r);
        r = OTF2_AttributeList_AddStringRef(rgn->attributes, attr_source_file_name,
            rgn->attr.task.source_file_name_ref);
        CHECK_OTF2_ERROR_CODE(r);
        r = OTF2_AttributeList_AddStringRef(rgn->attributes, attr_source_func_name,
            rgn->attr.task.source_func_name_ref);
        CHECK_OTF2_ERROR_CODE(r);
    }
    return;
}
