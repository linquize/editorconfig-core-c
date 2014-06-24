/*
 * Copyright (c) 2011-2012 EditorConfig Team
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
*/

#include "global.h"
#include "editorconfig.h"
#include "misc.h"
#include "ini.h"
#include "ec_fnmatch.h"

/* could be used to fast locate these properties in an
 * array_editorconfig_name_value */
typedef struct
{
    const editorconfig_name_value*        indent_style;
    const editorconfig_name_value*        indent_size;
    const editorconfig_name_value*        tab_width;
} special_property_name_value_pointers;

typedef struct
{
    int* ptr;
    int count;
    int size;
} array_of_int;

typedef struct
{
    editorconfig_name_value*                name_values;
    array_of_int                            file_counts;
    int                                     current_value_count;
    int                                     max_value_count;
    special_property_name_value_pointers    spnvp;
} array_editorconfig_name_value;

typedef struct
{
    char*                           full_filename;
    char*                           editorconfig_file_dir;
    array_editorconfig_name_value   array_name_value;
    int                             root;
    int                             file_index;
} handler_first_param;

/*
 * Set the special pointers for a name
 */
static void set_special_property_name_value_pointers(
        const editorconfig_name_value* nv,
        special_property_name_value_pointers* spnvp)
{
    /* set speical pointers */
    if (!strcmp(nv->name, "indent_style"))
        spnvp->indent_style = nv;
    else if (!strcmp(nv->name, "indent_size"))
        spnvp->indent_size = nv;
    else if (!strcmp(nv->name, "tab_width"))
        spnvp->tab_width = nv;
}

/*
 * Set the name and value of a editorconfig_name_value structure
 */
static void set_name_value(editorconfig_name_value* nv, const char* name,
        const char* value, special_property_name_value_pointers* spnvp)
{
    if (name)
        nv->name = strdup(name);
    if (value)
        nv->value = strdup(value);
    /* lowercase the value when the name is one of the following */
    if (!strcmp(nv->name, "end_of_line") ||
            !strcmp(nv->name, "indent_style") ||
            !strcmp(nv->name, "indent_size") ||
            !strcmp(nv->name, "insert_final_newline") ||
            !strcmp(nv->name, "trim_trailing_whitespace") ||
            !strcmp(nv->name, "charset"))
        strlwr(nv->value);

    /* set speical pointers */
    set_special_property_name_value_pointers(nv, spnvp);
}

/*
 * reset special property name value pointers
 */
static void reset_special_property_name_value_pointers(
        array_editorconfig_name_value* aenv)
{
    int         i;
    
    for (i = 0; i < aenv->current_value_count; ++ i)
        set_special_property_name_value_pointers(
                &aenv->name_values[i], &aenv->spnvp);
}

/*
 * Find the editorconfig_name_value from name in a editorconfig_name_value
 * array.
 */
static int find_name_value_from_name(const editorconfig_name_value* env,
        int count, const char* name)
{
    int         i;

    for (i = 0; i < count; ++i)
        if (!strcmp(env[i].name, name)) /* found */
            return i;

    return -1;
}

/* initialize array_editorconfig_name_value */
static void array_editorconfig_name_value_init(
        array_editorconfig_name_value* aenv)
{
    memset(aenv, 0, sizeof(array_editorconfig_name_value));
}

