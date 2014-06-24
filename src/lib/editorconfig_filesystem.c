/*
 * Copyright (c) 2014 EditorConfig Team
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
#include "editorconfig_filesystem.h"
#include "misc.h"


typedef struct {
    editorconfig_filesystem base;
    char* reflect_filename;
} editorconfig_filesystem_default;

typedef struct {
    char* conf_file_name;
    int unc;
    char* src;
    char* cur;
} editorconfig_default_conf_file_iterator;


static int check_filename(editorconfig_filesystem* fs, const char* full_filename)
{
    return is_file_path_absolute(full_filename);
}

static char* normalize_filename(editorconfig_filesystem* fs, const char* full_filename)
{
#ifdef WIN32
    /* replace all backslashes with slashes on Windows */
    return str_replace(strdup(full_filename), '\\', '/');
#else
    return strdup(full_filename);
#endif
}

static void free_normalize_filename(editorconfig_filesystem* fs, char* filename)
{
    if (filename != NULL)
        free(filename);
}

static editorconfig_conf_file_iterator list_conf_files(editorconfig_filesystem* fs, const char* full_filename, const char* conf_file_name)
{
    editorconfig_filesystem_default* rfs = (editorconfig_filesystem_default*)fs;
    editorconfig_default_conf_file_iterator* it = (editorconfig_default_conf_file_iterator*)calloc(1, sizeof(editorconfig_default_conf_file_iterator));
    const char* filename = rfs->reflect_filename ? rfs->reflect_filename : full_filename;
    it->conf_file_name = strdup(conf_file_name);
    it->unc = !strncmp(filename, "//", 2);
    it->src = strdup(filename);
    it->cur = calloc(strlen(filename) + strlen(conf_file_name) + 10, 1);
    return (editorconfig_conf_file_iterator)it;
}

static const char* next_conf_file(editorconfig_conf_file_iterator it)
{
    editorconfig_default_conf_file_iterator* rit = (editorconfig_default_conf_file_iterator*)it;
    char* ptr = strrchr(rit->src, '/');
    if (ptr == NULL)
        return NULL;
#ifdef WIN32
    if (rit->unc && ptr <= rit->src + 1)
        return NULL;
#endif
    *ptr = '\0';
    strcpy(rit->cur, rit->src);
    strcat(rit->cur, "/");
    strcat(rit->cur, rit->conf_file_name);
    return rit->cur;
}

static void free_list_conf_files(editorconfig_conf_file_iterator it)
{
    editorconfig_default_conf_file_iterator* rit = (editorconfig_default_conf_file_iterator*)it;
    if (rit != NULL) {
        free(rit->conf_file_name);
        free(rit->src);
        free(rit->cur);
        free(rit);
    }
}

static editorconfig_filesystem_pointer open_file(editorconfig_filesystem* fs, const char* file)
{
    return (editorconfig_filesystem_pointer)fopen(file, "r");
}

static char* read_file(editorconfig_filesystem* fs, char *buf, int size, editorconfig_filesystem_pointer fp)
{
    return fgets(buf, size, (FILE*)fp);
}

static void close_file(editorconfig_filesystem* fs, editorconfig_filesystem_pointer fp)
{
    if (fp)
        fclose((FILE*)fp);
}

EDITORCONFIG_EXPORT
editorconfig_filesystem* editorconfig_filesystem_default_init(void)
{
    editorconfig_filesystem_default* fs = (editorconfig_filesystem_default*)
        calloc(sizeof(editorconfig_filesystem_default), 1);
    fs->base.check_filename = check_filename;
    fs->base.normalize_filename = normalize_filename;
    fs->base.free_normalize_filename = free_normalize_filename;
    fs->base.list_conf_files = list_conf_files;
    fs->base.next_conf_file = next_conf_file;
    fs->base.free_list_conf_files = free_list_conf_files;
    fs->base.open_file = open_file;
    fs->base.read_file = read_file;
    fs->base.close_file = close_file;
    return (editorconfig_filesystem*)fs;
}

EDITORCONFIG_EXPORT
int editorconfig_filesystem_default_destroy(editorconfig_filesystem* fs)
{
    editorconfig_filesystem_default* rfs = (editorconfig_filesystem_default*)fs;
    if (rfs == NULL)
        return 0;
    if (rfs->reflect_filename != NULL)
        free(rfs->reflect_filename);
    free(rfs);
    return 0;
}

EDITORCONFIG_EXPORT
editorconfig_filesystem* editorconfig_filesystem_reflect_init(const char* filename)
{
    editorconfig_filesystem_default* fs = (editorconfig_filesystem_default*)
        editorconfig_filesystem_default_init();
    fs->reflect_filename = normalize_filename((editorconfig_filesystem*)fs, filename);
    return (editorconfig_filesystem*)fs;
}

EDITORCONFIG_EXPORT
int editorconfig_filesystem_reflect_destroy(editorconfig_filesystem* fs)
{
    return editorconfig_filesystem_default_destroy(fs);
}