static int array_editorconfig_name_value_add(
        array_editorconfig_name_value* aenv,
        const char* name, const char* value, int file_index)
{
#define VALUE_COUNT_INITIAL      30
#define VALUE_COUNT_INCREASEMENT 10
    int         name_value_pos;
    /* always use name_lwr but not name, since property names are case
     * insensitive */
    char        name_lwr[MAX_PROPERTY_NAME];
    /* For the first time we came here, aenv->name_values is NULL */
    if (aenv->name_values == NULL) {
        aenv->name_values = (editorconfig_name_value*)malloc(
                sizeof(editorconfig_name_value) * VALUE_COUNT_INITIAL);

        if (aenv->name_values == NULL)
            return -1;

        aenv->max_value_count = VALUE_COUNT_INITIAL;
        aenv->current_value_count = 0;
    }


    /* name_lwr is the lowercase property name */
    strlwr(strcpy(name_lwr, name));

    name_value_pos = find_name_value_from_name(
            aenv->name_values, aenv->current_value_count, name_lwr);

    if (name_value_pos >= 0) { /* current name has already been used */
        if (file_index != 0 && aenv->name_values[name_value_pos].file_index != file_index)
            return 0;
        free(aenv->name_values[name_value_pos].value);
        set_name_value(&aenv->name_values[name_value_pos],
                (const char*)NULL, value, &aenv->spnvp);
        return 0;
    }

    /* if the space is not enough, allocate more before add the new name and
     * value */
    if (aenv->current_value_count >= aenv->max_value_count) {

        editorconfig_name_value*        new_values;
        int                             new_max_value_count;

        new_max_value_count = aenv->current_value_count +
            VALUE_COUNT_INCREASEMENT;
        new_values = (editorconfig_name_value*)realloc(aenv->name_values,
                sizeof(editorconfig_name_value) * new_max_value_count);

        if (new_values == NULL) /* error occured */
            return -1;

        aenv->name_values = new_values;
        aenv->max_value_count = new_max_value_count;

        /* reset special pointers */
        reset_special_property_name_value_pointers(aenv);
    }

    if (file_index >= aenv->file_counts.count) {
        if (file_index >= aenv->file_counts.size) {
            int new_size = aenv->file_counts.count + VALUE_COUNT_INCREASEMENT;
            int* ptr = (int*)realloc(aenv->file_counts.ptr,
                    sizeof(int) * new_size);
            if (ptr == NULL) /* error occured */
                return -1;

            memset(ptr + file_index, 0, sizeof(int) * (aenv->file_counts.size - new_size));
            aenv->file_counts.ptr = ptr;
            aenv->file_counts.size = new_size;
            aenv->file_counts.count = file_index + 1;
        }
        else
            aenv->file_counts.count = file_index + 1;
    }

    aenv->name_values[aenv->current_value_count].file_index = file_index;
    aenv->file_counts.ptr[file_index]++;
    set_name_value(&aenv->name_values[aenv->current_value_count],
            name_lwr, value, &aenv->spnvp);
    ++ aenv->current_value_count;

    return 0;
#undef VALUE_COUNT_INITIAL
#undef VALUE_COUNT_INCREASEMENT
}

static void array_editorconfig_name_value_clear(
        array_editorconfig_name_value* aenv)
{
    int             i;

    for (i = 0; i < aenv->current_value_count; ++i) {
        free(aenv->name_values[i].name);
        free(aenv->name_values[i].value);
    }

    free(aenv->name_values);
    if (aenv->file_counts.ptr)
        free(aenv->file_counts.ptr);
}

/*
 * Accept INI property value and store known values in handler_first_param
 * struct.
 */
static int ini_handler(void* hfp, const char* section, const char* name,
        const char* value)
{
    handler_first_param* hfparam = (handler_first_param*)hfp;
    /* prepend ** to pattern */
    char*                pattern;

    /* root = true, clear all previous values */
    if (*section == '\0' && !strcasecmp(name, "root") &&
            !strcasecmp(value, "true")) {
        hfparam->root = 1;
    }

    /* pattern would be: /dir/of/editorconfig/file[double_star]/[section] if
     * section does not contain '/', or /dir/of/editorconfig/file[section]
     * if section starts with a '/', or /dir/of/editorconfig/file/[section] if
     * section contains '/' but does not start with '/' */
    pattern = (char*)malloc(
            strlen(hfparam->editorconfig_file_dir) * sizeof(char) +
            sizeof("**/") + strlen(section) * sizeof(char));
    if (!pattern)
        return 0;
    strcpy(pattern, hfparam->editorconfig_file_dir);

    if (strchr(section, '/') == NULL) /* No / is found, append '[star][star]/' */
        strcat(pattern, "**/");
    else if (*section != '/') /* The first char is not '/' but section contains
                                 '/', append a '/' */
        strcat(pattern, "/");

    strcat(pattern, section);

    if (ec_fnmatch(pattern, hfparam->full_filename, EC_FNM_PATHNAME) == 0) {
        if (array_editorconfig_name_value_add(&hfparam->array_name_value, name,
                value, hfparam->file_index)) {
            free(pattern);
            return 0;
        }
    }

    free(pattern);
    return 1;
}

/* 
 * Split an absolute file path into directory and filename parts.
 *
 * If absolute_path does not contain a path separator, set directory and
 * filename to NULL pointers.
 */ 
static void split_file_path(char** directory, char** filename,
        const char* absolute_path)
{
    char* path_char = strrchr(absolute_path, '/');

    if (path_char == NULL) {
        if (directory)
            *directory = NULL;
        if (filename)
            *filename = NULL;
        return;
    }

    if (directory != NULL) {
        *directory = strndup(absolute_path,
                (size_t)(path_char - absolute_path));
    }
    if (filename != NULL) {
        *filename = strndup(path_char+1, strlen(path_char)-1);
    }
}

/*
 * version number comparison
 */
static int editorconfig_compare_version(
        const struct editorconfig_version* v0,
        const struct editorconfig_version* v1)
{
    /* compare major */
    if (v0->major > v1->major)
        return 1;
    else if (v0->major < v1->major)
        return -1;

    /* compare minor */
    if (v0->minor > v1->minor)
        return 1;
    else if (v0->minor < v1->minor)
        return -1;

    /* compare subminor */
    if (v0->subminor > v1->subminor)
        return 1;
    else if (v0->subminor < v1->subminor)
        return -1;

    return 0;
}

EDITORCONFIG_EXPORT
const char* editorconfig_get_error_msg(int err_num)
{
    if(err_num > 0)
        return "Failed to parse file.";

    switch(err_num) {
    case 0:
        return "No error occurred.";
    case EDITORCONFIG_PARSE_NOT_FULL_PATH:
        return "Input file must be a full path name.";
    case EDITORCONFIG_PARSE_MEMORY_ERROR:
        return "Memory error.";
    case EDITORCONFIG_PARSE_VERSION_TOO_NEW:
        return "Required version is greater than the current version.";
    }

    return "Unknown error.";
}

/* 
 * See the header file for the use of this function
 */
EDITORCONFIG_EXPORT
int editorconfig_parse(const char* full_filename, editorconfig_handle h)
{
    editorconfig_filesystem* fs = editorconfig_filesystem_default_init();
    int result = editorconfig_parse2(full_filename, h, fs);
    editorconfig_filesystem_default_destroy(fs);
    return result;
}

EDITORCONFIG_EXPORT
int editorconfig_parse2(const char* full_filename, editorconfig_handle h, editorconfig_filesystem* fs)
{
    handler_first_param                 hfp;
    const char*                         config_file;
    int                                 err_num;
    int                                 i;
    struct editorconfig_handle*         eh = (struct editorconfig_handle*)h;
    struct editorconfig_version         cur_ver;
    struct editorconfig_version         tmp_ver;
    editorconfig_conf_file_iterator     it;

    /* get current version */
    editorconfig_get_version(&cur_ver.major, &cur_ver.minor,
            &cur_ver.subminor);

    /* if version is set to 0.0.0, we set it to current version */
    if (eh->ver.major == 0 &&
            eh->ver.minor == 0 &&
            eh->ver.subminor == 0)
        eh->ver = cur_ver;

    if (editorconfig_compare_version(&eh->ver, &cur_ver) > 0)
        return EDITORCONFIG_PARSE_VERSION_TOO_NEW;

    if (!eh->err_file) {
        free(eh->err_file);
        eh->err_file = NULL;
    }

    /* if eh->conf_file_name is NULL, we set ".editorconfig" as the default
     * conf file name */
    if (!eh->conf_file_name)
        eh->conf_file_name = ".editorconfig";

    if (eh->name_values) {
        /* free name_values */
        for (i = 0; i < eh->name_value_count; ++i) {
            free(eh->name_values[i].name);
            free(eh->name_values[i].value);
        }
        free(eh->name_values);

        eh->name_values = NULL;
        eh->name_value_count = 0;
    }
    memset(&hfp, 0, sizeof(hfp));

    /* return an error if file path is not absolute */
    if (!fs->check_filename(fs, full_filename)) {
        return EDITORCONFIG_PARSE_NOT_FULL_PATH;
    }

    hfp.full_filename = fs->normalize_filename(fs, full_filename);

    array_editorconfig_name_value_init(&hfp.array_name_value);

    it = fs->list_conf_files(fs, hfp.full_filename, eh->conf_file_name);
    while (!hfp.root && (config_file = fs->next_conf_file(it)) != NULL) {
        hfp.file_index++;
        split_file_path(&hfp.editorconfig_file_dir, NULL, config_file);
        if ((err_num = ini_parse(fs, config_file, ini_handler, &hfp)) != 0 &&
                /* ignore error caused by I/O, maybe caused by non exist file */
                err_num != -1) {
            eh->err_file = strdup(config_file);
            fs->free_normalize_filename(fs, hfp.full_filename);
            free(hfp.editorconfig_file_dir);
            fs->free_list_conf_files(it);
            return err_num;
        }

        free(hfp.editorconfig_file_dir);
    }
    fs->free_list_conf_files(it);

    /* value proprocessing */

    /* For v0.9 */
    SET_EDITORCONFIG_VERSION(&tmp_ver, 0, 9, 0);
    if (editorconfig_compare_version(&eh->ver, &tmp_ver) >= 0) {
    /* Set indent_size to "tab" if indent_size is not specified and
     * indent_style is set to "tab". Only should be done after v0.9 */
        if (hfp.array_name_value.spnvp.indent_style &&
                !hfp.array_name_value.spnvp.indent_size &&
                !strcmp(hfp.array_name_value.spnvp.indent_style->value, "tab"))
            array_editorconfig_name_value_add(&hfp.array_name_value,
                    "indent_size", "tab", 0);
    /* Set indent_size to tab_width if indent_size is "tab" and tab_width is
     * specified. This behavior is specified for v0.9 and up. */
        if (hfp.array_name_value.spnvp.indent_size &&
            hfp.array_name_value.spnvp.tab_width &&
            !strcmp(hfp.array_name_value.spnvp.indent_size->value, "tab"))
        array_editorconfig_name_value_add(&hfp.array_name_value, "indent_size",
                hfp.array_name_value.spnvp.tab_width->value, 0);
    }

    /* Set tab_width to indent_size if indent_size is specified. If version is
     * not less than 0.9.0, we also need to check when the indent_size is set
     * to "tab", we should not duplicate the value to tab_width */
    if (hfp.array_name_value.spnvp.indent_size &&
            !hfp.array_name_value.spnvp.tab_width &&
            (editorconfig_compare_version(&eh->ver, &tmp_ver) < 0 ||
             strcmp(hfp.array_name_value.spnvp.indent_size->value, "tab")))
        array_editorconfig_name_value_add(&hfp.array_name_value, "tab_width",
                hfp.array_name_value.spnvp.indent_size->value, 0);

    eh->name_value_count = hfp.array_name_value.current_value_count;

    if (eh->name_value_count == 0) {  /* no value is set, just return 0. */
        fs->free_normalize_filename(fs, hfp.full_filename);
        return 0;
    }
    eh->name_values = hfp.array_name_value.name_values;
    eh->name_values = realloc(      /* realloc to truncate the unused spaces */
            eh->name_values,
            sizeof(editorconfig_name_value) * eh->name_value_count);
    if (eh->name_values == NULL) {
        fs->free_normalize_filename(fs, hfp.full_filename);
        return EDITORCONFIG_PARSE_MEMORY_ERROR;
    }

    fs->free_normalize_filename(fs, hfp.full_filename);

    return 0;
}

/*
 * See header file
 */
EDITORCONFIG_EXPORT
void editorconfig_get_version(int* major, int* minor, int* subminor)
{
    if (major)
        *major = editorconfig_VERSION_MAJOR;
    if (minor)
        *minor = editorconfig_VERSION_MINOR;
    if (subminor)
        *subminor = editorconfig_VERSION_SUBMINOR;
}

/*
 * See header file
 */
EDITORCONFIG_EXPORT
const char* editorconfig_get_version_suffix(void)
{
    return editorconfig_VERSION_SUFFIX;
}
